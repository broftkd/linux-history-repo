/*
 * Copyright (C) 2008, 2009 Intel Corporation
 * Authors: Andi Kleen, Fengguang Wu
 *
 * This software may be redistributed and/or modified under the terms of
 * the GNU General Public License ("GPL") version 2 only as published by the
 * Free Software Foundation.
 *
 * High level machine check handler. Handles pages reported by the
 * hardware as being corrupted usually due to a 2bit ECC memory or cache
 * failure.
 *
 * Handles page cache pages in various states.	The tricky part
 * here is that we can access any page asynchronous to other VM
 * users, because memory failures could happen anytime and anywhere,
 * possibly violating some of their assumptions. This is why this code
 * has to be extremely careful. Generally it tries to use normal locking
 * rules, as in get the standard locks, even if that means the
 * error handling takes potentially a long time.
 *
 * The operation to map back from RMAP chains to processes has to walk
 * the complete process list and has non linear complexity with the number
 * mappings. In short it can be quite slow. But since memory corruptions
 * are rare we hope to get away with this.
 */

/*
 * Notebook:
 * - hugetlb needs more code
 * - kcore/oldmem/vmcore/mem/kmem check for hwpoison pages
 * - pass bad pages to kdump next kernel
 */
#define DEBUG 1		/* remove me in 2.6.34 */
#include <linux/kernel.h>
#include <linux/mm.h>
#include <linux/page-flags.h>
#include <linux/sched.h>
#include <linux/ksm.h>
#include <linux/rmap.h>
#include <linux/pagemap.h>
#include <linux/swap.h>
#include <linux/backing-dev.h>
#include "internal.h"

int sysctl_memory_failure_early_kill __read_mostly = 0;

int sysctl_memory_failure_recovery __read_mostly = 1;

atomic_long_t mce_bad_pages __read_mostly = ATOMIC_LONG_INIT(0);

/*
 * Send all the processes who have the page mapped an ``action optional''
 * signal.
 */
static int kill_proc_ao(struct task_struct *t, unsigned long addr, int trapno,
			unsigned long pfn)
{
	struct siginfo si;
	int ret;

	printk(KERN_ERR
		"MCE %#lx: Killing %s:%d early due to hardware memory corruption\n",
		pfn, t->comm, t->pid);
	si.si_signo = SIGBUS;
	si.si_errno = 0;
	si.si_code = BUS_MCEERR_AO;
	si.si_addr = (void *)addr;
#ifdef __ARCH_SI_TRAPNO
	si.si_trapno = trapno;
#endif
	si.si_addr_lsb = PAGE_SHIFT;
	/*
	 * Don't use force here, it's convenient if the signal
	 * can be temporarily blocked.
	 * This could cause a loop when the user sets SIGBUS
	 * to SIG_IGN, but hopefully noone will do that?
	 */
	ret = send_sig_info(SIGBUS, &si, t);  /* synchronous? */
	if (ret < 0)
		printk(KERN_INFO "MCE: Error sending signal to %s:%d: %d\n",
		       t->comm, t->pid, ret);
	return ret;
}

/*
 * When a unknown page type is encountered drain as many buffers as possible
 * in the hope to turn the page into a LRU or free page, which we can handle.
 */
void shake_page(struct page *p)
{
	if (!PageSlab(p)) {
		lru_add_drain_all();
		if (PageLRU(p))
			return;
		drain_all_pages();
		if (PageLRU(p) || is_free_buddy_page(p))
			return;
	}
	/*
	 * Could call shrink_slab here (which would also
	 * shrink other caches). Unfortunately that might
	 * also access the corrupted page, which could be fatal.
	 */
}
EXPORT_SYMBOL_GPL(shake_page);

/*
 * Kill all processes that have a poisoned page mapped and then isolate
 * the page.
 *
 * General strategy:
 * Find all processes having the page mapped and kill them.
 * But we keep a page reference around so that the page is not
 * actually freed yet.
 * Then stash the page away
 *
 * There's no convenient way to get back to mapped processes
 * from the VMAs. So do a brute-force search over all
 * running processes.
 *
 * Remember that machine checks are not common (or rather
 * if they are common you have other problems), so this shouldn't
 * be a performance issue.
 *
 * Also there are some races possible while we get from the
 * error detection to actually handle it.
 */

