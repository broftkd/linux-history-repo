/*
 * Copyright (C) 2007 Oracle.  All rights reserved.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public
 * License v2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public
 * License along with this program; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 021110-1307, USA.
 */
#include <linux/sched.h>
#include <linux/pagemap.h>
#include <linux/writeback.h>
#include "hash.h"
#include "crc32c.h"
#include "ctree.h"
#include "disk-io.h"
#include "print-tree.h"
#include "transaction.h"
#include "volumes.h"
#include "locking.h"
#include "ref-cache.h"

#define BLOCK_GROUP_DATA     EXTENT_WRITEBACK
#define BLOCK_GROUP_METADATA EXTENT_UPTODATE
#define BLOCK_GROUP_SYSTEM   EXTENT_NEW

#define BLOCK_GROUP_DIRTY EXTENT_DIRTY

static int finish_current_insert(struct btrfs_trans_handle *trans, struct
				 btrfs_root *extent_root);
static int del_pending_extents(struct btrfs_trans_handle *trans, struct
			       btrfs_root *extent_root);
static struct btrfs_block_group_cache *
__btrfs_find_block_group(struct btrfs_root *root,
			 struct btrfs_block_group_cache *hint,
			 u64 search_start, int data, int owner);

void maybe_lock_mutex(struct btrfs_root *root)
{
	if (root != root->fs_info->extent_root &&
	    root != root->fs_info->chunk_root &&
	    root != root->fs_info->dev_root) {
		mutex_lock(&root->fs_info->alloc_mutex);
	}
}

void maybe_unlock_mutex(struct btrfs_root *root)
{
	if (root != root->fs_info->extent_root &&
	    root != root->fs_info->chunk_root &&
	    root != root->fs_info->dev_root) {
		mutex_unlock(&root->fs_info->alloc_mutex);
	}
}

static int cache_block_group(struct btrfs_root *root,
			     struct btrfs_block_group_cache *block_group)
{
	struct btrfs_path *path;
	int ret;
	struct btrfs_key key;
	struct extent_buffer *leaf;
	struct extent_io_tree *free_space_cache;
	int slot;
	u64 last = 0;
	u64 hole_size;
	u64 first_free;
	int found = 0;

	if (!block_group)
		return 0;

	root = root->fs_info->extent_root;
	free_space_cache = &root->fs_info->free_space_cache;

	if (block_group->cached)
		return 0;

	path = btrfs_alloc_path();
	if (!path)
		return -ENOMEM;

	path->reada = 2;
	/*
	 * we get into deadlocks with paths held by callers of this function.
	 * since the alloc_mutex is protecting things right now, just
	 * skip the locking here
	 */
	path->skip_locking = 1;
	first_free = block_group->key.objectid;
	key.objectid = block_group->key.objectid;
	key.offset = 0;
	btrfs_set_key_type(&key, BTRFS_EXTENT_ITEM_KEY);
	ret = btrfs_search_slot(NULL, root, &key, path, 0, 0);
	if (ret < 0)
		return ret;
	ret = btrfs_previous_item(root, path, 0, BTRFS_EXTENT_ITEM_KEY);
	if (ret < 0)
		return ret;
	if (ret == 0) {
		leaf = path->nodes[0];
		btrfs_item_key_to_cpu(leaf, &key, path->slots[0]);
		if (key.objectid + key.offset > first_free)
			first_free = key.objectid + key.offset;
	}
	while(1) {
		leaf = path->nodes[0];
		slot = path->slots[0];
		if (slot >= btrfs_header_nritems(leaf)) {
			ret = btrfs_next_leaf(root, path);
			if (ret < 0)
				goto err;
			if (ret == 0) {
				continue;
			} else {
				break;
			}
		}
		btrfs_item_key_to_cpu(leaf, &key, slot);
		if (key.objectid < block_group->key.objectid) {
			goto next;
		}
		if (key.objectid >= block_group->key.objectid +
		    block_group->key.offset) {
			break;
		}

		if (btrfs_key_type(&key) == BTRFS_EXTENT_ITEM_KEY) {
			if (!found) {
				last = first_free;
				found = 1;
			}
			if (key.objectid > last) {
				hole_size = key.objectid - last;
				set_extent_dirty(free_space_cache, last,
						 last + hole_size - 1,
						 GFP_NOFS);
			}
			last = key.objectid + key.offset;
		}
next:
		path->slots[0]++;
	}

	if (!found)
		last = first_free;
	if (block_group->key.objectid +
	    block_group->key.offset > last) {
		hole_size = block_group->key.objectid +
			block_group->key.offset - last;
		set_extent_dirty(free_space_cache, last,
				 last + hole_size - 1, GFP_NOFS);
	}
	block_group->cached = 1;
err:
	btrfs_free_path(path);
	return 0;
}

struct btrfs_block_group_cache *btrfs_lookup_first_block_group(struct
						       btrfs_fs_info *info,
							 u64 bytenr)
{
	struct extent_io_tree *block_group_cache;
	struct btrfs_block_group_cache *block_group = NULL;
	u64 ptr;
	u64 start;
	u64 end;
	int ret;

	bytenr = max_t(u64, bytenr,
		       BTRFS_SUPER_INFO_OFFSET + BTRFS_SUPER_INFO_SIZE);
	block_group_cache = &info->block_group_cache;
	ret = find_first_extent_bit(block_group_cache,
				    bytenr, &start, &end,
				    BLOCK_GROUP_DATA | BLOCK_GROUP_METADATA |
				    BLOCK_GROUP_SYSTEM);
	if (ret) {
		return NULL;
	}
	ret = get_state_private(block_group_cache, start, &ptr);
	if (ret)
		return NULL;

	block_group = (struct btrfs_block_group_cache *)(unsigned long)ptr;
	return block_group;
}

struct btrfs_block_group_cache *btrfs_lookup_block_group(struct
							 btrfs_fs_info *info,
							 u64 bytenr)
{
	struct extent_io_tree *block_group_cache;
	struct btrfs_block_group_cache *block_group = NULL;
	u64 ptr;
	u64 start;
	u64 end;
	int ret;

	bytenr = max_t(u64, bytenr,
		       BTRFS_SUPER_INFO_OFFSET + BTRFS_SUPER_INFO_SIZE);
	block_group_cache = &info->block_group_cache;
	ret = find_first_extent_bit(block_group_cache,
				    bytenr, &start, &end,
				    BLOCK_GROUP_DATA | BLOCK_GROUP_METADATA |
				    BLOCK_GROUP_SYSTEM);
	if (ret) {
		return NULL;
	}
	ret = get_state_private(block_group_cache, start, &ptr);
	if (ret)
		return NULL;

	block_group = (struct btrfs_block_group_cache *)(unsigned long)ptr;
	if (block_group->key.objectid <= bytenr && bytenr <
	    block_group->key.objectid + block_group->key.offset)
		return block_group;
	return NULL;
}

static int block_group_bits(struct btrfs_block_group_cache *cache, u64 bits)
{
	return (cache->flags & bits) == bits;
}

static int noinline find_search_start(struct btrfs_root *root,
			      struct btrfs_block_group_cache **cache_ret,
			      u64 *start_ret, u64 num, int data)
{
	int ret;
	struct btrfs_block_group_cache *cache = *cache_ret;
	struct extent_io_tree *free_space_cache;
	struct extent_state *state;
	u64 last;
	u64 start = 0;
	u64 cache_miss = 0;
	u64 total_fs_bytes;
	u64 search_start = *start_ret;
	int wrapped = 0;

	WARN_ON(!mutex_is_locked(&root->fs_info->alloc_mutex));
	total_fs_bytes = btrfs_super_total_bytes(&root->fs_info->super_copy);
	free_space_cache = &root->fs_info->free_space_cache;

	if (!cache)
		goto out;

again:
	ret = cache_block_group(root, cache);
	if (ret) {
		goto out;
	}

	last = max(search_start, cache->key.objectid);
	if (!block_group_bits(cache, data) || cache->ro)
		goto new_group;

	spin_lock_irq(&free_space_cache->lock);
	state = find_first_extent_bit_state(free_space_cache, last, EXTENT_DIRTY);
	while(1) {
		if (!state) {
			if (!cache_miss)
				cache_miss = last;
			spin_unlock_irq(&free_space_cache->lock);
			goto new_group;
		}

		start = max(last, state->start);
		last = state->end + 1;
		if (last - start < num) {
			do {
				state = extent_state_next(state);
			} while(state && !(state->state & EXTENT_DIRTY));
			continue;
		}
		spin_unlock_irq(&free_space_cache->lock);
		if (cache->ro) {
			goto new_group;
		}
		if (start + num > cache->key.objectid + cache->key.offset)
			goto new_group;
		if (!block_group_bits(cache, data)) {
			printk("block group bits don't match %Lu %d\n", cache->flags, data);
		}
		*start_ret = start;
		return 0;
	}
out:
	cache = btrfs_lookup_block_group(root->fs_info, search_start);
	if (!cache) {
		printk("Unable to find block group for %Lu\n", search_start);
		WARN_ON(1);
	}
	return -ENOSPC;

new_group:
	last = cache->key.objectid + cache->key.offset;
wrapped:
	cache = btrfs_lookup_first_block_group(root->fs_info, last);
	if (!cache || cache->key.objectid >= total_fs_bytes) {
no_cache:
		if (!wrapped) {
			wrapped = 1;
			last = search_start;
			goto wrapped;
		}
		goto out;
	}
	if (cache_miss && !cache->cached) {
		cache_block_group(root, cache);
		last = cache_miss;
		cache = btrfs_lookup_first_block_group(root->fs_info, last);
	}
	cache_miss = 0;
	cache = btrfs_find_block_group(root, cache, last, data, 0);
	if (!cache)
		goto no_cache;
	*cache_ret = cache;
	goto again;
}

static u64 div_factor(u64 num, int factor)
{
	if (factor == 10)
		return num;
	num *= factor;
	do_div(num, 10);
	return num;
}

static int block_group_state_bits(u64 flags)
{
	int bits = 0;
	if (flags & BTRFS_BLOCK_GROUP_DATA)
		bits |= BLOCK_GROUP_DATA;
	if (flags & BTRFS_BLOCK_GROUP_METADATA)
		bits |= BLOCK_GROUP_METADATA;
	if (flags & BTRFS_BLOCK_GROUP_SYSTEM)
		bits |= BLOCK_GROUP_SYSTEM;
	return bits;
}

static struct btrfs_block_group_cache *
__btrfs_find_block_group(struct btrfs_root *root,
			 struct btrfs_block_group_cache *hint,
			 u64 search_start, int data, int owner)
{
	struct btrfs_block_group_cache *cache;
	struct extent_io_tree *block_group_cache;
	struct btrfs_block_group_cache *found_group = NULL;
	struct btrfs_fs_info *info = root->fs_info;
	u64 used;
	u64 last = 0;
	u64 start;
	u64 end;
	u64 free_check;
	u64 ptr;
	int bit;
	int ret;
	int full_search = 0;
	int factor = 10;
	int wrapped = 0;

	block_group_cache = &info->block_group_cache;

	if (data & BTRFS_BLOCK_GROUP_METADATA)
		factor = 9;

	bit = block_group_state_bits(data);

	if (search_start) {
		struct btrfs_block_group_cache *shint;
		shint = btrfs_lookup_first_block_group(info, search_start);
		if (shint && block_group_bits(shint, data) && !shint->ro) {
			spin_lock(&shint->lock);
			used = btrfs_block_group_used(&shint->item);
			if (used + shint->pinned <
			    div_factor(shint->key.offset, factor)) {
				spin_unlock(&shint->lock);
				return shint;
			}
			spin_unlock(&shint->lock);
		}
	}
	if (hint && !hint->ro && block_group_bits(hint, data)) {
		spin_lock(&hint->lock);
		used = btrfs_block_group_used(&hint->item);
		if (used + hint->pinned <
		    div_factor(hint->key.offset, factor)) {
			spin_unlock(&hint->lock);
			return hint;
		}
		spin_unlock(&hint->lock);
		last = hint->key.objectid + hint->key.offset;
	} else {
		if (hint)
			last = max(hint->key.objectid, search_start);
		else
			last = search_start;
	}
again:
	while(1) {
		ret = find_first_extent_bit(block_group_cache, last,
					    &start, &end, bit);
		if (ret)
			break;

		ret = get_state_private(block_group_cache, start, &ptr);
		if (ret) {
			last = end + 1;
			continue;
		}

		cache = (struct btrfs_block_group_cache *)(unsigned long)ptr;
		spin_lock(&cache->lock);
		last = cache->key.objectid + cache->key.offset;
		used = btrfs_block_group_used(&cache->item);

		if (!cache->ro && block_group_bits(cache, data)) {
			free_check = div_factor(cache->key.offset, factor);
			if (used + cache->pinned < free_check) {
				found_group = cache;
				spin_unlock(&cache->lock);
				goto found;
			}
		}
		spin_unlock(&cache->lock);
		cond_resched();
	}
	if (!wrapped) {
		last = search_start;
		wrapped = 1;
		goto again;
	}
	if (!full_search && factor < 10) {
		last = search_start;
		full_search = 1;
		factor = 10;
		goto again;
	}
found:
	return found_group;
}

struct btrfs_block_group_cache *btrfs_find_block_group(struct btrfs_root *root,
						 struct btrfs_block_group_cache
						 *hint, u64 search_start,
						 int data, int owner)
{

	struct btrfs_block_group_cache *ret;
	ret = __btrfs_find_block_group(root, hint, search_start, data, owner);
	return ret;
}
static u64 hash_extent_ref(u64 root_objectid, u64 ref_generation,
			   u64 owner, u64 owner_offset)
{
	u32 high_crc = ~(u32)0;
	u32 low_crc = ~(u32)0;
	__le64 lenum;
	lenum = cpu_to_le64(root_objectid);
	high_crc = btrfs_crc32c(high_crc, &lenum, sizeof(lenum));
	lenum = cpu_to_le64(ref_generation);
	low_crc = btrfs_crc32c(low_crc, &lenum, sizeof(lenum));
	if (owner >= BTRFS_FIRST_FREE_OBJECTID) {
		lenum = cpu_to_le64(owner);
		low_crc = btrfs_crc32c(low_crc, &lenum, sizeof(lenum));
		lenum = cpu_to_le64(owner_offset);
		low_crc = btrfs_crc32c(low_crc, &lenum, sizeof(lenum));
	}
	return ((u64)high_crc << 32) | (u64)low_crc;
}

static int match_extent_ref(struct extent_buffer *leaf,
			    struct btrfs_extent_ref *disk_ref,
			    struct btrfs_extent_ref *cpu_ref)
{
	int ret;
	int len;

	if (cpu_ref->objectid)
		len = sizeof(*cpu_ref);
	else
		len = 2 * sizeof(u64);
	ret = memcmp_extent_buffer(leaf, cpu_ref, (unsigned long)disk_ref,
				   len);
	return ret == 0;
}

