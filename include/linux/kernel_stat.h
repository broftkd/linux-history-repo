#ifndef _LINUX_KERNEL_STAT_H
#define _LINUX_KERNEL_STAT_H

#include <linux/smp.h>
#include <linux/threads.h>
#include <linux/percpu.h>
#include <linux/cpumask.h>
#include <asm/irq.h>
#include <asm/cputime.h>

/*
 * 'kernel_stat.h' contains the definitions needed for doing
 * some kernel statistics (CPU usage, context switches ...),
 * used by rstatd/perfmeter
 */

struct cpu_usage_stat {
	cputime64_t user;
	cputime64_t nice;
	cputime64_t system;
	cputime64_t softirq;
	cputime64_t irq;
	cputime64_t idle;
	cputime64_t iowait;
	cputime64_t steal;
	cputime64_t guest;
};

struct kernel_stat {
	struct cpu_usage_stat	cpustat;
	unsigned int irqs[NR_IRQS];
};

DECLARE_PER_CPU(struct kernel_stat, kstat);

#define kstat_cpu(cpu)	per_cpu(kstat, cpu)
/* Must have preemption disabled for this to be meaningful. */
#define kstat_this_cpu	__get_cpu_var(kstat)

extern unsigned long long nr_context_switches(void);

struct irq_desc;

static inline void kstat_incr_irqs_this_cpu(unsigned int irq,
					    struct irq_desc *desc)
{
	kstat_this_cpu.irqs[irq]++;
}

static inline unsigned int kstat_irqs_cpu(unsigned int irq, int cpu)
{
       return kstat_cpu(cpu).irqs[irq];
}

/*
 * Number of interrupts per specific IRQ source, since bootup
 */
static inline unsigned int kstat_irqs(unsigned int irq)
{
	unsigned int sum = 0;
	int cpu;

	for_each_possible_cpu(cpu)
		sum += kstat_irqs_cpu(irq, cpu);

	return sum;
}

extern unsigned long long task_delta_exec(struct task_struct *);
extern void account_user_time(struct task_struct *, cputime_t);
extern void account_user_time_scaled(struct task_struct *, cputime_t);
extern void account_system_time(struct task_struct *, int, cputime_t);
extern void account_system_time_scaled(struct task_struct *, cputime_t);
extern void account_steal_time(struct task_struct *, cputime_t);

#endif /* _LINUX_KERNEL_STAT_H */