struct to_kill {
	struct list_head nd;
	struct task_struct *tsk;
	unsigned long addr;
	unsigned addr_valid:1;
};

/*
 * Failure handling: if we can't find or can't kill a process there's
 * not much we can do.	We just print a message and ignore otherwise.
 */

/*
 * Schedule a process for later kill.
 * Uses GFP_ATOMIC allocations to avoid potential recursions in the VM.
 * TBD would GFP_NOIO be enough?
 */
static void add_to_kill(struct task_struct *tsk, struct page *p,
		       struct vm_area_struct *vma,
		       struct list_head *to_kill,
		       struct to_kill **tkc)
{
	struct to_kill *tk;

	if (*tkc) {
		tk = *tkc;
		*tkc = NULL;
	} else {
		tk = kmalloc(sizeof(struct to_kill), GFP_ATOMIC);
		if (!tk) {
			printk(KERN_ERR
		"MCE: Out of memory while machine check handling\n");
			return;
		}
	}
	tk->addr = page_address_in_vma(p, vma);
	tk->addr_valid = 1;

	/*
	 * In theory we don't have to kill when the page was
	 * munmaped. But it could be also a mremap. Since that's
	 * likely very rare kill anyways just out of paranoia, but use
	 * a SIGKILL because the error is not contained anymore.
	 */
	if (tk->addr == -EFAULT) {
		pr_debug("MCE: Unable to find user space address %lx in %s\n",
			page_to_pfn(p), tsk->comm);
		tk->addr_valid = 0;
	}
	get_task_struct(tsk);
	tk->tsk = tsk;
	list_add_tail(&tk->nd, to_kill);
}

/*
 * Kill the processes that have been collected earlier.
 *
 * Only do anything when DOIT is set, otherwise just free the list
 * (this is used for clean pages which do not need killing)
 * Also when FAIL is set do a force kill because something went
 * wrong earlier.
 */
static void kill_procs_ao(struct list_head *to_kill, int doit, int trapno,
			  int fail, unsigned long pfn)
{
	struct to_kill *tk, *next;

	list_for_each_entry_safe (tk, next, to_kill, nd) {
		if (doit) {
			/*
			 * In case something went wrong with munmapping
			 * make sure the process doesn't catch the
			 * signal and then access the memory. Just kill it.
			 * the signal handlers
			 */
			if (fail || tk->addr_valid == 0) {
				printk(KERN_ERR
		"MCE %#lx: forcibly killing %s:%d because of failure to unmap corrupted page\n",
					pfn, tk->tsk->comm, tk->tsk->pid);
				force_sig(SIGKILL, tk->tsk);
			}

			/*
			 * In theory the process could have mapped
			 * something else on the address in-between. We could
			 * check for that, but we need to tell the
			 * process anyways.
			 */
			else if (kill_proc_ao(tk->tsk, tk->addr, trapno,
					      pfn) < 0)
				printk(KERN_ERR
		"MCE %#lx: Cannot send advisory machine check signal to %s:%d\n",
					pfn, tk->tsk->comm, tk->tsk->pid);
		}
		put_task_struct(tk->tsk);
		kfree(tk);
	}
}

static int task_early_kill(struct task_struct *tsk)
{
	if (!tsk->mm)
		return 0;
	if (tsk->flags & PF_MCE_PROCESS)
		return !!(tsk->flags & PF_MCE_EARLY);
	return sysctl_memory_failure_early_kill;
}

/*
 * Collect processes when the error hit an anonymous page.
 */
static void collect_procs_anon(struct page *page, struct list_head *to_kill,
			      struct to_kill **tkc)
{
	struct vm_area_struct *vma;
	struct task_struct *tsk;
	struct anon_vma *av;