static int noinline lookup_extent_backref(struct btrfs_trans_handle *trans,
					  struct btrfs_root *root,
					  struct btrfs_path *path, u64 bytenr,
					  u64 root_objectid,
					  u64 ref_generation, u64 owner,
					  u64 owner_offset, int del)
{
	u64 hash;
	struct btrfs_key key;
	struct btrfs_key found_key;
	struct btrfs_extent_ref ref;
	struct extent_buffer *leaf;
	struct btrfs_extent_ref *disk_ref;
	int ret;
	int ret2;

	btrfs_set_stack_ref_root(&ref, root_objectid);
	btrfs_set_stack_ref_generation(&ref, ref_generation);
	btrfs_set_stack_ref_objectid(&ref, owner);
	btrfs_set_stack_ref_offset(&ref, owner_offset);

	hash = hash_extent_ref(root_objectid, ref_generation, owner,
			       owner_offset);
	key.offset = hash;
	key.objectid = bytenr;
	key.type = BTRFS_EXTENT_REF_KEY;

	while (1) {
		ret = btrfs_search_slot(trans, root, &key, path,
					del ? -1 : 0, del);
		if (ret < 0)
			goto out;
		leaf = path->nodes[0];
		if (ret != 0) {
			u32 nritems = btrfs_header_nritems(leaf);
			if (path->slots[0] >= nritems) {
				ret2 = btrfs_next_leaf(root, path);
				if (ret2)
					goto out;
				leaf = path->nodes[0];
			}
			btrfs_item_key_to_cpu(leaf, &found_key, path->slots[0]);
			if (found_key.objectid != bytenr ||
			    found_key.type != BTRFS_EXTENT_REF_KEY)
				goto out;
			key.offset = found_key.offset;
			if (del) {
				btrfs_release_path(root, path);
				continue;
			}
		}
		disk_ref = btrfs_item_ptr(path->nodes[0],
					  path->slots[0],
					  struct btrfs_extent_ref);
		if (match_extent_ref(path->nodes[0], disk_ref, &ref)) {
			ret = 0;
			goto out;
		}
		btrfs_item_key_to_cpu(leaf, &found_key, path->slots[0]);
		key.offset = found_key.offset + 1;
		btrfs_release_path(root, path);
	}
out:
	return ret;
}

/*
 * Back reference rules.  Back refs have three main goals:
 *
 * 1) differentiate between all holders of references to an extent so that
 *    when a reference is dropped we can make sure it was a valid reference
 *    before freeing the extent.
 *
 * 2) Provide enough information to quickly find the holders of an extent
 *    if we notice a given block is corrupted or bad.
 *
 * 3) Make it easy to migrate blocks for FS shrinking or storage pool
 *    maintenance.  This is actually the same as #2, but with a slightly
 *    different use case.
 *
 * File extents can be referenced by:
 *
 * - multiple snapshots, subvolumes, or different generations in one subvol
 * - different files inside a single subvolume (in theory, not implemented yet)
 * - different offsets inside a file (bookend extents in file.c)
 *
 * The extent ref structure has fields for:
 *
 * - Objectid of the subvolume root
 * - Generation number of the tree holding the reference
 * - objectid of the file holding the reference
 * - offset in the file corresponding to the key holding the reference
 *
 * When a file extent is allocated the fields are filled in:
 *     (root_key.objectid, trans->transid, inode objectid, offset in file)
 *
 * When a leaf is cow'd new references are added for every file extent found
 * in the leaf.  It looks the same as the create case, but trans->transid
 * will be different when the block is cow'd.
 *
 *     (root_key.objectid, trans->transid, inode objectid, offset in file)
 *
 * When a file extent is removed either during snapshot deletion or file
 * truncation, the corresponding back reference is found
 * by searching for:
 *
 *     (btrfs_header_owner(leaf), btrfs_header_generation(leaf),
 *      inode objectid, offset in file)
 *
 * Btree extents can be referenced by:
 *
 * - Different subvolumes
 * - Different generations of the same subvolume
 *
 * Storing sufficient information for a full reverse mapping of a btree
 * block would require storing the lowest key of the block in the backref,
 * and it would require updating that lowest key either before write out or
 * every time it changed.  Instead, the objectid of the lowest key is stored
 * along with the level of the tree block.  This provides a hint
 * about where in the btree the block can be found.  Searches through the
 * btree only need to look for a pointer to that block, so they stop one
 * level higher than the level recorded in the backref.
 *
 * Some btrees do not do reference counting on their extents.  These
 * include the extent tree and the tree of tree roots.  Backrefs for these
 * trees always have a generation of zero.
 *
 * When a tree block is created, back references are inserted:
 *
 * (root->root_key.objectid, trans->transid or zero, level, lowest_key_objectid)
 *
 * When a tree block is cow'd in a reference counted root,
 * new back references are added for all the blocks it points to.
 * These are of the form (trans->transid will have increased since creation):
 *
 * (root->root_key.objectid, trans->transid, level, lowest_key_objectid)
 *
 * Because the lowest_key_objectid and the level are just hints
 * they are not used when backrefs are deleted.  When a backref is deleted:
 *
 * if backref was for a tree root:
 *     root_objectid = root->root_key.objectid
 * else
 *     root_objectid = btrfs_header_owner(parent)
 *
 * (root_objectid, btrfs_header_generation(parent) or zero, 0, 0)
 *
 * Back Reference Key hashing:
 *
 * Back references have four fields, each 64 bits long.  Unfortunately,
 * This is hashed into a single 64 bit number and placed into the key offset.
 * The key objectid corresponds to the first byte in the extent, and the
 * key type is set to BTRFS_EXTENT_REF_KEY
 */
int btrfs_insert_extent_backref(struct btrfs_trans_handle *trans,
				 struct btrfs_root *root,
				 struct btrfs_path *path, u64 bytenr,
				 u64 root_objectid, u64 ref_generation,
				 u64 owner, u64 owner_offset)
{
	u64 hash;
	struct btrfs_key key;
	struct btrfs_extent_ref ref;
	struct btrfs_extent_ref *disk_ref;
	int ret;

	btrfs_set_stack_ref_root(&ref, root_objectid);
	btrfs_set_stack_ref_generation(&ref, ref_generation);
	btrfs_set_stack_ref_objectid(&ref, owner);
	btrfs_set_stack_ref_offset(&ref, owner_offset);

	hash = hash_extent_ref(root_objectid, ref_generation, owner,
			       owner_offset);
	key.offset = hash;
	key.objectid = bytenr;
	key.type = BTRFS_EXTENT_REF_KEY;

	ret = btrfs_insert_empty_item(trans, root, path, &key, sizeof(ref));
	while (ret == -EEXIST) {
		disk_ref = btrfs_item_ptr(path->nodes[0], path->slots[0],
					  struct btrfs_extent_ref);
		if (match_extent_ref(path->nodes[0], disk_ref, &ref))
			goto out;
		key.offset++;
		btrfs_release_path(root, path);
		ret = btrfs_insert_empty_item(trans, root, path, &key,
					      sizeof(ref));
	}
	if (ret)
		goto out;
	disk_ref = btrfs_item_ptr(path->nodes[0], path->slots[0],
				  struct btrfs_extent_ref);
	write_extent_buffer(path->nodes[0], &ref, (unsigned long)disk_ref,
			    sizeof(ref));
	btrfs_mark_buffer_dirty(path->nodes[0]);
out:
	btrfs_release_path(root, path);
	return ret;
}

static int __btrfs_inc_extent_ref(struct btrfs_trans_handle *trans,
				struct btrfs_root *root,
				u64 bytenr, u64 num_bytes,
				u64 root_objectid, u64 ref_generation,
				u64 owner, u64 owner_offset)
{
	struct btrfs_path *path;
	int ret;
	struct btrfs_key key;
	struct extent_buffer *l;
	struct btrfs_extent_item *item;
	u32 refs;

	WARN_ON(num_bytes < root->sectorsize);
	path = btrfs_alloc_path();
	if (!path)
		return -ENOMEM;

	path->reada = 1;
	key.objectid = bytenr;
	btrfs_set_key_type(&key, BTRFS_EXTENT_ITEM_KEY);
	key.offset = num_bytes;
	ret = btrfs_search_slot(trans, root->fs_info->extent_root, &key, path,
				0, 1);
	if (ret < 0)
		return ret;
	if (ret != 0) {
		BUG();
	}
	BUG_ON(ret != 0);
	l = path->nodes[0];
	item = btrfs_item_ptr(l, path->slots[0], struct btrfs_extent_item);
	refs = btrfs_extent_refs(l, item);
	btrfs_set_extent_refs(l, item, refs + 1);
	btrfs_mark_buffer_dirty(path->nodes[0]);

	btrfs_release_path(root->fs_info->extent_root, path);

	path->reada = 1;
	ret = btrfs_insert_extent_backref(trans, root->fs_info->extent_root,
					  path, bytenr, root_objectid,
					  ref_generation, owner, owner_offset);
	BUG_ON(ret);
	finish_current_insert(trans, root->fs_info->extent_root);
	del_pending_extents(trans, root->fs_info->extent_root);

	btrfs_free_path(path);
	return 0;
}

int btrfs_inc_extent_ref(struct btrfs_trans_handle *trans,
				struct btrfs_root *root,
				u64 bytenr, u64 num_bytes,
				u64 root_objectid, u64 ref_generation,
				u64 owner, u64 owner_offset)
{
	int ret;

	mutex_lock(&root->fs_info->alloc_mutex);
	ret = __btrfs_inc_extent_ref(trans, root, bytenr, num_bytes,
				     root_objectid, ref_generation,
				     owner, owner_offset);
	mutex_unlock(&root->fs_info->alloc_mutex);
	return ret;
}

int btrfs_extent_post_op(struct btrfs_trans_handle *trans,
			 struct btrfs_root *root)
{
	finish_current_insert(trans, root->fs_info->extent_root);
	del_pending_extents(trans, root->fs_info->extent_root);
	return 0;
}

static int lookup_extent_ref(struct btrfs_trans_handle *trans,
			     struct btrfs_root *root, u64 bytenr,
			     u64 num_bytes, u32 *refs)
{
	struct btrfs_path *path;
	int ret;
	struct btrfs_key key;
	struct extent_buffer *l;
	struct btrfs_extent_item *item;

	WARN_ON(num_bytes < root->sectorsize);
	path = btrfs_alloc_path();
	path->reada = 1;
	key.objectid = bytenr;
	key.offset = num_bytes;
	btrfs_set_key_type(&key, BTRFS_EXTENT_ITEM_KEY);
	ret = btrfs_search_slot(trans, root->fs_info->extent_root, &key, path,
				0, 0);
	if (ret < 0)
		goto out;
	if (ret != 0) {
		btrfs_print_leaf(root, path->nodes[0]);
		printk("failed to find block number %Lu\n", bytenr);
		BUG();
	}
	l = path->nodes[0];
	item = btrfs_item_ptr(l, path->slots[0], struct btrfs_extent_item);
	*refs = btrfs_extent_refs(l, item);
out:
	btrfs_free_path(path);
	return 0;
}


static int get_reference_status(struct btrfs_root *root, u64 bytenr,
				u64 parent_gen, u64 ref_objectid,
			        u64 *min_generation, u32 *ref_count)
{
	struct btrfs_root *extent_root = root->fs_info->extent_root;
	struct btrfs_path *path;
	struct extent_buffer *leaf;
	struct btrfs_extent_ref *ref_item;
	struct btrfs_key key;
	struct btrfs_key found_key;
	u64 root_objectid = root->root_key.objectid;
	u64 ref_generation;
	u32 nritems;
	int ret;

	key.objectid = bytenr;
	key.offset = 0;
	key.type = BTRFS_EXTENT_ITEM_KEY;

	path = btrfs_alloc_path();
	mutex_lock(&root->fs_info->alloc_mutex);
	ret = btrfs_search_slot(NULL, extent_root, &key, path, 0, 0);
	if (ret < 0)
		goto out;
	BUG_ON(ret == 0);

	leaf = path->nodes[0];
	btrfs_item_key_to_cpu(leaf, &found_key, path->slots[0]);

	if (found_key.objectid != bytenr ||
	    found_key.type != BTRFS_EXTENT_ITEM_KEY) {
		ret = 1;
		goto out;
	}

	*ref_count = 0;
	*min_generation = (u64)-1;

	while (1) {
		leaf = path->nodes[0];
		nritems = btrfs_header_nritems(leaf);
		if (path->slots[0] >= nritems) {
			ret = btrfs_next_leaf(extent_root, path);
			if (ret < 0)
				goto out;
			if (ret == 0)
				continue;
			break;
		}
		btrfs_item_key_to_cpu(leaf, &found_key, path->slots[0]);
		if (found_key.objectid != bytenr)
			break;

		if (found_key.type != BTRFS_EXTENT_REF_KEY) {
			path->slots[0]++;
			continue;
		}

		ref_item = btrfs_item_ptr(leaf, path->slots[0],
					  struct btrfs_extent_ref);
		ref_generation = btrfs_ref_generation(leaf, ref_item);
		/*
		 * For (parent_gen > 0 && parent_gen > ref_gen):
		 *
		 * we reach here through the oldest root, therefore
		 * all other reference from same snapshot should have
		 * a larger generation.
		 */
		if ((root_objectid != btrfs_ref_root(leaf, ref_item)) ||
		    (parent_gen > 0 && parent_gen > ref_generation) ||
		    (ref_objectid >= BTRFS_FIRST_FREE_OBJECTID &&
		     ref_objectid != btrfs_ref_objectid(leaf, ref_item))) {
			if (ref_count)
				*ref_count = 2;
			break;
		}

		*ref_count = 1;
		if (*min_generation > ref_generation)
			*min_generation = ref_generation;

		path->slots[0]++;
	}
	ret = 0;
out:
	mutex_unlock(&root->fs_info->alloc_mutex);
	btrfs_free_path(path);
	return ret;
}

int btrfs_cross_ref_exists(struct btrfs_root *root,
			   struct btrfs_key *key, u64 bytenr)
{
	struct btrfs_trans_handle *trans;
	struct btrfs_root *old_root;
	struct btrfs_path *path = NULL;
	struct extent_buffer *eb;
	struct btrfs_file_extent_item *item;
	u64 ref_generation;
	u64 min_generation;
	u64 extent_start;
	u32 ref_count;
	int level;
	int ret;

	BUG_ON(key->type != BTRFS_EXTENT_DATA_KEY);
	ret = get_reference_status(root, bytenr, 0, key->objectid,
				   &min_generation, &ref_count);
	if (ret)
		return ret;

	if (ref_count != 1)
		return 1;

	trans = btrfs_start_transaction(root, 0);
	old_root = root->dirty_root->root;
	ref_generation = old_root->root_key.offset;

	/* all references are created in running transaction */
	if (min_generation > ref_generation) {
		ret = 0;
		goto out;
	}

	path = btrfs_alloc_path();
	if (!path) {
		ret = -ENOMEM;
		goto out;
	}

