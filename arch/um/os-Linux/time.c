/*
 * Copyright (C) 2000 - 2007 Jeff Dike (jdike{addtoit,linux.intel}.com)
 * Licensed under the GPL
 */

#include <stddef.h>
#include <errno.h>
#include <signal.h>
#include <time.h>
#include <sys/time.h>
#include "kern_constants.h"
#include "os.h"
#include "user.h"

int set_interval(void)
{
	int usec = 1000000/UM_HZ;
	struct itimerval interval = ((struct itimerval) { { 0, usec },
							  { 0, usec } });

	if (setitimer(ITIMER_VIRTUAL, &interval, NULL) == -1)
		return -errno;

	return 0;
}

int timer_one_shot(int ticks)
{
	unsigned long usec = ticks * 1000000 / UM_HZ;
	unsigned long sec = usec / 1000000;
	struct itimerval interval;

	usec %= 1000000;
	interval = ((struct itimerval) { { 0, 0 }, { sec, usec } });

	if (setitimer(ITIMER_VIRTUAL, &interval, NULL) == -1)
		return -errno;

	return 0;
}

unsigned long long disable_timer(void)
{
	struct itimerval time = ((struct itimerval) { { 0, 0 }, { 0, 0 } });

	if(setitimer(ITIMER_VIRTUAL, &time, &time) < 0)
		printk(UM_KERN_ERR "disable_timer - setitimer failed, "
		       "errno = %d\n", errno);

	return tv_to_nsec(&time.it_value);
}

unsigned long long os_nsecs(void)
{
	struct timeval tv;

	gettimeofday(&tv, NULL);
	return timeval_to_ns(&tv);
}

extern void alarm_handler(int sig, struct sigcontext *sc);

void idle_sleep(unsigned long long nsecs)
{
	struct timespec ts = { .tv_sec	= nsecs / BILLION,
			       .tv_nsec = nsecs % BILLION };

	if (nanosleep(&ts, &ts) == 0)
		alarm_handler(SIGVTALRM, NULL);
}
