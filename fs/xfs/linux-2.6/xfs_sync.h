/*
 * Copyright (c) 2000-2006 Silicon Graphics, Inc.
 * All Rights Reserved.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it would be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write the Free Software Foundation,
 * Inc.,  51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 */
#ifndef XFS_SYNC_H
#define XFS_SYNC_H 1

struct xfs_mount;

typedef struct bhv_vfs_sync_work {
	struct list_head	w_list;
	struct xfs_mount	*w_mount;
	void			*w_data;	/* syncer routine argument */
	void			(*w_syncer)(struct xfs_mount *, void *);
} bhv_vfs_sync_work_t;

#define SYNC_ATTR		0x0001	/* sync attributes */
#define SYNC_DELWRI		0x0004	/* look at delayed writes */
#define SYNC_WAIT		0x0008	/* wait for i/o to complete */
#define SYNC_BDFLUSH		0x0010	/* BDFLUSH is calling -- don't block */
#define SYNC_FSDATA		0x0020	/* flush fs data (e.g. superblocks) */
#define SYNC_REFCACHE		0x0040  /* prune some of the nfs ref cache */
#define SYNC_REMOUNT		0x0080  /* remount readonly, no dummy LRs */
#define SYNC_IOWAIT		0x0100  /* wait for all I/O to complete */

/*
 * When remounting a filesystem read-only or freezing the filesystem,
 * we have two phases to execute. This first phase is syncing the data
 * before we quiesce the fielsystem, and the second is flushing all the
 * inodes out after we've waited for all the transactions created by
 * the first phase to complete. The second phase uses SYNC_INODE_QUIESCE
 * to ensure that the inodes are written to their location on disk
 * rather than just existing in transactions in the log. This means
 * after a quiesce there is no log replay required to write the inodes
 * to disk (this is the main difference between a sync and a quiesce).
 */
#define SYNC_DATA_QUIESCE	(SYNC_DELWRI|SYNC_FSDATA|SYNC_WAIT|SYNC_IOWAIT)

int xfs_syncd_init(struct xfs_mount *mp);
void xfs_syncd_stop(struct xfs_mount *mp);

int xfs_sync(struct xfs_mount *mp, int flags);
int xfs_sync_inodes(struct xfs_mount *mp, int flags);
int xfs_sync_fsdata(struct xfs_mount *mp, int flags);

int xfs_quiesce_data(struct xfs_mount *mp);

void xfs_flush_inode(struct xfs_inode *ip);
void xfs_flush_device(struct xfs_inode *ip);

#endif
