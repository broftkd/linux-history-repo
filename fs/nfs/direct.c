/*
 * linux/fs/nfs/direct.c
 *
 * Copyright (C) 2003 by Chuck Lever <cel@netapp.com>
 *
 * High-performance uncached I/O for the Linux NFS client
 *
 * There are important applications whose performance or correctness
 * depends on uncached access to file data.  Database clusters
 * (multiple copies of the same instance running on separate hosts) 
 * implement their own cache coherency protocol that subsumes file
 * system cache protocols.  Applications that process datasets 
 * considerably larger than the client's memory do not always benefit 
 * from a local cache.  A streaming video server, for instance, has no 
 * need to cache the contents of a file.
 *
 * When an application requests uncached I/O, all read and write requests
 * are made directly to the server; data stored or fetched via these
 * requests is not cached in the Linux page cache.  The client does not
 * correct unaligned requests from applications.  All requested bytes are
 * held on permanent storage before a direct write system call returns to
 * an application.
 *
 * Solaris implements an uncached I/O facility called directio() that
 * is used for backups and sequential I/O to very large files.  Solaris
 * also supports uncaching whole NFS partitions with "-o forcedirectio,"
 * an undocumented mount option.
 *
 * Designed by Jeff Kimmel, Chuck Lever, and Trond Myklebust, with
 * help from Andrew Morton.
 *
 * 18 Dec 2001	Initial implementation for 2.4  --cel
 * 08 Jul 2002	Version for 2.4.19, with bug fixes --trondmy
 * 08 Jun 2003	Port to 2.5 APIs  --cel
 * 31 Mar 2004	Handle direct I/O without VFS support  --cel
 * 15 Sep 2004	Parallel async reads  --cel
 *
 */

#include <linux/config.h>
#include <linux/errno.h>
#include <linux/sched.h>
#include <linux/kernel.h>
#include <linux/smp_lock.h>
#include <linux/file.h>
#include <linux/pagemap.h>
#include <linux/kref.h>

#include <linux/nfs_fs.h>
#include <linux/nfs_page.h>
#include <linux/sunrpc/clnt.h>

#include <asm/system.h>
#include <asm/uaccess.h>
#include <asm/atomic.h>

#include "iostat.h"

#define NFSDBG_FACILITY		NFSDBG_VFS
#define MAX_DIRECTIO_SIZE	(4096UL << PAGE_SHIFT)

static void nfs_free_user_pages(struct page **pages, int npages, int do_dirty);
static kmem_cache_t *nfs_direct_cachep;

/*
 * This represents a set of asynchronous requests that we're waiting on
 */
struct nfs_direct_req {
	struct kref		kref;		/* release manager */
	struct list_head	list;		/* nfs_read_data structs */
	struct file *		filp;		/* file descriptor */
	struct kiocb *		iocb;		/* controlling i/o request */
	wait_queue_head_t	wait;		/* wait for i/o completion */
	struct inode *		inode;		/* target file of I/O */
	struct page **		pages;		/* pages in our buffer */
	unsigned int		npages;		/* count of pages */
	atomic_t		complete,	/* i/os we're waiting for */
				count,		/* bytes actually processed */
				error;		/* any reported error */
};


/**
 * nfs_direct_IO - NFS address space operation for direct I/O
 * @rw: direction (read or write)
 * @iocb: target I/O control block
 * @iov: array of vectors that define I/O buffer
 * @pos: offset in file to begin the operation
 * @nr_segs: size of iovec array
 *
 * The presence of this routine in the address space ops vector means
 * the NFS client supports direct I/O.  However, we shunt off direct
 * read and write requests before the VFS gets them, so this method
 * should never be called.
 */
ssize_t nfs_direct_IO(int rw, struct kiocb *iocb, const struct iovec *iov, loff_t pos, unsigned long nr_segs)
{
	struct dentry *dentry = iocb->ki_filp->f_dentry;

	dprintk("NFS: nfs_direct_IO (%s) off/no(%Ld/%lu) EINVAL\n",
			dentry->d_name.name, (long long) pos, nr_segs);

	return -EINVAL;
}

