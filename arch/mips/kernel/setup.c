/*
 * This file is subject to the terms and conditions of the GNU General Public
 * License.  See the file "COPYING" in the main directory of this archive
 * for more details.
 *
 * Copyright (C) 1995 Linus Torvalds
 * Copyright (C) 1995 Waldorf Electronics
 * Copyright (C) 1994, 95, 96, 97, 98, 99, 2000, 01, 02, 03  Ralf Baechle
 * Copyright (C) 1996 Stoned Elipot
 * Copyright (C) 1999 Silicon Graphics, Inc.
 * Copyright (C) 2000 2001, 2002  Maciej W. Rozycki
 */
#include <linux/errno.h>
#include <linux/init.h>
#include <linux/ioport.h>
#include <linux/sched.h>
#include <linux/kernel.h>
#include <linux/mm.h>
#include <linux/module.h>
#include <linux/stddef.h>
#include <linux/string.h>
#include <linux/unistd.h>
#include <linux/slab.h>
#include <linux/user.h>
#include <linux/utsname.h>
#include <linux/a.out.h>
#include <linux/screen_info.h>
#include <linux/bootmem.h>
#include <linux/initrd.h>
#include <linux/major.h>
#include <linux/kdev_t.h>
#include <linux/root_dev.h>
#include <linux/highmem.h>
#include <linux/console.h>
#include <linux/mmzone.h>
#include <linux/pfn.h>

#include <asm/addrspace.h>
#include <asm/bootinfo.h>
#include <asm/cache.h>
#include <asm/cpu.h>
#include <asm/sections.h>
#include <asm/setup.h>
#include <asm/system.h>

struct cpuinfo_mips cpu_data[NR_CPUS] __read_mostly;

EXPORT_SYMBOL(cpu_data);

#ifdef CONFIG_VT
struct screen_info screen_info;
#endif

/*
 * Despite it's name this variable is even if we don't have PCI
 */
unsigned int PCI_DMA_BUS_IS_PHYS;

EXPORT_SYMBOL(PCI_DMA_BUS_IS_PHYS);

/*
 * Setup information
 *
 * These are initialized so they are in the .data section
 */
unsigned long mips_machtype __read_mostly = MACH_UNKNOWN;
unsigned long mips_machgroup __read_mostly = MACH_GROUP_UNKNOWN;

EXPORT_SYMBOL(mips_machtype);
EXPORT_SYMBOL(mips_machgroup);

struct boot_mem_map boot_mem_map;

static char command_line[CL_SIZE];
       char arcs_cmdline[CL_SIZE]=CONFIG_CMDLINE;

/*
 * mips_io_port_base is the begin of the address space to which x86 style
 * I/O ports are mapped.
 */
const unsigned long mips_io_port_base __read_mostly = -1;
EXPORT_SYMBOL(mips_io_port_base);

/*
 * isa_slot_offset is the address where E(ISA) busaddress 0 is mapped
 * for the processor.
 */
unsigned long isa_slot_offset;
EXPORT_SYMBOL(isa_slot_offset);

static struct resource code_resource = { .name = "Kernel code", };
static struct resource data_resource = { .name = "Kernel data", };

void __init add_memory_region(phys_t start, phys_t size, long type)
{
	int x = boot_mem_map.nr_map;
	struct boot_mem_map_entry *prev = boot_mem_map.map + x - 1;

	/* Sanity check */
	if (start + size < start) {
		printk("Trying to add an invalid memory region, skipped\n");
		return;
	}

	/*
	 * Try to merge with previous entry if any.  This is far less than
	 * perfect but is sufficient for most real world cases.
	 */
	if (x && prev->addr + prev->size == start && prev->type == type) {
		prev->size += size;
		return;
	}

	if (x == BOOT_MEM_MAP_MAX) {
		printk("Ooops! Too many entries in the memory map!\n");
		return;
	}

	boot_mem_map.map[x].addr = start;
	boot_mem_map.map[x].size = size;
	boot_mem_map.map[x].type = type;
	boot_mem_map.nr_map++;
}

