/*
 *  linux/fs/nfs/iostat.h
 *
 *  Declarations for NFS client per-mount statistics
 *
 *  Copyright (C) 2005, 2006 Chuck Lever <cel@netapp.com>
 *
 */

#ifndef _NFS_IOSTAT
#define _NFS_IOSTAT

#include <linux/percpu.h>
#include <linux/cache.h>
#include <linux/nfs_iostat.h>

struct nfs_iostats {
	unsigned long long	bytes[__NFSIOS_BYTESMAX];
	unsigned long		events[__NFSIOS_COUNTSMAX];
} ____cacheline_aligned;

static inline void nfs_inc_server_stats(struct nfs_server *server,
					enum nfs_stat_eventcounters stat)
{
	struct nfs_iostats *iostats;
	int cpu;

	cpu = get_cpu();
	iostats = per_cpu_ptr(server->io_stats, cpu);
	iostats->events[stat]++;
	put_cpu_no_resched();
}

static inline void nfs_inc_stats(struct inode *inode,
				 enum nfs_stat_eventcounters stat)
{
	nfs_inc_server_stats(NFS_SERVER(inode), stat);
}

static inline void nfs_add_server_stats(struct nfs_server *server,
					enum nfs_stat_bytecounters stat,
					unsigned long addend)
{
	struct nfs_iostats *iostats;
	int cpu;

	cpu = get_cpu();
	iostats = per_cpu_ptr(server->io_stats, cpu);
	iostats->bytes[stat] += addend;
	put_cpu_no_resched();
}

static inline void nfs_add_stats(struct inode *inode,
				 enum nfs_stat_bytecounters stat,
				 unsigned long addend)
{
	nfs_add_server_stats(NFS_SERVER(inode), stat, addend);
}

static inline struct nfs_iostats *nfs_alloc_iostats(void)
{
	return alloc_percpu(struct nfs_iostats);
}

static inline void nfs_free_iostats(struct nfs_iostats *stats)
{
	if (stats != NULL)
		free_percpu(stats);
}

#endif /* _NFS_IOSTAT */