static inline int nfs_get_user_pages(int rw, unsigned long user_addr, size_t size, struct page ***pages)
{
	int result = -ENOMEM;
	unsigned long page_count;
	size_t array_size;

	/* set an arbitrary limit to prevent type overflow */
	/* XXX: this can probably be as large as INT_MAX */
	if (size > MAX_DIRECTIO_SIZE) {
		*pages = NULL;
		return -EFBIG;
	}

	page_count = (user_addr + size + PAGE_SIZE - 1) >> PAGE_SHIFT;
	page_count -= user_addr >> PAGE_SHIFT;

	array_size = (page_count * sizeof(struct page *));
	*pages = kmalloc(array_size, GFP_KERNEL);
	if (*pages) {
		down_read(&current->mm->mmap_sem);
		result = get_user_pages(current, current->mm, user_addr,
					page_count, (rw == READ), 0,
					*pages, NULL);
		up_read(&current->mm->mmap_sem);
		/*
		 * If we got fewer pages than expected from get_user_pages(),
		 * the user buffer runs off the end of a mapping; return EFAULT.
		 */
		if (result >= 0 && result < page_count) {
			nfs_free_user_pages(*pages, result, 0);
			*pages = NULL;
			result = -EFAULT;
		}
	}
	return result;
}

static void nfs_free_user_pages(struct page **pages, int npages, int do_dirty)
{
	int i;
	for (i = 0; i < npages; i++) {
		struct page *page = pages[i];
		if (do_dirty && !PageCompound(page))
			set_page_dirty_lock(page);
		page_cache_release(page);
	}
	kfree(pages);
}

static void nfs_direct_req_release(struct kref *kref)
{
	struct nfs_direct_req *dreq = container_of(kref, struct nfs_direct_req, kref);
	kmem_cache_free(nfs_direct_cachep, dreq);
}

/*
 * Collects and returns the final error value/byte-count.
 */
static ssize_t nfs_direct_wait(struct nfs_direct_req *dreq)
{
	int result = -EIOCBQUEUED;

	/* Async requests don't wait here */
	if (dreq->iocb)
		goto out;

	result = wait_event_interruptible(dreq->wait,
					(atomic_read(&dreq->complete) == 0));

	if (!result)
		result = atomic_read(&dreq->error);
	if (!result)
		result = atomic_read(&dreq->count);

out:
	kref_put(&dreq->kref, nfs_direct_req_release);
	return (ssize_t) result;
}

/*
 * Note we also set the number of requests we have in the dreq when we are
 * done.  This prevents races with I/O completion so we will always wait
 * until all requests have been dispatched and completed.
 */
static struct nfs_direct_req *nfs_direct_read_alloc(size_t nbytes, size_t rsize)
{
	struct list_head *list;
	struct nfs_direct_req *dreq;
	unsigned int reads = 0;
	unsigned int rpages = (rsize + PAGE_CACHE_SIZE - 1) >> PAGE_CACHE_SHIFT;

	dreq = kmem_cache_alloc(nfs_direct_cachep, SLAB_KERNEL);
	if (!dreq)
		return NULL;

	kref_init(&dreq->kref);
	init_waitqueue_head(&dreq->wait);
	INIT_LIST_HEAD(&dreq->list);
	dreq->iocb = NULL;
	atomic_set(&dreq->count, 0);
	atomic_set(&dreq->error, 0);

	list = &dreq->list;
	for(;;) {
		struct nfs_read_data *data = nfs_readdata_alloc(rpages);

		if (unlikely(!data)) {
			while (!list_empty(list)) {
				data = list_entry(list->next,
						  struct nfs_read_data, pages);
				list_del(&data->pages);
				nfs_readdata_free(data);
			}
			kref_put(&dreq->kref, nfs_direct_req_release);
			return NULL;
		}

		INIT_LIST_HEAD(&data->pages);
		list_add(&data->pages, list);

		data->req = (struct nfs_page *) dreq;
		reads++;
		if (nbytes <= rsize)
			break;
		nbytes -= rsize;
	}
	kref_get(&dreq->kref);
	atomic_set(&dreq->complete, reads);
	return dreq;
}

/*
 * We must hold a reference to all the pages in this direct read request
 * until the RPCs complete.  This could be long *after* we are woken up in
 * nfs_direct_wait (for instance, if someone hits ^C on a slow server).
 *
 * In addition, synchronous I/O uses a stack-allocated iocb.  Thus we
 * can't trust the iocb is still valid here if this is a synchronous
 * request.  If the waiter is woken prematurely, the iocb is long gone.
 */
static void nfs_direct_read_result(struct rpc_task *task, void *calldata)
{
	struct nfs_read_data *data = calldata;
	struct nfs_direct_req *dreq = (struct nfs_direct_req *) data->req;

	if (nfs_readpage_result(task, data) != 0)
		return;
	if (likely(task->tk_status >= 0))
		atomic_add(data->res.count, &dreq->count);
	else
		atomic_set(&dreq->error, task->tk_status);

	if (unlikely(atomic_dec_and_test(&dreq->complete))) {
		nfs_free_user_pages(dreq->pages, dreq->npages, 1);
		if (dreq->iocb) {
			long res = atomic_read(&dreq->error);
			if (!res)
				res = atomic_read(&dreq->count);
			aio_complete(dreq->iocb, res, 0);
		} else
			wake_up(&dreq->wait);
		kref_put(&dreq->kref, nfs_direct_req_release);
	}
}