static void __init print_memory_map(void)
{
	int i;
	const int field = 2 * sizeof(unsigned long);

	for (i = 0; i < boot_mem_map.nr_map; i++) {
		printk(" memory: %0*Lx @ %0*Lx ",
		       field, (unsigned long long) boot_mem_map.map[i].size,
		       field, (unsigned long long) boot_mem_map.map[i].addr);

		switch (boot_mem_map.map[i].type) {
		case BOOT_MEM_RAM:
			printk("(usable)\n");
			break;
		case BOOT_MEM_ROM_DATA:
			printk("(ROM data)\n");
			break;
		case BOOT_MEM_RESERVED:
			printk("(reserved)\n");
			break;
		default:
			printk("type %lu\n", boot_mem_map.map[i].type);
			break;
		}
	}
}

static inline void parse_cmdline_early(void)
{
	char c = ' ', *to = command_line, *from = saved_command_line;
	unsigned long start_at, mem_size;
	int len = 0;
	int usermem = 0;

	printk("Determined physical RAM map:\n");
	print_memory_map();

	for (;;) {
		/*
		 * "mem=XXX[kKmM]" defines a memory region from
		 * 0 to <XXX>, overriding the determined size.
		 * "mem=XXX[KkmM]@YYY[KkmM]" defines a memory region from
		 * <YYY> to <YYY>+<XXX>, overriding the determined size.
		 */
		if (c == ' ' && !memcmp(from, "mem=", 4)) {
			if (to != command_line)
				to--;
			/*
			 * If a user specifies memory size, we
			 * blow away any automatically generated
			 * size.
			 */
			if (usermem == 0) {
				boot_mem_map.nr_map = 0;
				usermem = 1;
			}
			mem_size = memparse(from + 4, &from);
			if (*from == '@')
				start_at = memparse(from + 1, &from);
			else
				start_at = 0;
			add_memory_region(start_at, mem_size, BOOT_MEM_RAM);
		}
		c = *(from++);
		if (!c)
			break;
		if (CL_SIZE <= ++len)
			break;
		*(to++) = c;
	}
	*to = '\0';

	if (usermem) {
		printk("User-defined physical RAM map:\n");
		print_memory_map();
	}
}

static inline int parse_rd_cmdline(unsigned long* rd_start, unsigned long* rd_end)
{
	/*
	 * "rd_start=0xNNNNNNNN" defines the memory address of an initrd
	 * "rd_size=0xNN" it's size
	 */
	unsigned long start = 0;
	unsigned long size = 0;
	unsigned long end;
	char cmd_line[CL_SIZE];
	char *start_str;
	char *size_str;
	char *tmp;

	strcpy(cmd_line, command_line);
	*command_line = 0;
	tmp = cmd_line;
	/* Ignore "rd_start=" strings in other parameters. */
	start_str = strstr(cmd_line, "rd_start=");
	if (start_str && start_str != cmd_line && *(start_str - 1) != ' ')
		start_str = strstr(start_str, " rd_start=");
	while (start_str) {
		if (start_str != cmd_line)
			strncat(command_line, tmp, start_str - tmp);
		start = memparse(start_str + 9, &start_str);
		tmp = start_str + 1;
		start_str = strstr(start_str, " rd_start=");
	}
	if (*tmp)
		strcat(command_line, tmp);

	strcpy(cmd_line, command_line);
	*command_line = 0;
	tmp = cmd_line;
	/* Ignore "rd_size" strings in other parameters. */
	size_str = strstr(cmd_line, "rd_size=");
	if (size_str && size_str != cmd_line && *(size_str - 1) != ' ')
		size_str = strstr(size_str, " rd_size=");
	while (size_str) {
		if (size_str != cmd_line)
			strncat(command_line, tmp, size_str - tmp);
		size = memparse(size_str + 8, &size_str);
		tmp = size_str + 1;
		size_str = strstr(size_str, " rd_size=");
	}
	if (*tmp)
		strcat(command_line, tmp);

#ifdef CONFIG_64BIT
	/* HACK: Guess if the sign extension was forgotten */
	if (start > 0x0000000080000000 && start < 0x00000000ffffffff)
		start |= 0xffffffff00000000UL;
#endif

	end = start + size;
	if (start && end) {
		*rd_start = start;
		*rd_end = end;
		return 1;
	}
	return 0;
}

/*
 * Initialize the bootmem allocator. It also setup initrd related data
 * if needed.
 */
