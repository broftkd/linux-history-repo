/*
 *	x86 SMP booting functions
 *
 *	(c) 1995 Alan Cox, Building #3 <alan@redhat.com>
 *	(c) 1998, 1999, 2000 Ingo Molnar <mingo@redhat.com>
 *	Copyright 2001 Andi Kleen, SuSE Labs.
 *
 *	Much of the core SMP work is based on previous work by Thomas Radke, to
 *	whom a great many thanks are extended.
 *
 *	Thanks to Intel for making available several different Pentium,
 *	Pentium Pro and Pentium-II/Xeon MP machines.
 *	Original development of Linux SMP code supported by Caldera.
 *
 *	This code is released under the GNU General Public License version 2
 *
 *	Fixes
 *		Felix Koop	:	NR_CPUS used properly
 *		Jose Renau	:	Handle single CPU case.
 *		Alan Cox	:	By repeated request 8) - Total BogoMIP report.
 *		Greg Wright	:	Fix for kernel stacks panic.
 *		Erich Boleyn	:	MP v1.4 and additional changes.
 *	Matthias Sattler	:	Changes for 2.1 kernel map.
 *	Michel Lespinasse	:	Changes for 2.1 kernel map.
 *	Michael Chastain	:	Change trampoline.S to gnu as.
 *		Alan Cox	:	Dumb bug: 'B' step PPro's are fine
 *		Ingo Molnar	:	Added APIC timers, based on code
 *					from Jose Renau
 *		Ingo Molnar	:	various cleanups and rewrites
 *		Tigran Aivazian	:	fixed "0.00 in /proc/uptime on SMP" bug.
 *	Maciej W. Rozycki	:	Bits for genuine 82489DX APICs
 *	Andi Kleen		:	Changed for SMP boot into long mode.
 *		Rusty Russell	:	Hacked into shape for new "hotplug" boot process.
 *      Andi Kleen              :       Converted to new state machine.
 *					Various cleanups.
 *					Probably mostly hotplug CPU ready now.
 *	Ashok Raj			: CPU hotplug support
 */


#include <linux/init.h>

#include <linux/mm.h>
#include <linux/kernel_stat.h>
#include <linux/bootmem.h>
#include <linux/thread_info.h>
#include <linux/module.h>
#include <linux/delay.h>
#include <linux/mc146818rtc.h>
#include <linux/smp.h>
#include <linux/kdebug.h>

#include <asm/mtrr.h>
#include <asm/pgalloc.h>
#include <asm/desc.h>
#include <asm/tlbflush.h>
#include <asm/proto.h>
#include <asm/nmi.h>
#include <asm/irq.h>
#include <asm/hw_irq.h>
#include <asm/numa.h>

/* Set when the idlers are all forked */
int smp_threads_ready;

/* State of each CPU */
DEFINE_PER_CPU(int, cpu_state) = { 0 };

/*
 * Store all idle threads, this can be reused instead of creating
 * a new thread. Also avoids complicated thread destroy functionality
 * for idle threads.
 */
#ifdef CONFIG_HOTPLUG_CPU
/*
 * Needed only for CONFIG_HOTPLUG_CPU because __cpuinitdata is
 * removed after init for !CONFIG_HOTPLUG_CPU.
 */
static DEFINE_PER_CPU(struct task_struct *, idle_thread_array);
#define get_idle_for_cpu(x)     (per_cpu(idle_thread_array, x))
#define set_idle_for_cpu(x,p)   (per_cpu(idle_thread_array, x) = (p))
#else
struct task_struct *idle_thread_array[NR_CPUS] __cpuinitdata ;
#define get_idle_for_cpu(x)     (idle_thread_array[(x)])
#define set_idle_for_cpu(x,p)   (idle_thread_array[(x)] = (p))
#endif

/*
 * The bootstrap kernel entry code has set these up. Save them for
 * a given CPU
 */

static void __cpuinit smp_store_cpu_info(int id)
{
	struct cpuinfo_x86 *c = &cpu_data(id);

	*c = boot_cpu_data;
	c->cpu_index = id;
	if (id != 0)
		identify_secondary_cpu(c);
}

static inline void wait_for_init_deassert(atomic_t *deassert)
{
	while (!atomic_read(deassert))
		cpu_relax();
	return;
}

static atomic_t init_deasserted __cpuinitdata;

/*
 * Report back to the Boot Processor.
 * Running on AP.
 */
