/*P:400 This contains run_guest() which actually calls into the Host<->Guest
 * Switcher and analyzes the return, such as determining if the Guest wants the
 * Host to do something.  This file also contains useful helper routines, and a
 * couple of non-obvious setup and teardown pieces which were implemented after
 * days of debugging pain. :*/
#include <linux/module.h>
#include <linux/stringify.h>
#include <linux/stddef.h>
#include <linux/io.h>
#include <linux/mm.h>
#include <linux/vmalloc.h>
#include <linux/cpu.h>
#include <linux/freezer.h>
#include <linux/highmem.h>
#include <asm/paravirt.h>
#include <asm/pgtable.h>
#include <asm/uaccess.h>
#include <asm/poll.h>
#include <asm/asm-offsets.h>
#include "lg.h"


static struct vm_struct *switcher_vma;
static struct page **switcher_page;

/* This One Big lock protects all inter-guest data structures. */
DEFINE_MUTEX(lguest_lock);

/*H:010 We need to set up the Switcher at a high virtual address.  Remember the
 * Switcher is a few hundred bytes of assembler code which actually changes the
 * CPU to run the Guest, and then changes back to the Host when a trap or
 * interrupt happens.
 *
 * The Switcher code must be at the same virtual address in the Guest as the
 * Host since it will be running as the switchover occurs.
 *
 * Trying to map memory at a particular address is an unusual thing to do, so
 * it's not a simple one-liner. */
static __init int map_switcher(void)
{
	int i, err;
	struct page **pagep;

	/*
	 * Map the Switcher in to high memory.
	 *
	 * It turns out that if we choose the address 0xFFC00000 (4MB under the
	 * top virtual address), it makes setting up the page tables really
	 * easy.
	 */

	/* We allocate an array of "struct page"s.  map_vm_area() wants the
	 * pages in this form, rather than just an array of pointers. */
	switcher_page = kmalloc(sizeof(switcher_page[0])*TOTAL_SWITCHER_PAGES,
				GFP_KERNEL);
	if (!switcher_page) {
		err = -ENOMEM;
		goto out;
	}

	/* Now we actually allocate the pages.  The Guest will see these pages,
	 * so we make sure they're zeroed. */
	for (i = 0; i < TOTAL_SWITCHER_PAGES; i++) {
		unsigned long addr = get_zeroed_page(GFP_KERNEL);
		if (!addr) {
			err = -ENOMEM;
			goto free_some_pages;
		}
		switcher_page[i] = virt_to_page(addr);
	}

	/* Now we reserve the "virtual memory area" we want: 0xFFC00000
	 * (SWITCHER_ADDR).  We might not get it in theory, but in practice
	 * it's worked so far. */
	switcher_vma = __get_vm_area(TOTAL_SWITCHER_PAGES * PAGE_SIZE,
				       VM_ALLOC, SWITCHER_ADDR, VMALLOC_END);
	if (!switcher_vma) {
		err = -ENOMEM;
		printk("lguest: could not map switcher pages high\n");
		goto free_pages;
	}

	/* This code actually sets up the pages we've allocated to appear at
	 * SWITCHER_ADDR.  map_vm_area() takes the vma we allocated above, the
	 * kind of pages we're mapping (kernel pages), and a pointer to our
	 * array of struct pages.  It increments that pointer, but we don't
	 * care. */
	pagep = switcher_page;
	err = map_vm_area(switcher_vma, PAGE_KERNEL, &pagep);
	if (err) {
		printk("lguest: map_vm_area failed: %i\n", err);
		goto free_vma;
	}

	/* Now the Switcher is mapped at the right address, we can't fail!
	 * Copy in the compiled-in Switcher code (from <arch>_switcher.S). */
	memcpy(switcher_vma->addr, start_switcher_text,
	       end_switcher_text - start_switcher_text);

	printk(KERN_INFO "lguest: mapped switcher at %p\n",
	       switcher_vma->addr);
	/* And we succeeded... */
	return 0;

free_vma:
	vunmap(switcher_vma->addr);
free_pages:
	i = TOTAL_SWITCHER_PAGES;
free_some_pages:
	for (--i; i >= 0; i--)
		__free_pages(switcher_page[i], 0);
	kfree(switcher_page);
out:
	return err;
}
/*:*/