static void __init bootmem_init(void)
{
	unsigned long reserved_end = (unsigned long)&_end;
#ifndef CONFIG_SGI_IP27
	unsigned long highest = 0;
	unsigned long mapstart = -1UL;
	unsigned long bootmap_size;
	int i;
#endif
#ifdef CONFIG_BLK_DEV_INITRD
	int initrd_reserve_bootmem = 0;

	/* Board specific code should have set up initrd_start and initrd_end */
 	ROOT_DEV = Root_RAM0;
	if (parse_rd_cmdline(&initrd_start, &initrd_end)) {
		reserved_end = max(reserved_end, initrd_end);
		initrd_reserve_bootmem = 1;
	} else {
		unsigned long tmp;
		u32 *initrd_header;

		tmp = PAGE_ALIGN(reserved_end) - sizeof(u32) * 2;
		if (tmp < reserved_end)
			tmp += PAGE_SIZE;
		initrd_header = (u32 *)tmp;
		if (initrd_header[0] == 0x494E5244) {
			initrd_start = (unsigned long)&initrd_header[2];
			initrd_end = initrd_start + initrd_header[1];
			reserved_end = max(reserved_end, initrd_end);
			initrd_reserve_bootmem = 1;
		}
	}
#endif	/* CONFIG_BLK_DEV_INITRD */

#ifndef CONFIG_SGI_IP27
	/*
	 * reserved_end is now a pfn
	 */
	reserved_end = PFN_UP(CPHYSADDR(reserved_end));

	/*
	 * Find the highest page frame number we have available.
	 */
	for (i = 0; i < boot_mem_map.nr_map; i++) {
		unsigned long start, end;

		if (boot_mem_map.map[i].type != BOOT_MEM_RAM)
			continue;

		start = PFN_UP(boot_mem_map.map[i].addr);
		end = PFN_DOWN(boot_mem_map.map[i].addr
				+ boot_mem_map.map[i].size);

		if (end > highest)
			highest = end;
		if (end <= reserved_end)
			continue;
		if (start >= mapstart)
			continue;
		mapstart = max(reserved_end, start);
	}

	/*
	 * Determine low and high memory ranges
	 */
	if (highest > PFN_DOWN(HIGHMEM_START)) {
#ifdef CONFIG_HIGHMEM
		highstart_pfn = PFN_DOWN(HIGHMEM_START);
		highend_pfn = highest;
#endif
		highest = PFN_DOWN(HIGHMEM_START);
	}

	/*
	 * Initialize the boot-time allocator with low memory only.
	 */
	bootmap_size = init_bootmem(mapstart, highest);

	/*
	 * Register fully available low RAM pages with the bootmem allocator.
	 */
	for (i = 0; i < boot_mem_map.nr_map; i++) {
		unsigned long start, end, size;

		/*
		 * Reserve usable memory.
		 */
		if (boot_mem_map.map[i].type != BOOT_MEM_RAM)
			continue;

		start = PFN_UP(boot_mem_map.map[i].addr);
		end   = PFN_DOWN(boot_mem_map.map[i].addr
				    + boot_mem_map.map[i].size);
		/*
		 * We are rounding up the start address of usable memory
		 * and at the end of the usable range downwards.
		 */
		if (start >= max_low_pfn)
			continue;
		if (start < reserved_end)
			start = reserved_end;
		if (end > max_low_pfn)
			end = max_low_pfn;

		/*
		 * ... finally, is the area going away?
		 */
		if (end <= start)
			continue;
		size = end - start;

		/* Register lowmem ranges */
		free_bootmem(PFN_PHYS(start), size << PAGE_SHIFT);
		memory_present(0, start, end);
	}

	/*
	 * Reserve the bootmap memory.
	 */
	reserve_bootmem(PFN_PHYS(mapstart), bootmap_size);

#endif /* CONFIG_SGI_IP27 */

#ifdef CONFIG_BLK_DEV_INITRD
	initrd_below_start_ok = 1;
	if (initrd_start) {
		unsigned long initrd_size = ((unsigned char *)initrd_end) -
			((unsigned char *)initrd_start);
		const int width = sizeof(long) * 2;

		printk("Initial ramdisk at: 0x%p (%lu bytes)\n",
		       (void *)initrd_start, initrd_size);

		if (CPHYSADDR(initrd_end) > PFN_PHYS(max_low_pfn)) {
			printk("initrd extends beyond end of memory "
			       "(0x%0*Lx > 0x%0*Lx)\ndisabling initrd\n",
			       width,
			       (unsigned long long) CPHYSADDR(initrd_end),
			       width,
			       (unsigned long long) PFN_PHYS(max_low_pfn));
			initrd_start = initrd_end = 0;
			initrd_reserve_bootmem = 0;
		}

		if (initrd_reserve_bootmem)
			reserve_bootmem(CPHYSADDR(initrd_start), initrd_size);
	}
#endif /* CONFIG_BLK_DEV_INITRD  */
}

