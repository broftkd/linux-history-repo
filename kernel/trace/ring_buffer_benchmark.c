/*
 * ring buffer tester and benchmark
 *
 * Copyright (C) 2009 Steven Rostedt <srostedt@redhat.com>
 */
#include <linux/ring_buffer.h>
#include <linux/completion.h>
#include <linux/kthread.h>
#include <linux/module.h>
#include <linux/time.h>

struct rb_page {
	u64		ts;
	local_t		commit;
	char		data[4080];
};

/* run time and sleep time in seconds */
#define RUN_TIME	10
#define SLEEP_TIME	10

/* number of events for writer to wake up the reader */
static int wakeup_interval = 100;

static int reader_finish;
static struct completion read_start;
static struct completion read_done;

static struct ring_buffer *buffer;
static struct task_struct *producer;
static struct task_struct *consumer;
static unsigned long read;

static int disable_reader;
module_param(disable_reader, uint, 0644);
MODULE_PARM_DESC(disable_reader, "only run producer");

static int read_events;

static int kill_test;

#define KILL_TEST()				\
	do {					\
		if (!kill_test) {		\
			kill_test = 1;		\
			WARN_ON(1);		\
		}				\
	} while (0)

enum event_status {
	EVENT_FOUND,
	EVENT_DROPPED,
};

static enum event_status read_event(int cpu)
{
	struct ring_buffer_event *event;
	int *entry;
	u64 ts;

	event = ring_buffer_consume(buffer, cpu, &ts);
	if (!event)
		return EVENT_DROPPED;

	entry = ring_buffer_event_data(event);
	if (*entry != cpu) {
		KILL_TEST();
		return EVENT_DROPPED;
	}

	read++;
	return EVENT_FOUND;
}

static enum event_status read_page(int cpu)
{
	struct ring_buffer_event *event;
	struct rb_page *rpage;
	unsigned long commit;
	void *bpage;
	int *entry;
	int ret;
	int inc;
	int i;

	bpage = ring_buffer_alloc_read_page(buffer);
	if (!bpage)
		return EVENT_DROPPED;

	ret = ring_buffer_read_page(buffer, &bpage, PAGE_SIZE, cpu, 1);
	if (ret >= 0) {
		rpage = bpage;
		commit = local_read(&rpage->commit);
		for (i = 0; i < commit && !kill_test; i += inc) {

			if (i >= (PAGE_SIZE - offsetof(struct rb_page, data))) {
				KILL_TEST();
				break;
			}

			inc = -1;
			event = (void *)&rpage->data[i];
			switch (event->type_len) {
			case RINGBUF_TYPE_PADDING:
				/* We don't expect any padding */
				KILL_TEST();
				break;
			case RINGBUF_TYPE_TIME_EXTEND:
				inc = 8;
				break;
			case 0:
				entry = ring_buffer_event_data(event);
				if (*entry != cpu) {
					KILL_TEST();
					break;
				}
				read++;
				if (!event->array[0]) {
					KILL_TEST();
					break;
				}
				inc = event->array[0];
				break;
			default:
				entry = ring_buffer_event_data(event);
				if (*entry != cpu) {
					KILL_TEST();
					break;
				}
				read++;
				inc = ((event->type_len + 1) * 4);
			}
			if (kill_test)
				break;

			if (inc <= 0) {
				KILL_TEST();
				break;
			}
		}
	}
	ring_buffer_free_read_page(buffer, bpage);

	if (ret < 0)
		return EVENT_DROPPED;
	return EVENT_FOUND;
}

static void ring_buffer_consumer(void)
{
	/* toggle between reading pages and events */
	read_events ^= 1;

	read = 0;
	while (!reader_finish && !kill_test) {
		int found;

		do {
			int cpu;

			found = 0;
			for_each_online_cpu(cpu) {
				enum event_status stat;

				if (read_events)
					stat = read_event(cpu);
				else
					stat = read_page(cpu);

				if (kill_test)
					break;
				if (stat == EVENT_FOUND)
					found = 1;
			}
		} while (found && !kill_test);

		set_current_state(TASK_INTERRUPTIBLE);
		if (reader_finish)
			break;

		schedule();
		__set_current_state(TASK_RUNNING);
	}
	reader_finish = 0;
	complete(&read_done);
}