static const struct rpc_call_ops nfs_read_direct_ops = {
	.rpc_call_done = nfs_direct_read_result,
	.rpc_release = nfs_readdata_release,
};

/*
 * For each nfs_read_data struct that was allocated on the list, dispatch
 * an NFS READ operation
 */
static void nfs_direct_read_schedule(struct nfs_direct_req *dreq, unsigned long user_addr, size_t count, loff_t file_offset)
{
	struct file *file = dreq->filp;
	struct inode *inode = file->f_mapping->host;
	struct nfs_open_context *ctx = (struct nfs_open_context *)
							file->private_data;
	struct list_head *list = &dreq->list;
	struct page **pages = dreq->pages;
	size_t rsize = NFS_SERVER(inode)->rsize;
	unsigned int curpage, pgbase;

	curpage = 0;
	pgbase = user_addr & ~PAGE_MASK;
	do {
		struct nfs_read_data *data;
		size_t bytes;

		bytes = rsize;
		if (count < rsize)
			bytes = count;

		data = list_entry(list->next, struct nfs_read_data, pages);
		list_del_init(&data->pages);

		data->inode = inode;
		data->cred = ctx->cred;
		data->args.fh = NFS_FH(inode);
		data->args.context = ctx;
		data->args.offset = file_offset;
		data->args.pgbase = pgbase;
		data->args.pages = &pages[curpage];
		data->args.count = bytes;
		data->res.fattr = &data->fattr;
		data->res.eof = 0;
		data->res.count = bytes;

		rpc_init_task(&data->task, NFS_CLIENT(inode), RPC_TASK_ASYNC,
				&nfs_read_direct_ops, data);
		NFS_PROTO(inode)->read_setup(data);

		data->task.tk_cookie = (unsigned long) inode;

		lock_kernel();
		rpc_execute(&data->task);
		unlock_kernel();

		dfprintk(VFS, "NFS: %4d initiated direct read call (req %s/%Ld, %u bytes @ offset %Lu)\n",
				data->task.tk_pid,
				inode->i_sb->s_id,
				(long long)NFS_FILEID(inode),
				bytes,
				(unsigned long long)data->args.offset);

		file_offset += bytes;
		pgbase += bytes;
		curpage += pgbase >> PAGE_SHIFT;
		pgbase &= ~PAGE_MASK;

		count -= bytes;
	} while (count != 0);
}

static ssize_t nfs_direct_read(struct kiocb *iocb, unsigned long user_addr, size_t count, loff_t file_offset, struct page **pages, unsigned int nr_pages)
{
	ssize_t result;
	sigset_t oldset;
	struct inode *inode = iocb->ki_filp->f_mapping->host;
	struct rpc_clnt *clnt = NFS_CLIENT(inode);
	struct nfs_direct_req *dreq;

	dreq = nfs_direct_read_alloc(count, NFS_SERVER(inode)->rsize);
	if (!dreq)
		return -ENOMEM;

	dreq->pages = pages;
	dreq->npages = nr_pages;
	dreq->inode = inode;
	dreq->filp = iocb->ki_filp;
	if (!is_sync_kiocb(iocb))
		dreq->iocb = iocb;

	nfs_add_stats(inode, NFSIOS_DIRECTREADBYTES, count);
	rpc_clnt_sigmask(clnt, &oldset);
	nfs_direct_read_schedule(dreq, user_addr, count, file_offset);
	result = nfs_direct_wait(dreq);
	rpc_clnt_sigunmask(clnt, &oldset);

	return result;
}