/*
 * arch_mem_init - initialize memory managment subsystem
 *
 *  o plat_mem_setup() detects the memory configuration and will record detected
 *    memory areas using add_memory_region.
 *  o parse_cmdline_early() parses the command line for mem= options which,
 *    iff detected, will override the results of the automatic detection.
 *
 * At this stage the memory configuration of the system is known to the
 * kernel but generic memory managment system is still entirely uninitialized.
 *
 *  o bootmem_init()
 *  o sparse_init()
 *  o paging_init()
 *
 * At this stage the bootmem allocator is ready to use.
 *
 * NOTE: historically plat_mem_setup did the entire platform initialization.
 *       This was rather impractical because it meant plat_mem_setup had to
 * get away without any kind of memory allocator.  To keep old code from
 * breaking plat_setup was just renamed to plat_setup and a second platform
 * initialization hook for anything else was introduced.
 */

extern void plat_mem_setup(void);

static void __init arch_mem_init(char **cmdline_p)
{
	/* call board setup routine */
	plat_mem_setup();

	strlcpy(command_line, arcs_cmdline, sizeof(command_line));
	strlcpy(saved_command_line, command_line, COMMAND_LINE_SIZE);

	*cmdline_p = command_line;

	parse_cmdline_early();
	bootmem_init();
	sparse_init();
	paging_init();
}

#define MAXMEM		HIGHMEM_START
#define MAXMEM_PFN	PFN_DOWN(MAXMEM)

static inline void resource_init(void)
{
	int i;

	if (UNCAC_BASE != IO_BASE)
		return;

	code_resource.start = virt_to_phys(&_text);
	code_resource.end = virt_to_phys(&_etext) - 1;
	data_resource.start = virt_to_phys(&_etext);
	data_resource.end = virt_to_phys(&_edata) - 1;

	/*
	 * Request address space for all standard RAM.
	 */
	for (i = 0; i < boot_mem_map.nr_map; i++) {
		struct resource *res;
		unsigned long start, end;

		start = boot_mem_map.map[i].addr;
		end = boot_mem_map.map[i].addr + boot_mem_map.map[i].size - 1;
		if (start >= MAXMEM)
			continue;
		if (end >= MAXMEM)
			end = MAXMEM - 1;

		res = alloc_bootmem(sizeof(struct resource));
		switch (boot_mem_map.map[i].type) {
		case BOOT_MEM_RAM:
		case BOOT_MEM_ROM_DATA:
			res->name = "System RAM";
			break;
		case BOOT_MEM_RESERVED:
		default:
			res->name = "reserved";
		}

		res->start = start;
		res->end = end;

		res->flags = IORESOURCE_MEM | IORESOURCE_BUSY;
		request_resource(&iomem_resource, res);

		/*
		 *  We don't know which RAM region contains kernel data,
		 *  so we try it repeatedly and let the resource manager
		 *  test it.
		 */
		request_resource(res, &code_resource);
		request_resource(res, &data_resource);
	}
}

#undef MAXMEM
#undef MAXMEM_PFN

void __init setup_arch(char **cmdline_p)
{
	cpu_probe();
	prom_init();
	cpu_report();

#if defined(CONFIG_VT)
#if defined(CONFIG_VGA_CONSOLE)
        conswitchp = &vga_con;
#elif defined(CONFIG_DUMMY_CONSOLE)
        conswitchp = &dummy_con;
#endif
#endif

	arch_mem_init(cmdline_p);

	resource_init();
#ifdef CONFIG_SMP
	plat_smp_setup();
#endif
}

int __init fpu_disable(char *s)
{
	int i;

	for (i = 0; i < NR_CPUS; i++)
		cpu_data[i].options &= ~MIPS_CPU_FPU;

	return 1;
}

__setup("nofpu", fpu_disable);

int __init dsp_disable(char *s)
{
	cpu_data[0].ases &= ~MIPS_ASE_DSP;

	return 1;
}

__setup("nodsp", dsp_disable);