	path->skip_locking = 1;
	/* if no item found, the extent is referenced by other snapshot */
	ret = btrfs_search_slot(NULL, old_root, key, path, 0, 0);
	if (ret)
		goto out;

	eb = path->nodes[0];
	item = btrfs_item_ptr(eb, path->slots[0],
			      struct btrfs_file_extent_item);
	if (btrfs_file_extent_type(eb, item) != BTRFS_FILE_EXTENT_REG ||
	    btrfs_file_extent_disk_bytenr(eb, item) != bytenr) {
		ret = 1;
		goto out;
	}

	for (level = BTRFS_MAX_LEVEL - 1; level >= -1; level--) {
		if (level >= 0) {
			eb = path->nodes[level];
			if (!eb)
				continue;
			extent_start = eb->start;
		} else
			extent_start = bytenr;

		ret = get_reference_status(root, extent_start, ref_generation,
					   0, &min_generation, &ref_count);
		if (ret)
			goto out;

		if (ref_count != 1) {
			ret = 1;
			goto out;
		}
		if (level >= 0)
			ref_generation = btrfs_header_generation(eb);
	}
	ret = 0;
out:
	if (path)
		btrfs_free_path(path);
	btrfs_end_transaction(trans, root);
	return ret;
}

int btrfs_inc_ref(struct btrfs_trans_handle *trans, struct btrfs_root *root,
		  struct extent_buffer *buf, int cache_ref)
{
	u64 bytenr;
	u32 nritems;
	struct btrfs_key key;
	struct btrfs_file_extent_item *fi;
	int i;
	int level;
	int ret;
	int faili;
	int nr_file_extents = 0;

	if (!root->ref_cows)
		return 0;

	level = btrfs_header_level(buf);
	nritems = btrfs_header_nritems(buf);
	for (i = 0; i < nritems; i++) {
		cond_resched();
		if (level == 0) {
			u64 disk_bytenr;
			btrfs_item_key_to_cpu(buf, &key, i);
			if (btrfs_key_type(&key) != BTRFS_EXTENT_DATA_KEY)
				continue;
			fi = btrfs_item_ptr(buf, i,
					    struct btrfs_file_extent_item);
			if (btrfs_file_extent_type(buf, fi) ==
			    BTRFS_FILE_EXTENT_INLINE)
				continue;
			disk_bytenr = btrfs_file_extent_disk_bytenr(buf, fi);
			if (disk_bytenr == 0)
				continue;

			if (buf != root->commit_root)
				nr_file_extents++;

			mutex_lock(&root->fs_info->alloc_mutex);
			ret = __btrfs_inc_extent_ref(trans, root, disk_bytenr,
				    btrfs_file_extent_disk_num_bytes(buf, fi),
				    root->root_key.objectid, trans->transid,
				    key.objectid, key.offset);
			mutex_unlock(&root->fs_info->alloc_mutex);
			if (ret) {
				faili = i;
				WARN_ON(1);
				goto fail;
			}
		} else {
			bytenr = btrfs_node_blockptr(buf, i);
			btrfs_node_key_to_cpu(buf, &key, i);

			mutex_lock(&root->fs_info->alloc_mutex);
			ret = __btrfs_inc_extent_ref(trans, root, bytenr,
					   btrfs_level_size(root, level - 1),
					   root->root_key.objectid,
					   trans->transid,
					   level - 1, key.objectid);
			mutex_unlock(&root->fs_info->alloc_mutex);
			if (ret) {
				faili = i;
				WARN_ON(1);
				goto fail;
			}
		}
	}
	/* cache orignal leaf block's references */
	if (level == 0 && cache_ref && buf != root->commit_root) {
		struct btrfs_leaf_ref *ref;
		struct btrfs_extent_info *info;

		ref = btrfs_alloc_leaf_ref(root, nr_file_extents);
		if (!ref) {
			WARN_ON(1);
			goto out;
		}

		ref->root_gen = root->root_key.offset;
		ref->bytenr = buf->start;
		ref->owner = btrfs_header_owner(buf);
		ref->generation = btrfs_header_generation(buf);
		ref->nritems = nr_file_extents;
		info = ref->extents;

		for (i = 0; nr_file_extents > 0 && i < nritems; i++) {
			u64 disk_bytenr;
			btrfs_item_key_to_cpu(buf, &key, i);
			if (btrfs_key_type(&key) != BTRFS_EXTENT_DATA_KEY)
				continue;
			fi = btrfs_item_ptr(buf, i,
					    struct btrfs_file_extent_item);
			if (btrfs_file_extent_type(buf, fi) ==
			    BTRFS_FILE_EXTENT_INLINE)
				continue;
			disk_bytenr = btrfs_file_extent_disk_bytenr(buf, fi);
			if (disk_bytenr == 0)
				continue;

			info->bytenr = disk_bytenr;
			info->num_bytes =
				btrfs_file_extent_disk_num_bytes(buf, fi);
			info->objectid = key.objectid;
			info->offset = key.offset;
			info++;
		}

		BUG_ON(!root->ref_tree);
		ret = btrfs_add_leaf_ref(root, ref);
		WARN_ON(ret);
		btrfs_free_leaf_ref(root, ref);
	}
out:
	return 0;
fail:
	WARN_ON(1);
#if 0
	for (i =0; i < faili; i++) {
		if (level == 0) {
			u64 disk_bytenr;
			btrfs_item_key_to_cpu(buf, &key, i);
			if (btrfs_key_type(&key) != BTRFS_EXTENT_DATA_KEY)
				continue;
			fi = btrfs_item_ptr(buf, i,
					    struct btrfs_file_extent_item);
			if (btrfs_file_extent_type(buf, fi) ==
			    BTRFS_FILE_EXTENT_INLINE)
				continue;
			disk_bytenr = btrfs_file_extent_disk_bytenr(buf, fi);
			if (disk_bytenr == 0)
				continue;
			err = btrfs_free_extent(trans, root, disk_bytenr,
				    btrfs_file_extent_disk_num_bytes(buf,
								      fi), 0);
			BUG_ON(err);
		} else {
			bytenr = btrfs_node_blockptr(buf, i);
			err = btrfs_free_extent(trans, root, bytenr,
					btrfs_level_size(root, level - 1), 0);
			BUG_ON(err);
		}
	}
#endif
	return ret;
}

static int write_one_cache_group(struct btrfs_trans_handle *trans,
				 struct btrfs_root *root,
				 struct btrfs_path *path,
				 struct btrfs_block_group_cache *cache)
{
	int ret;
	int pending_ret;
	struct btrfs_root *extent_root = root->fs_info->extent_root;
	unsigned long bi;
	struct extent_buffer *leaf;

	ret = btrfs_search_slot(trans, extent_root, &cache->key, path, 0, 1);
	if (ret < 0)
		goto fail;
	BUG_ON(ret);

	leaf = path->nodes[0];
	bi = btrfs_item_ptr_offset(leaf, path->slots[0]);
	write_extent_buffer(leaf, &cache->item, bi, sizeof(cache->item));
	btrfs_mark_buffer_dirty(leaf);
	btrfs_release_path(extent_root, path);
fail:
	finish_current_insert(trans, extent_root);
	pending_ret = del_pending_extents(trans, extent_root);
	if (ret)
		return ret;
	if (pending_ret)
		return pending_ret;
	return 0;

}

int btrfs_write_dirty_block_groups(struct btrfs_trans_handle *trans,
				   struct btrfs_root *root)
{
	struct extent_io_tree *block_group_cache;
	struct btrfs_block_group_cache *cache;
	int ret;
	int err = 0;
	int werr = 0;
	struct btrfs_path *path;
	u64 last = 0;
	u64 start;
	u64 end;
	u64 ptr;

	block_group_cache = &root->fs_info->block_group_cache;
	path = btrfs_alloc_path();
	if (!path)
		return -ENOMEM;

	mutex_lock(&root->fs_info->alloc_mutex);
	while(1) {
		ret = find_first_extent_bit(block_group_cache, last,
					    &start, &end, BLOCK_GROUP_DIRTY);
		if (ret)
			break;

		last = end + 1;
		ret = get_state_private(block_group_cache, start, &ptr);
		if (ret)
			break;
		cache = (struct btrfs_block_group_cache *)(unsigned long)ptr;
		err = write_one_cache_group(trans, root,
					    path, cache);
		/*
		 * if we fail to write the cache group, we want
		 * to keep it marked dirty in hopes that a later
		 * write will work
		 */
		if (err) {
			werr = err;
			continue;
		}
		clear_extent_bits(block_group_cache, start, end,
				  BLOCK_GROUP_DIRTY, GFP_NOFS);
	}
	btrfs_free_path(path);
	mutex_unlock(&root->fs_info->alloc_mutex);
	return werr;
}

static struct btrfs_space_info *__find_space_info(struct btrfs_fs_info *info,
						  u64 flags)
{
	struct list_head *head = &info->space_info;
	struct list_head *cur;
	struct btrfs_space_info *found;
	list_for_each(cur, head) {
		found = list_entry(cur, struct btrfs_space_info, list);
		if (found->flags == flags)
			return found;
	}
	return NULL;

}

static int update_space_info(struct btrfs_fs_info *info, u64 flags,
			     u64 total_bytes, u64 bytes_used,
			     struct btrfs_space_info **space_info)
{
	struct btrfs_space_info *found;

	found = __find_space_info(info, flags);
	if (found) {
		found->total_bytes += total_bytes;
		found->bytes_used += bytes_used;
		found->full = 0;
		WARN_ON(found->total_bytes < found->bytes_used);
		*space_info = found;
		return 0;
	}
	found = kmalloc(sizeof(*found), GFP_NOFS);
	if (!found)
		return -ENOMEM;

	list_add(&found->list, &info->space_info);
	found->flags = flags;
	found->total_bytes = total_bytes;
	found->bytes_used = bytes_used;
	found->bytes_pinned = 0;
	found->full = 0;
	found->force_alloc = 0;
	*space_info = found;
	return 0;
}

static void set_avail_alloc_bits(struct btrfs_fs_info *fs_info, u64 flags)
{
	u64 extra_flags = flags & (BTRFS_BLOCK_GROUP_RAID0 |
				   BTRFS_BLOCK_GROUP_RAID1 |
				   BTRFS_BLOCK_GROUP_RAID10 |
				   BTRFS_BLOCK_GROUP_DUP);
	if (extra_flags) {
		if (flags & BTRFS_BLOCK_GROUP_DATA)
			fs_info->avail_data_alloc_bits |= extra_flags;
		if (flags & BTRFS_BLOCK_GROUP_METADATA)
			fs_info->avail_metadata_alloc_bits |= extra_flags;
		if (flags & BTRFS_BLOCK_GROUP_SYSTEM)
			fs_info->avail_system_alloc_bits |= extra_flags;
	}
}

static u64 reduce_alloc_profile(struct btrfs_root *root, u64 flags)
{
	u64 num_devices = root->fs_info->fs_devices->num_devices;

	if (num_devices == 1)
		flags &= ~(BTRFS_BLOCK_GROUP_RAID1 | BTRFS_BLOCK_GROUP_RAID0);
	if (num_devices < 4)
		flags &= ~BTRFS_BLOCK_GROUP_RAID10;

	if ((flags & BTRFS_BLOCK_GROUP_DUP) &&
	    (flags & (BTRFS_BLOCK_GROUP_RAID1 |
		      BTRFS_BLOCK_GROUP_RAID10))) {
		flags &= ~BTRFS_BLOCK_GROUP_DUP;
	}

	if ((flags & BTRFS_BLOCK_GROUP_RAID1) &&
	    (flags & BTRFS_BLOCK_GROUP_RAID10)) {
		flags &= ~BTRFS_BLOCK_GROUP_RAID1;
	}

	if ((flags & BTRFS_BLOCK_GROUP_RAID0) &&
	    ((flags & BTRFS_BLOCK_GROUP_RAID1) |
	     (flags & BTRFS_BLOCK_GROUP_RAID10) |
	     (flags & BTRFS_BLOCK_GROUP_DUP)))
		flags &= ~BTRFS_BLOCK_GROUP_RAID0;
	return flags;
}

static int do_chunk_alloc(struct btrfs_trans_handle *trans,
			  struct btrfs_root *extent_root, u64 alloc_bytes,
			  u64 flags, int force)
{
	struct btrfs_space_info *space_info;
	u64 thresh;
	u64 start;
	u64 num_bytes;
	int ret;

	flags = reduce_alloc_profile(extent_root, flags);

	space_info = __find_space_info(extent_root->fs_info, flags);
	if (!space_info) {
		ret = update_space_info(extent_root->fs_info, flags,
					0, 0, &space_info);
		BUG_ON(ret);
	}
	BUG_ON(!space_info);

	if (space_info->force_alloc) {
		force = 1;
		space_info->force_alloc = 0;
	}
	if (space_info->full)
		goto out;

	thresh = div_factor(space_info->total_bytes, 6);
	if (!force &&
	   (space_info->bytes_used + space_info->bytes_pinned + alloc_bytes) <
	    thresh)
		goto out;

	mutex_lock(&extent_root->fs_info->chunk_mutex);
	ret = btrfs_alloc_chunk(trans, extent_root, &start, &num_bytes, flags);
	if (ret == -ENOSPC) {
printk("space info full %Lu\n", flags);
		space_info->full = 1;
		goto out_unlock;
	}
	BUG_ON(ret);

	ret = btrfs_make_block_group(trans, extent_root, 0, flags,
		     BTRFS_FIRST_CHUNK_TREE_OBJECTID, start, num_bytes);
	BUG_ON(ret);
out_unlock:
	mutex_unlock(&extent_root->fs_info->chunk_mutex);
out:
	return 0;
}

static int update_block_group(struct btrfs_trans_handle *trans,
			      struct btrfs_root *root,
			      u64 bytenr, u64 num_bytes, int alloc,
			      int mark_free)
{
	struct btrfs_block_group_cache *cache;
	struct btrfs_fs_info *info = root->fs_info;
	u64 total = num_bytes;
	u64 old_val;
	u64 byte_in_group;
	u64 start;
	u64 end;

	WARN_ON(!mutex_is_locked(&root->fs_info->alloc_mutex));
	while(total) {
		cache = btrfs_lookup_block_group(info, bytenr);
		if (!cache) {
			return -1;
		}
		byte_in_group = bytenr - cache->key.objectid;
		WARN_ON(byte_in_group > cache->key.offset);
		start = cache->key.objectid;
		end = start + cache->key.offset - 1;
		set_extent_bits(&info->block_group_cache, start, end,
				BLOCK_GROUP_DIRTY, GFP_NOFS);

		spin_lock(&cache->lock);
		old_val = btrfs_block_group_used(&cache->item);
		num_bytes = min(total, cache->key.offset - byte_in_group);
		if (alloc) {
			old_val += num_bytes;
			cache->space_info->bytes_used += num_bytes;
			btrfs_set_block_group_used(&cache->item, old_val);
			spin_unlock(&cache->lock);
		} else {
			old_val -= num_bytes;
			cache->space_info->bytes_used -= num_bytes;
			btrfs_set_block_group_used(&cache->item, old_val);
			spin_unlock(&cache->lock);
			if (mark_free) {
				set_extent_dirty(&info->free_space_cache,
						 bytenr, bytenr + num_bytes - 1,
						 GFP_NOFS);
			}
		}
		total -= num_bytes;
		bytenr += num_bytes;
	}
	return 0;
}

