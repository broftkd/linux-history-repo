/* arch/sparc64/kernel/sparc64_ksyms.c: Sparc64 specific ksyms support.
 *
 * Copyright (C) 1996, 2007 David S. Miller (davem@davemloft.net)
 * Copyright (C) 1996 Eddie C. Dost (ecd@skynet.be)
 * Copyright (C) 1999 Jakub Jelinek (jj@ultra.linux.cz)
 */

#define PROMLIB_INTERNAL

#include <linux/module.h>
#include <linux/types.h>
#include <linux/sched.h>
#include <linux/in6.h>
#include <linux/pci.h>
#include <linux/interrupt.h>
#include <linux/fs_struct.h>
#include <linux/fs.h>
#include <linux/mm.h>
#include <linux/socket.h>
#include <linux/syscalls.h>
#include <linux/percpu.h>
#include <linux/init.h>
#include <linux/rwsem.h>
#include <net/compat.h>

#include <asm/oplib.h>
#include <asm/system.h>
#include <asm/auxio.h>
#include <asm/pgtable.h>
#include <asm/io.h>
#include <asm/irq.h>
#include <asm/elf.h>
#include <asm/head.h>
#include <asm/smp.h>
#include <asm/ptrace.h>
#include <asm/uaccess.h>
#include <asm/checksum.h>
#include <asm/fpumacro.h>
#include <asm/pgalloc.h>
#include <asm/cacheflush.h>
#ifdef CONFIG_SBUS
#include <asm/dma.h>
#endif
#include <asm/ns87303.h>
#include <asm/timer.h>
#include <asm/cpudata.h>
#include <asm/ftrace.h>
#include <asm/hypervisor.h>

struct poll {
	int fd;
	short events;
	short revents;
};

extern void die_if_kernel(char *str, struct pt_regs *regs);
extern pid_t kernel_thread(int (*fn)(void *), void * arg, unsigned long flags);
extern void sys_sigsuspend(void);
extern int compat_sys_ioctl(unsigned int fd, unsigned int cmd, u32 arg);
extern int (*handle_mathemu)(struct pt_regs *, struct fpustate *);
extern long sparc32_open(const char __user * filename, int flags, int mode);
extern int io_remap_pfn_range(struct vm_area_struct *vma, unsigned long from,
	unsigned long pfn, unsigned long size, pgprot_t prot);

extern int __ashrdi3(int, int);

extern int dump_fpu (struct pt_regs * regs, elf_fpregset_t * fpregs);

/* used by various drivers */
#ifdef CONFIG_SMP
/* Out of line rw-locking implementation. */
EXPORT_SYMBOL(__read_lock);
EXPORT_SYMBOL(__read_unlock);
EXPORT_SYMBOL(__write_lock);
EXPORT_SYMBOL(__write_unlock);
EXPORT_SYMBOL(__write_trylock);
#endif /* CONFIG_SMP */

EXPORT_SYMBOL(sparc64_get_clock_tick);

EXPORT_SYMBOL(__flushw_user);

EXPORT_SYMBOL(tlb_type);
EXPORT_SYMBOL(sun4v_chip_type);
EXPORT_SYMBOL(get_fb_unmapped_area);
EXPORT_SYMBOL(flush_icache_range);

EXPORT_SYMBOL(flush_dcache_page);
#ifdef DCACHE_ALIASING_POSSIBLE
EXPORT_SYMBOL(__flush_dcache_range);
#endif

EXPORT_SYMBOL(sun4v_niagara_getperf);
EXPORT_SYMBOL(sun4v_niagara_setperf);
EXPORT_SYMBOL(sun4v_niagara2_getperf);
EXPORT_SYMBOL(sun4v_niagara2_setperf);

EXPORT_SYMBOL(auxio_set_led);
EXPORT_SYMBOL(auxio_set_lte);
#ifdef CONFIG_SBUS
EXPORT_SYMBOL(sbus_set_sbus64);
#endif
EXPORT_SYMBOL(outsb);
EXPORT_SYMBOL(outsw);
EXPORT_SYMBOL(outsl);
EXPORT_SYMBOL(insb);
EXPORT_SYMBOL(insw);
EXPORT_SYMBOL(insl);
#ifdef CONFIG_PCI
EXPORT_SYMBOL(pci_alloc_consistent);
EXPORT_SYMBOL(pci_free_consistent);
EXPORT_SYMBOL(pci_map_single);
EXPORT_SYMBOL(pci_unmap_single);
EXPORT_SYMBOL(pci_map_sg);
EXPORT_SYMBOL(pci_unmap_sg);
EXPORT_SYMBOL(pci_dma_sync_single_for_cpu);
EXPORT_SYMBOL(pci_dma_sync_sg_for_cpu);
EXPORT_SYMBOL(pci_dma_supported);
#endif

/* I/O device mmaping on Sparc64. */
EXPORT_SYMBOL(io_remap_pfn_range);

EXPORT_SYMBOL(dump_fpu);

/* math-emu wants this */
EXPORT_SYMBOL(die_if_kernel);

/* Kernel thread creation. */
EXPORT_SYMBOL(kernel_thread);

/* prom symbols */
EXPORT_SYMBOL(prom_root_node);
EXPORT_SYMBOL(prom_getchild);
EXPORT_SYMBOL(prom_getsibling);
EXPORT_SYMBOL(prom_searchsiblings);
EXPORT_SYMBOL(prom_firstprop);
EXPORT_SYMBOL(prom_nextprop);
EXPORT_SYMBOL(prom_getproplen);
EXPORT_SYMBOL(prom_getproperty);
EXPORT_SYMBOL(prom_node_has_property);
EXPORT_SYMBOL(prom_setprop);
EXPORT_SYMBOL(saved_command_line);
EXPORT_SYMBOL(prom_finddevice);
EXPORT_SYMBOL(prom_feval);
EXPORT_SYMBOL(prom_getbool);
EXPORT_SYMBOL(prom_getstring);
EXPORT_SYMBOL(prom_getint);
EXPORT_SYMBOL(prom_getintdefault);
EXPORT_SYMBOL(__prom_getchild);
EXPORT_SYMBOL(__prom_getsibling);

/* Moving data to/from/in userspace. */
EXPORT_SYMBOL(copy_to_user_fixup);
EXPORT_SYMBOL(copy_from_user_fixup);
EXPORT_SYMBOL(copy_in_user_fixup);

/* Various address conversion macros use this. */
EXPORT_SYMBOL(sparc64_valid_addr_bitmap);

/* No version information on this, heavily used in inline asm,
 * and will always be 'void __ret_efault(void)'.
 */
EXPORT_SYMBOL(__ret_efault);

/* for input/keybdev */
EXPORT_SYMBOL(sun_do_break);
EXPORT_SYMBOL(stop_a_enabled);

#ifdef CONFIG_DEBUG_BUGVERBOSE
EXPORT_SYMBOL(do_BUG);
#endif

/* for ns8703 */
EXPORT_SYMBOL(ns87303_lock);

EXPORT_SYMBOL(tick_ops);

EXPORT_SYMBOL_GPL(real_hard_smp_processor_id);
