/*
 * osd_initiator - Main body of the osd initiator library.
 *
 * Note: The file does not contain the advanced security functionality which
 * is only needed by the security_manager's initiators.
 *
 * Copyright (C) 2008 Panasas Inc.  All rights reserved.
 *
 * Authors:
 *   Boaz Harrosh <bharrosh@panasas.com>
 *   Benny Halevy <bhalevy@panasas.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 *  1. Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 *  2. Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 *  3. Neither the name of the Panasas company nor the names of its
 *     contributors may be used to endorse or promote products derived
 *     from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED ``AS IS'' AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
 * BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <scsi/osd_initiator.h>
#include <scsi/osd_sec.h>
#include <scsi/scsi_device.h>

#include "osd_debug.h"

#ifndef __unused
#    define __unused			__attribute__((unused))
#endif

enum { OSD_REQ_RETRIES = 1 };

MODULE_AUTHOR("Boaz Harrosh <bharrosh@panasas.com>");
MODULE_DESCRIPTION("open-osd initiator library libosd.ko");
MODULE_LICENSE("GPL");

static inline void build_test(void)
{
	/* structures were not packed */
	BUILD_BUG_ON(sizeof(struct osd_capability) != OSD_CAP_LEN);
	BUILD_BUG_ON(sizeof(struct osdv1_cdb) != OSDv1_TOTAL_CDB_LEN);
}

static unsigned _osd_req_cdb_len(struct osd_request *or)
{
	return OSDv1_TOTAL_CDB_LEN;
}

static unsigned _osd_req_alist_elem_size(struct osd_request *or, unsigned len)
{
	return osdv1_attr_list_elem_size(len);
}

static unsigned _osd_req_alist_size(struct osd_request *or, void *list_head)
{
	return osdv1_list_size(list_head);
}

static unsigned _osd_req_sizeof_alist_header(struct osd_request *or)
{
	return sizeof(struct osdv1_attributes_list_header);
}

static void _osd_req_set_alist_type(struct osd_request *or,
	void *list, int list_type)
{
	struct osdv1_attributes_list_header *attr_list = list;

	memset(attr_list, 0, sizeof(*attr_list));
	attr_list->type = list_type;
}

static bool _osd_req_is_alist_type(struct osd_request *or,
	void *list, int list_type)
{
	if (!list)
		return false;

	if (1) {
		struct osdv1_attributes_list_header *attr_list = list;

		return attr_list->type == list_type;
	}
}

/* This is for List-objects not Attributes-Lists */
static void _osd_req_encode_olist(struct osd_request *or,
	struct osd_obj_id_list *list)
{
	struct osd_cdb_head *cdbh = osd_cdb_head(&or->cdb);

	cdbh->v1.list_identifier = list->list_identifier;
	cdbh->v1.start_address = list->continuation_id;
}

static osd_cdb_offset osd_req_encode_offset(struct osd_request *or,
	u64 offset, unsigned *padding)
{
	return __osd_encode_offset(offset, padding,
				  OSDv1_OFFSET_MIN_SHIFT, OSD_OFFSET_MAX_SHIFT);
}

static struct osd_security_parameters *
_osd_req_sec_params(struct osd_request *or)
{
	struct osd_cdb *ocdb = &or->cdb;

	return &ocdb->v1.sec_params;
}

void osd_dev_init(struct osd_dev *osdd, struct scsi_device *scsi_device)
{
	memset(osdd, 0, sizeof(*osdd));
	osdd->scsi_device = scsi_device;
	osdd->def_timeout = BLK_DEFAULT_SG_TIMEOUT;
	/* TODO: Allocate pools for osd_request attributes ... */
}
EXPORT_SYMBOL(osd_dev_init);

void osd_dev_fini(struct osd_dev *osdd)
{
	/* TODO: De-allocate pools */

	osdd->scsi_device = NULL;
}
EXPORT_SYMBOL(osd_dev_fini);

static struct osd_request *_osd_request_alloc(gfp_t gfp)
{
	struct osd_request *or;

	/* TODO: Use mempool with one saved request */
	or = kzalloc(sizeof(*or), gfp);
	return or;
}

static void _osd_request_free(struct osd_request *or)
{
	kfree(or);
}

struct osd_request *osd_start_request(struct osd_dev *dev, gfp_t gfp)
{
	struct osd_request *or;

	or = _osd_request_alloc(gfp);
	if (!or)
		return NULL;

	or->osd_dev = dev;
	or->alloc_flags = gfp;
	or->timeout = dev->def_timeout;
	or->retries = OSD_REQ_RETRIES;

	return or;
}
EXPORT_SYMBOL(osd_start_request);

/*
 * If osd_finalize_request() was called but the request was not executed through
 * the block layer, then we must release BIOs.
 */
static void _abort_unexecuted_bios(struct request *rq)
{
	struct bio *bio;

	while ((bio = rq->bio) != NULL) {
		rq->bio = bio->bi_next;
		bio_endio(bio, 0);
	}
}

static void _osd_free_seg(struct osd_request *or __unused,
	struct _osd_req_data_segment *seg)
{
	if (!seg->buff || !seg->alloc_size)
		return;

	kfree(seg->buff);
	seg->buff = NULL;
	seg->alloc_size = 0;
}