static u64 first_logical_byte(struct btrfs_root *root, u64 search_start)
{
	u64 start;
	u64 end;
	int ret;
	ret = find_first_extent_bit(&root->fs_info->block_group_cache,
				    search_start, &start, &end,
				    BLOCK_GROUP_DATA | BLOCK_GROUP_METADATA |
				    BLOCK_GROUP_SYSTEM);
	if (ret)
		return 0;
	return start;
}


static int update_pinned_extents(struct btrfs_root *root,
				u64 bytenr, u64 num, int pin)
{
	u64 len;
	struct btrfs_block_group_cache *cache;
	struct btrfs_fs_info *fs_info = root->fs_info;

	WARN_ON(!mutex_is_locked(&root->fs_info->alloc_mutex));
	if (pin) {
		set_extent_dirty(&fs_info->pinned_extents,
				bytenr, bytenr + num - 1, GFP_NOFS);
	} else {
		clear_extent_dirty(&fs_info->pinned_extents,
				bytenr, bytenr + num - 1, GFP_NOFS);
	}
	while (num > 0) {
		cache = btrfs_lookup_block_group(fs_info, bytenr);
		if (!cache) {
			u64 first = first_logical_byte(root, bytenr);
			WARN_ON(first < bytenr);
			len = min(first - bytenr, num);
		} else {
			len = min(num, cache->key.offset -
				  (bytenr - cache->key.objectid));
		}
		if (pin) {
			if (cache) {
				spin_lock(&cache->lock);
				cache->pinned += len;
				cache->space_info->bytes_pinned += len;
				spin_unlock(&cache->lock);
			}
			fs_info->total_pinned += len;
		} else {
			if (cache) {
				spin_lock(&cache->lock);
				cache->pinned -= len;
				cache->space_info->bytes_pinned -= len;
				spin_unlock(&cache->lock);
			}
			fs_info->total_pinned -= len;
		}
		bytenr += len;
		num -= len;
	}
	return 0;
}

int btrfs_copy_pinned(struct btrfs_root *root, struct extent_io_tree *copy)
{
	u64 last = 0;
	u64 start;
	u64 end;
	struct extent_io_tree *pinned_extents = &root->fs_info->pinned_extents;
	int ret;

	while(1) {
		ret = find_first_extent_bit(pinned_extents, last,
					    &start, &end, EXTENT_DIRTY);
		if (ret)
			break;
		set_extent_dirty(copy, start, end, GFP_NOFS);
		last = end + 1;
	}
	return 0;
}

int btrfs_finish_extent_commit(struct btrfs_trans_handle *trans,
			       struct btrfs_root *root,
			       struct extent_io_tree *unpin)
{
	u64 start;
	u64 end;
	int ret;
	struct extent_io_tree *free_space_cache;
	free_space_cache = &root->fs_info->free_space_cache;

	mutex_lock(&root->fs_info->alloc_mutex);
	while(1) {
		ret = find_first_extent_bit(unpin, 0, &start, &end,
					    EXTENT_DIRTY);
		if (ret)
			break;
		update_pinned_extents(root, start, end + 1 - start, 0);
		clear_extent_dirty(unpin, start, end, GFP_NOFS);
		set_extent_dirty(free_space_cache, start, end, GFP_NOFS);
		if (need_resched()) {
			mutex_unlock(&root->fs_info->alloc_mutex);
			cond_resched();
			mutex_lock(&root->fs_info->alloc_mutex);
		}
	}
	mutex_unlock(&root->fs_info->alloc_mutex);
	return 0;
}

static int finish_current_insert(struct btrfs_trans_handle *trans,
				 struct btrfs_root *extent_root)
{
	u64 start;
	u64 end;
	struct btrfs_fs_info *info = extent_root->fs_info;
	struct extent_buffer *eb;
	struct btrfs_path *path;
	struct btrfs_key ins;
	struct btrfs_disk_key first;
	struct btrfs_extent_item extent_item;
	int ret;
	int level;
	int err = 0;

	WARN_ON(!mutex_is_locked(&extent_root->fs_info->alloc_mutex));
	btrfs_set_stack_extent_refs(&extent_item, 1);
	btrfs_set_key_type(&ins, BTRFS_EXTENT_ITEM_KEY);
	path = btrfs_alloc_path();

	while(1) {
		ret = find_first_extent_bit(&info->extent_ins, 0, &start,
					    &end, EXTENT_LOCKED);
		if (ret)
			break;

		ins.objectid = start;
		ins.offset = end + 1 - start;
		err = btrfs_insert_item(trans, extent_root, &ins,
					&extent_item, sizeof(extent_item));
		clear_extent_bits(&info->extent_ins, start, end, EXTENT_LOCKED,
				  GFP_NOFS);

		eb = btrfs_find_tree_block(extent_root, ins.objectid,
					   ins.offset);

		if (!btrfs_buffer_uptodate(eb, trans->transid)) {
			mutex_unlock(&extent_root->fs_info->alloc_mutex);
			btrfs_read_buffer(eb, trans->transid);
			mutex_lock(&extent_root->fs_info->alloc_mutex);
		}

		btrfs_tree_lock(eb);
		level = btrfs_header_level(eb);
		if (level == 0) {
			btrfs_item_key(eb, &first, 0);
		} else {
			btrfs_node_key(eb, &first, 0);
		}
		btrfs_tree_unlock(eb);
		free_extent_buffer(eb);
		/*
		 * the first key is just a hint, so the race we've created
		 * against reading it is fine
		 */
		err = btrfs_insert_extent_backref(trans, extent_root, path,
					  start, extent_root->root_key.objectid,
					  0, level,
					  btrfs_disk_key_objectid(&first));
		BUG_ON(err);
		if (need_resched()) {
			mutex_unlock(&extent_root->fs_info->alloc_mutex);
			cond_resched();
			mutex_lock(&extent_root->fs_info->alloc_mutex);
		}
	}
	btrfs_free_path(path);
	return 0;
}

static int pin_down_bytes(struct btrfs_root *root, u64 bytenr, u32 num_bytes,
			  int pending)
{
	int err = 0;

	WARN_ON(!mutex_is_locked(&root->fs_info->alloc_mutex));
	if (!pending) {
		struct extent_buffer *buf;
		buf = btrfs_find_tree_block(root, bytenr, num_bytes);
		if (buf) {
			if (btrfs_buffer_uptodate(buf, 0) &&
			    btrfs_try_tree_lock(buf)) {
				u64 transid =
				    root->fs_info->running_transaction->transid;
				u64 header_transid =
					btrfs_header_generation(buf);
				if (header_transid == transid &&
				    !btrfs_header_flag(buf,
					       BTRFS_HEADER_FLAG_WRITTEN)) {
					clean_tree_block(NULL, root, buf);
					btrfs_tree_unlock(buf);
					free_extent_buffer(buf);
					return 1;
				}
				btrfs_tree_unlock(buf);
			}
			free_extent_buffer(buf);
		}
		update_pinned_extents(root, bytenr, num_bytes, 1);
	} else {
		set_extent_bits(&root->fs_info->pending_del,
				bytenr, bytenr + num_bytes - 1,
				EXTENT_LOCKED, GFP_NOFS);
	}
	BUG_ON(err < 0);
	return 0;
}

/*
 * remove an extent from the root, returns 0 on success
 */
static int __free_extent(struct btrfs_trans_handle *trans, struct btrfs_root
			 *root, u64 bytenr, u64 num_bytes,
			 u64 root_objectid, u64 ref_generation,
			 u64 owner_objectid, u64 owner_offset, int pin,
			 int mark_free)
{
	struct btrfs_path *path;
	struct btrfs_key key;
	struct btrfs_fs_info *info = root->fs_info;
	struct btrfs_root *extent_root = info->extent_root;
	struct extent_buffer *leaf;
	int ret;
	int extent_slot = 0;
	int found_extent = 0;
	int num_to_del = 1;
	struct btrfs_extent_item *ei;
	u32 refs;

	WARN_ON(!mutex_is_locked(&root->fs_info->alloc_mutex));
	key.objectid = bytenr;
	btrfs_set_key_type(&key, BTRFS_EXTENT_ITEM_KEY);
	key.offset = num_bytes;
	path = btrfs_alloc_path();
	if (!path)
		return -ENOMEM;

	path->reada = 1;
	ret = lookup_extent_backref(trans, extent_root, path,
				    bytenr, root_objectid,
				    ref_generation,
				    owner_objectid, owner_offset, 1);
	if (ret == 0) {
		struct btrfs_key found_key;
		extent_slot = path->slots[0];
		while(extent_slot > 0) {
			extent_slot--;
			btrfs_item_key_to_cpu(path->nodes[0], &found_key,
					      extent_slot);
			if (found_key.objectid != bytenr)
				break;
			if (found_key.type == BTRFS_EXTENT_ITEM_KEY &&
			    found_key.offset == num_bytes) {
				found_extent = 1;
				break;
			}
			if (path->slots[0] - extent_slot > 5)
				break;
		}
		if (!found_extent)
			ret = btrfs_del_item(trans, extent_root, path);
	} else {
		btrfs_print_leaf(extent_root, path->nodes[0]);
		WARN_ON(1);
		printk("Unable to find ref byte nr %Lu root %Lu "
		       " gen %Lu owner %Lu offset %Lu\n", bytenr,
		       root_objectid, ref_generation, owner_objectid,
		       owner_offset);
	}
	if (!found_extent) {
		btrfs_release_path(extent_root, path);
		ret = btrfs_search_slot(trans, extent_root, &key, path, -1, 1);
		if (ret < 0)
			return ret;
		BUG_ON(ret);
		extent_slot = path->slots[0];
	}

	leaf = path->nodes[0];
	ei = btrfs_item_ptr(leaf, extent_slot,
			    struct btrfs_extent_item);
	refs = btrfs_extent_refs(leaf, ei);
	BUG_ON(refs == 0);
	refs -= 1;
	btrfs_set_extent_refs(leaf, ei, refs);

	btrfs_mark_buffer_dirty(leaf);

	if (refs == 0 && found_extent && path->slots[0] == extent_slot + 1) {
		/* if the back ref and the extent are next to each other
		 * they get deleted below in one shot
		 */
		path->slots[0] = extent_slot;
		num_to_del = 2;
	} else if (found_extent) {
		/* otherwise delete the extent back ref */
		ret = btrfs_del_item(trans, extent_root, path);
		BUG_ON(ret);
		/* if refs are 0, we need to setup the path for deletion */
		if (refs == 0) {
			btrfs_release_path(extent_root, path);
			ret = btrfs_search_slot(trans, extent_root, &key, path,
						-1, 1);
			if (ret < 0)
				return ret;
			BUG_ON(ret);
		}
	}

	if (refs == 0) {
		u64 super_used;
		u64 root_used;

		if (pin) {
			ret = pin_down_bytes(root, bytenr, num_bytes, 0);
			if (ret > 0)
				mark_free = 1;
			BUG_ON(ret < 0);
		}

		/* block accounting for super block */
		spin_lock_irq(&info->delalloc_lock);
		super_used = btrfs_super_bytes_used(&info->super_copy);
		btrfs_set_super_bytes_used(&info->super_copy,
					   super_used - num_bytes);
		spin_unlock_irq(&info->delalloc_lock);

		/* block accounting for root item */
		root_used = btrfs_root_used(&root->root_item);
		btrfs_set_root_used(&root->root_item,
					   root_used - num_bytes);
		ret = btrfs_del_items(trans, extent_root, path, path->slots[0],
				      num_to_del);
		if (ret) {
			return ret;
		}
		ret = update_block_group(trans, root, bytenr, num_bytes, 0,
					 mark_free);
		BUG_ON(ret);
	}
	btrfs_free_path(path);
	finish_current_insert(trans, extent_root);
	return ret;
}

/*
 * find all the blocks marked as pending in the radix tree and remove
 * them from the extent map
 */
static int del_pending_extents(struct btrfs_trans_handle *trans, struct
			       btrfs_root *extent_root)
{
	int ret;
	int err = 0;
	u64 start;
	u64 end;
	struct extent_io_tree *pending_del;
	struct extent_io_tree *pinned_extents;

	WARN_ON(!mutex_is_locked(&extent_root->fs_info->alloc_mutex));
	pending_del = &extent_root->fs_info->pending_del;
	pinned_extents = &extent_root->fs_info->pinned_extents;

	while(1) {
		ret = find_first_extent_bit(pending_del, 0, &start, &end,
					    EXTENT_LOCKED);
		if (ret)
			break;
		clear_extent_bits(pending_del, start, end, EXTENT_LOCKED,
				  GFP_NOFS);
		if (!test_range_bit(&extent_root->fs_info->extent_ins,
				    start, end, EXTENT_LOCKED, 0)) {
			update_pinned_extents(extent_root, start,
					      end + 1 - start, 1);
			ret = __free_extent(trans, extent_root,
					     start, end + 1 - start,
					     extent_root->root_key.objectid,
					     0, 0, 0, 0, 0);
		} else {
			clear_extent_bits(&extent_root->fs_info->extent_ins,
					  start, end, EXTENT_LOCKED, GFP_NOFS);
		}
		if (ret)
			err = ret;

		if (need_resched()) {
			mutex_unlock(&extent_root->fs_info->alloc_mutex);
			cond_resched();
			mutex_lock(&extent_root->fs_info->alloc_mutex);
		}
	}
	return err;
}

/*
 * remove an extent from the root, returns 0 on success
 */
static int __btrfs_free_extent(struct btrfs_trans_handle *trans,
			       struct btrfs_root *root, u64 bytenr,
			       u64 num_bytes, u64 root_objectid,
			       u64 ref_generation, u64 owner_objectid,
			       u64 owner_offset, int pin)
{
	struct btrfs_root *extent_root = root->fs_info->extent_root;
	int pending_ret;
	int ret;

	WARN_ON(num_bytes < root->sectorsize);
	if (!root->ref_cows)
		ref_generation = 0;

	if (root == extent_root) {
		pin_down_bytes(root, bytenr, num_bytes, 1);
		return 0;
	}
	ret = __free_extent(trans, root, bytenr, num_bytes, root_objectid,
			    ref_generation, owner_objectid, owner_offset,
			    pin, pin == 0);

	finish_current_insert(trans, root->fs_info->extent_root);
	pending_ret = del_pending_extents(trans, root->fs_info->extent_root);
	return ret ? ret : pending_ret;
}