/* Cleaning up the mapping when the module is unloaded is almost...
 * too easy. */
static void unmap_switcher(void)
{
	unsigned int i;

	/* vunmap() undoes *both* map_vm_area() and __get_vm_area(). */
	vunmap(switcher_vma->addr);
	/* Now we just need to free the pages we copied the switcher into */
	for (i = 0; i < TOTAL_SWITCHER_PAGES; i++)
		__free_pages(switcher_page[i], 0);
}

/*L:305
 * Dealing With Guest Memory.
 *
 * When the Guest gives us (what it thinks is) a physical address, we can use
 * the normal copy_from_user() & copy_to_user() on the corresponding place in
 * the memory region allocated by the Launcher.
 *
 * But we can't trust the Guest: it might be trying to access the Launcher
 * code.  We have to check that the range is below the pfn_limit the Launcher
 * gave us.  We have to make sure that addr + len doesn't give us a false
 * positive by overflowing, too. */
int lguest_address_ok(const struct lguest *lg,
		      unsigned long addr, unsigned long len)
{
	return (addr+len) / PAGE_SIZE < lg->pfn_limit && (addr+len >= addr);
}

/* This is a convenient routine to get a 32-bit value from the Guest (a very
 * common operation).  Here we can see how useful the kill_lguest() routine we
 * met in the Launcher can be: we return a random value (0) instead of needing
 * to return an error. */
u32 lgread_u32(struct lguest *lg, unsigned long addr)
{
	u32 val = 0;

	/* Don't let them access lguest binary. */
	if (!lguest_address_ok(lg, addr, sizeof(val))
	    || get_user(val, (u32 *)(lg->mem_base + addr)) != 0)
		kill_guest(lg, "bad read address %#lx: pfn_limit=%u membase=%p", addr, lg->pfn_limit, lg->mem_base);
	return val;
}

/* Same thing for writing a value. */
void lgwrite_u32(struct lguest *lg, unsigned long addr, u32 val)
{
	if (!lguest_address_ok(lg, addr, sizeof(val))
	    || put_user(val, (u32 *)(lg->mem_base + addr)) != 0)
		kill_guest(lg, "bad write address %#lx", addr);
}

/* This routine is more generic, and copies a range of Guest bytes into a
 * buffer.  If the copy_from_user() fails, we fill the buffer with zeroes, so
 * the caller doesn't end up using uninitialized kernel memory. */
void lgread(struct lguest *lg, void *b, unsigned long addr, unsigned bytes)
{
	if (!lguest_address_ok(lg, addr, bytes)
	    || copy_from_user(b, lg->mem_base + addr, bytes) != 0) {
		/* copy_from_user should do this, but as we rely on it... */
		memset(b, 0, bytes);
		kill_guest(lg, "bad read address %#lx len %u", addr, bytes);
	}
}

/* Similarly, our generic routine to copy into a range of Guest bytes. */
void lgwrite(struct lguest *lg, unsigned long addr, const void *b,
	     unsigned bytes)
{
	if (!lguest_address_ok(lg, addr, bytes)
	    || copy_to_user(lg->mem_base + addr, b, bytes) != 0)
		kill_guest(lg, "bad write address %#lx len %u", addr, bytes);
}
/* (end of memory access helper routines) :*/

/*H:030 Let's jump straight to the the main loop which runs the Guest.
 * Remember, this is called by the Launcher reading /dev/lguest, and we keep
 * going around and around until something interesting happens. */