void osd_end_request(struct osd_request *or)
{
	struct request *rq = or->request;

	_osd_free_seg(or, &or->set_attr);
	_osd_free_seg(or, &or->enc_get_attr);
	_osd_free_seg(or, &or->get_attr);

	if (rq) {
		if (rq->next_rq) {
			_abort_unexecuted_bios(rq->next_rq);
			blk_put_request(rq->next_rq);
		}

		_abort_unexecuted_bios(rq);
		blk_put_request(rq);
	}
	_osd_request_free(or);
}
EXPORT_SYMBOL(osd_end_request);

int osd_execute_request(struct osd_request *or)
{
	return blk_execute_rq(or->request->q, NULL, or->request, 0);
}
EXPORT_SYMBOL(osd_execute_request);

static void osd_request_async_done(struct request *req, int error)
{
	struct osd_request *or = req->end_io_data;

	or->async_error = error;

	if (error)
		OSD_DEBUG("osd_request_async_done error recieved %d\n", error);

	if (or->async_done)
		or->async_done(or, or->async_private);
	else
		osd_end_request(or);
}

int osd_execute_request_async(struct osd_request *or,
	osd_req_done_fn *done, void *private)
{
	or->request->end_io_data = or;
	or->async_private = private;
	or->async_done = done;

	blk_execute_rq_nowait(or->request->q, NULL, or->request, 0,
			      osd_request_async_done);
	return 0;
}
EXPORT_SYMBOL(osd_execute_request_async);

u8 sg_out_pad_buffer[1 << OSDv1_OFFSET_MIN_SHIFT];
u8 sg_in_pad_buffer[1 << OSDv1_OFFSET_MIN_SHIFT];

static int _osd_realloc_seg(struct osd_request *or,
	struct _osd_req_data_segment *seg, unsigned max_bytes)
{
	void *buff;

	if (seg->alloc_size >= max_bytes)
		return 0;

	buff = krealloc(seg->buff, max_bytes, or->alloc_flags);
	if (!buff) {
		OSD_ERR("Failed to Realloc %d-bytes was-%d\n", max_bytes,
			seg->alloc_size);
		return -ENOMEM;
	}

	memset(buff + seg->alloc_size, 0, max_bytes - seg->alloc_size);
	seg->buff = buff;
	seg->alloc_size = max_bytes;
	return 0;
}

static int _alloc_set_attr_list(struct osd_request *or,
	const struct osd_attr *oa, unsigned nelem, unsigned add_bytes)
{
	unsigned total_bytes = add_bytes;

	for (; nelem; --nelem, ++oa)
		total_bytes += _osd_req_alist_elem_size(or, oa->len);

	OSD_DEBUG("total_bytes=%d\n", total_bytes);
	return _osd_realloc_seg(or, &or->set_attr, total_bytes);
}

static int _alloc_get_attr_desc(struct osd_request *or, unsigned max_bytes)
{
	OSD_DEBUG("total_bytes=%d\n", max_bytes);
	return _osd_realloc_seg(or, &or->enc_get_attr, max_bytes);
}

static int _alloc_get_attr_list(struct osd_request *or)
{
	OSD_DEBUG("total_bytes=%d\n", or->get_attr.total_bytes);
	return _osd_realloc_seg(or, &or->get_attr, or->get_attr.total_bytes);
}

/*
 * Common to all OSD commands
 */

static void _osdv1_req_encode_common(struct osd_request *or,
	__be16 act, const struct osd_obj_id *obj, u64 offset, u64 len)
{
	struct osdv1_cdb *ocdb = &or->cdb.v1;

	/*
	 * For speed, the commands
	 *	OSD_ACT_PERFORM_SCSI_COMMAND	, V1 0x8F7E, V2 0x8F7C
	 *	OSD_ACT_SCSI_TASK_MANAGEMENT	, V1 0x8F7F, V2 0x8F7D
	 * are not supported here. Should pass zero and set after the call
	 */
	act &= cpu_to_be16(~0x0080); /* V1 action code */

	OSD_DEBUG("OSDv1 execute opcode 0x%x\n", be16_to_cpu(act));

	ocdb->h.varlen_cdb.opcode = VARIABLE_LENGTH_CMD;
	ocdb->h.varlen_cdb.additional_cdb_length = OSD_ADDITIONAL_CDB_LENGTH;
	ocdb->h.varlen_cdb.service_action = act;

	ocdb->h.partition = cpu_to_be64(obj->partition);
	ocdb->h.object = cpu_to_be64(obj->id);
	ocdb->h.v1.length = cpu_to_be64(len);
	ocdb->h.v1.start_address = cpu_to_be64(offset);
}

static void _osd_req_encode_common(struct osd_request *or,
	__be16 act, const struct osd_obj_id *obj, u64 offset, u64 len)
{
	_osdv1_req_encode_common(or, act, obj, offset, len);
}

/*
 * Device commands
 */
void osd_req_format(struct osd_request *or, u64 tot_capacity)
{
	_osd_req_encode_common(or, OSD_ACT_FORMAT_OSD, &osd_root_object, 0,
				tot_capacity);
}
EXPORT_SYMBOL(osd_req_format);

int osd_req_list_dev_partitions(struct osd_request *or,
	osd_id initial_id, struct osd_obj_id_list *list, unsigned nelem)
{
	return osd_req_list_partition_objects(or, 0, initial_id, list, nelem);
}
EXPORT_SYMBOL(osd_req_list_dev_partitions);

