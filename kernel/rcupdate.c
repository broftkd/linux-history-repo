/*
 * Read-Copy Update mechanism for mutual exclusion
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
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 *
 * Copyright IBM Corporation, 2001
 *
 * Authors: Dipankar Sarma <dipankar@in.ibm.com>
 *	    Manfred Spraul <manfred@colorfullife.com>
 * 
 * Based on the original work by Paul McKenney <paulmck@us.ibm.com>
 * and inputs from Rusty Russell, Andrea Arcangeli and Andi Kleen.
 * Papers:
 * http://www.rdrop.com/users/paulmck/paper/rclockpdcsproof.pdf
 * http://lse.sourceforge.net/locking/rclock_OLS.2001.05.01c.sc.pdf (OLS2001)
 *
 * For detailed explanation of Read-Copy Update mechanism see -
 * 		http://lse.sourceforge.net/locking/rcupdate.html
 *
 */
#include <linux/types.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/spinlock.h>
#include <linux/smp.h>
#include <linux/interrupt.h>
#include <linux/sched.h>
#include <asm/atomic.h>
#include <linux/bitops.h>
#include <linux/percpu.h>
#include <linux/notifier.h>
#include <linux/cpu.h>
#include <linux/mutex.h>
#include <linux/module.h>

enum rcu_barrier {
	RCU_BARRIER_STD,
	RCU_BARRIER_BH,
	RCU_BARRIER_SCHED,
};

static DEFINE_PER_CPU(struct rcu_head, rcu_barrier_head) = {NULL};
static atomic_t rcu_barrier_cpu_count;
static DEFINE_MUTEX(rcu_barrier_mutex);
static struct completion rcu_barrier_completion;

/*
 * Awaken the corresponding synchronize_rcu() instance now that a
 * grace period has elapsed.
 */
void wakeme_after_rcu(struct rcu_head  *head)
{
	struct rcu_synchronize *rcu;

	rcu = container_of(head, struct rcu_synchronize, head);
	complete(&rcu->completion);
}

/**
 * synchronize_rcu - wait until a grace period has elapsed.
 *
 * Control will return to the caller some time after a full grace
 * period has elapsed, in other words after all currently executing RCU
 * read-side critical sections have completed.  RCU read-side critical
 * sections are delimited by rcu_read_lock() and rcu_read_unlock(),
 * and may be nested.
 */
void synchronize_rcu(void);	/* Makes kernel-doc tools happy */
synchronize_rcu_xxx(synchronize_rcu, call_rcu)
EXPORT_SYMBOL_GPL(synchronize_rcu);

static void rcu_barrier_callback(struct rcu_head *notused)
{
	if (atomic_dec_and_test(&rcu_barrier_cpu_count))
		complete(&rcu_barrier_completion);
}

/*
 * Called with preemption disabled, and from cross-cpu IRQ context.
 */
static void rcu_barrier_func(void *type)
{
	int cpu = smp_processor_id();
	struct rcu_head *head = &per_cpu(rcu_barrier_head, cpu);

	atomic_inc(&rcu_barrier_cpu_count);
	switch ((enum rcu_barrier)type) {
	case RCU_BARRIER_STD:
		call_rcu(head, rcu_barrier_callback);
		break;
	case RCU_BARRIER_BH:
		call_rcu_bh(head, rcu_barrier_callback);
		break;
	case RCU_BARRIER_SCHED:
		call_rcu_sched(head, rcu_barrier_callback);
		break;
	}
}

/*
 * Orchestrate the specified type of RCU barrier, waiting for all
 * RCU callbacks of the specified type to complete.
 */
static void _rcu_barrier(enum rcu_barrier type)
{
	BUG_ON(in_interrupt());
	/* Take cpucontrol mutex to protect against CPU hotplug */
	mutex_lock(&rcu_barrier_mutex);
	init_completion(&rcu_barrier_completion);
	atomic_set(&rcu_barrier_cpu_count, 0);
	/*
	 * The queueing of callbacks in all CPUs must be atomic with
	 * respect to RCU, otherwise one CPU may queue a callback,
	 * wait for a grace period, decrement barrier count and call
	 * complete(), while other CPUs have not yet queued anything.
	 * So, we need to make sure that grace periods cannot complete
	 * until all the callbacks are queued.
	 */
	rcu_read_lock();
	on_each_cpu(rcu_barrier_func, (void *)type, 1);
	rcu_read_unlock();
	wait_for_completion(&rcu_barrier_completion);
	mutex_unlock(&rcu_barrier_mutex);
}

/**
 * rcu_barrier - Wait until all in-flight call_rcu() callbacks complete.
 */
void rcu_barrier(void)
{
	_rcu_barrier(RCU_BARRIER_STD);
}
EXPORT_SYMBOL_GPL(rcu_barrier);

/**
 * rcu_barrier_bh - Wait until all in-flight call_rcu_bh() callbacks complete.
 */
void rcu_barrier_bh(void)
{
	_rcu_barrier(RCU_BARRIER_BH);
}
EXPORT_SYMBOL_GPL(rcu_barrier_bh);

/**
 * rcu_barrier_sched - Wait for in-flight call_rcu_sched() callbacks.
 */
void rcu_barrier_sched(void)
{
	_rcu_barrier(RCU_BARRIER_SCHED);
}
EXPORT_SYMBOL_GPL(rcu_barrier_sched);

void __init rcu_init(void)
{
	__rcu_init();
}