	read_lock(&tasklist_lock);
	av = page_lock_anon_vma(page);
	if (av == NULL)	/* Not actually mapped anymore */
		goto out;
	for_each_process (tsk) {
		if (!task_early_kill(tsk))
			continue;
		list_for_each_entry (vma, &av->head, anon_vma_node) {
			if (!page_mapped_in_vma(page, vma))
				continue;
			if (vma->vm_mm == tsk->mm)
				add_to_kill(tsk, page, vma, to_kill, tkc);
		}
	}
	page_unlock_anon_vma(av);
out:
	read_unlock(&tasklist_lock);
}

/*
 * Collect processes when the error hit a file mapped page.
 */
static void collect_procs_file(struct page *page, struct list_head *to_kill,
			      struct to_kill **tkc)
{
	struct vm_area_struct *vma;
	struct task_struct *tsk;
	struct prio_tree_iter iter;
	struct address_space *mapping = page->mapping;

	/*
	 * A note on the locking order between the two locks.
	 * We don't rely on this particular order.
	 * If you have some other code that needs a different order
	 * feel free to switch them around. Or add a reverse link
	 * from mm_struct to task_struct, then this could be all
	 * done without taking tasklist_lock and looping over all tasks.
	 */

	read_lock(&tasklist_lock);
	spin_lock(&mapping->i_mmap_lock);
	for_each_process(tsk) {
		pgoff_t pgoff = page->index << (PAGE_CACHE_SHIFT - PAGE_SHIFT);

		if (!task_early_kill(tsk))
			continue;

		vma_prio_tree_foreach(vma, &iter, &mapping->i_mmap, pgoff,
				      pgoff) {
			/*
			 * Send early kill signal to tasks where a vma covers
			 * the page but the corrupted page is not necessarily
			 * mapped it in its pte.
			 * Assume applications who requested early kill want
			 * to be informed of all such data corruptions.
			 */
			if (vma->vm_mm == tsk->mm)
				add_to_kill(tsk, page, vma, to_kill, tkc);
		}
	}
	spin_unlock(&mapping->i_mmap_lock);
	read_unlock(&tasklist_lock);
}

/*
 * Collect the processes who have the corrupted page mapped to kill.
 * This is done in two steps for locking reasons.
 * First preallocate one tokill structure outside the spin locks,
 * so that we can kill at least one process reasonably reliable.
 */
static void collect_procs(struct page *page, struct list_head *tokill)
{
	struct to_kill *tk;

	if (!page->mapping)
		return;

	tk = kmalloc(sizeof(struct to_kill), GFP_NOIO);
	if (!tk)
		return;
	if (PageAnon(page))
		collect_procs_anon(page, tokill, &tk);
	else
		collect_procs_file(page, tokill, &tk);
	kfree(tk);
}

/*
 * Error handlers for various types of pages.
 */

enum outcome {
	FAILED,		/* Error handling failed */
	DELAYED,	/* Will be handled later */
	IGNORED,	/* Error safely ignored */
	RECOVERED,	/* Successfully recovered */
};

static const char *action_name[] = {
	[FAILED] = "Failed",
	[DELAYED] = "Delayed",
	[IGNORED] = "Ignored",
	[RECOVERED] = "Recovered",
};

/*
 * XXX: It is possible that a page is isolated from LRU cache,
 * and then kept in swap cache or failed to remove from page cache.
 * The page count will stop it from being freed by unpoison.
 * Stress tests should be aware of this memory leak problem.
 */
static int delete_from_lru_cache(struct page *p)
{
	if (!isolate_lru_page(p)) {
		/*
		 * Clear sensible page flags, so that the buddy system won't
		 * complain when the page is unpoison-and-freed.
		 */
		ClearPageActive(p);
		ClearPageUnevictable(p);
		/*
		 * drop the page count elevated by isolate_lru_page()
		 */
		page_cache_release(p);
		return 0;
	}
	return -EIO;
}

/*
 * Error hit kernel page.
 * Do nothing, try to be lucky and not touch this instead. For a few cases we
 * could be more sophisticated.
 */
static int me_kernel(struct page *p, unsigned long pfn)
{
	return DELAYED;
}

/*
 * Already poisoned page.
 */
static int me_ignore(struct page *p, unsigned long pfn)
{
	return IGNORED;
}