void __cpuinit smp_callin(void)
{
	int cpuid, phys_id;
	unsigned long timeout;

	/*
	 * If waken up by an INIT in an 82489DX configuration
	 * we may get here before an INIT-deassert IPI reaches
	 * our local APIC.  We have to wait for the IPI or we'll
	 * lock up on an APIC access.
	 */
	wait_for_init_deassert(&init_deasserted);

	/*
	 * (This works even if the APIC is not enabled.)
	 */
	phys_id = GET_APIC_ID(apic_read(APIC_ID));
	cpuid = smp_processor_id();
	if (cpu_isset(cpuid, cpu_callin_map)) {
		panic("smp_callin: phys CPU#%d, CPU#%d already present??\n",
					phys_id, cpuid);
	}
	Dprintk("CPU#%d (phys ID: %d) waiting for CALLOUT\n", cpuid, phys_id);

	/*
	 * STARTUP IPIs are fragile beasts as they might sometimes
	 * trigger some glue motherboard logic. Complete APIC bus
	 * silence for 1 second, this overestimates the time the
	 * boot CPU is spending to send the up to 2 STARTUP IPIs
	 * by a factor of two. This should be enough.
	 */

	/*
	 * Waiting 2s total for startup (udelay is not yet working)
	 */
	timeout = jiffies + 2*HZ;
	while (time_before(jiffies, timeout)) {
		/*
		 * Has the boot CPU finished it's STARTUP sequence?
		 */
		if (cpu_isset(cpuid, cpu_callout_map))
			break;
		cpu_relax();
	}

	if (!time_before(jiffies, timeout)) {
		panic("smp_callin: CPU%d started up but did not get a callout!\n",
			cpuid);
	}

	/*
	 * the boot CPU has finished the init stage and is spinning
	 * on callin_map until we finish. We are free to set up this
	 * CPU, first the APIC. (this is probably redundant on most
	 * boards)
	 */

	Dprintk("CALLIN, before setup_local_APIC().\n");
	setup_local_APIC();
	end_local_APIC_setup();

	/*
	 * Get our bogomips.
 	 *
 	 * Need to enable IRQs because it can take longer and then
	 * the NMI watchdog might kill us.
	 */
	local_irq_enable();
	calibrate_delay();
	local_irq_disable();
	Dprintk("Stack at about %p\n",&cpuid);

	/*
	 * Save our processor parameters
	 */
 	smp_store_cpu_info(cpuid);

	/*
	 * Allow the master to continue.
	 */
	cpu_set(cpuid, cpu_callin_map);
}

/*
 * Setup code on secondary processor (after comming out of the trampoline)
 */
void __cpuinit start_secondary(void)
{
	/*
	 * Dont put anything before smp_callin(), SMP
	 * booting is too fragile that we want to limit the
	 * things done here to the most necessary things.
	 */
	cpu_init();
	preempt_disable();
	smp_callin();

	/* otherwise gcc will move up the smp_processor_id before the cpu_init */
	barrier();

	/*
  	 * Check TSC sync first:
 	 */
	check_tsc_sync_target();

	if (nmi_watchdog == NMI_IO_APIC) {
		disable_8259A_irq(0);
		enable_NMI_through_LVT0();
		enable_8259A_irq(0);
	}

	/*
	 * The sibling maps must be set before turing the online map on for
	 * this cpu
	 */
	set_cpu_sibling_map(smp_processor_id());

	/*
	 * We need to hold call_lock, so there is no inconsistency
	 * between the time smp_call_function() determines number of
	 * IPI recipients, and the time when the determination is made
	 * for which cpus receive the IPI in genapic_flat.c. Holding this
	 * lock helps us to not include this cpu in a currently in progress
	 * smp_call_function().
	 */
	lock_ipi_call_lock();
	spin_lock(&vector_lock);

	/* Setup the per cpu irq handling data structures */
	__setup_vector_irq(smp_processor_id());
	/*
	 * Allow the master to continue.
	 */
	spin_unlock(&vector_lock);
	cpu_set(smp_processor_id(), cpu_online_map);
	per_cpu(cpu_state, smp_processor_id()) = CPU_ONLINE;

	unlock_ipi_call_lock();

	setup_secondary_clock();

	cpu_idle();
}

extern volatile unsigned long init_rsp;
extern void (*initial_code)(void);