int btrfs_free_extent(struct btrfs_trans_handle *trans,
		      struct btrfs_root *root, u64 bytenr,
		      u64 num_bytes, u64 root_objectid,
		      u64 ref_generation, u64 owner_objectid,
		      u64 owner_offset, int pin)
{
	int ret;

	maybe_lock_mutex(root);
	ret = __btrfs_free_extent(trans, root, bytenr, num_bytes,
				  root_objectid, ref_generation,
				  owner_objectid, owner_offset, pin);
	maybe_unlock_mutex(root);
	return ret;
}

static u64 stripe_align(struct btrfs_root *root, u64 val)
{
	u64 mask = ((u64)root->stripesize - 1);
	u64 ret = (val + mask) & ~mask;
	return ret;
}

/*
 * walks the btree of allocated extents and find a hole of a given size.
 * The key ins is changed to record the hole:
 * ins->objectid == block start
 * ins->flags = BTRFS_EXTENT_ITEM_KEY
 * ins->offset == number of blocks
 * Any available blocks before search_start are skipped.
 */
static int noinline find_free_extent(struct btrfs_trans_handle *trans,
				     struct btrfs_root *orig_root,
				     u64 num_bytes, u64 empty_size,
				     u64 search_start, u64 search_end,
				     u64 hint_byte, struct btrfs_key *ins,
				     u64 exclude_start, u64 exclude_nr,
				     int data)
{
	int ret;
	u64 orig_search_start;
	struct btrfs_root * root = orig_root->fs_info->extent_root;
	struct btrfs_fs_info *info = root->fs_info;
	u64 total_needed = num_bytes;
	u64 *last_ptr = NULL;
	struct btrfs_block_group_cache *block_group;
	int full_scan = 0;
	int wrapped = 0;
	int chunk_alloc_done = 0;
	int empty_cluster = 2 * 1024 * 1024;
	int allowed_chunk_alloc = 0;

	WARN_ON(num_bytes < root->sectorsize);
	btrfs_set_key_type(ins, BTRFS_EXTENT_ITEM_KEY);

	if (orig_root->ref_cows || empty_size)
		allowed_chunk_alloc = 1;

	if (data & BTRFS_BLOCK_GROUP_METADATA) {
		last_ptr = &root->fs_info->last_alloc;
		empty_cluster = 256 * 1024;
	}

	if ((data & BTRFS_BLOCK_GROUP_DATA) && btrfs_test_opt(root, SSD)) {
		last_ptr = &root->fs_info->last_data_alloc;
	}

	if (last_ptr) {
		if (*last_ptr)
			hint_byte = *last_ptr;
		else {
			empty_size += empty_cluster;
		}
	}

	search_start = max(search_start, first_logical_byte(root, 0));
	orig_search_start = search_start;

	if (search_end == (u64)-1)
		search_end = btrfs_super_total_bytes(&info->super_copy);

	if (hint_byte) {
		block_group = btrfs_lookup_first_block_group(info, hint_byte);
		if (!block_group)
			hint_byte = search_start;
		block_group = btrfs_find_block_group(root, block_group,
						     hint_byte, data, 1);
		if (last_ptr && *last_ptr == 0 && block_group)
			hint_byte = block_group->key.objectid;
	} else {
		block_group = btrfs_find_block_group(root,
						     trans->block_group,
						     search_start, data, 1);
	}
	search_start = max(search_start, hint_byte);

	total_needed += empty_size;

check_failed:
	if (!block_group) {
		block_group = btrfs_lookup_first_block_group(info,
							     search_start);
		if (!block_group)
			block_group = btrfs_lookup_first_block_group(info,
						       orig_search_start);
	}
	if (full_scan && !chunk_alloc_done) {
		if (allowed_chunk_alloc) {
			do_chunk_alloc(trans, root,
				     num_bytes + 2 * 1024 * 1024, data, 1);
			allowed_chunk_alloc = 0;
		} else if (block_group && block_group_bits(block_group, data)) {
			block_group->space_info->force_alloc = 1;
		}
		chunk_alloc_done = 1;
	}
	ret = find_search_start(root, &block_group, &search_start,
				total_needed, data);
	if (ret == -ENOSPC && last_ptr && *last_ptr) {
		*last_ptr = 0;
		block_group = btrfs_lookup_first_block_group(info,
							     orig_search_start);
		search_start = orig_search_start;
		ret = find_search_start(root, &block_group, &search_start,
					total_needed, data);
	}
	if (ret == -ENOSPC)
		goto enospc;
	if (ret)
		goto error;

	if (last_ptr && *last_ptr && search_start != *last_ptr) {
		*last_ptr = 0;
		if (!empty_size) {
			empty_size += empty_cluster;
			total_needed += empty_size;
		}
		block_group = btrfs_lookup_first_block_group(info,
						       orig_search_start);
		search_start = orig_search_start;
		ret = find_search_start(root, &block_group,
					&search_start, total_needed, data);
		if (ret == -ENOSPC)
			goto enospc;
		if (ret)
			goto error;
	}

	search_start = stripe_align(root, search_start);
	ins->objectid = search_start;
	ins->offset = num_bytes;

	if (ins->objectid + num_bytes >= search_end)
		goto enospc;

	if (ins->objectid + num_bytes >
	    block_group->key.objectid + block_group->key.offset) {
		search_start = block_group->key.objectid +
			block_group->key.offset;
		goto new_group;
	}

	if (test_range_bit(&info->extent_ins, ins->objectid,
			   ins->objectid + num_bytes -1, EXTENT_LOCKED, 0)) {
		search_start = ins->objectid + num_bytes;
		goto new_group;
	}

	if (test_range_bit(&info->pinned_extents, ins->objectid,
			   ins->objectid + num_bytes -1, EXTENT_DIRTY, 0)) {
		search_start = ins->objectid + num_bytes;
		goto new_group;
	}

	if (exclude_nr > 0 && (ins->objectid + num_bytes > exclude_start &&
	    ins->objectid < exclude_start + exclude_nr)) {
		search_start = exclude_start + exclude_nr;
		goto new_group;
	}

	if (!(data & BTRFS_BLOCK_GROUP_DATA)) {
		block_group = btrfs_lookup_block_group(info, ins->objectid);
		if (block_group)
			trans->block_group = block_group;
	}
	ins->offset = num_bytes;
	if (last_ptr) {
		*last_ptr = ins->objectid + ins->offset;
		if (*last_ptr ==
		    btrfs_super_total_bytes(&root->fs_info->super_copy)) {
			*last_ptr = 0;
		}
	}
	return 0;

new_group:
	if (search_start + num_bytes >= search_end) {
enospc:
		search_start = orig_search_start;
		if (full_scan) {
			ret = -ENOSPC;
			goto error;
		}
		if (wrapped) {
			if (!full_scan)
				total_needed -= empty_size;
			full_scan = 1;
		} else
			wrapped = 1;
	}
	block_group = btrfs_lookup_first_block_group(info, search_start);
	cond_resched();
	block_group = btrfs_find_block_group(root, block_group,
					     search_start, data, 0);
	goto check_failed;

error:
	return ret;
}

static int __btrfs_reserve_extent(struct btrfs_trans_handle *trans,
				  struct btrfs_root *root,
				  u64 num_bytes, u64 min_alloc_size,
				  u64 empty_size, u64 hint_byte,
				  u64 search_end, struct btrfs_key *ins,
				  u64 data)
{
	int ret;
	u64 search_start = 0;
	u64 alloc_profile;
	struct btrfs_fs_info *info = root->fs_info;

	if (data) {
		alloc_profile = info->avail_data_alloc_bits &
			        info->data_alloc_profile;
		data = BTRFS_BLOCK_GROUP_DATA | alloc_profile;
	} else if (root == root->fs_info->chunk_root) {
		alloc_profile = info->avail_system_alloc_bits &
			        info->system_alloc_profile;
		data = BTRFS_BLOCK_GROUP_SYSTEM | alloc_profile;
	} else {
		alloc_profile = info->avail_metadata_alloc_bits &
			        info->metadata_alloc_profile;
		data = BTRFS_BLOCK_GROUP_METADATA | alloc_profile;
	}
again:
	data = reduce_alloc_profile(root, data);
	/*
	 * the only place that sets empty_size is btrfs_realloc_node, which
	 * is not called recursively on allocations
	 */
	if (empty_size || root->ref_cows) {
		if (!(data & BTRFS_BLOCK_GROUP_METADATA)) {
			ret = do_chunk_alloc(trans, root->fs_info->extent_root,
				     2 * 1024 * 1024,
				     BTRFS_BLOCK_GROUP_METADATA |
				     (info->metadata_alloc_profile &
				      info->avail_metadata_alloc_bits), 0);
			BUG_ON(ret);
		}
		ret = do_chunk_alloc(trans, root->fs_info->extent_root,
				     num_bytes + 2 * 1024 * 1024, data, 0);
		BUG_ON(ret);
	}

	WARN_ON(num_bytes < root->sectorsize);
	ret = find_free_extent(trans, root, num_bytes, empty_size,
			       search_start, search_end, hint_byte, ins,
			       trans->alloc_exclude_start,
			       trans->alloc_exclude_nr, data);

	if (ret == -ENOSPC && num_bytes > min_alloc_size) {
		num_bytes = num_bytes >> 1;
		num_bytes = max(num_bytes, min_alloc_size);
		do_chunk_alloc(trans, root->fs_info->extent_root,
			       num_bytes, data, 1);
		goto again;
	}
	if (ret) {
		printk("allocation failed flags %Lu\n", data);
		BUG();
	}
	clear_extent_dirty(&root->fs_info->free_space_cache,
			   ins->objectid, ins->objectid + ins->offset - 1,
			   GFP_NOFS);
	return 0;
}

int btrfs_reserve_extent(struct btrfs_trans_handle *trans,
				  struct btrfs_root *root,
				  u64 num_bytes, u64 min_alloc_size,
				  u64 empty_size, u64 hint_byte,
				  u64 search_end, struct btrfs_key *ins,
				  u64 data)
{
	int ret;
	maybe_lock_mutex(root);
	ret = __btrfs_reserve_extent(trans, root, num_bytes, min_alloc_size,
				     empty_size, hint_byte, search_end, ins,
				     data);
	maybe_unlock_mutex(root);
	return ret;
}

static int __btrfs_alloc_reserved_extent(struct btrfs_trans_handle *trans,
					 struct btrfs_root *root,
					 u64 root_objectid, u64 ref_generation,
					 u64 owner, u64 owner_offset,
					 struct btrfs_key *ins)
{
	int ret;
	int pending_ret;
	u64 super_used;
	u64 root_used;
	u64 num_bytes = ins->offset;
	u32 sizes[2];
	struct btrfs_fs_info *info = root->fs_info;
	struct btrfs_root *extent_root = info->extent_root;
	struct btrfs_extent_item *extent_item;
	struct btrfs_extent_ref *ref;
	struct btrfs_path *path;
	struct btrfs_key keys[2];

	/* block accounting for super block */
	spin_lock_irq(&info->delalloc_lock);
	super_used = btrfs_super_bytes_used(&info->super_copy);
	btrfs_set_super_bytes_used(&info->super_copy, super_used + num_bytes);
	spin_unlock_irq(&info->delalloc_lock);

	/* block accounting for root item */
	root_used = btrfs_root_used(&root->root_item);
	btrfs_set_root_used(&root->root_item, root_used + num_bytes);

	if (root == extent_root) {
		set_extent_bits(&root->fs_info->extent_ins, ins->objectid,
				ins->objectid + ins->offset - 1,
				EXTENT_LOCKED, GFP_NOFS);
		goto update_block;
	}

	memcpy(&keys[0], ins, sizeof(*ins));
	keys[1].offset = hash_extent_ref(root_objectid, ref_generation,
					 owner, owner_offset);
	keys[1].objectid = ins->objectid;
	keys[1].type = BTRFS_EXTENT_REF_KEY;
	sizes[0] = sizeof(*extent_item);
	sizes[1] = sizeof(*ref);

	path = btrfs_alloc_path();
	BUG_ON(!path);

	ret = btrfs_insert_empty_items(trans, extent_root, path, keys,
				       sizes, 2);

	BUG_ON(ret);
	extent_item = btrfs_item_ptr(path->nodes[0], path->slots[0],
				     struct btrfs_extent_item);
	btrfs_set_extent_refs(path->nodes[0], extent_item, 1);
	ref = btrfs_item_ptr(path->nodes[0], path->slots[0] + 1,
			     struct btrfs_extent_ref);

	btrfs_set_ref_root(path->nodes[0], ref, root_objectid);
	btrfs_set_ref_generation(path->nodes[0], ref, ref_generation);
	btrfs_set_ref_objectid(path->nodes[0], ref, owner);
	btrfs_set_ref_offset(path->nodes[0], ref, owner_offset);

	btrfs_mark_buffer_dirty(path->nodes[0]);

	trans->alloc_exclude_start = 0;
	trans->alloc_exclude_nr = 0;
	btrfs_free_path(path);
	finish_current_insert(trans, extent_root);
	pending_ret = del_pending_extents(trans, extent_root);

	if (ret)
		goto out;
	if (pending_ret) {
		ret = pending_ret;
		goto out;
	}

update_block:
	ret = update_block_group(trans, root, ins->objectid, ins->offset, 1, 0);
	if (ret) {
		printk("update block group failed for %Lu %Lu\n",
		       ins->objectid, ins->offset);
		BUG();
	}
out:
	return ret;
}

int btrfs_alloc_reserved_extent(struct btrfs_trans_handle *trans,
				struct btrfs_root *root,
				u64 root_objectid, u64 ref_generation,
				u64 owner, u64 owner_offset,
				struct btrfs_key *ins)
{
	int ret;
	maybe_lock_mutex(root);
	ret = __btrfs_alloc_reserved_extent(trans, root, root_objectid,
					    ref_generation, owner,
					    owner_offset, ins);
	maybe_unlock_mutex(root);
	return ret;
}
/*
 * finds a free extent and does all the dirty work required for allocation
 * returns the key for the extent through ins, and a tree buffer for
 * the first block of the extent through buf.
 *
 * returns 0 if everything worked, non-zero otherwise.
 */
int btrfs_alloc_extent(struct btrfs_trans_handle *trans,
		       struct btrfs_root *root,
		       u64 num_bytes, u64 min_alloc_size,
		       u64 root_objectid, u64 ref_generation,
		       u64 owner, u64 owner_offset,
		       u64 empty_size, u64 hint_byte,
		       u64 search_end, struct btrfs_key *ins, u64 data)
{
	int ret;

	maybe_lock_mutex(root);

	ret = __btrfs_reserve_extent(trans, root, num_bytes,
				     min_alloc_size, empty_size, hint_byte,
				     search_end, ins, data);
	BUG_ON(ret);
	ret = __btrfs_alloc_reserved_extent(trans, root, root_objectid,
					    ref_generation, owner,
					    owner_offset, ins);
	BUG_ON(ret);

	maybe_unlock_mutex(root);
	return ret;
}
/*
 * helper function to allocate a block for a given tree
 * returns the tree buffer or NULL.
 */