/*
 * Page in unknown state. Do nothing.
 */
static int me_unknown(struct page *p, unsigned long pfn)
{
	printk(KERN_ERR "MCE %#lx: Unknown page state\n", pfn);
	return FAILED;
}

/*
 * Clean (or cleaned) page cache page.
 */
static int me_pagecache_clean(struct page *p, unsigned long pfn)
{
	int err;
	int ret = FAILED;
	struct address_space *mapping;

	delete_from_lru_cache(p);

	/*
	 * For anonymous pages we're done the only reference left
	 * should be the one m_f() holds.
	 */
	if (PageAnon(p))
		return RECOVERED;

	/*
	 * Now truncate the page in the page cache. This is really
	 * more like a "temporary hole punch"
	 * Don't do this for block devices when someone else
	 * has a reference, because it could be file system metadata
	 * and that's not safe to truncate.
	 */
	mapping = page_mapping(p);
	if (!mapping) {
		/*
		 * Page has been teared down in the meanwhile
		 */
		return FAILED;
	}

	/*
	 * Truncation is a bit tricky. Enable it per file system for now.
	 *
	 * Open: to take i_mutex or not for this? Right now we don't.
	 */
	if (mapping->a_ops->error_remove_page) {
		err = mapping->a_ops->error_remove_page(mapping, p);
		if (err != 0) {
			printk(KERN_INFO "MCE %#lx: Failed to punch page: %d\n",
					pfn, err);
		} else if (page_has_private(p) &&
				!try_to_release_page(p, GFP_NOIO)) {
			pr_debug("MCE %#lx: failed to release buffers\n", pfn);
		} else {
			ret = RECOVERED;
		}
	} else {
		/*
		 * If the file system doesn't support it just invalidate
		 * This fails on dirty or anything with private pages
		 */
		if (invalidate_inode_page(p))
			ret = RECOVERED;
		else
			printk(KERN_INFO "MCE %#lx: Failed to invalidate\n",
				pfn);
	}
	return ret;
}

/*
 * Dirty cache page page
 * Issues: when the error hit a hole page the error is not properly
 * propagated.
 */
static int me_pagecache_dirty(struct page *p, unsigned long pfn)
{
	struct address_space *mapping = page_mapping(p);

	SetPageError(p);
	/* TBD: print more information about the file. */
	if (mapping) {
		/*
		 * IO error will be reported by write(), fsync(), etc.
		 * who check the mapping.
		 * This way the application knows that something went
		 * wrong with its dirty file data.
		 *
		 * There's one open issue:
		 *
		 * The EIO will be only reported on the next IO
		 * operation and then cleared through the IO map.
		 * Normally Linux has two mechanisms to pass IO error
		 * first through the AS_EIO flag in the address space
		 * and then through the PageError flag in the page.
		 * Since we drop pages on memory failure handling the
		 * only mechanism open to use is through AS_AIO.
		 *
		 * This has the disadvantage that it gets cleared on
		 * the first operation that returns an error, while
		 * the PageError bit is more sticky and only cleared
		 * when the page is reread or dropped.  If an
		 * application assumes it will always get error on
		 * fsync, but does other operations on the fd before
		 * and the page is dropped inbetween then the error
		 * will not be properly reported.
		 *
		 * This can already happen even without hwpoisoned
		 * pages: first on metadata IO errors (which only
		 * report through AS_EIO) or when the page is dropped
		 * at the wrong time.
		 *
		 * So right now we assume that the application DTRT on
		 * the first EIO, but we're not worse than other parts
		 * of the kernel.
		 */
		mapping_set_error(mapping, EIO);
	}

	return me_pagecache_clean(p, pfn);
}

/*
 * Clean and dirty swap cache.
 *
 * Dirty swap cache page is tricky to handle. The page could live both in page
 * cache and swap cache(ie. page is freshly swapped in). So it could be
 * referenced concurrently by 2 types of PTEs:
 * normal PTEs and swap PTEs. We try to handle them consistently by calling
 * try_to_unmap(TTU_IGNORE_HWPOISON) to convert the normal PTEs to swap PTEs,
 * and then
 *      - clear dirty bit to prevent IO
 *      - remove from LRU
 *      - but keep in the swap cache, so that when we return to it on
 *        a later page fault, we know the application is accessing
 *        corrupted data and shall be killed (we installed simple
 *        interception code in do_swap_page to catch it).
 *
 * Clean swap cache pages can be directly isolated. A later page fault will
 * bring in the known good data from disk.
 */