#ifdef APIC_DEBUG
static void inquire_remote_apic(int apicid)
{
	unsigned i, regs[] = { APIC_ID >> 4, APIC_LVR >> 4, APIC_SPIV >> 4 };
	char *names[] = { "ID", "VERSION", "SPIV" };
	int timeout;
	u32 status;

	printk(KERN_INFO "Inquiring remote APIC #%d...\n", apicid);

	for (i = 0; i < ARRAY_SIZE(regs); i++) {
		printk(KERN_INFO "... APIC #%d %s: ", apicid, names[i]);

		/*
		 * Wait for idle.
		 */
		status = safe_apic_wait_icr_idle();
		if (status)
			printk(KERN_CONT
			       "a previous APIC delivery may have failed\n");

		apic_write_around(APIC_ICR2, SET_APIC_DEST_FIELD(apicid));
		apic_write_around(APIC_ICR, APIC_DM_REMRD | regs[i]);

		timeout = 0;
		do {
			udelay(100);
			status = apic_read(APIC_ICR) & APIC_ICR_RR_MASK;
		} while (status == APIC_ICR_RR_INPROG && timeout++ < 1000);

		switch (status) {
		case APIC_ICR_RR_VALID:
			status = apic_read(APIC_RRR);
			printk(KERN_CONT "%08x\n", status);
			break;
		default:
			printk(KERN_CONT "failed\n");
		}
	}
}
#endif

/*
 * Kick the secondary to wake up.
 */
static int __cpuinit wakeup_secondary_via_INIT(int phys_apicid, unsigned int start_rip)
{
	unsigned long send_status, accept_status = 0;
	int maxlvt, num_starts, j;

	Dprintk("Asserting INIT.\n");

	/*
	 * Turn INIT on target chip
	 */
	apic_write_around(APIC_ICR2, SET_APIC_DEST_FIELD(phys_apicid));

	/*
	 * Send IPI
	 */
	apic_write_around(APIC_ICR, APIC_INT_LEVELTRIG | APIC_INT_ASSERT
				| APIC_DM_INIT);

	Dprintk("Waiting for send to finish...\n");
	send_status = safe_apic_wait_icr_idle();

	mdelay(10);

	Dprintk("Deasserting INIT.\n");

	/* Target chip */
	apic_write_around(APIC_ICR2, SET_APIC_DEST_FIELD(phys_apicid));

	/* Send IPI */
	apic_write_around(APIC_ICR, APIC_INT_LEVELTRIG | APIC_DM_INIT);

	Dprintk("Waiting for send to finish...\n");
	send_status = safe_apic_wait_icr_idle();

	mb();
	atomic_set(&init_deasserted, 1);

	num_starts = 2;

	/*
	 * Paravirt / VMI wants a startup IPI hook here to set up the
	 * target processor state.
	 */
	startup_ipi_hook(phys_apicid, (unsigned long) start_secondary,
			(unsigned long) init_rsp);


	/*
	 * Run STARTUP IPI loop.
	 */
	Dprintk("#startup loops: %d.\n", num_starts);

	maxlvt = lapic_get_maxlvt();

	for (j = 1; j <= num_starts; j++) {
		Dprintk("Sending STARTUP #%d.\n",j);
		apic_read_around(APIC_SPIV);
		apic_write(APIC_ESR, 0);
		apic_read(APIC_ESR);
		Dprintk("After apic_write.\n");

		/*
		 * STARTUP IPI
		 */

		/* Target chip */
		apic_write_around(APIC_ICR2, SET_APIC_DEST_FIELD(phys_apicid));

		/* Boot on the stack */
		/* Kick the second */
		apic_write_around(APIC_ICR, APIC_DM_STARTUP | (start_rip>>12));

		/*
		 * Give the other CPU some time to accept the IPI.
		 */
		udelay(300);

		Dprintk("Startup point 1.\n");

		Dprintk("Waiting for send to finish...\n");
		send_status = safe_apic_wait_icr_idle();

		/*
		 * Give the other CPU some time to accept the IPI.
		 */
		udelay(200);
		/*
		 * Due to the Pentium erratum 3AP.
		 */
		if (maxlvt > 3) {
			apic_read_around(APIC_SPIV);
			apic_write(APIC_ESR, 0);
		}
		accept_status = (apic_read(APIC_ESR) & 0xEF);
		if (send_status || accept_status)
			break;
	}
	Dprintk("After Startup.\n");

	if (send_status)
		printk(KERN_ERR "APIC never delivered???\n");
	if (accept_status)
		printk(KERN_ERR "APIC delivery error (%lx).\n", accept_status);

	return (send_status | accept_status);
}