static void _osd_req_encode_flush(struct osd_request *or,
	enum osd_options_flush_scope_values op)
{
	struct osd_cdb_head *ocdb = osd_cdb_head(&or->cdb);

	ocdb->command_specific_options = op;
}

void osd_req_flush_obsd(struct osd_request *or,
	enum osd_options_flush_scope_values op)
{
	_osd_req_encode_common(or, OSD_ACT_FLUSH_OSD, &osd_root_object, 0, 0);
	_osd_req_encode_flush(or, op);
}
EXPORT_SYMBOL(osd_req_flush_obsd);

/*
 * Partition commands
 */
static void _osd_req_encode_partition(struct osd_request *or,
	__be16 act, osd_id partition)
{
	struct osd_obj_id par = {
		.partition = partition,
		.id = 0,
	};

	_osd_req_encode_common(or, act, &par, 0, 0);
}

void osd_req_create_partition(struct osd_request *or, osd_id partition)
{
	_osd_req_encode_partition(or, OSD_ACT_CREATE_PARTITION, partition);
}
EXPORT_SYMBOL(osd_req_create_partition);

void osd_req_remove_partition(struct osd_request *or, osd_id partition)
{
	_osd_req_encode_partition(or, OSD_ACT_REMOVE_PARTITION, partition);
}
EXPORT_SYMBOL(osd_req_remove_partition);

static int _osd_req_list_objects(struct osd_request *or,
	__be16 action, const struct osd_obj_id *obj, osd_id initial_id,
	struct osd_obj_id_list *list, unsigned nelem)
{
	struct request_queue *q = or->osd_dev->scsi_device->request_queue;
	u64 len = nelem * sizeof(osd_id) + sizeof(*list);
	struct bio *bio;

	_osd_req_encode_common(or, action, obj, (u64)initial_id, len);

	if (list->list_identifier)
		_osd_req_encode_olist(or, list);

	WARN_ON(or->in.bio);
	bio = bio_map_kern(q, list, len, or->alloc_flags);
	if (!bio) {
		OSD_ERR("!!! Failed to allocate list_objects BIO\n");
		return -ENOMEM;
	}

	bio->bi_rw &= ~(1 << BIO_RW);
	or->in.bio = bio;
	or->in.total_bytes = bio->bi_size;
	return 0;
}

int osd_req_list_partition_collections(struct osd_request *or,
	osd_id partition, osd_id initial_id, struct osd_obj_id_list *list,
	unsigned nelem)
{
	struct osd_obj_id par = {
		.partition = partition,
		.id = 0,
	};

	return osd_req_list_collection_objects(or, &par, initial_id, list,
					       nelem);
}
EXPORT_SYMBOL(osd_req_list_partition_collections);

int osd_req_list_partition_objects(struct osd_request *or,
	osd_id partition, osd_id initial_id, struct osd_obj_id_list *list,
	unsigned nelem)
{
	struct osd_obj_id par = {
		.partition = partition,
		.id = 0,
	};

	return _osd_req_list_objects(or, OSD_ACT_LIST, &par, initial_id, list,
				     nelem);
}
EXPORT_SYMBOL(osd_req_list_partition_objects);

void osd_req_flush_partition(struct osd_request *or,
	osd_id partition, enum osd_options_flush_scope_values op)
{
	_osd_req_encode_partition(or, OSD_ACT_FLUSH_PARTITION, partition);
	_osd_req_encode_flush(or, op);
}
EXPORT_SYMBOL(osd_req_flush_partition);

/*
 * Collection commands
 */
int osd_req_list_collection_objects(struct osd_request *or,
	const struct osd_obj_id *obj, osd_id initial_id,
	struct osd_obj_id_list *list, unsigned nelem)
{
	return _osd_req_list_objects(or, OSD_ACT_LIST_COLLECTION, obj,
				     initial_id, list, nelem);
}
EXPORT_SYMBOL(osd_req_list_collection_objects);

void osd_req_flush_collection(struct osd_request *or,
	const struct osd_obj_id *obj, enum osd_options_flush_scope_values op)
{
	_osd_req_encode_common(or, OSD_ACT_FLUSH_PARTITION, obj, 0, 0);
	_osd_req_encode_flush(or, op);
}
EXPORT_SYMBOL(osd_req_flush_collection);

/*
 * Object commands
 */
void osd_req_create_object(struct osd_request *or, struct osd_obj_id *obj)
{
	_osd_req_encode_common(or, OSD_ACT_CREATE, obj, 0, 0);
}
EXPORT_SYMBOL(osd_req_create_object);

void osd_req_remove_object(struct osd_request *or, struct osd_obj_id *obj)
{
	_osd_req_encode_common(or, OSD_ACT_REMOVE, obj, 0, 0);
}
EXPORT_SYMBOL(osd_req_remove_object);

void osd_req_write(struct osd_request *or,
	const struct osd_obj_id *obj, struct bio *bio, u64 offset)
{
	_osd_req_encode_common(or, OSD_ACT_WRITE, obj, offset, bio->bi_size);
	WARN_ON(or->out.bio || or->out.total_bytes);
	bio->bi_rw |= (1 << BIO_RW);
	or->out.bio = bio;
	or->out.total_bytes = bio->bi_size;
}
EXPORT_SYMBOL(osd_req_write);

