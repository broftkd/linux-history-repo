/*
 * File:         arch/blackfin/mm/init.c
 * Based on:
 * Author:
 *
 * Created:
 * Description:
 *
 * Modified:
 *               Copyright 2004-2007 Analog Devices Inc.
 *
 * Bugs:         Enter bugs at http://blackfin.uclinux.org/
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, see the file COPYING, or write
 * to the Free Software Foundation, Inc.,
 * 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 */

#include <linux/swap.h>
#include <linux/bootmem.h>
#include <linux/uaccess.h>
#include <asm/bfin-global.h>
#include <asm/pda.h>
#include <asm/cplbinit.h>
#include <asm/early_printk.h>
#include "blackfin_sram.h"

/*
 * BAD_PAGE is the page that is used for page faults when linux
 * is out-of-memory. Older versions of linux just did a
 * do_exit(), but using this instead means there is less risk
 * for a process dying in kernel mode, possibly leaving a inode
 * unused etc..
 *
 * BAD_PAGETABLE is the accompanying page-table: it is initialized
 * to point to BAD_PAGE entries.
 *
 * ZERO_PAGE is a special page that is used for zero-initialized
 * data and COW.
 */
static unsigned long empty_bad_page_table;

static unsigned long empty_bad_page;

static unsigned long empty_zero_page;

#ifndef CONFIG_EXCEPTION_L1_SCRATCH
#if defined CONFIG_SYSCALL_TAB_L1
__attribute__((l1_data))
#endif
static unsigned long exception_stack[NR_CPUS][1024];
#endif

struct blackfin_pda cpu_pda[NR_CPUS];
EXPORT_SYMBOL(cpu_pda);

/*
 * paging_init() continues the virtual memory environment setup which
 * was begun by the code in arch/head.S.
 * The parameters are pointers to where to stick the starting and ending
 * addresses  of available kernel virtual memory.
 */
void __init paging_init(void)
{
	/*
	 * make sure start_mem is page aligned,  otherwise bootmem and
	 * page_alloc get different views og the world
	 */
	unsigned long end_mem = memory_end & PAGE_MASK;

	pr_debug("start_mem is %#lx   virtual_end is %#lx\n", PAGE_ALIGN(memory_start), end_mem);

	/*
	 * initialize the bad page table and bad page to point
	 * to a couple of allocated pages
	 */
	empty_bad_page_table = (unsigned long)alloc_bootmem_pages(PAGE_SIZE);
	empty_bad_page = (unsigned long)alloc_bootmem_pages(PAGE_SIZE);
	empty_zero_page = (unsigned long)alloc_bootmem_pages(PAGE_SIZE);
	memset((void *)empty_zero_page, 0, PAGE_SIZE);

	/*
	 * Set up SFC/DFC registers (user data space)
	 */
	set_fs(KERNEL_DS);

	pr_debug("free_area_init -> start_mem is %#lx   virtual_end is %#lx\n",
	        PAGE_ALIGN(memory_start), end_mem);

	{
		unsigned long zones_size[MAX_NR_ZONES] = { 0, };

		zones_size[ZONE_DMA] = (end_mem - PAGE_OFFSET) >> PAGE_SHIFT;
		zones_size[ZONE_NORMAL] = 0;
#ifdef CONFIG_HIGHMEM
		zones_size[ZONE_HIGHMEM] = 0;
#endif
		free_area_init(zones_size);
	}
}

asmlinkage void __init init_pda(void)
{
	unsigned int cpu = raw_smp_processor_id();

	early_shadow_stamp();

	/* Initialize the PDA fields holding references to other parts
	   of the memory. The content of such memory is still
	   undefined at the time of the call, we are only setting up
	   valid pointers to it. */
	memset(&cpu_pda[cpu], 0, sizeof(cpu_pda[cpu]));

	cpu_pda[0].next = &cpu_pda[1];
	cpu_pda[1].next = &cpu_pda[0];

#ifdef CONFIG_EXCEPTION_L1_SCRATCH
	cpu_pda[cpu].ex_stack = (unsigned long *)(L1_SCRATCH_START + \
					L1_SCRATCH_LENGTH);
#else
	cpu_pda[cpu].ex_stack = exception_stack[cpu + 1];
#endif

#ifdef CONFIG_SMP
	cpu_pda[cpu].imask = 0x1f;
#endif
}

void __init mem_init(void)
{
	unsigned int codek = 0, datak = 0, initk = 0;
	unsigned int reservedpages = 0, freepages = 0;
	unsigned long tmp;
	unsigned long start_mem = memory_start;
	unsigned long end_mem = memory_end;

	end_mem &= PAGE_MASK;
	high_memory = (void *)end_mem;

	start_mem = PAGE_ALIGN(start_mem);
	max_mapnr = num_physpages = MAP_NR(high_memory);
	printk(KERN_DEBUG "Kernel managed physical pages: %lu\n", num_physpages);

	/* This will put all memory onto the freelists. */
	totalram_pages = free_all_bootmem();

	reservedpages = 0;
	for (tmp = 0; tmp < max_mapnr; tmp++)
		if (PageReserved(pfn_to_page(tmp)))
			reservedpages++;
	freepages =  max_mapnr - reservedpages;

	/* do not count in kernel image between _rambase and _ramstart */
	reservedpages -= (_ramstart - _rambase) >> PAGE_SHIFT;
#if (defined(CONFIG_BFIN_EXTMEM_ICACHEABLE) && ANOMALY_05000263)
	reservedpages += (_ramend - memory_end - DMA_UNCACHED_REGION) >> PAGE_SHIFT;
#endif

	codek = (_etext - _stext) >> 10;
	initk = (__init_end - __init_begin) >> 10;
	datak = ((_ramstart - _rambase) >> 10) - codek - initk;

	printk(KERN_INFO
	     "Memory available: %luk/%luk RAM, "
		"(%uk init code, %uk kernel code, %uk data, %uk dma, %uk reserved)\n",
		(unsigned long) freepages << (PAGE_SHIFT-10), _ramend >> 10,
		initk, codek, datak, DMA_UNCACHED_REGION >> 10, (reservedpages << (PAGE_SHIFT-10)));
}

static void __init free_init_pages(const char *what, unsigned long begin, unsigned long end)
{
	unsigned long addr;
	/* next to check that the page we free is not a partial page */
	for (addr = begin; addr + PAGE_SIZE <= end; addr += PAGE_SIZE) {
		ClearPageReserved(virt_to_page(addr));
		init_page_count(virt_to_page(addr));
		free_page(addr);
		totalram_pages++;
	}
	printk(KERN_INFO "Freeing %s: %ldk freed\n", what, (end - begin) >> 10);
}

#ifdef CONFIG_BLK_DEV_INITRD
void __init free_initrd_mem(unsigned long start, unsigned long end)
{
#ifndef CONFIG_MPU
	free_init_pages("initrd memory", start, end);
#endif
}
#endif

void __init_refok free_initmem(void)
{
#if defined CONFIG_RAMKERNEL && !defined CONFIG_MPU
	free_init_pages("unused kernel memory",
			(unsigned long)(&__init_begin),
			(unsigned long)(&__init_end));
#endif
}