struct extent_buffer *btrfs_alloc_free_block(struct btrfs_trans_handle *trans,
					     struct btrfs_root *root,
					     u32 blocksize,
					     u64 root_objectid,
					     u64 ref_generation,
					     u64 first_objectid,
					     int level,
					     u64 hint,
					     u64 empty_size)
{
	struct btrfs_key ins;
	int ret;
	struct extent_buffer *buf;

	ret = btrfs_alloc_extent(trans, root, blocksize, blocksize,
				 root_objectid, ref_generation,
				 level, first_objectid, empty_size, hint,
				 (u64)-1, &ins, 0);
	if (ret) {
		BUG_ON(ret > 0);
		return ERR_PTR(ret);
	}
	buf = btrfs_find_create_tree_block(root, ins.objectid, blocksize);
	if (!buf) {
		btrfs_free_extent(trans, root, ins.objectid, blocksize,
				  root->root_key.objectid, ref_generation,
				  0, 0, 0);
		return ERR_PTR(-ENOMEM);
	}
	btrfs_set_header_generation(buf, trans->transid);
	btrfs_tree_lock(buf);
	clean_tree_block(trans, root, buf);
	btrfs_set_buffer_uptodate(buf);

	if (PageDirty(buf->first_page)) {
		printk("page %lu dirty\n", buf->first_page->index);
		WARN_ON(1);
	}

	set_extent_dirty(&trans->transaction->dirty_pages, buf->start,
			 buf->start + buf->len - 1, GFP_NOFS);
	trans->blocks_used++;
	return buf;
}

static int noinline drop_leaf_ref_no_cache(struct btrfs_trans_handle *trans,
					   struct btrfs_root *root,
					   struct extent_buffer *leaf)
{
	u64 leaf_owner;
	u64 leaf_generation;
	struct btrfs_key key;
	struct btrfs_file_extent_item *fi;
	int i;
	int nritems;
	int ret;

	BUG_ON(!btrfs_is_leaf(leaf));
	nritems = btrfs_header_nritems(leaf);
	leaf_owner = btrfs_header_owner(leaf);
	leaf_generation = btrfs_header_generation(leaf);

	for (i = 0; i < nritems; i++) {
		u64 disk_bytenr;
		cond_resched();

		btrfs_item_key_to_cpu(leaf, &key, i);
		if (btrfs_key_type(&key) != BTRFS_EXTENT_DATA_KEY)
			continue;
		fi = btrfs_item_ptr(leaf, i, struct btrfs_file_extent_item);
		if (btrfs_file_extent_type(leaf, fi) ==
		    BTRFS_FILE_EXTENT_INLINE)
			continue;
		/*
		 * FIXME make sure to insert a trans record that
		 * repeats the snapshot del on crash
		 */
		disk_bytenr = btrfs_file_extent_disk_bytenr(leaf, fi);
		if (disk_bytenr == 0)
			continue;

		mutex_lock(&root->fs_info->alloc_mutex);
		ret = __btrfs_free_extent(trans, root, disk_bytenr,
				btrfs_file_extent_disk_num_bytes(leaf, fi),
				leaf_owner, leaf_generation,
				key.objectid, key.offset, 0);
		mutex_unlock(&root->fs_info->alloc_mutex);
		BUG_ON(ret);
	}
	return 0;
}

static int noinline drop_leaf_ref(struct btrfs_trans_handle *trans,
					 struct btrfs_root *root,
					 struct btrfs_leaf_ref *ref)
{
	int i;
	int ret;
	struct btrfs_extent_info *info = ref->extents;

	for (i = 0; i < ref->nritems; i++) {
		mutex_lock(&root->fs_info->alloc_mutex);
		ret = __btrfs_free_extent(trans, root,
					info->bytenr, info->num_bytes,
					ref->owner, ref->generation,
					info->objectid, info->offset, 0);
		mutex_unlock(&root->fs_info->alloc_mutex);
		BUG_ON(ret);
		info++;
	}

	return 0;
}

static void noinline reada_walk_down(struct btrfs_root *root,
				     struct extent_buffer *node,
				     int slot)
{
	u64 bytenr;
	u64 last = 0;
	u32 nritems;
	u32 refs;
	u32 blocksize;
	int ret;
	int i;
	int level;
	int skipped = 0;

	nritems = btrfs_header_nritems(node);
	level = btrfs_header_level(node);
	if (level)
		return;

	for (i = slot; i < nritems && skipped < 32; i++) {
		bytenr = btrfs_node_blockptr(node, i);
		if (last && ((bytenr > last && bytenr - last > 32 * 1024) ||
			     (last > bytenr && last - bytenr > 32 * 1024))) {
			skipped++;
			continue;
		}
		blocksize = btrfs_level_size(root, level - 1);
		if (i != slot) {
			ret = lookup_extent_ref(NULL, root, bytenr,
						blocksize, &refs);
			BUG_ON(ret);
			if (refs != 1) {
				skipped++;
				continue;
			}
		}
		ret = readahead_tree_block(root, bytenr, blocksize,
					   btrfs_node_ptr_generation(node, i));
		last = bytenr + blocksize;
		cond_resched();
		if (ret)
			break;
	}
}

int drop_snap_lookup_refcount(struct btrfs_root *root, u64 start, u64 len,
			      u32 *refs)
{
	int ret;

	ret = lookup_extent_ref(NULL, root, start, len, refs);
	BUG_ON(ret);

#if 0 // some debugging code in case we see problems here
	/* if the refs count is one, it won't get increased again.  But
	 * if the ref count is > 1, someone may be decreasing it at
	 * the same time we are.
	 */
	if (*refs != 1) {
		struct extent_buffer *eb = NULL;
		eb = btrfs_find_create_tree_block(root, start, len);
		if (eb)
			btrfs_tree_lock(eb);

		mutex_lock(&root->fs_info->alloc_mutex);
		ret = lookup_extent_ref(NULL, root, start, len, refs);
		BUG_ON(ret);
		mutex_unlock(&root->fs_info->alloc_mutex);

		if (eb) {
			btrfs_tree_unlock(eb);
			free_extent_buffer(eb);
		}
		if (*refs == 1) {
			printk("block %llu went down to one during drop_snap\n",
			       (unsigned long long)start);
		}

	}
#endif

	cond_resched();
	return ret;
}

/*
 * helper function for drop_snapshot, this walks down the tree dropping ref
 * counts as it goes.
 */
static int noinline walk_down_tree(struct btrfs_trans_handle *trans,
				   struct btrfs_root *root,
				   struct btrfs_path *path, int *level)
{
	u64 root_owner;
	u64 root_gen;
	u64 bytenr;
	u64 ptr_gen;
	struct extent_buffer *next;
	struct extent_buffer *cur;
	struct extent_buffer *parent;
	struct btrfs_leaf_ref *ref;
	u32 blocksize;
	int ret;
	u32 refs;

	WARN_ON(*level < 0);
	WARN_ON(*level >= BTRFS_MAX_LEVEL);
	ret = drop_snap_lookup_refcount(root, path->nodes[*level]->start,
				path->nodes[*level]->len, &refs);
	BUG_ON(ret);
	if (refs > 1)
		goto out;

	/*
	 * walk down to the last node level and free all the leaves
	 */
	while(*level >= 0) {
		WARN_ON(*level < 0);
		WARN_ON(*level >= BTRFS_MAX_LEVEL);
		cur = path->nodes[*level];

		if (btrfs_header_level(cur) != *level)
			WARN_ON(1);

		if (path->slots[*level] >=
		    btrfs_header_nritems(cur))
			break;
		if (*level == 0) {
			ret = drop_leaf_ref_no_cache(trans, root, cur);
			BUG_ON(ret);
			break;
		}
		bytenr = btrfs_node_blockptr(cur, path->slots[*level]);
		ptr_gen = btrfs_node_ptr_generation(cur, path->slots[*level]);
		blocksize = btrfs_level_size(root, *level - 1);

		ret = drop_snap_lookup_refcount(root, bytenr, blocksize, &refs);
		BUG_ON(ret);
		if (refs != 1) {
			parent = path->nodes[*level];
			root_owner = btrfs_header_owner(parent);
			root_gen = btrfs_header_generation(parent);
			path->slots[*level]++;

			mutex_lock(&root->fs_info->alloc_mutex);
			ret = __btrfs_free_extent(trans, root, bytenr,
						blocksize, root_owner,
						root_gen, 0, 0, 1);
			BUG_ON(ret);
			mutex_unlock(&root->fs_info->alloc_mutex);

			atomic_inc(&root->fs_info->throttle_gen);
			wake_up(&root->fs_info->transaction_throttle);

			continue;
		}
		/*
		 * at this point, we have a single ref, and since the
		 * only place referencing this extent is a dead root
		 * the reference count should never go higher.
		 * So, we don't need to check it again
		 */
		if (*level == 1) {
			struct btrfs_key key;
			btrfs_node_key_to_cpu(cur, &key, path->slots[*level]);
			ref = btrfs_lookup_leaf_ref(root, bytenr);
			if (ref) {
				ret = drop_leaf_ref(trans, root, ref);
				BUG_ON(ret);
				btrfs_remove_leaf_ref(root, ref);
				btrfs_free_leaf_ref(root, ref);
				*level = 0;
				break;
			}
			if (printk_ratelimit())
				printk("leaf ref miss for bytenr %llu\n",
				       (unsigned long long)bytenr);
		}
		next = btrfs_find_tree_block(root, bytenr, blocksize);
		if (!next || !btrfs_buffer_uptodate(next, ptr_gen)) {
			free_extent_buffer(next);

			if (path->slots[*level] == 0)
				reada_walk_down(root, cur, path->slots[*level]);
			next = read_tree_block(root, bytenr, blocksize,
					       ptr_gen);
			cond_resched();
#if 0
			/*
			 * this is a debugging check and can go away
			 * the ref should never go all the way down to 1
			 * at this point
			 */
			ret = lookup_extent_ref(NULL, root, bytenr, blocksize,
						&refs);
			BUG_ON(ret);
			WARN_ON(refs != 1);
#endif
		}
		WARN_ON(*level <= 0);
		if (path->nodes[*level-1])
			free_extent_buffer(path->nodes[*level-1]);
		path->nodes[*level-1] = next;
		*level = btrfs_header_level(next);
		path->slots[*level] = 0;
	}
out:
	WARN_ON(*level < 0);
	WARN_ON(*level >= BTRFS_MAX_LEVEL);

	if (path->nodes[*level] == root->node) {
		parent = path->nodes[*level];
		bytenr = path->nodes[*level]->start;
	} else {
		parent = path->nodes[*level + 1];
		bytenr = btrfs_node_blockptr(parent, path->slots[*level + 1]);
	}

	blocksize = btrfs_level_size(root, *level);
	root_owner = btrfs_header_owner(parent);
	root_gen = btrfs_header_generation(parent);

	mutex_lock(&root->fs_info->alloc_mutex);
	ret = __btrfs_free_extent(trans, root, bytenr, blocksize,
				  root_owner, root_gen, 0, 0, 1);
	free_extent_buffer(path->nodes[*level]);
	path->nodes[*level] = NULL;
	*level += 1;
	BUG_ON(ret);
	mutex_unlock(&root->fs_info->alloc_mutex);

	cond_resched();
	return 0;
}

/*
 * helper for dropping snapshots.  This walks back up the tree in the path
 * to find the first node higher up where we haven't yet gone through
 * all the slots
 */
static int noinline walk_up_tree(struct btrfs_trans_handle *trans,
				 struct btrfs_root *root,
				 struct btrfs_path *path, int *level)
{
	u64 root_owner;
	u64 root_gen;
	struct btrfs_root_item *root_item = &root->root_item;
	int i;
	int slot;
	int ret;

	for(i = *level; i < BTRFS_MAX_LEVEL - 1 && path->nodes[i]; i++) {
		slot = path->slots[i];
		if (slot < btrfs_header_nritems(path->nodes[i]) - 1) {
			struct extent_buffer *node;
			struct btrfs_disk_key disk_key;
			node = path->nodes[i];
			path->slots[i]++;
			*level = i;
			WARN_ON(*level == 0);
			btrfs_node_key(node, &disk_key, path->slots[i]);
			memcpy(&root_item->drop_progress,
			       &disk_key, sizeof(disk_key));
			root_item->drop_level = i;
			return 0;
		} else {
			if (path->nodes[*level] == root->node) {
				root_owner = root->root_key.objectid;
				root_gen =
				   btrfs_header_generation(path->nodes[*level]);
			} else {
				struct extent_buffer *node;
				node = path->nodes[*level + 1];
				root_owner = btrfs_header_owner(node);
				root_gen = btrfs_header_generation(node);
			}
			ret = btrfs_free_extent(trans, root,
						path->nodes[*level]->start,
						path->nodes[*level]->len,
						root_owner, root_gen, 0, 0, 1);
			BUG_ON(ret);
			free_extent_buffer(path->nodes[*level]);
			path->nodes[*level] = NULL;
			*level = i + 1;
		}
	}
	return 1;
}

/*
 * drop the reference count on the tree rooted at 'snap'.  This traverses
 * the tree freeing any blocks that have a ref count of zero after being
 * decremented.
 */
int btrfs_drop_snapshot(struct btrfs_trans_handle *trans, struct btrfs_root
			*root)
{
	int ret = 0;
	int wret;
	int level;
	struct btrfs_path *path;
	int i;
	int orig_level;
	struct btrfs_root_item *root_item = &root->root_item;

	WARN_ON(!mutex_is_locked(&root->fs_info->drop_mutex));
	path = btrfs_alloc_path();
	BUG_ON(!path);

	level = btrfs_header_level(root->node);
	orig_level = level;
	if (btrfs_disk_key_objectid(&root_item->drop_progress) == 0) {
		path->nodes[level] = root->node;
		extent_buffer_get(root->node);
		path->slots[level] = 0;
	} else {
		struct btrfs_key key;
		struct btrfs_disk_key found_key;
		struct extent_buffer *node;

		btrfs_disk_key_to_cpu(&key, &root_item->drop_progress);
		level = root_item->drop_level;
		path->lowest_level = level;
		wret = btrfs_search_slot(NULL, root, &key, path, 0, 0);
		if (wret < 0) {
			ret = wret;
			goto out;
		}
		node = path->nodes[level];
		btrfs_node_key(node, &found_key, path->slots[level]);
		WARN_ON(memcmp(&found_key, &root_item->drop_progress,
			       sizeof(found_key)));
		/*
		 * unlock our path, this is safe because only this
		 * function is allowed to delete this snapshot
		 */
		for (i = 0; i < BTRFS_MAX_LEVEL; i++) {
			if (path->nodes[i] && path->locks[i]) {
				path->locks[i] = 0;
				btrfs_tree_unlock(path->nodes[i]);
			}
		}
	}
	while(1) {
		wret = walk_down_tree(trans, root, path, &level);
		if (wret > 0)
			break;
		if (wret < 0)
			ret = wret;

		wret = walk_up_tree(trans, root, path, &level);
		if (wret > 0)
			break;
		if (wret < 0)
			ret = wret;
		if (trans->transaction->in_commit) {
			ret = -EAGAIN;
			break;
		}
		atomic_inc(&root->fs_info->throttle_gen);
		wake_up(&root->fs_info->transaction_throttle);
	}
	for (i = 0; i <= orig_level; i++) {
		if (path->nodes[i]) {
			free_extent_buffer(path->nodes[i]);
			path->nodes[i] = NULL;
		}
	}
out:
	btrfs_free_path(path);
	return ret;
}