void osd_req_flush_object(struct osd_request *or,
	const struct osd_obj_id *obj, enum osd_options_flush_scope_values op,
	/*V2*/ u64 offset, /*V2*/ u64 len)
{
	_osd_req_encode_common(or, OSD_ACT_FLUSH, obj, offset, len);
	_osd_req_encode_flush(or, op);
}
EXPORT_SYMBOL(osd_req_flush_object);

void osd_req_read(struct osd_request *or,
	const struct osd_obj_id *obj, struct bio *bio, u64 offset)
{
	_osd_req_encode_common(or, OSD_ACT_READ, obj, offset, bio->bi_size);
	WARN_ON(or->in.bio || or->in.total_bytes);
	bio->bi_rw &= ~(1 << BIO_RW);
	or->in.bio = bio;
	or->in.total_bytes = bio->bi_size;
}
EXPORT_SYMBOL(osd_req_read);

void osd_req_get_attributes(struct osd_request *or,
	const struct osd_obj_id *obj)
{
	_osd_req_encode_common(or, OSD_ACT_GET_ATTRIBUTES, obj, 0, 0);
}
EXPORT_SYMBOL(osd_req_get_attributes);

void osd_req_set_attributes(struct osd_request *or,
	const struct osd_obj_id *obj)
{
	_osd_req_encode_common(or, OSD_ACT_SET_ATTRIBUTES, obj, 0, 0);
}
EXPORT_SYMBOL(osd_req_set_attributes);

/*
 * Attributes List-mode
 */

int osd_req_add_set_attr_list(struct osd_request *or,
	const struct osd_attr *oa, unsigned nelem)
{
	unsigned total_bytes = or->set_attr.total_bytes;
	void *attr_last;
	int ret;

	if (or->attributes_mode &&
	    or->attributes_mode != OSD_CDB_GET_SET_ATTR_LISTS) {
		WARN_ON(1);
		return -EINVAL;
	}
	or->attributes_mode = OSD_CDB_GET_SET_ATTR_LISTS;

	if (!total_bytes) { /* first-time: allocate and put list header */
		total_bytes = _osd_req_sizeof_alist_header(or);
		ret = _alloc_set_attr_list(or, oa, nelem, total_bytes);
		if (ret)
			return ret;
		_osd_req_set_alist_type(or, or->set_attr.buff,
					OSD_ATTR_LIST_SET_RETRIEVE);
	}
	attr_last = or->set_attr.buff + total_bytes;

	for (; nelem; --nelem) {
		struct osd_attributes_list_element *attr;
		unsigned elem_size = _osd_req_alist_elem_size(or, oa->len);

		total_bytes += elem_size;
		if (unlikely(or->set_attr.alloc_size < total_bytes)) {
			or->set_attr.total_bytes = total_bytes - elem_size;
			ret = _alloc_set_attr_list(or, oa, nelem, total_bytes);
			if (ret)
				return ret;
			attr_last =
				or->set_attr.buff + or->set_attr.total_bytes;
		}

		attr = attr_last;
		attr->attr_page = cpu_to_be32(oa->attr_page);
		attr->attr_id = cpu_to_be32(oa->attr_id);
		attr->attr_bytes = cpu_to_be16(oa->len);
		memcpy(attr->attr_val, oa->val_ptr, oa->len);

		attr_last += elem_size;
		++oa;
	}

	or->set_attr.total_bytes = total_bytes;
	return 0;
}
EXPORT_SYMBOL(osd_req_add_set_attr_list);

static int _append_map_kern(struct request *req,
	void *buff, unsigned len, gfp_t flags)
{
	struct bio *bio;
	int ret;

	bio = bio_map_kern(req->q, buff, len, flags);
	if (IS_ERR(bio)) {
		OSD_ERR("Failed bio_map_kern(%p, %d) => %ld\n", buff, len,
			PTR_ERR(bio));
		return PTR_ERR(bio);
	}
	ret = blk_rq_append_bio(req->q, req, bio);
	if (ret) {
		OSD_ERR("Failed blk_rq_append_bio(%p) => %d\n", bio, ret);
		bio_put(bio);
	}
	return ret;
}

static int _req_append_segment(struct osd_request *or,
	unsigned padding, struct _osd_req_data_segment *seg,
	struct _osd_req_data_segment *last_seg, struct _osd_io_info *io)
{
	void *pad_buff;
	int ret;

	if (padding) {
		/* check if we can just add it to last buffer */
		if (last_seg &&
		    (padding <= last_seg->alloc_size - last_seg->total_bytes))
			pad_buff = last_seg->buff + last_seg->total_bytes;
		else
			pad_buff = io->pad_buff;

		ret = _append_map_kern(io->req, pad_buff, padding,
				       or->alloc_flags);
		if (ret)
			return ret;
		io->total_bytes += padding;
	}

	ret = _append_map_kern(io->req, seg->buff, seg->total_bytes,
			       or->alloc_flags);
	if (ret)
		return ret;

	io->total_bytes += seg->total_bytes;
	OSD_DEBUG("padding=%d buff=%p total_bytes=%d\n", padding, seg->buff,
		  seg->total_bytes);
	return 0;
}