static ssize_t nfs_direct_write_seg(struct inode *inode, struct nfs_open_context *ctx, unsigned long user_addr, size_t count, loff_t file_offset, struct page **pages, int nr_pages)
{
	const unsigned int wsize = NFS_SERVER(inode)->wsize;
	size_t request;
	int curpage, need_commit;
	ssize_t result, tot_bytes;
	struct nfs_writeverf first_verf;
	struct nfs_write_data *wdata;

	wdata = nfs_writedata_alloc(NFS_SERVER(inode)->wpages);
	if (!wdata)
		return -ENOMEM;

	wdata->inode = inode;
	wdata->cred = ctx->cred;
	wdata->args.fh = NFS_FH(inode);
	wdata->args.context = ctx;
	wdata->args.stable = NFS_UNSTABLE;
	if (IS_SYNC(inode) || NFS_PROTO(inode)->version == 2 || count <= wsize)
		wdata->args.stable = NFS_FILE_SYNC;
	wdata->res.fattr = &wdata->fattr;
	wdata->res.verf = &wdata->verf;

	nfs_begin_data_update(inode);
retry:
	need_commit = 0;
	tot_bytes = 0;
	curpage = 0;
	request = count;
	wdata->args.pgbase = user_addr & ~PAGE_MASK;
	wdata->args.offset = file_offset;
	do {
		wdata->args.count = request;
		if (wdata->args.count > wsize)
			wdata->args.count = wsize;
		wdata->args.pages = &pages[curpage];

		dprintk("NFS: direct write: c=%u o=%Ld ua=%lu, pb=%u, cp=%u\n",
			wdata->args.count, (long long) wdata->args.offset,
			user_addr + tot_bytes, wdata->args.pgbase, curpage);

		lock_kernel();
		result = NFS_PROTO(inode)->write(wdata);
		unlock_kernel();

		if (result <= 0) {
			if (tot_bytes > 0)
				break;
			goto out;
		}

		if (tot_bytes == 0)
			memcpy(&first_verf.verifier, &wdata->verf.verifier,
						sizeof(first_verf.verifier));
		if (wdata->verf.committed != NFS_FILE_SYNC) {
			need_commit = 1;
			if (memcmp(&first_verf.verifier, &wdata->verf.verifier,
					sizeof(first_verf.verifier)))
				goto sync_retry;
		}

		tot_bytes += result;

		/* in case of a short write: stop now, let the app recover */
		if (result < wdata->args.count)
			break;

		wdata->args.offset += result;
		wdata->args.pgbase += result;
		curpage += wdata->args.pgbase >> PAGE_SHIFT;
		wdata->args.pgbase &= ~PAGE_MASK;
		request -= result;
	} while (request != 0);

	/*
	 * Commit data written so far, even in the event of an error
	 */
	if (need_commit) {
		wdata->args.count = tot_bytes;
		wdata->args.offset = file_offset;

		lock_kernel();
		result = NFS_PROTO(inode)->commit(wdata);
		unlock_kernel();

		if (result < 0 || memcmp(&first_verf.verifier,
					 &wdata->verf.verifier,
					 sizeof(first_verf.verifier)) != 0)
			goto sync_retry;
	}
	result = tot_bytes;

out:
	nfs_end_data_update(inode);
	nfs_writedata_free(wdata);
	return result;

sync_retry:
	wdata->args.stable = NFS_FILE_SYNC;
	goto retry;
}

/*
 * Upon return, generic_file_direct_IO invalidates any cached pages
 * that non-direct readers might access, so they will pick up these
 * writes immediately.
 */
static ssize_t nfs_direct_write(struct inode *inode, struct nfs_open_context *ctx, const struct iovec *iov, loff_t file_offset, unsigned long nr_segs)
{
	ssize_t tot_bytes = 0;
	unsigned long seg = 0;

	while ((seg < nr_segs) && (tot_bytes >= 0)) {
		ssize_t result;
		int page_count;
		struct page **pages;
		const struct iovec *vec = &iov[seg++];
		unsigned long user_addr = (unsigned long) vec->iov_base;
		size_t size = vec->iov_len;

                page_count = nfs_get_user_pages(WRITE, user_addr, size, &pages);
                if (page_count < 0) {
                        nfs_free_user_pages(pages, 0, 0);
			if (tot_bytes > 0)
				break;
                        return page_count;
                }

		nfs_add_stats(inode, NFSIOS_DIRECTWRITTENBYTES, size);
		result = nfs_direct_write_seg(inode, ctx, user_addr, size,
				file_offset, pages, page_count);
		nfs_free_user_pages(pages, page_count, 0);

		if (result <= 0) {
			if (tot_bytes > 0)
				break;
			return result;
		}
		nfs_add_stats(inode, NFSIOS_SERVERWRITTENBYTES, result);
		tot_bytes += result;
		file_offset += result;
		if (result < size)
			break;
	}
	return tot_bytes;
}

/**
 * nfs_file_direct_read - file direct read operation for NFS files
 * @iocb: target I/O control block
 * @buf: user's buffer into which to read data
 * count: number of bytes to read
 * pos: byte offset in file where reading starts
 *
 * We use this function for direct reads instead of calling
 * generic_file_aio_read() in order to avoid gfar's check to see if
 * the request starts before the end of the file.  For that check
 * to work, we must generate a GETATTR before each direct read, and
 * even then there is a window between the GETATTR and the subsequent
 * READ where the file size could change.  So our preference is simply
 * to do all reads the application wants, and the server will take
 * care of managing the end of file boundary.
 * 
 * This function also eliminates unnecessarily updating the file's
 * atime locally, as the NFS server sets the file's atime, and this
 * client must read the updated atime from the server back into its
 * cache.
 */