struct create_idle {
	struct work_struct work;
	struct task_struct *idle;
	struct completion done;
	int cpu;
};

static void __cpuinit do_fork_idle(struct work_struct *work)
{
	struct create_idle *c_idle =
		container_of(work, struct create_idle, work);

	c_idle->idle = fork_idle(c_idle->cpu);
	complete(&c_idle->done);
}

/*
 * Boot one CPU.
 */
static int __cpuinit do_boot_cpu(int cpu, int apicid)
{
	unsigned long boot_error;
	int timeout;
	unsigned long start_rip;
	struct create_idle c_idle = {
		.cpu = cpu,
		.done = COMPLETION_INITIALIZER_ONSTACK(c_idle.done),
	};
	INIT_WORK(&c_idle.work, do_fork_idle);

	/* allocate memory for gdts of secondary cpus. Hotplug is considered */
	if (!cpu_gdt_descr[cpu].address &&
		!(cpu_gdt_descr[cpu].address = get_zeroed_page(GFP_KERNEL))) {
		printk(KERN_ERR "Failed to allocate GDT for CPU %d\n", cpu);
		return -1;
	}

	/* Allocate node local memory for AP pdas */
	if (cpu_pda(cpu) == &boot_cpu_pda[cpu]) {
		struct x8664_pda *newpda, *pda;
		int node = cpu_to_node(cpu);
		pda = cpu_pda(cpu);
		newpda = kmalloc_node(sizeof (struct x8664_pda), GFP_ATOMIC,
				      node);
		if (newpda) {
			memcpy(newpda, pda, sizeof (struct x8664_pda));
			cpu_pda(cpu) = newpda;
		} else
			printk(KERN_ERR
		"Could not allocate node local PDA for CPU %d on node %d\n",
				cpu, node);
	}

	alternatives_smp_switch(1);

	c_idle.idle = get_idle_for_cpu(cpu);

	if (c_idle.idle) {
		c_idle.idle->thread.sp = (unsigned long) (((struct pt_regs *)
			(THREAD_SIZE +  task_stack_page(c_idle.idle))) - 1);
		init_idle(c_idle.idle, cpu);
		goto do_rest;
	}

	/*
	 * During cold boot process, keventd thread is not spun up yet.
	 * When we do cpu hot-add, we create idle threads on the fly, we should
	 * not acquire any attributes from the calling context. Hence the clean
	 * way to create kernel_threads() is to do that from keventd().
	 * We do the current_is_keventd() due to the fact that ACPI notifier
	 * was also queuing to keventd() and when the caller is already running
	 * in context of keventd(), we would end up with locking up the keventd
	 * thread.
	 */
	if (!keventd_up() || current_is_keventd())
		c_idle.work.func(&c_idle.work);
	else {
		schedule_work(&c_idle.work);
		wait_for_completion(&c_idle.done);
	}

	if (IS_ERR(c_idle.idle)) {
		printk("failed fork for CPU %d\n", cpu);
		return PTR_ERR(c_idle.idle);
	}

	set_idle_for_cpu(cpu, c_idle.idle);

do_rest:

	cpu_pda(cpu)->pcurrent = c_idle.idle;

	start_rip = setup_trampoline();

	init_rsp = c_idle.idle->thread.sp;
	load_sp0(&per_cpu(init_tss, cpu), &c_idle.idle->thread);
	initial_code = start_secondary;
	clear_tsk_thread_flag(c_idle.idle, TIF_FORK);

	printk(KERN_INFO "Booting processor %d/%d APIC 0x%x\n", cpu,
		cpus_weight(cpu_present_map),
		apicid);

	/*
	 * This grunge runs the startup process for
	 * the targeted processor.
	 */

	atomic_set(&init_deasserted, 0);

	Dprintk("Setting warm reset code and vector.\n");

	CMOS_WRITE(0xa, 0xf);
	local_flush_tlb();
	Dprintk("1.\n");
	*((volatile unsigned short *) phys_to_virt(0x469)) = start_rip >> 4;
	Dprintk("2.\n");
	*((volatile unsigned short *) phys_to_virt(0x467)) = start_rip & 0xf;
	Dprintk("3.\n");

	/*
	 * Be paranoid about clearing APIC errors.
	 */
	apic_write(APIC_ESR, 0);
	apic_read(APIC_ESR);

	/*
	 * Status is now clean
	 */
	boot_error = 0;

	/*
	 * Starting actual IPI sequence...
	 */
	boot_error = wakeup_secondary_via_INIT(apicid, start_rip);

	if (!boot_error) {
		/*
		 * allow APs to start initializing.
		 */
		Dprintk("Before Callout %d.\n", cpu);
		cpu_set(cpu, cpu_callout_map);
		Dprintk("After Callout %d.\n", cpu);

		/*
		 * Wait 5s total for a response
		 */
		for (timeout = 0; timeout < 50000; timeout++) {
			if (cpu_isset(cpu, cpu_callin_map))
				break;	/* It has booted */
			udelay(100);
		}

		if (cpu_isset(cpu, cpu_callin_map)) {
			/* number CPUs logically, starting from 1 (BSP is 0) */
			Dprintk("CPU has booted.\n");
			printk(KERN_INFO "CPU%d: ", cpu);
			print_cpu_info(&cpu_data(cpu));
		} else {
			boot_error = 1;
			if (*((volatile unsigned char *)phys_to_virt(SMP_TRAMPOLINE_BASE))
					== 0xA5)
				/* trampoline started but...? */
				printk("Stuck ??\n");
			else
				/* trampoline code not run */
				printk("Not responding.\n");
#ifdef APIC_DEBUG
			inquire_remote_apic(apicid);
#endif
		}
	}
	if (boot_error) {
		cpu_clear(cpu, cpu_callout_map); /* was set here (do_boot_cpu()) */
		clear_bit(cpu, (unsigned long *)&cpu_initialized); /* was set by cpu_init() */
		clear_node_cpumask(cpu); /* was set by numa_add_cpu */
		cpu_clear(cpu, cpu_present_map);
		cpu_clear(cpu, cpu_possible_map);
		per_cpu(x86_cpu_to_apicid, cpu) = BAD_APICID;
		return -EIO;
	}

	return 0;
}