static void ring_buffer_producer(void)
{
	struct timeval start_tv;
	struct timeval end_tv;
	unsigned long long time;
	unsigned long long entries;
	unsigned long long overruns;
	unsigned long missed = 0;
	unsigned long hit = 0;
	unsigned long avg;
	int cnt = 0;

	/*
	 * Hammer the buffer for 10 secs (this may
	 * make the system stall)
	 */
	pr_info("Starting ring buffer hammer\n");
	do_gettimeofday(&start_tv);
	do {
		struct ring_buffer_event *event;
		int *entry;

		event = ring_buffer_lock_reserve(buffer, 10);
		if (!event) {
			missed++;
		} else {
			hit++;
			entry = ring_buffer_event_data(event);
			*entry = smp_processor_id();
			ring_buffer_unlock_commit(buffer, event);
		}
		do_gettimeofday(&end_tv);

		cnt++;
		if (consumer && !(cnt % wakeup_interval))
			wake_up_process(consumer);

#ifndef CONFIG_PREEMPT
		/*
		 * If we are a non preempt kernel, the 10 second run will
		 * stop everything while it runs. Instead, we will call
		 * cond_resched and also add any time that was lost by a
		 * rescedule.
		 *
		 * Do a cond resched at the same frequency we would wake up
		 * the reader.
		 */
		if (cnt % wakeup_interval)
			cond_resched();
#endif

	} while (end_tv.tv_sec < (start_tv.tv_sec + RUN_TIME) && !kill_test);
	pr_info("End ring buffer hammer\n");

	if (consumer) {
		/* Init both completions here to avoid races */
		init_completion(&read_start);
		init_completion(&read_done);
		/* the completions must be visible before the finish var */
		smp_wmb();
		reader_finish = 1;
		/* finish var visible before waking up the consumer */
		smp_wmb();
		wake_up_process(consumer);
		wait_for_completion(&read_done);
	}

	time = end_tv.tv_sec - start_tv.tv_sec;
	time *= USEC_PER_SEC;
	time += (long long)((long)end_tv.tv_usec - (long)start_tv.tv_usec);

	entries = ring_buffer_entries(buffer);
	overruns = ring_buffer_overruns(buffer);

	if (kill_test)
		pr_info("ERROR!\n");
	pr_info("Time:     %lld (usecs)\n", time);
	pr_info("Overruns: %lld\n", overruns);
	if (disable_reader)
		pr_info("Read:     (reader disabled)\n");
	else
		pr_info("Read:     %ld  (by %s)\n", read,
			read_events ? "events" : "pages");
	pr_info("Entries:  %lld\n", entries);
	pr_info("Total:    %lld\n", entries + overruns + read);
	pr_info("Missed:   %ld\n", missed);
	pr_info("Hit:      %ld\n", hit);

	/* Convert time from usecs to millisecs */
	do_div(time, USEC_PER_MSEC);
	if (time)
		hit /= (long)time;
	else
		pr_info("TIME IS ZERO??\n");

	pr_info("Entries per millisec: %ld\n", hit);

	if (hit) {
		/* Calculate the average time in nanosecs */
		avg = NSEC_PER_MSEC / hit;
		pr_info("%ld ns per entry\n", avg);
	}

	if (missed) {
		if (time)
			missed /= (long)time;

		pr_info("Total iterations per millisec: %ld\n", hit + missed);

		/* it is possible that hit + missed will overflow and be zero */
		if (!(hit + missed)) {
			pr_info("hit + missed overflowed and totalled zero!\n");
			hit--; /* make it non zero */
		}

		/* Caculate the average time in nanosecs */
		avg = NSEC_PER_MSEC / (hit + missed);
		pr_info("%ld ns per entry\n", avg);
	}
}

static void wait_to_die(void)
{
	set_current_state(TASK_INTERRUPTIBLE);
	while (!kthread_should_stop()) {
		schedule();
		set_current_state(TASK_INTERRUPTIBLE);
	}
	__set_current_state(TASK_RUNNING);
}

static int ring_buffer_consumer_thread(void *arg)
{
	while (!kthread_should_stop() && !kill_test) {
		complete(&read_start);

		ring_buffer_consumer();

		set_current_state(TASK_INTERRUPTIBLE);
		if (kthread_should_stop() || kill_test)
			break;

		schedule();
		__set_current_state(TASK_RUNNING);
	}
	__set_current_state(TASK_RUNNING);

	if (kill_test)
		wait_to_die();

	return 0;
}

static int ring_buffer_producer_thread(void *arg)
{
	init_completion(&read_start);

	while (!kthread_should_stop() && !kill_test) {
		ring_buffer_reset(buffer);

		if (consumer) {
			smp_wmb();
			wake_up_process(consumer);
			wait_for_completion(&read_start);
		}

		ring_buffer_producer();

		pr_info("Sleeping for 10 secs\n");
		set_current_state(TASK_INTERRUPTIBLE);
		schedule_timeout(HZ * SLEEP_TIME);
		__set_current_state(TASK_RUNNING);
	}

	if (kill_test)
		wait_to_die();

	return 0;
}

static int __init ring_buffer_benchmark_init(void)
{
	int ret;

	/* make a one meg buffer in overwite mode */
	buffer = ring_buffer_alloc(1000000, RB_FL_OVERWRITE);
	if (!buffer)
		return -ENOMEM;

	if (!disable_reader) {
		consumer = kthread_create(ring_buffer_consumer_thread,
					  NULL, "rb_consumer");
		ret = PTR_ERR(consumer);
		if (IS_ERR(consumer))
			goto out_fail;
	}

	producer = kthread_run(ring_buffer_producer_thread,
			       NULL, "rb_producer");
	ret = PTR_ERR(producer);

	if (IS_ERR(producer))
		goto out_kill;

	return 0;

 out_kill:
	if (consumer)
		kthread_stop(consumer);

 out_fail:
	ring_buffer_free(buffer);
	return ret;
}

static void __exit ring_buffer_benchmark_exit(void)
{
	kthread_stop(producer);
	if (consumer)
		kthread_stop(consumer);
	ring_buffer_free(buffer);
}

module_init(ring_buffer_benchmark_init);
module_exit(ring_buffer_benchmark_exit);

MODULE_AUTHOR("Steven Rostedt");
MODULE_DESCRIPTION("ring_buffer_benchmark");
MODULE_LICENSE("GPL");