static int _osd_req_finalize_set_attr_list(struct osd_request *or)
{
	struct osd_cdb_head *cdbh = osd_cdb_head(&or->cdb);
	unsigned padding;
	int ret;

	if (!or->set_attr.total_bytes) {
		cdbh->attrs_list.set_attr_offset = OSD_OFFSET_UNUSED;
		return 0;
	}

	cdbh->attrs_list.set_attr_bytes = cpu_to_be32(or->set_attr.total_bytes);
	cdbh->attrs_list.set_attr_offset =
		osd_req_encode_offset(or, or->out.total_bytes, &padding);

	ret = _req_append_segment(or, padding, &or->set_attr,
				  or->out.last_seg, &or->out);
	if (ret)
		return ret;

	or->out.last_seg = &or->set_attr;
	return 0;
}

int osd_req_add_get_attr_list(struct osd_request *or,
	const struct osd_attr *oa, unsigned nelem)
{
	unsigned total_bytes = or->enc_get_attr.total_bytes;
	void *attr_last;
	int ret;

	if (or->attributes_mode &&
	    or->attributes_mode != OSD_CDB_GET_SET_ATTR_LISTS) {
		WARN_ON(1);
		return -EINVAL;
	}
	or->attributes_mode = OSD_CDB_GET_SET_ATTR_LISTS;

	/* first time calc data-in list header size */
	if (!or->get_attr.total_bytes)
		or->get_attr.total_bytes = _osd_req_sizeof_alist_header(or);

	/* calc data-out info */
	if (!total_bytes) { /* first-time: allocate and put list header */
		unsigned max_bytes;

		total_bytes = _osd_req_sizeof_alist_header(or);
		max_bytes = total_bytes +
			nelem * sizeof(struct osd_attributes_list_attrid);
		ret = _alloc_get_attr_desc(or, max_bytes);
		if (ret)
			return ret;

		_osd_req_set_alist_type(or, or->enc_get_attr.buff,
					OSD_ATTR_LIST_GET);
	}
	attr_last = or->enc_get_attr.buff + total_bytes;

	for (; nelem; --nelem) {
		struct osd_attributes_list_attrid *attrid;
		const unsigned cur_size = sizeof(*attrid);

		total_bytes += cur_size;
		if (unlikely(or->enc_get_attr.alloc_size < total_bytes)) {
			or->enc_get_attr.total_bytes = total_bytes - cur_size;
			ret = _alloc_get_attr_desc(or,
					total_bytes + nelem * sizeof(*attrid));
			if (ret)
				return ret;
			attr_last = or->enc_get_attr.buff +
				or->enc_get_attr.total_bytes;
		}

		attrid = attr_last;
		attrid->attr_page = cpu_to_be32(oa->attr_page);
		attrid->attr_id = cpu_to_be32(oa->attr_id);

		attr_last += cur_size;

		/* calc data-in size */
		or->get_attr.total_bytes +=
			_osd_req_alist_elem_size(or, oa->len);
		++oa;
	}

	or->enc_get_attr.total_bytes = total_bytes;

	OSD_DEBUG(
	       "get_attr.total_bytes=%u(%u) enc_get_attr.total_bytes=%u(%Zu)\n",
	       or->get_attr.total_bytes,
	       or->get_attr.total_bytes - _osd_req_sizeof_alist_header(or),
	       or->enc_get_attr.total_bytes,
	       (or->enc_get_attr.total_bytes - _osd_req_sizeof_alist_header(or))
			/ sizeof(struct osd_attributes_list_attrid));

	return 0;
}
EXPORT_SYMBOL(osd_req_add_get_attr_list);

static int _osd_req_finalize_get_attr_list(struct osd_request *or)
{
	struct osd_cdb_head *cdbh = osd_cdb_head(&or->cdb);
	unsigned out_padding;
	unsigned in_padding;
	int ret;

	if (!or->enc_get_attr.total_bytes) {
		cdbh->attrs_list.get_attr_desc_offset = OSD_OFFSET_UNUSED;
		cdbh->attrs_list.get_attr_offset = OSD_OFFSET_UNUSED;
		return 0;
	}

	ret = _alloc_get_attr_list(or);
	if (ret)
		return ret;

	/* The out-going buffer info update */
	OSD_DEBUG("out-going\n");
	cdbh->attrs_list.get_attr_desc_bytes =
		cpu_to_be32(or->enc_get_attr.total_bytes);

	cdbh->attrs_list.get_attr_desc_offset =
		osd_req_encode_offset(or, or->out.total_bytes, &out_padding);

	ret = _req_append_segment(or, out_padding, &or->enc_get_attr,
				  or->out.last_seg, &or->out);
	if (ret)
		return ret;
	or->out.last_seg = &or->enc_get_attr;

	/* The incoming buffer info update */
	OSD_DEBUG("in-coming\n");
	cdbh->attrs_list.get_attr_alloc_length =
		cpu_to_be32(or->get_attr.total_bytes);

	cdbh->attrs_list.get_attr_offset =
		osd_req_encode_offset(or, or->in.total_bytes, &in_padding);

	ret = _req_append_segment(or, in_padding, &or->get_attr, NULL,
				  &or->in);
	if (ret)
		return ret;
	or->in.last_seg = &or->get_attr;

	return 0;
}