cycles_t cacheflush_time;
unsigned long cache_decay_ticks;

/*
 * Cleanup possible dangling ends...
 */
static __cpuinit void smp_cleanup_boot(void)
{
	/*
	 * Paranoid:  Set warm reset code and vector here back
	 * to default values.
	 */
	CMOS_WRITE(0, 0xf);

	/*
	 * Reset trampoline flag
	 */
	*((volatile int *) phys_to_virt(0x467)) = 0;
}

/*
 * Fall back to non SMP mode after errors.
 *
 * RED-PEN audit/test this more. I bet there is more state messed up here.
 */
static __init void disable_smp(void)
{
	cpu_present_map = cpumask_of_cpu(0);
	cpu_possible_map = cpumask_of_cpu(0);
	if (smp_found_config)
		phys_cpu_present_map = physid_mask_of_physid(boot_cpu_id);
	else
		phys_cpu_present_map = physid_mask_of_physid(0);
	cpu_set(0, per_cpu(cpu_sibling_map, 0));
	cpu_set(0, per_cpu(cpu_core_map, 0));
}

/*
 * Various sanity checks.
 */
static int __init smp_sanity_check(unsigned max_cpus)
{
	if (!physid_isset(hard_smp_processor_id(), phys_cpu_present_map)) {
		printk("weird, boot CPU (#%d) not listed by the BIOS.\n",
		       hard_smp_processor_id());
		physid_set(hard_smp_processor_id(), phys_cpu_present_map);
	}

	/*
	 * If we couldn't find an SMP configuration at boot time,
	 * get out of here now!
	 */
	if (!smp_found_config) {
		printk(KERN_NOTICE "SMP motherboard not detected.\n");
		disable_smp();
		if (APIC_init_uniprocessor())
			printk(KERN_NOTICE "Local APIC not detected."
					   " Using dummy APIC emulation.\n");
		return -1;
	}

	/*
	 * Should not be necessary because the MP table should list the boot
	 * CPU too, but we do it for the sake of robustness anyway.
	 */
	if (!physid_isset(boot_cpu_id, phys_cpu_present_map)) {
		printk(KERN_NOTICE "weird, boot CPU (#%d) not listed by the BIOS.\n",
								 boot_cpu_id);
		physid_set(hard_smp_processor_id(), phys_cpu_present_map);
	}

	/*
	 * If we couldn't find a local APIC, then get out of here now!
	 */
	if (!cpu_has_apic) {
		printk(KERN_ERR "BIOS bug, local APIC #%d not detected!...\n",
			boot_cpu_id);
		printk(KERN_ERR "... forcing use of dummy APIC emulation. (tell your hw vendor)\n");
		nr_ioapics = 0;
		return -1;
	}

	/*
	 * If SMP should be disabled, then really disable it!
	 */
	if (!max_cpus) {
		printk(KERN_INFO "SMP mode deactivated, forcing use of dummy APIC emulation.\n");
		nr_ioapics = 0;
		return -1;
	}

	return 0;
}