static int me_swapcache_dirty(struct page *p, unsigned long pfn)
{
	ClearPageDirty(p);
	/* Trigger EIO in shmem: */
	ClearPageUptodate(p);

	if (!delete_from_lru_cache(p))
		return DELAYED;
	else
		return FAILED;
}

static int me_swapcache_clean(struct page *p, unsigned long pfn)
{
	delete_from_swap_cache(p);

	if (!delete_from_lru_cache(p))
		return RECOVERED;
	else
		return FAILED;
}

/*
 * Huge pages. Needs work.
 * Issues:
 * No rmap support so we cannot find the original mapper. In theory could walk
 * all MMs and look for the mappings, but that would be non atomic and racy.
 * Need rmap for hugepages for this. Alternatively we could employ a heuristic,
 * like just walking the current process and hoping it has it mapped (that
 * should be usually true for the common "shared database cache" case)
 * Should handle free huge pages and dequeue them too, but this needs to
 * handle huge page accounting correctly.
 */
static int me_huge_page(struct page *p, unsigned long pfn)
{
	return FAILED;
}

/*
 * Various page states we can handle.
 *
 * A page state is defined by its current page->flags bits.
 * The table matches them in order and calls the right handler.
 *
 * This is quite tricky because we can access page at any time
 * in its live cycle, so all accesses have to be extremly careful.
 *
 * This is not complete. More states could be added.
 * For any missing state don't attempt recovery.
 */

#define dirty		(1UL << PG_dirty)
#define sc		(1UL << PG_swapcache)
#define unevict		(1UL << PG_unevictable)
#define mlock		(1UL << PG_mlocked)
#define writeback	(1UL << PG_writeback)
#define lru		(1UL << PG_lru)
#define swapbacked	(1UL << PG_swapbacked)
#define head		(1UL << PG_head)
#define tail		(1UL << PG_tail)
#define compound	(1UL << PG_compound)
#define slab		(1UL << PG_slab)
#define reserved	(1UL << PG_reserved)

static struct page_state {
	unsigned long mask;
	unsigned long res;
	char *msg;
	int (*action)(struct page *p, unsigned long pfn);
} error_states[] = {
	{ reserved,	reserved,	"reserved kernel",	me_ignore },
	/*
	 * free pages are specially detected outside this table:
	 * PG_buddy pages only make a small fraction of all free pages.
	 */

	/*
	 * Could in theory check if slab page is free or if we can drop
	 * currently unused objects without touching them. But just
	 * treat it as standard kernel for now.
	 */
	{ slab,		slab,		"kernel slab",	me_kernel },

#ifdef CONFIG_PAGEFLAGS_EXTENDED
	{ head,		head,		"huge",		me_huge_page },
	{ tail,		tail,		"huge",		me_huge_page },
#else
	{ compound,	compound,	"huge",		me_huge_page },
#endif

	{ sc|dirty,	sc|dirty,	"swapcache",	me_swapcache_dirty },
	{ sc|dirty,	sc,		"swapcache",	me_swapcache_clean },

	{ unevict|dirty, unevict|dirty,	"unevictable LRU", me_pagecache_dirty},
	{ unevict,	unevict,	"unevictable LRU", me_pagecache_clean},

	{ mlock|dirty,	mlock|dirty,	"mlocked LRU",	me_pagecache_dirty },
	{ mlock,	mlock,		"mlocked LRU",	me_pagecache_clean },

	{ lru|dirty,	lru|dirty,	"LRU",		me_pagecache_dirty },
	{ lru|dirty,	lru,		"clean LRU",	me_pagecache_clean },

	/*
	 * Catchall entry: must be at end.
	 */
	{ 0,		0,		"unknown page state",	me_unknown },
};

