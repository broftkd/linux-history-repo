#ifndef _LINUX_SCATTERLIST_H
#define _LINUX_SCATTERLIST_H

#include <asm/scatterlist.h>
#include <asm/io.h>
#include <linux/mm.h>
#include <linux/string.h>

/**
 * sg_set_page - Set sg entry to point at given page
 * @sg:		 SG entry
 * @page:	 The page
 *
 * Description:
 *   Use this function to set an sg entry pointing at a page, never assign
 *   the page directly. We encode sg table information in the lower bits
 *   of the page pointer. See sg_page() for looking up the page belonging
 *   to an sg entry.
 *
 **/
static inline void sg_set_page(struct scatterlist *sg, struct page *page)
{
	sg->page = page;
}

#define sg_page(sg)	((sg)->page)

static inline void sg_set_buf(struct scatterlist *sg, const void *buf,
			      unsigned int buflen)
{
	sg_set_page(sg, virt_to_page(buf));
	sg->offset = offset_in_page(buf);
	sg->length = buflen;
}

/*
 * We overload the LSB of the page pointer to indicate whether it's
 * a valid sg entry, or whether it points to the start of a new scatterlist.
 * Those low bits are there for everyone! (thanks mason :-)
 */
#define sg_is_chain(sg)		((unsigned long) (sg)->page & 0x01)
#define sg_chain_ptr(sg)	\
	((struct scatterlist *) ((unsigned long) (sg)->page & ~0x01))

/**
 * sg_next - return the next scatterlist entry in a list
 * @sg:		The current sg entry
 *
 * Usually the next entry will be @sg@ + 1, but if this sg element is part
 * of a chained scatterlist, it could jump to the start of a new
 * scatterlist array.
 *
 * Note that the caller must ensure that there are further entries after
 * the current entry, this function will NOT return NULL for an end-of-list.
 *
 */
static inline struct scatterlist *sg_next(struct scatterlist *sg)
{
	sg++;

	if (unlikely(sg_is_chain(sg)))
		sg = sg_chain_ptr(sg);

	return sg;
}

/*
 * Loop over each sg element, following the pointer to a new list if necessary
 */
#define for_each_sg(sglist, sg, nr, __i)	\
	for (__i = 0, sg = (sglist); __i < (nr); __i++, sg = sg_next(sg))

/**
 * sg_last - return the last scatterlist entry in a list
 * @sgl:	First entry in the scatterlist
 * @nents:	Number of entries in the scatterlist
 *
 * Should only be used casually, it (currently) scan the entire list
 * to get the last entry.
 *
 * Note that the @sgl@ pointer passed in need not be the first one,
 * the important bit is that @nents@ denotes the number of entries that
 * exist from @sgl@.
 *
 */
static inline struct scatterlist *sg_last(struct scatterlist *sgl,
					  unsigned int nents)
{
#ifndef ARCH_HAS_SG_CHAIN
	struct scatterlist *ret = &sgl[nents - 1];
#else
	struct scatterlist *sg, *ret = NULL;
	int i;

	for_each_sg(sgl, sg, nents, i)
		ret = sg;

#endif
	return ret;
}

/**
 * sg_chain - Chain two sglists together
 * @prv:	First scatterlist
 * @prv_nents:	Number of entries in prv
 * @sgl:	Second scatterlist
 *
 * Links @prv@ and @sgl@ together, to form a longer scatterlist.
 *
 */
static inline void sg_chain(struct scatterlist *prv, unsigned int prv_nents,
			    struct scatterlist *sgl)
{
#ifndef ARCH_HAS_SG_CHAIN
	BUG();
#endif
	prv[prv_nents - 1].page = (struct page *) ((unsigned long) sgl | 0x01);
}

/**
 * sg_mark_end - Mark the end of the scatterlist
 * @sgl:	Scatterlist
 * @nents:	Number of entries in sgl
 *
 * Description:
 *   Marks the last entry as the termination point for sg_next()
 *
 **/
static inline void sg_mark_end(struct scatterlist *sgl, unsigned int nents)
{
}

static inline void __sg_mark_end(struct scatterlist *sg)
{
}


/**
 * sg_init_one - Initialize a single entry sg list
 * @sg:		 SG entry
 * @buf:	 Virtual address for IO
 * @buflen:	 IO length
 *
 * Notes:
 *   This should not be used on a single entry that is part of a larger
 *   table. Use sg_init_table() for that.
 *
 **/
static inline void sg_init_one(struct scatterlist *sg, const void *buf,
			       unsigned int buflen)
{
	memset(sg, 0, sizeof(*sg));
	sg_mark_end(sg, 1);
	sg_set_buf(sg, buf, buflen);
}

/**
 * sg_init_table - Initialize SG table
 * @sgl:	   The SG table
 * @nents:	   Number of entries in table
 *
 * Notes:
 *   If this is part of a chained sg table, sg_mark_end() should be
 *   used only on the last table part.
 *
 **/
static inline void sg_init_table(struct scatterlist *sgl, unsigned int nents)
{
	memset(sgl, 0, sizeof(*sgl) * nents);
	sg_mark_end(sgl, nents);
}

/**
 * sg_phys - Return physical address of an sg entry
 * @sg:	     SG entry
 *
 * Description:
 *   This calls page_to_phys() on the page in this sg entry, and adds the
 *   sg offset. The caller must know that it is legal to call page_to_phys()
 *   on the sg page.
 *
 **/
static inline unsigned long sg_phys(struct scatterlist *sg)
{
	return page_to_phys(sg_page(sg)) + sg->offset;
}

/**
 * sg_virt - Return virtual address of an sg entry
 * @sg:	     SG entry
 *
 * Description:
 *   This calls page_address() on the page in this sg entry, and adds the
 *   sg offset. The caller must know that the sg page has a valid virtual
 *   mapping.
 *
 **/
static inline void *sg_virt(struct scatterlist *sg)
{
	return page_address(sg_page(sg)) + sg->offset;
}

#endif /* _LINUX_SCATTERLIST_H */