int osd_req_decode_get_attr_list(struct osd_request *or,
	struct osd_attr *oa, int *nelem, void **iterator)
{
	unsigned cur_bytes, returned_bytes;
	int n;
	const unsigned sizeof_attr_list = _osd_req_sizeof_alist_header(or);
	void *cur_p;

	if (!_osd_req_is_alist_type(or, or->get_attr.buff,
				    OSD_ATTR_LIST_SET_RETRIEVE)) {
		oa->attr_page = 0;
		oa->attr_id = 0;
		oa->val_ptr = NULL;
		oa->len = 0;
		*iterator = NULL;
		return 0;
	}

	if (*iterator) {
		BUG_ON((*iterator < or->get_attr.buff) ||
		     (or->get_attr.buff + or->get_attr.alloc_size < *iterator));
		cur_p = *iterator;
		cur_bytes = (*iterator - or->get_attr.buff) - sizeof_attr_list;
		returned_bytes = or->get_attr.total_bytes;
	} else { /* first time decode the list header */
		cur_bytes = sizeof_attr_list;
		returned_bytes = _osd_req_alist_size(or, or->get_attr.buff) +
					sizeof_attr_list;

		cur_p = or->get_attr.buff + sizeof_attr_list;

		if (returned_bytes > or->get_attr.alloc_size) {
			OSD_DEBUG("target report: space was not big enough! "
				  "Allocate=%u Needed=%u\n",
				  or->get_attr.alloc_size,
				  returned_bytes + sizeof_attr_list);

			returned_bytes =
				or->get_attr.alloc_size - sizeof_attr_list;
		}
		or->get_attr.total_bytes = returned_bytes;
	}

	for (n = 0; (n < *nelem) && (cur_bytes < returned_bytes); ++n) {
		struct osd_attributes_list_element *attr = cur_p;
		unsigned inc;

		oa->len = be16_to_cpu(attr->attr_bytes);
		inc = _osd_req_alist_elem_size(or, oa->len);
		OSD_DEBUG("oa->len=%d inc=%d cur_bytes=%d\n",
			  oa->len, inc, cur_bytes);
		cur_bytes += inc;
		if (cur_bytes > returned_bytes) {
			OSD_ERR("BAD FOOD from target. list not valid!"
				"c=%d r=%d n=%d\n",
				cur_bytes, returned_bytes, n);
			oa->val_ptr = NULL;
			break;
		}

		oa->attr_page = be32_to_cpu(attr->attr_page);
		oa->attr_id = be32_to_cpu(attr->attr_id);
		oa->val_ptr = attr->attr_val;

		cur_p += inc;
		++oa;
	}

	*iterator = (returned_bytes - cur_bytes) ? cur_p : NULL;
	*nelem = n;
	return returned_bytes - cur_bytes;
}
EXPORT_SYMBOL(osd_req_decode_get_attr_list);

/*
 * Attributes Page-mode
 */

int osd_req_add_get_attr_page(struct osd_request *or,
	u32 page_id, void *attar_page, unsigned max_page_len,
	const struct osd_attr *set_one_attr)
{
	struct osd_cdb_head *cdbh = osd_cdb_head(&or->cdb);

	if (or->attributes_mode &&
	    or->attributes_mode != OSD_CDB_GET_ATTR_PAGE_SET_ONE) {
		WARN_ON(1);
		return -EINVAL;
	}
	or->attributes_mode = OSD_CDB_GET_ATTR_PAGE_SET_ONE;

	or->get_attr.buff = attar_page;
	or->get_attr.total_bytes = max_page_len;

	or->set_attr.buff = set_one_attr->val_ptr;
	or->set_attr.total_bytes = set_one_attr->len;

	cdbh->attrs_page.get_attr_page = cpu_to_be32(page_id);
	cdbh->attrs_page.get_attr_alloc_length = cpu_to_be32(max_page_len);
	/* ocdb->attrs_page.get_attr_offset; */

	cdbh->attrs_page.set_attr_page = cpu_to_be32(set_one_attr->attr_page);
	cdbh->attrs_page.set_attr_id = cpu_to_be32(set_one_attr->attr_id);
	cdbh->attrs_page.set_attr_length = cpu_to_be32(set_one_attr->len);
	/* ocdb->attrs_page.set_attr_offset; */
	return 0;
}
EXPORT_SYMBOL(osd_req_add_get_attr_page);

static int _osd_req_finalize_attr_page(struct osd_request *or)
{
	struct osd_cdb_head *cdbh = osd_cdb_head(&or->cdb);
	unsigned in_padding, out_padding;
	int ret;

	/* returned page */
	cdbh->attrs_page.get_attr_offset =
		osd_req_encode_offset(or, or->in.total_bytes, &in_padding);

	ret = _req_append_segment(or, in_padding, &or->get_attr, NULL,
				  &or->in);
	if (ret)
		return ret;

	/* set one value */
	cdbh->attrs_page.set_attr_offset =
		osd_req_encode_offset(or, or->out.total_bytes, &out_padding);

	ret = _req_append_segment(or, out_padding, &or->enc_get_attr, NULL,
				  &or->out);
	return ret;
}