int btrfs_free_block_groups(struct btrfs_fs_info *info)
{
	u64 start;
	u64 end;
	u64 ptr;
	int ret;

	mutex_lock(&info->alloc_mutex);
	while(1) {
		ret = find_first_extent_bit(&info->block_group_cache, 0,
					    &start, &end, (unsigned int)-1);
		if (ret)
			break;
		ret = get_state_private(&info->block_group_cache, start, &ptr);
		if (!ret)
			kfree((void *)(unsigned long)ptr);
		clear_extent_bits(&info->block_group_cache, start,
				  end, (unsigned int)-1, GFP_NOFS);
	}
	while(1) {
		ret = find_first_extent_bit(&info->free_space_cache, 0,
					    &start, &end, EXTENT_DIRTY);
		if (ret)
			break;
		clear_extent_dirty(&info->free_space_cache, start,
				   end, GFP_NOFS);
	}
	mutex_unlock(&info->alloc_mutex);
	return 0;
}

static unsigned long calc_ra(unsigned long start, unsigned long last,
			     unsigned long nr)
{
	return min(last, start + nr - 1);
}

static int noinline relocate_inode_pages(struct inode *inode, u64 start,
					 u64 len)
{
	u64 page_start;
	u64 page_end;
	unsigned long last_index;
	unsigned long i;
	struct page *page;
	struct extent_io_tree *io_tree = &BTRFS_I(inode)->io_tree;
	struct file_ra_state *ra;
	unsigned long total_read = 0;
	unsigned long ra_pages;
	struct btrfs_ordered_extent *ordered;
	struct btrfs_trans_handle *trans;

	ra = kzalloc(sizeof(*ra), GFP_NOFS);

	mutex_lock(&inode->i_mutex);
	i = start >> PAGE_CACHE_SHIFT;
	last_index = (start + len - 1) >> PAGE_CACHE_SHIFT;

	ra_pages = BTRFS_I(inode)->root->fs_info->bdi.ra_pages;

	file_ra_state_init(ra, inode->i_mapping);

	for (; i <= last_index; i++) {
		if (total_read % ra_pages == 0) {
			btrfs_force_ra(inode->i_mapping, ra, NULL, i,
				       calc_ra(i, last_index, ra_pages));
		}
		total_read++;
again:
		if (((u64)i << PAGE_CACHE_SHIFT) > i_size_read(inode))
			goto truncate_racing;
		page = grab_cache_page(inode->i_mapping, i);
		if (!page) {
			goto out_unlock;
		}
		if (!PageUptodate(page)) {
			btrfs_readpage(NULL, page);
			lock_page(page);
			if (!PageUptodate(page)) {
				unlock_page(page);
				page_cache_release(page);
				goto out_unlock;
			}
		}
		wait_on_page_writeback(page);

		page_start = (u64)page->index << PAGE_CACHE_SHIFT;
		page_end = page_start + PAGE_CACHE_SIZE - 1;
		lock_extent(io_tree, page_start, page_end, GFP_NOFS);

		ordered = btrfs_lookup_ordered_extent(inode, page_start);
		if (ordered) {
			unlock_extent(io_tree, page_start, page_end, GFP_NOFS);
			unlock_page(page);
			page_cache_release(page);
			btrfs_start_ordered_extent(inode, ordered, 1);
			btrfs_put_ordered_extent(ordered);
			goto again;
		}
		set_page_extent_mapped(page);

		/*
		 * make sure page_mkwrite is called for this page if userland
		 * wants to change it from mmap
		 */
		clear_page_dirty_for_io(page);

		set_extent_delalloc(io_tree, page_start,
				    page_end, GFP_NOFS);
		set_page_dirty(page);

		unlock_extent(io_tree, page_start, page_end, GFP_NOFS);
		unlock_page(page);
		page_cache_release(page);
	}

out_unlock:
	/* we have to start the IO in order to get the ordered extents
	 * instantiated.  This allows the relocation to code to wait
	 * for all the ordered extents to hit the disk.
	 *
	 * Otherwise, it would constantly loop over the same extents
	 * because the old ones don't get deleted  until the IO is
	 * started
	 */
	btrfs_fdatawrite_range(inode->i_mapping, start, start + len - 1,
			       WB_SYNC_NONE);
	kfree(ra);
	trans = btrfs_start_transaction(BTRFS_I(inode)->root, 1);
	if (trans) {
		btrfs_end_transaction(trans, BTRFS_I(inode)->root);
		mark_inode_dirty(inode);
	}
	mutex_unlock(&inode->i_mutex);
	return 0;

truncate_racing:
	vmtruncate(inode, inode->i_size);
	balance_dirty_pages_ratelimited_nr(inode->i_mapping,
					   total_read);
	goto out_unlock;
}

/*
 * The back references tell us which tree holds a ref on a block,
 * but it is possible for the tree root field in the reference to
 * reflect the original root before a snapshot was made.  In this
 * case we should search through all the children of a given root
 * to find potential holders of references on a block.
 *
 * Instead, we do something a little less fancy and just search
 * all the roots for a given key/block combination.
 */
static int find_root_for_ref(struct btrfs_root *root,
			     struct btrfs_path *path,
			     struct btrfs_key *key0,
			     int level,
			     int file_key,
			     struct btrfs_root **found_root,
			     u64 bytenr)
{
	struct btrfs_key root_location;
	struct btrfs_root *cur_root = *found_root;
	struct btrfs_file_extent_item *file_extent;
	u64 root_search_start = BTRFS_FS_TREE_OBJECTID;
	u64 found_bytenr;
	int ret;

	root_location.offset = (u64)-1;
	root_location.type = BTRFS_ROOT_ITEM_KEY;
	path->lowest_level = level;
	path->reada = 0;
	while(1) {
		ret = btrfs_search_slot(NULL, cur_root, key0, path, 0, 0);
		found_bytenr = 0;
		if (ret == 0 && file_key) {
			struct extent_buffer *leaf = path->nodes[0];
			file_extent = btrfs_item_ptr(leaf, path->slots[0],
					     struct btrfs_file_extent_item);
			if (btrfs_file_extent_type(leaf, file_extent) ==
			    BTRFS_FILE_EXTENT_REG) {
				found_bytenr =
					btrfs_file_extent_disk_bytenr(leaf,
							       file_extent);
		       }
		} else if (!file_key) {
			if (path->nodes[level])
				found_bytenr = path->nodes[level]->start;
		}

		btrfs_release_path(cur_root, path);

		if (found_bytenr == bytenr) {
			*found_root = cur_root;
			ret = 0;
			goto out;
		}
		ret = btrfs_search_root(root->fs_info->tree_root,
					root_search_start, &root_search_start);
		if (ret)
			break;

		root_location.objectid = root_search_start;
		cur_root = btrfs_read_fs_root_no_name(root->fs_info,
						      &root_location);
		if (!cur_root) {
			ret = 1;
			break;
		}
	}
out:
	path->lowest_level = 0;
	return ret;
}

/*
 * note, this releases the path
 */
static int noinline relocate_one_reference(struct btrfs_root *extent_root,
				  struct btrfs_path *path,
				  struct btrfs_key *extent_key,
				  u64 *last_file_objectid,
				  u64 *last_file_offset,
				  u64 *last_file_root,
				  u64 last_extent)
{
	struct inode *inode;
	struct btrfs_root *found_root;
	struct btrfs_key root_location;
	struct btrfs_key found_key;
	struct btrfs_extent_ref *ref;
	u64 ref_root;
	u64 ref_gen;
	u64 ref_objectid;
	u64 ref_offset;
	int ret;
	int level;

	WARN_ON(!mutex_is_locked(&extent_root->fs_info->alloc_mutex));

	ref = btrfs_item_ptr(path->nodes[0], path->slots[0],
			     struct btrfs_extent_ref);
	ref_root = btrfs_ref_root(path->nodes[0], ref);
	ref_gen = btrfs_ref_generation(path->nodes[0], ref);
	ref_objectid = btrfs_ref_objectid(path->nodes[0], ref);
	ref_offset = btrfs_ref_offset(path->nodes[0], ref);
	btrfs_release_path(extent_root, path);

	root_location.objectid = ref_root;
	if (ref_gen == 0)
		root_location.offset = 0;
	else
		root_location.offset = (u64)-1;
	root_location.type = BTRFS_ROOT_ITEM_KEY;

	found_root = btrfs_read_fs_root_no_name(extent_root->fs_info,
						&root_location);
	BUG_ON(!found_root);
	mutex_unlock(&extent_root->fs_info->alloc_mutex);

	if (ref_objectid >= BTRFS_FIRST_FREE_OBJECTID) {
		found_key.objectid = ref_objectid;
		found_key.type = BTRFS_EXTENT_DATA_KEY;
		found_key.offset = ref_offset;
		level = 0;

		if (last_extent == extent_key->objectid &&
		    *last_file_objectid == ref_objectid &&
		    *last_file_offset == ref_offset &&
		    *last_file_root == ref_root)
			goto out;

		ret = find_root_for_ref(extent_root, path, &found_key,
					level, 1, &found_root,
					extent_key->objectid);

		if (ret)
			goto out;

		if (last_extent == extent_key->objectid &&
		    *last_file_objectid == ref_objectid &&
		    *last_file_offset == ref_offset &&
		    *last_file_root == ref_root)
			goto out;

		inode = btrfs_iget_locked(extent_root->fs_info->sb,
					  ref_objectid, found_root);
		if (inode->i_state & I_NEW) {
			/* the inode and parent dir are two different roots */
			BTRFS_I(inode)->root = found_root;
			BTRFS_I(inode)->location.objectid = ref_objectid;
			BTRFS_I(inode)->location.type = BTRFS_INODE_ITEM_KEY;
			BTRFS_I(inode)->location.offset = 0;
			btrfs_read_locked_inode(inode);
			unlock_new_inode(inode);

		}
		/* this can happen if the reference is not against
		 * the latest version of the tree root
		 */
		if (is_bad_inode(inode))
			goto out;

		*last_file_objectid = inode->i_ino;
		*last_file_root = found_root->root_key.objectid;
		*last_file_offset = ref_offset;

		relocate_inode_pages(inode, ref_offset, extent_key->offset);
		iput(inode);
	} else {
		struct btrfs_trans_handle *trans;
		struct extent_buffer *eb;
		int needs_lock = 0;

		eb = read_tree_block(found_root, extent_key->objectid,
				     extent_key->offset, 0);
		btrfs_tree_lock(eb);
		level = btrfs_header_level(eb);

		if (level == 0)
			btrfs_item_key_to_cpu(eb, &found_key, 0);
		else
			btrfs_node_key_to_cpu(eb, &found_key, 0);

		btrfs_tree_unlock(eb);
		free_extent_buffer(eb);

		ret = find_root_for_ref(extent_root, path, &found_key,
					level, 0, &found_root,
					extent_key->objectid);

		if (ret)
			goto out;

		/*
		 * right here almost anything could happen to our key,
		 * but that's ok.  The cow below will either relocate it
		 * or someone else will have relocated it.  Either way,
		 * it is in a different spot than it was before and
		 * we're happy.
		 */

		trans = btrfs_start_transaction(found_root, 1);

		if (found_root == extent_root->fs_info->extent_root ||
		    found_root == extent_root->fs_info->chunk_root ||
		    found_root == extent_root->fs_info->dev_root) {
			needs_lock = 1;
			mutex_lock(&extent_root->fs_info->alloc_mutex);
		}

		path->lowest_level = level;
		path->reada = 2;
		ret = btrfs_search_slot(trans, found_root, &found_key, path,
					0, 1);
		path->lowest_level = 0;
		btrfs_release_path(found_root, path);

		if (found_root == found_root->fs_info->extent_root)
			btrfs_extent_post_op(trans, found_root);
		if (needs_lock)
			mutex_unlock(&extent_root->fs_info->alloc_mutex);

		btrfs_end_transaction(trans, found_root);

	}
out:
	mutex_lock(&extent_root->fs_info->alloc_mutex);
	return 0;
}

static int noinline del_extent_zero(struct btrfs_root *extent_root,
				    struct btrfs_path *path,
				    struct btrfs_key *extent_key)
{
	int ret;
	struct btrfs_trans_handle *trans;

	trans = btrfs_start_transaction(extent_root, 1);
	ret = btrfs_search_slot(trans, extent_root, extent_key, path, -1, 1);
	if (ret > 0) {
		ret = -EIO;
		goto out;
	}
	if (ret < 0)
		goto out;
	ret = btrfs_del_item(trans, extent_root, path);
out:
	btrfs_end_transaction(trans, extent_root);
	return ret;
}

static int noinline relocate_one_extent(struct btrfs_root *extent_root,
					struct btrfs_path *path,
					struct btrfs_key *extent_key)
{
	struct btrfs_key key;
	struct btrfs_key found_key;
	struct extent_buffer *leaf;
	u64 last_file_objectid = 0;
	u64 last_file_root = 0;
	u64 last_file_offset = (u64)-1;
	u64 last_extent = 0;
	u32 nritems;
	u32 item_size;
	int ret = 0;

	if (extent_key->objectid == 0) {
		ret = del_extent_zero(extent_root, path, extent_key);
		goto out;
	}
	key.objectid = extent_key->objectid;
	key.type = BTRFS_EXTENT_REF_KEY;
	key.offset = 0;

	while(1) {
		ret = btrfs_search_slot(NULL, extent_root, &key, path, 0, 0);

		if (ret < 0)
			goto out;

		ret = 0;
		leaf = path->nodes[0];
		nritems = btrfs_header_nritems(leaf);
		if (path->slots[0] == nritems) {
			ret = btrfs_next_leaf(extent_root, path);
			if (ret > 0) {
				ret = 0;
				goto out;
			}
			if (ret < 0)
				goto out;
			leaf = path->nodes[0];
		}

		btrfs_item_key_to_cpu(leaf, &found_key, path->slots[0]);
		if (found_key.objectid != extent_key->objectid) {
			break;
		}

		if (found_key.type != BTRFS_EXTENT_REF_KEY) {
			break;
		}

		key.offset = found_key.offset + 1;
		item_size = btrfs_item_size_nr(leaf, path->slots[0]);

		ret = relocate_one_reference(extent_root, path, extent_key,
					     &last_file_objectid,
					     &last_file_offset,
					     &last_file_root, last_extent);
		if (ret)
			goto out;
		last_extent = extent_key->objectid;
	}
	ret = 0;
out:
	btrfs_release_path(extent_root, path);
	return ret;
}