ssize_t nfs_file_direct_read(struct kiocb *iocb, char __user *buf, size_t count, loff_t pos)
{
	ssize_t retval = -EINVAL;
	int page_count;
	struct page **pages;
	struct file *file = iocb->ki_filp;
	struct address_space *mapping = file->f_mapping;

	dprintk("nfs: direct read(%s/%s, %lu@%Ld)\n",
		file->f_dentry->d_parent->d_name.name,
		file->f_dentry->d_name.name,
		(unsigned long) count, (long long) pos);

	if (count < 0)
		goto out;
	retval = -EFAULT;
	if (!access_ok(VERIFY_WRITE, buf, count))
		goto out;
	retval = 0;
	if (!count)
		goto out;

	retval = nfs_sync_mapping(mapping);
	if (retval)
		goto out;

	page_count = nfs_get_user_pages(READ, (unsigned long) buf,
						count, &pages);
	if (page_count < 0) {
		nfs_free_user_pages(pages, 0, 0);
		retval = page_count;
		goto out;
	}

	retval = nfs_direct_read(iocb, (unsigned long) buf, count, pos,
						pages, page_count);
	if (retval > 0)
		iocb->ki_pos = pos + retval;

out:
	return retval;
}

/**
 * nfs_file_direct_write - file direct write operation for NFS files
 * @iocb: target I/O control block
 * @buf: user's buffer from which to write data
 * count: number of bytes to write
 * pos: byte offset in file where writing starts
 *
 * We use this function for direct writes instead of calling
 * generic_file_aio_write() in order to avoid taking the inode
 * semaphore and updating the i_size.  The NFS server will set
 * the new i_size and this client must read the updated size
 * back into its cache.  We let the server do generic write
 * parameter checking and report problems.
 *
 * We also avoid an unnecessary invocation of generic_osync_inode(),
 * as it is fairly meaningless to sync the metadata of an NFS file.
 *
 * We eliminate local atime updates, see direct read above.
 *
 * We avoid unnecessary page cache invalidations for normal cached
 * readers of this file.
 *
 * Note that O_APPEND is not supported for NFS direct writes, as there
 * is no atomic O_APPEND write facility in the NFS protocol.
 */
ssize_t nfs_file_direct_write(struct kiocb *iocb, const char __user *buf, size_t count, loff_t pos)
{
	ssize_t retval;
	struct file *file = iocb->ki_filp;
	struct nfs_open_context *ctx =
			(struct nfs_open_context *) file->private_data;
	struct address_space *mapping = file->f_mapping;
	struct inode *inode = mapping->host;
	struct iovec iov = {
		.iov_base = (char __user *)buf,
	};

	dfprintk(VFS, "nfs: direct write(%s/%s, %lu@%Ld)\n",
		file->f_dentry->d_parent->d_name.name,
		file->f_dentry->d_name.name,
		(unsigned long) count, (long long) pos);

	retval = -EINVAL;
	if (!is_sync_kiocb(iocb))
		goto out;

	retval = generic_write_checks(file, &pos, &count, 0);
	if (retval)
		goto out;

	retval = -EINVAL;
	if ((ssize_t) count < 0)
		goto out;
	retval = 0;
	if (!count)
		goto out;
	iov.iov_len = count,

	retval = -EFAULT;
	if (!access_ok(VERIFY_READ, iov.iov_base, iov.iov_len))
		goto out;

	retval = nfs_sync_mapping(mapping);
	if (retval)
		goto out;

	retval = nfs_direct_write(inode, ctx, &iov, pos, 1);
	if (mapping->nrpages)
		invalidate_inode_pages2(mapping);
	if (retval > 0)
		iocb->ki_pos = pos + retval;

out:
	return retval;
}

int nfs_init_directcache(void)
{
	nfs_direct_cachep = kmem_cache_create("nfs_direct_cache",
						sizeof(struct nfs_direct_req),
						0, SLAB_RECLAIM_ACCOUNT,
						NULL, NULL);
	if (nfs_direct_cachep == NULL)
		return -ENOMEM;

	return 0;
}

void nfs_destroy_directcache(void)
{
	if (kmem_cache_destroy(nfs_direct_cachep))
		printk(KERN_INFO "nfs_direct_cache: not all structures were freed\n");
}