static void action_result(unsigned long pfn, char *msg, int result)
{
	struct page *page = pfn_to_page(pfn);

	printk(KERN_ERR "MCE %#lx: %s%s page recovery: %s\n",
		pfn,
		PageDirty(page) ? "dirty " : "",
		msg, action_name[result]);
}

static int page_action(struct page_state *ps, struct page *p,
			unsigned long pfn)
{
	int result;
	int count;

	result = ps->action(p, pfn);
	action_result(pfn, ps->msg, result);

	count = page_count(p) - 1;
	if (count != 0)
		printk(KERN_ERR
		       "MCE %#lx: %s page still referenced by %d users\n",
		       pfn, ps->msg, count);

	/* Could do more checks here if page looks ok */
	/*
	 * Could adjust zone counters here to correct for the missing page.
	 */

	return result == RECOVERED ? 0 : -EBUSY;
}

#define N_UNMAP_TRIES 5

/*
 * Do all that is necessary to remove user space mappings. Unmap
 * the pages and send SIGBUS to the processes if the data was dirty.
 */
static int hwpoison_user_mappings(struct page *p, unsigned long pfn,
				  int trapno)
{
	enum ttu_flags ttu = TTU_UNMAP | TTU_IGNORE_MLOCK | TTU_IGNORE_ACCESS;
	struct address_space *mapping;
	LIST_HEAD(tokill);
	int ret;
	int i;
	int kill = 1;

	if (PageReserved(p) || PageSlab(p))
		return SWAP_SUCCESS;

	/*
	 * This check implies we don't kill processes if their pages
	 * are in the swap cache early. Those are always late kills.
	 */
	if (!page_mapped(p))
		return SWAP_SUCCESS;

	if (PageCompound(p) || PageKsm(p))
		return SWAP_FAIL;

	if (PageSwapCache(p)) {
		printk(KERN_ERR
		       "MCE %#lx: keeping poisoned page in swap cache\n", pfn);
		ttu |= TTU_IGNORE_HWPOISON;
	}

	/*
	 * Propagate the dirty bit from PTEs to struct page first, because we
	 * need this to decide if we should kill or just drop the page.
	 * XXX: the dirty test could be racy: set_page_dirty() may not always
	 * be called inside page lock (it's recommended but not enforced).
	 */
	mapping = page_mapping(p);
	if (!PageDirty(p) && mapping && mapping_cap_writeback_dirty(mapping)) {
		if (page_mkclean(p)) {
			SetPageDirty(p);
		} else {
			kill = 0;
			ttu |= TTU_IGNORE_HWPOISON;
			printk(KERN_INFO
	"MCE %#lx: corrupted page was clean: dropped without side effects\n",
				pfn);
		}
	}

	/*
	 * First collect all the processes that have the page
	 * mapped in dirty form.  This has to be done before try_to_unmap,
	 * because ttu takes the rmap data structures down.
	 *
	 * Error handling: We ignore errors here because
	 * there's nothing that can be done.
	 */
	if (kill)
		collect_procs(p, &tokill);

	/*
	 * try_to_unmap can fail temporarily due to races.
	 * Try a few times (RED-PEN better strategy?)
	 */
	for (i = 0; i < N_UNMAP_TRIES; i++) {
		ret = try_to_unmap(p, ttu);
		if (ret == SWAP_SUCCESS)
			break;
		pr_debug("MCE %#lx: try_to_unmap retry needed %d\n", pfn,  ret);
	}

	if (ret != SWAP_SUCCESS)
		printk(KERN_ERR "MCE %#lx: failed to unmap page (mapcount=%d)\n",
				pfn, page_mapcount(p));

	/*
	 * Now that the dirty bit has been propagated to the
	 * struct page and all unmaps done we can decide if
	 * killing is needed or not.  Only kill when the page
	 * was dirty, otherwise the tokill list is merely
	 * freed.  When there was a problem unmapping earlier
	 * use a more force-full uncatchable kill to prevent
	 * any accesses to the poisoned memory.
	 */
	kill_procs_ao(&tokill, !!PageDirty(p), trapno,
		      ret != SWAP_SUCCESS, pfn);

	return ret;
}