static void __init smp_cpu_index_default(void)
{
	int i;
	struct cpuinfo_x86 *c;

	for_each_cpu_mask(i, cpu_possible_map) {
		c = &cpu_data(i);
		/* mark all to hotplug */
		c->cpu_index = NR_CPUS;
	}
}

/*
 * Prepare for SMP bootup.  The MP table or ACPI has been read
 * earlier.  Just do some sanity checking here and enable APIC mode.
 */
void __init native_smp_prepare_cpus(unsigned int max_cpus)
{
	nmi_watchdog_default();
	smp_cpu_index_default();
	current_cpu_data = boot_cpu_data;
	current_thread_info()->cpu = 0;  /* needed? */
	set_cpu_sibling_map(0);

	if (smp_sanity_check(max_cpus) < 0) {
		printk(KERN_INFO "SMP disabled\n");
		disable_smp();
		return;
	}


	/*
	 * Switch from PIC to APIC mode.
	 */
	setup_local_APIC();

	/*
	 * Enable IO APIC before setting up error vector
	 */
	if (!skip_ioapic_setup && nr_ioapics)
		enable_IO_APIC();
	end_local_APIC_setup();

	if (GET_APIC_ID(apic_read(APIC_ID)) != boot_cpu_id) {
		panic("Boot APIC ID in local APIC unexpected (%d vs %d)",
		      GET_APIC_ID(apic_read(APIC_ID)), boot_cpu_id);
		/* Or can we switch back to PIC here? */
	}

	/*
	 * Now start the IO-APICs
	 */
	if (!skip_ioapic_setup && nr_ioapics)
		setup_IO_APIC();
	else
		nr_ioapics = 0;

	/*
	 * Set up local APIC timer on boot CPU.
	 */

	setup_boot_clock();
	printk(KERN_INFO "CPU%d: ", 0);
	print_cpu_info(&cpu_data(0));
}

/*
 * Early setup to make printk work.
 */
void __init native_smp_prepare_boot_cpu(void)
{
	int me = smp_processor_id();
	/* already set me in cpu_online_map in boot_cpu_init() */
	cpu_set(me, cpu_callout_map);
	per_cpu(cpu_state, me) = CPU_ONLINE;
}

/*
 * Entry point to boot a CPU.
 */
int __cpuinit native_cpu_up(unsigned int cpu)
{
	int apicid = cpu_present_to_apicid(cpu);
	unsigned long flags;
	int err;

	WARN_ON(irqs_disabled());

	Dprintk("++++++++++++++++++++=_---CPU UP  %u\n", cpu);

	if (apicid == BAD_APICID || apicid == boot_cpu_id ||
	    !physid_isset(apicid, phys_cpu_present_map)) {
		printk("__cpu_up: bad cpu %d\n", cpu);
		return -EINVAL;
	}

	/*
	 * Already booted CPU?
	 */
 	if (cpu_isset(cpu, cpu_callin_map)) {
		Dprintk("do_boot_cpu %d Already started\n", cpu);
 		return -ENOSYS;
	}

	/*
	 * Save current MTRR state in case it was changed since early boot
	 * (e.g. by the ACPI SMI) to initialize new CPUs with MTRRs in sync:
	 */
	mtrr_save_state();

	per_cpu(cpu_state, cpu) = CPU_UP_PREPARE;
	/* Boot it! */
	err = do_boot_cpu(cpu, apicid);
	if (err < 0) {
		Dprintk("do_boot_cpu failed %d\n", err);
		return err;
	}

	/* Unleash the CPU! */
	Dprintk("waiting for cpu %d\n", cpu);

	/*
  	 * Make sure and check TSC sync:
 	 */
	local_irq_save(flags);
	check_tsc_sync_source(cpu);
	local_irq_restore(flags);

	while (!cpu_isset(cpu, cpu_online_map))
		cpu_relax();
	err = 0;

	return err;
}

/*
 * Finish the SMP boot.
 */
void __init native_smp_cpus_done(unsigned int max_cpus)
{
	smp_cleanup_boot();
	setup_ioapic_dest();
	check_nmi_watchdog();
}
