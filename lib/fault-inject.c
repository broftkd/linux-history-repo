#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/random.h>
#include <linux/stat.h>
#include <linux/types.h>
#include <linux/fs.h>
#include <linux/module.h>
#include <linux/interrupt.h>
#include <linux/fault-inject.h>

/*
 * setup_fault_attr() is a helper function for various __setup handlers, so it
 * returns 0 on error, because that is what __setup handlers do.
 */
int __init setup_fault_attr(struct fault_attr *attr, char *str)
{
	unsigned long probability;
	unsigned long interval;
	int times;
	int space;

	/* "<interval>,<probability>,<space>,<times>" */
	if (sscanf(str, "%lu,%lu,%d,%d",
			&interval, &probability, &space, &times) < 4) {
		printk(KERN_WARNING
			"FAULT_INJECTION: failed to parse arguments\n");
		return 0;
	}

	attr->probability = probability;
	attr->interval = interval;
	atomic_set(&attr->times, times);
	atomic_set(&attr->space, space);

	return 1;
}

static void fail_dump(struct fault_attr *attr)
{
	if (attr->verbose > 0)
		printk(KERN_NOTICE "FAULT_INJECTION: forcing a failure\n");
	if (attr->verbose > 1)
		dump_stack();
}

#define atomic_dec_not_zero(v)		atomic_add_unless((v), -1, 0)

static int fail_task(struct fault_attr *attr, struct task_struct *task)
{
	return !in_interrupt() && task->make_it_fail;
}

/*
 * This code is stolen from failmalloc-1.0
 * http://www.nongnu.org/failmalloc/
 */

int should_fail(struct fault_attr *attr, ssize_t size)
{
	if (attr->task_filter && !fail_task(attr, current))
		return 0;

	if (atomic_read(&attr->times) == 0)
		return 0;

	if (atomic_read(&attr->space) > size) {
		atomic_sub(size, &attr->space);
		return 0;
	}

	if (attr->interval > 1) {
		attr->count++;
		if (attr->count % attr->interval)
			return 0;
	}

	if (attr->probability > random32() % 100)
		goto fail;

	return 0;

fail:
	fail_dump(attr);

	if (atomic_read(&attr->times) != -1)
		atomic_dec_not_zero(&attr->times);

	return 1;
}

#ifdef CONFIG_FAULT_INJECTION_DEBUG_FS

static void debugfs_ul_set(void *data, u64 val)
{
	*(unsigned long *)data = val;
}

static u64 debugfs_ul_get(void *data)
{
	return *(unsigned long *)data;
}

DEFINE_SIMPLE_ATTRIBUTE(fops_ul, debugfs_ul_get, debugfs_ul_set, "%llu\n");

static struct dentry *debugfs_create_ul(const char *name, mode_t mode,
				struct dentry *parent, unsigned long *value)
{
	return debugfs_create_file(name, mode, parent, value, &fops_ul);
}

static void debugfs_atomic_t_set(void *data, u64 val)
{
	atomic_set((atomic_t *)data, val);
}

static u64 debugfs_atomic_t_get(void *data)
{
	return atomic_read((atomic_t *)data);
}

DEFINE_SIMPLE_ATTRIBUTE(fops_atomic_t, debugfs_atomic_t_get,
			debugfs_atomic_t_set, "%lld\n");

static struct dentry *debugfs_create_atomic_t(const char *name, mode_t mode,
				struct dentry *parent, atomic_t *value)
{
	return debugfs_create_file(name, mode, parent, value, &fops_atomic_t);
}

void cleanup_fault_attr_dentries(struct fault_attr *attr)
{
	debugfs_remove(attr->dentries.probability_file);
	attr->dentries.probability_file = NULL;

	debugfs_remove(attr->dentries.interval_file);
	attr->dentries.interval_file = NULL;

	debugfs_remove(attr->dentries.times_file);
	attr->dentries.times_file = NULL;

	debugfs_remove(attr->dentries.space_file);
	attr->dentries.space_file = NULL;

	debugfs_remove(attr->dentries.verbose_file);
	attr->dentries.verbose_file = NULL;

	debugfs_remove(attr->dentries.task_filter_file);
	attr->dentries.task_filter_file = NULL;

	if (attr->dentries.dir)
		WARN_ON(!simple_empty(attr->dentries.dir));

	debugfs_remove(attr->dentries.dir);
	attr->dentries.dir = NULL;
}

int init_fault_attr_dentries(struct fault_attr *attr, const char *name)
{
	mode_t mode = S_IFREG | S_IRUSR | S_IWUSR;
	struct dentry *dir;

	memset(&attr->dentries, 0, sizeof(attr->dentries));

	dir = debugfs_create_dir(name, NULL);
	if (!dir)
		goto fail;
	attr->dentries.dir = dir;

	attr->dentries.probability_file =
		debugfs_create_ul("probability", mode, dir, &attr->probability);

	attr->dentries.interval_file =
		debugfs_create_ul("interval", mode, dir, &attr->interval);

	attr->dentries.times_file =
		debugfs_create_atomic_t("times", mode, dir, &attr->times);

	attr->dentries.space_file =
		debugfs_create_atomic_t("space", mode, dir, &attr->space);

	attr->dentries.verbose_file =
		debugfs_create_ul("verbose", mode, dir, &attr->verbose);

	attr->dentries.task_filter_file = debugfs_create_bool("task-filter",
						mode, dir, &attr->task_filter);

	if (!attr->dentries.probability_file || !attr->dentries.interval_file
	    || !attr->dentries.times_file || !attr->dentries.space_file
	    || !attr->dentries.verbose_file || !attr->dentries.task_filter_file)
		goto fail;

	return 0;
fail:
	cleanup_fault_attr_dentries(attr);
	return -ENOMEM;
}

#endif /* CONFIG_FAULT_INJECTION_DEBUG_FS */