int __memory_failure(unsigned long pfn, int trapno, int flags)
{
	struct page_state *ps;
	struct page *p;
	int res;

	if (!sysctl_memory_failure_recovery)
		panic("Memory failure from trap %d on page %lx", trapno, pfn);

	if (!pfn_valid(pfn)) {
		printk(KERN_ERR
		       "MCE %#lx: memory outside kernel control\n",
		       pfn);
		return -ENXIO;
	}

	p = pfn_to_page(pfn);
	if (TestSetPageHWPoison(p)) {
		action_result(pfn, "already hardware poisoned", IGNORED);
		return 0;
	}

	atomic_long_add(1, &mce_bad_pages);

	/*
	 * We need/can do nothing about count=0 pages.
	 * 1) it's a free page, and therefore in safe hand:
	 *    prep_new_page() will be the gate keeper.
	 * 2) it's part of a non-compound high order page.
	 *    Implies some kernel user: cannot stop them from
	 *    R/W the page; let's pray that the page has been
	 *    used and will be freed some time later.
	 * In fact it's dangerous to directly bump up page count from 0,
	 * that may make page_freeze_refs()/page_unfreeze_refs() mismatch.
	 */
	if (!(flags & MF_COUNT_INCREASED) &&
		!get_page_unless_zero(compound_head(p))) {
		if (is_free_buddy_page(p)) {
			action_result(pfn, "free buddy", DELAYED);
			return 0;
		} else {
			action_result(pfn, "high order kernel", IGNORED);
			return -EBUSY;
		}
	}

	/*
	 * We ignore non-LRU pages for good reasons.
	 * - PG_locked is only well defined for LRU pages and a few others
	 * - to avoid races with __set_page_locked()
	 * - to avoid races with __SetPageSlab*() (and more non-atomic ops)
	 * The check (unnecessarily) ignores LRU pages being isolated and
	 * walked by the page reclaim code, however that's not a big loss.
	 */
	if (!PageLRU(p))
		lru_add_drain_all();
	if (!PageLRU(p)) {
		action_result(pfn, "non LRU", IGNORED);
		put_page(p);
		return -EBUSY;
	}

	/*
	 * Lock the page and wait for writeback to finish.
	 * It's very difficult to mess with pages currently under IO
	 * and in many cases impossible, so we just avoid it here.
	 */
	lock_page_nosync(p);
	wait_on_page_writeback(p);

	/*
	 * Now take care of user space mappings.
	 * Abort on fail: __remove_from_page_cache() assumes unmapped page.
	 */
	if (hwpoison_user_mappings(p, pfn, trapno) != SWAP_SUCCESS) {
		printk(KERN_ERR "MCE %#lx: cannot unmap page, give up\n", pfn);
		res = -EBUSY;
		goto out;
	}

	/*
	 * Torn down by someone else?
	 */
	if (PageLRU(p) && !PageSwapCache(p) && p->mapping == NULL) {
		action_result(pfn, "already truncated LRU", IGNORED);
		res = 0;
		goto out;
	}

	res = -EBUSY;
	for (ps = error_states;; ps++) {
		if ((p->flags & ps->mask) == ps->res) {
			res = page_action(ps, p, pfn);
			break;
		}
	}
out:
	unlock_page(p);
	return res;
}
EXPORT_SYMBOL_GPL(__memory_failure);

/**
 * memory_failure - Handle memory failure of a page.
 * @pfn: Page Number of the corrupted page
 * @trapno: Trap number reported in the signal to user space.
 *
 * This function is called by the low level machine check code
 * of an architecture when it detects hardware memory corruption
 * of a page. It tries its best to recover, which includes
 * dropping pages, killing processes etc.
 *
 * The function is primarily of use for corruptions that
 * happen outside the current execution context (e.g. when
 * detected by a background scrubber)
 *
 * Must run in process context (e.g. a work queue) with interrupts
 * enabled and no spinlocks hold.
 */
void memory_failure(unsigned long pfn, int trapno)
{
	__memory_failure(pfn, trapno, 0);
}