int run_guest(struct lguest *lg, unsigned long __user *user)
{
	/* We stop running once the Guest is dead. */
	while (!lg->dead) {
		/* First we run any hypercalls the Guest wants done. */
		if (lg->hcall)
			do_hypercalls(lg);

		/* It's possible the Guest did a SEND_DMA hypercall to the
		 * Launcher, in which case we return from the read() now. */
		if (lg->dma_is_pending) {
			if (put_user(lg->pending_dma, user) ||
			    put_user(lg->pending_key, user+1))
				return -EFAULT;
			return sizeof(unsigned long)*2;
		}

		/* Check for signals */
		if (signal_pending(current))
			return -ERESTARTSYS;

		/* If Waker set break_out, return to Launcher. */
		if (lg->break_out)
			return -EAGAIN;

		/* Check if there are any interrupts which can be delivered
		 * now: if so, this sets up the hander to be executed when we
		 * next run the Guest. */
		maybe_do_interrupt(lg);

		/* All long-lived kernel loops need to check with this horrible
		 * thing called the freezer.  If the Host is trying to suspend,
		 * it stops us. */
		try_to_freeze();

		/* Just make absolutely sure the Guest is still alive.  One of
		 * those hypercalls could have been fatal, for example. */
		if (lg->dead)
			break;

		/* If the Guest asked to be stopped, we sleep.  The Guest's
		 * clock timer or LHCALL_BREAK from the Waker will wake us. */
		if (lg->halted) {
			set_current_state(TASK_INTERRUPTIBLE);
			schedule();
			continue;
		}

		/* OK, now we're ready to jump into the Guest.  First we put up
		 * the "Do Not Disturb" sign: */
		local_irq_disable();

		/* Actually run the Guest until something happens. */
		lguest_arch_run_guest(lg);

		/* Now we're ready to be interrupted or moved to other CPUs */
		local_irq_enable();

		/* Now we deal with whatever happened to the Guest. */
		lguest_arch_handle_trap(lg);
	}

	/* The Guest is dead => "No such file or directory" */
	return -ENOENT;
}

/*H:000
 * Welcome to the Host!
 *
 * By this point your brain has been tickled by the Guest code and numbed by
 * the Launcher code; prepare for it to be stretched by the Host code.  This is
 * the heart.  Let's begin at the initialization routine for the Host's lg
 * module.
 */
static int __init init(void)
{
	int err;

	/* Lguest can't run under Xen, VMI or itself.  It does Tricky Stuff. */
	if (paravirt_enabled()) {
		printk("lguest is afraid of %s\n", pv_info.name);
		return -EPERM;
	}

	/* First we put the Switcher up in very high virtual memory. */
	err = map_switcher();
	if (err)
		goto out;

	/* Now we set up the pagetable implementation for the Guests. */
	err = init_pagetables(switcher_page, SHARED_SWITCHER_PAGES);
	if (err)
		goto unmap;

	/* The I/O subsystem needs some things initialized. */
	lguest_io_init();

	/* We might need to reserve an interrupt vector. */
	err = init_interrupts();
	if (err)
		goto free_pgtables;

	/* /dev/lguest needs to be registered. */
	err = lguest_device_init();
	if (err)
		goto free_interrupts;

	/* Finally we do some architecture-specific setup. */
	lguest_arch_host_init();

	/* All good! */
	return 0;

free_interrupts:
	free_interrupts();
free_pgtables:
	free_pagetables();
unmap:
	unmap_switcher();
out:
	return err;
}

/* Cleaning up is just the same code, backwards.  With a little French. */
static void __exit fini(void)
{
	lguest_device_remove();
	free_interrupts();
	free_pagetables();
	unmap_switcher();

	lguest_arch_host_fini();
}
/*:*/

/* The Host side of lguest can be a module.  This is a nice way for people to
 * play with it.  */
module_init(init);
module_exit(fini);
MODULE_LICENSE("GPL");
MODULE_AUTHOR("Rusty Russell <rusty@rustcorp.com.au>");