static int _osd_req_finalize_data_integrity(struct osd_request *or,
	bool has_in, bool has_out, const u8 *cap_key)
{
	struct osd_security_parameters *sec_parms = _osd_req_sec_params(or);
	int ret;

	if (!osd_is_sec_alldata(sec_parms))
		return 0;

	if (has_out) {
		struct _osd_req_data_segment seg = {
			.buff = &or->out_data_integ,
			.total_bytes = sizeof(or->out_data_integ),
		};
		unsigned pad;

		or->out_data_integ.data_bytes = cpu_to_be64(
			or->out.bio ? or->out.bio->bi_size : 0);
		or->out_data_integ.set_attributes_bytes = cpu_to_be64(
			or->set_attr.total_bytes);
		or->out_data_integ.get_attributes_bytes = cpu_to_be64(
			or->enc_get_attr.total_bytes);

		sec_parms->data_out_integrity_check_offset =
			osd_req_encode_offset(or, or->out.total_bytes, &pad);

		ret = _req_append_segment(or, pad, &seg, or->out.last_seg,
					  &or->out);
		if (ret)
			return ret;
		or->out.last_seg = NULL;

		/* they are now all chained to request sign them all together */
		osd_sec_sign_data(&or->out_data_integ, or->out.req->bio,
				  cap_key);
	}

	if (has_in) {
		struct _osd_req_data_segment seg = {
			.buff = &or->in_data_integ,
			.total_bytes = sizeof(or->in_data_integ),
		};
		unsigned pad;

		sec_parms->data_in_integrity_check_offset =
			osd_req_encode_offset(or, or->in.total_bytes, &pad);

		ret = _req_append_segment(or, pad, &seg, or->in.last_seg,
					  &or->in);
		if (ret)
			return ret;

		or->in.last_seg = NULL;
	}

	return 0;
}

/*
 * osd_finalize_request and helpers
 */

static int _init_blk_request(struct osd_request *or,
	bool has_in, bool has_out)
{
	gfp_t flags = or->alloc_flags;
	struct scsi_device *scsi_device = or->osd_dev->scsi_device;
	struct request_queue *q = scsi_device->request_queue;
	struct request *req;
	int ret = -ENOMEM;

	req = blk_get_request(q, has_out, flags);
	if (!req)
		goto out;

	or->request = req;
	req->cmd_type = REQ_TYPE_BLOCK_PC;
	req->timeout = or->timeout;
	req->retries = or->retries;
	req->sense = or->sense;
	req->sense_len = 0;

	if (has_out) {
		or->out.req = req;
		if (has_in) {
			/* allocate bidi request */
			req = blk_get_request(q, READ, flags);
			if (!req) {
				OSD_DEBUG("blk_get_request for bidi failed\n");
				goto out;
			}
			req->cmd_type = REQ_TYPE_BLOCK_PC;
			or->in.req = or->request->next_rq = req;
		}
	} else if (has_in)
		or->in.req = req;

	ret = 0;
out:
	OSD_DEBUG("or=%p has_in=%d has_out=%d => %d, %p\n",
			or, has_in, has_out, ret, or->request);
	return ret;
}

int osd_finalize_request(struct osd_request *or,
	u8 options, const void *cap, const u8 *cap_key)
{
	struct osd_cdb_head *cdbh = osd_cdb_head(&or->cdb);
	bool has_in, has_out;
	int ret;

	if (options & OSD_REQ_FUA)
		cdbh->options |= OSD_CDB_FUA;

	if (options & OSD_REQ_DPO)
		cdbh->options |= OSD_CDB_DPO;

	if (options & OSD_REQ_BYPASS_TIMESTAMPS)
		cdbh->timestamp_control = OSD_CDB_BYPASS_TIMESTAMPS;

	osd_set_caps(&or->cdb, cap);

	has_in = or->in.bio || or->get_attr.total_bytes;
	has_out = or->out.bio || or->set_attr.total_bytes ||
		or->enc_get_attr.total_bytes;

	ret = _init_blk_request(or, has_in, has_out);
	if (ret) {
		OSD_DEBUG("_init_blk_request failed\n");
		return ret;
	}

	if (or->out.bio) {
		ret = blk_rq_append_bio(or->request->q, or->out.req,
					or->out.bio);
		if (ret) {
			OSD_DEBUG("blk_rq_append_bio out failed\n");
			return ret;
		}
		OSD_DEBUG("out bytes=%llu (bytes_req=%u)\n",
			_LLU(or->out.total_bytes), or->out.req->data_len);
	}
	if (or->in.bio) {
		ret = blk_rq_append_bio(or->request->q, or->in.req, or->in.bio);
		if (ret) {
			OSD_DEBUG("blk_rq_append_bio in failed\n");
			return ret;
		}
		OSD_DEBUG("in bytes=%llu (bytes_req=%u)\n",
			_LLU(or->in.total_bytes), or->in.req->data_len);
	}

	or->out.pad_buff = sg_out_pad_buffer;
	or->in.pad_buff = sg_in_pad_buffer;

	if (!or->attributes_mode)
		or->attributes_mode = OSD_CDB_GET_SET_ATTR_LISTS;
	cdbh->command_specific_options |= or->attributes_mode;
	if (or->attributes_mode == OSD_CDB_GET_ATTR_PAGE_SET_ONE) {
		ret = _osd_req_finalize_attr_page(or);
	} else {
		/* TODO: I think that for the GET_ATTR command these 2 should
		 * be reversed to keep them in execution order (for embeded
		 * targets with low memory footprint)
		 */
		ret = _osd_req_finalize_set_attr_list(or);
		if (ret) {
			OSD_DEBUG("_osd_req_finalize_set_attr_list failed\n");
			return ret;
		}

		ret = _osd_req_finalize_get_attr_list(or);
		if (ret) {
			OSD_DEBUG("_osd_req_finalize_get_attr_list failed\n");
			return ret;
		}
	}

	ret = _osd_req_finalize_data_integrity(or, has_in, has_out, cap_key);
	if (ret)
		return ret;

	osd_sec_sign_cdb(&or->cdb, cap_key);

	or->request->cmd = or->cdb.buff;
	or->request->cmd_len = _osd_req_cdb_len(or);

	return 0;
}
EXPORT_SYMBOL(osd_finalize_request);