static u64 update_block_group_flags(struct btrfs_root *root, u64 flags)
{
	u64 num_devices;
	u64 stripped = BTRFS_BLOCK_GROUP_RAID0 |
		BTRFS_BLOCK_GROUP_RAID1 | BTRFS_BLOCK_GROUP_RAID10;

	num_devices = root->fs_info->fs_devices->num_devices;
	if (num_devices == 1) {
		stripped |= BTRFS_BLOCK_GROUP_DUP;
		stripped = flags & ~stripped;

		/* turn raid0 into single device chunks */
		if (flags & BTRFS_BLOCK_GROUP_RAID0)
			return stripped;

		/* turn mirroring into duplication */
		if (flags & (BTRFS_BLOCK_GROUP_RAID1 |
			     BTRFS_BLOCK_GROUP_RAID10))
			return stripped | BTRFS_BLOCK_GROUP_DUP;
		return flags;
	} else {
		/* they already had raid on here, just return */
		if (flags & stripped)
			return flags;

		stripped |= BTRFS_BLOCK_GROUP_DUP;
		stripped = flags & ~stripped;

		/* switch duplicated blocks with raid1 */
		if (flags & BTRFS_BLOCK_GROUP_DUP)
			return stripped | BTRFS_BLOCK_GROUP_RAID1;

		/* turn single device chunks into raid0 */
		return stripped | BTRFS_BLOCK_GROUP_RAID0;
	}
	return flags;
}

int __alloc_chunk_for_shrink(struct btrfs_root *root,
		     struct btrfs_block_group_cache *shrink_block_group,
		     int force)
{
	struct btrfs_trans_handle *trans;
	u64 new_alloc_flags;
	u64 calc;

	spin_lock(&shrink_block_group->lock);
	if (btrfs_block_group_used(&shrink_block_group->item) > 0) {
		spin_unlock(&shrink_block_group->lock);
		mutex_unlock(&root->fs_info->alloc_mutex);

		trans = btrfs_start_transaction(root, 1);
		mutex_lock(&root->fs_info->alloc_mutex);
		spin_lock(&shrink_block_group->lock);

		new_alloc_flags = update_block_group_flags(root,
						   shrink_block_group->flags);
		if (new_alloc_flags != shrink_block_group->flags) {
			calc =
			     btrfs_block_group_used(&shrink_block_group->item);
		} else {
			calc = shrink_block_group->key.offset;
		}
		spin_unlock(&shrink_block_group->lock);

		do_chunk_alloc(trans, root->fs_info->extent_root,
			       calc + 2 * 1024 * 1024, new_alloc_flags, force);

		mutex_unlock(&root->fs_info->alloc_mutex);
		btrfs_end_transaction(trans, root);
		mutex_lock(&root->fs_info->alloc_mutex);
	} else
		spin_unlock(&shrink_block_group->lock);
	return 0;
}

int btrfs_shrink_extent_tree(struct btrfs_root *root, u64 shrink_start)
{
	struct btrfs_trans_handle *trans;
	struct btrfs_root *tree_root = root->fs_info->tree_root;
	struct btrfs_path *path;
	u64 cur_byte;
	u64 total_found;
	u64 shrink_last_byte;
	struct btrfs_block_group_cache *shrink_block_group;
	struct btrfs_fs_info *info = root->fs_info;
	struct btrfs_key key;
	struct btrfs_key found_key;
	struct extent_buffer *leaf;
	u32 nritems;
	int ret;
	int progress;

	mutex_lock(&root->fs_info->alloc_mutex);
	shrink_block_group = btrfs_lookup_block_group(root->fs_info,
						      shrink_start);
	BUG_ON(!shrink_block_group);

	shrink_last_byte = shrink_block_group->key.objectid +
		shrink_block_group->key.offset;

	shrink_block_group->space_info->total_bytes -=
		shrink_block_group->key.offset;
	path = btrfs_alloc_path();
	root = root->fs_info->extent_root;
	path->reada = 2;

	printk("btrfs relocating block group %llu flags %llu\n",
	       (unsigned long long)shrink_start,
	       (unsigned long long)shrink_block_group->flags);

	__alloc_chunk_for_shrink(root, shrink_block_group, 1);

again:

	shrink_block_group->ro = 1;

	total_found = 0;
	progress = 0;
	key.objectid = shrink_start;
	key.offset = 0;
	key.type = 0;
	cur_byte = key.objectid;

	ret = btrfs_search_slot(NULL, root, &key, path, 0, 0);
	if (ret < 0)
		goto out;

	ret = btrfs_previous_item(root, path, 0, BTRFS_EXTENT_ITEM_KEY);
	if (ret < 0)
		goto out;

	if (ret == 0) {
		leaf = path->nodes[0];
		btrfs_item_key_to_cpu(leaf, &found_key, path->slots[0]);
		if (found_key.objectid + found_key.offset > shrink_start &&
		    found_key.objectid < shrink_last_byte) {
			cur_byte = found_key.objectid;
			key.objectid = cur_byte;
		}
	}
	btrfs_release_path(root, path);

	while(1) {
		ret = btrfs_search_slot(NULL, root, &key, path, 0, 0);
		if (ret < 0)
			goto out;

next:
		leaf = path->nodes[0];
		nritems = btrfs_header_nritems(leaf);
		if (path->slots[0] >= nritems) {
			ret = btrfs_next_leaf(root, path);
			if (ret < 0)
				goto out;
			if (ret == 1) {
				ret = 0;
				break;
			}
			leaf = path->nodes[0];
			nritems = btrfs_header_nritems(leaf);
		}

		btrfs_item_key_to_cpu(leaf, &found_key, path->slots[0]);

		if (found_key.objectid >= shrink_last_byte)
			break;

		if (progress && need_resched()) {
			memcpy(&key, &found_key, sizeof(key));
			cond_resched();
			btrfs_release_path(root, path);
			btrfs_search_slot(NULL, root, &key, path, 0, 0);
			progress = 0;
			goto next;
		}
		progress = 1;

		if (btrfs_key_type(&found_key) != BTRFS_EXTENT_ITEM_KEY ||
		    found_key.objectid + found_key.offset <= cur_byte) {
			memcpy(&key, &found_key, sizeof(key));
			key.offset++;
			path->slots[0]++;
			goto next;
		}

		total_found++;
		cur_byte = found_key.objectid + found_key.offset;
		key.objectid = cur_byte;
		btrfs_release_path(root, path);
		ret = relocate_one_extent(root, path, &found_key);
		__alloc_chunk_for_shrink(root, shrink_block_group, 0);
	}

	btrfs_release_path(root, path);

	if (total_found > 0) {
		printk("btrfs relocate found %llu last extent was %llu\n",
		       (unsigned long long)total_found,
		       (unsigned long long)found_key.objectid);
		mutex_unlock(&root->fs_info->alloc_mutex);
		trans = btrfs_start_transaction(tree_root, 1);
		btrfs_commit_transaction(trans, tree_root);

		btrfs_clean_old_snapshots(tree_root);

		btrfs_wait_ordered_extents(tree_root);

		trans = btrfs_start_transaction(tree_root, 1);
		btrfs_commit_transaction(trans, tree_root);
		mutex_lock(&root->fs_info->alloc_mutex);
		goto again;
	}

	/*
	 * we've freed all the extents, now remove the block
	 * group item from the tree
	 */
	mutex_unlock(&root->fs_info->alloc_mutex);

	trans = btrfs_start_transaction(root, 1);

	mutex_lock(&root->fs_info->alloc_mutex);
	memcpy(&key, &shrink_block_group->key, sizeof(key));

	ret = btrfs_search_slot(trans, root, &key, path, -1, 1);
	if (ret > 0)
		ret = -EIO;
	if (ret < 0) {
		btrfs_end_transaction(trans, root);
		goto out;
	}

	clear_extent_bits(&info->block_group_cache, key.objectid,
			  key.objectid + key.offset - 1,
			  (unsigned int)-1, GFP_NOFS);


	clear_extent_bits(&info->free_space_cache,
			   key.objectid, key.objectid + key.offset - 1,
			   (unsigned int)-1, GFP_NOFS);

	memset(shrink_block_group, 0, sizeof(*shrink_block_group));
	kfree(shrink_block_group);

	btrfs_del_item(trans, root, path);
	btrfs_release_path(root, path);
	mutex_unlock(&root->fs_info->alloc_mutex);
	btrfs_commit_transaction(trans, root);

	mutex_lock(&root->fs_info->alloc_mutex);

	/* the code to unpin extents might set a few bits in the free
	 * space cache for this range again
	 */
	clear_extent_bits(&info->free_space_cache,
			   key.objectid, key.objectid + key.offset - 1,
			   (unsigned int)-1, GFP_NOFS);
out:
	btrfs_free_path(path);
	mutex_unlock(&root->fs_info->alloc_mutex);
	return ret;
}

int find_first_block_group(struct btrfs_root *root, struct btrfs_path *path,
			   struct btrfs_key *key)
{
	int ret = 0;
	struct btrfs_key found_key;
	struct extent_buffer *leaf;
	int slot;

	ret = btrfs_search_slot(NULL, root, key, path, 0, 0);
	if (ret < 0)
		goto out;

	while(1) {
		slot = path->slots[0];
		leaf = path->nodes[0];
		if (slot >= btrfs_header_nritems(leaf)) {
			ret = btrfs_next_leaf(root, path);
			if (ret == 0)
				continue;
			if (ret < 0)
				goto out;
			break;
		}
		btrfs_item_key_to_cpu(leaf, &found_key, slot);

		if (found_key.objectid >= key->objectid &&
		    found_key.type == BTRFS_BLOCK_GROUP_ITEM_KEY) {
			ret = 0;
			goto out;
		}
		path->slots[0]++;
	}
	ret = -ENOENT;
out:
	return ret;
}

int btrfs_read_block_groups(struct btrfs_root *root)
{
	struct btrfs_path *path;
	int ret;
	int bit;
	struct btrfs_block_group_cache *cache;
	struct btrfs_fs_info *info = root->fs_info;
	struct btrfs_space_info *space_info;
	struct extent_io_tree *block_group_cache;
	struct btrfs_key key;
	struct btrfs_key found_key;
	struct extent_buffer *leaf;

	block_group_cache = &info->block_group_cache;
	root = info->extent_root;
	key.objectid = 0;
	key.offset = 0;
	btrfs_set_key_type(&key, BTRFS_BLOCK_GROUP_ITEM_KEY);
	path = btrfs_alloc_path();
	if (!path)
		return -ENOMEM;

	mutex_lock(&root->fs_info->alloc_mutex);
	while(1) {
		ret = find_first_block_group(root, path, &key);
		if (ret > 0) {
			ret = 0;
			goto error;
		}
		if (ret != 0)
			goto error;

		leaf = path->nodes[0];
		btrfs_item_key_to_cpu(leaf, &found_key, path->slots[0]);
		cache = kzalloc(sizeof(*cache), GFP_NOFS);
		if (!cache) {
			ret = -ENOMEM;
			break;
		}

		spin_lock_init(&cache->lock);
		read_extent_buffer(leaf, &cache->item,
				   btrfs_item_ptr_offset(leaf, path->slots[0]),
				   sizeof(cache->item));
		memcpy(&cache->key, &found_key, sizeof(found_key));

		key.objectid = found_key.objectid + found_key.offset;
		btrfs_release_path(root, path);
		cache->flags = btrfs_block_group_flags(&cache->item);
		bit = 0;
		if (cache->flags & BTRFS_BLOCK_GROUP_DATA) {
			bit = BLOCK_GROUP_DATA;
		} else if (cache->flags & BTRFS_BLOCK_GROUP_SYSTEM) {
			bit = BLOCK_GROUP_SYSTEM;
		} else if (cache->flags & BTRFS_BLOCK_GROUP_METADATA) {
			bit = BLOCK_GROUP_METADATA;
		}
		set_avail_alloc_bits(info, cache->flags);

		ret = update_space_info(info, cache->flags, found_key.offset,
					btrfs_block_group_used(&cache->item),
					&space_info);
		BUG_ON(ret);
		cache->space_info = space_info;

		/* use EXTENT_LOCKED to prevent merging */
		set_extent_bits(block_group_cache, found_key.objectid,
				found_key.objectid + found_key.offset - 1,
				EXTENT_LOCKED, GFP_NOFS);
		set_state_private(block_group_cache, found_key.objectid,
				  (unsigned long)cache);
		set_extent_bits(block_group_cache, found_key.objectid,
				found_key.objectid + found_key.offset - 1,
				bit | EXTENT_LOCKED, GFP_NOFS);
		if (key.objectid >=
		    btrfs_super_total_bytes(&info->super_copy))
			break;
	}
	ret = 0;
error:
	btrfs_free_path(path);
	mutex_unlock(&root->fs_info->alloc_mutex);
	return ret;
}

int btrfs_make_block_group(struct btrfs_trans_handle *trans,
			   struct btrfs_root *root, u64 bytes_used,
			   u64 type, u64 chunk_objectid, u64 chunk_offset,
			   u64 size)
{
	int ret;
	int bit = 0;
	struct btrfs_root *extent_root;
	struct btrfs_block_group_cache *cache;
	struct extent_io_tree *block_group_cache;

	WARN_ON(!mutex_is_locked(&root->fs_info->alloc_mutex));
	extent_root = root->fs_info->extent_root;
	block_group_cache = &root->fs_info->block_group_cache;

	cache = kzalloc(sizeof(*cache), GFP_NOFS);
	BUG_ON(!cache);
	cache->key.objectid = chunk_offset;
	cache->key.offset = size;
	spin_lock_init(&cache->lock);
	btrfs_set_key_type(&cache->key, BTRFS_BLOCK_GROUP_ITEM_KEY);

	btrfs_set_block_group_used(&cache->item, bytes_used);
	btrfs_set_block_group_chunk_objectid(&cache->item, chunk_objectid);
	cache->flags = type;
	btrfs_set_block_group_flags(&cache->item, type);

	ret = update_space_info(root->fs_info, cache->flags, size, bytes_used,
				&cache->space_info);
	BUG_ON(ret);

	bit = block_group_state_bits(type);
	set_extent_bits(block_group_cache, chunk_offset,
			chunk_offset + size - 1,
			EXTENT_LOCKED, GFP_NOFS);
	set_state_private(block_group_cache, chunk_offset,
			  (unsigned long)cache);
	set_extent_bits(block_group_cache, chunk_offset,
			chunk_offset + size - 1,
			bit | EXTENT_LOCKED, GFP_NOFS);

	ret = btrfs_insert_item(trans, extent_root, &cache->key, &cache->item,
				sizeof(cache->item));
	BUG_ON(ret);

	finish_current_insert(trans, extent_root);
	ret = del_pending_extents(trans, extent_root);
	BUG_ON(ret);
	set_avail_alloc_bits(extent_root->fs_info, type);

	return 0;
}