/*
 * Implementation of osd_sec.h API
 * TODO: Move to a separate osd_sec.c file at a later stage.
 */

enum { OSD_SEC_CAP_V1_ALL_CAPS =
	OSD_SEC_CAP_APPEND | OSD_SEC_CAP_OBJ_MGMT | OSD_SEC_CAP_REMOVE   |
	OSD_SEC_CAP_CREATE | OSD_SEC_CAP_SET_ATTR | OSD_SEC_CAP_GET_ATTR |
	OSD_SEC_CAP_WRITE  | OSD_SEC_CAP_READ     | OSD_SEC_CAP_POL_SEC  |
	OSD_SEC_CAP_GLOBAL | OSD_SEC_CAP_DEV_MGMT
};

void osd_sec_init_nosec_doall_caps(void *caps,
	const struct osd_obj_id *obj, bool is_collection, const bool is_v1)
{
	struct osd_capability *cap = caps;
	u8 type;
	u8 descriptor_type;

	if (likely(obj->id)) {
		if (unlikely(is_collection)) {
			type = OSD_SEC_OBJ_COLLECTION;
			descriptor_type = is_v1 ? OSD_SEC_OBJ_DESC_OBJ :
						  OSD_SEC_OBJ_DESC_COL;
		} else {
			type = OSD_SEC_OBJ_USER;
			descriptor_type = OSD_SEC_OBJ_DESC_OBJ;
		}
		WARN_ON(!obj->partition);
	} else {
		type = obj->partition ? OSD_SEC_OBJ_PARTITION :
					OSD_SEC_OBJ_ROOT;
		descriptor_type = OSD_SEC_OBJ_DESC_PAR;
	}

	memset(cap, 0, sizeof(*cap));

	cap->h.format = OSD_SEC_CAP_FORMAT_VER1;
	cap->h.integrity_algorithm__key_version = 0; /* MAKE_BYTE(0, 0); */
	cap->h.security_method = OSD_SEC_NOSEC;
/*	cap->expiration_time;
	cap->AUDIT[30-10];
	cap->discriminator[42-30];
	cap->object_created_time; */
	cap->h.object_type = type;
	osd_sec_set_caps(&cap->h, OSD_SEC_CAP_V1_ALL_CAPS);
	cap->h.object_descriptor_type = descriptor_type;
	cap->od.obj_desc.policy_access_tag = 0;
	cap->od.obj_desc.allowed_partition_id = cpu_to_be64(obj->partition);
	cap->od.obj_desc.allowed_object_id = cpu_to_be64(obj->id);
}
EXPORT_SYMBOL(osd_sec_init_nosec_doall_caps);

void osd_set_caps(struct osd_cdb *cdb, const void *caps)
{
	memcpy(&cdb->v1.caps, caps, OSDv1_CAP_LEN);
}

bool osd_is_sec_alldata(struct osd_security_parameters *sec_parms __unused)
{
	return false;
}

void osd_sec_sign_cdb(struct osd_cdb *ocdb __unused, const u8 *cap_key __unused)
{
}

void osd_sec_sign_data(void *data_integ __unused,
		       struct bio *bio __unused, const u8 *cap_key __unused)
{
}

/*
 * Declared in osd_protocol.h
 * 4.12.5 Data-In and Data-Out buffer offsets
 * byte offset = mantissa * (2^(exponent+8))
 * Returns the smallest allowed encoded offset that contains given @offset
 * The actual encoded offset returned is @offset + *@padding.
 */
osd_cdb_offset __osd_encode_offset(
	u64 offset, unsigned *padding, int min_shift, int max_shift)
{
	u64 try_offset = -1, mod, align;
	osd_cdb_offset be32_offset;
	int shift;

	*padding = 0;
	if (!offset)
		return 0;

	for (shift = min_shift; shift < max_shift; ++shift) {
		try_offset = offset >> shift;
		if (try_offset < (1 << OSD_OFFSET_MAX_BITS))
			break;
	}

	BUG_ON(shift == max_shift);

	align = 1 << shift;
	mod = offset & (align - 1);
	if (mod) {
		*padding = align - mod;
		try_offset += 1;
	}

	try_offset |= ((shift - 8) & 0xf) << 28;
	be32_offset = cpu_to_be32((u32)try_offset);

	OSD_DEBUG("offset=%llu mantissa=%llu exp=%d encoded=%x pad=%d\n",
		 _LLU(offset), _LLU(try_offset & 0x0FFFFFFF), shift,
		 be32_offset, *padding);
	return be32_offset;
}
