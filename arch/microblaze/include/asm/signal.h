/*
 * Copyright (C) 2006 Atmark Techno, Inc.
 *	Yasushi SHOJI <yashi@atmark-techno.com>
 *	Tetsuya OHKAWA <tetsuya@atmark-techno.com>
 *
 * This file is subject to the terms and conditions of the GNU General Public
 * License. See the file "COPYING" in the main directory of this archive
 * for more details.
 */

#ifndef _ASM_MICROBLAZE_SIGNAL_H
#define _ASM_MICROBLAZE_SIGNAL_H

#define SIGHUP		1
#define SIGINT		2
#define SIGQUIT		3
#define SIGILL		4
#define SIGTRAP		5
#define SIGABRT		6
#define SIGIOT		6
#define SIGBUS		7
#define SIGFPE		8
#define SIGKILL		9
#define SIGUSR1		10
#define SIGSEGV		11
#define SIGUSR2		12
#define SIGPIPE		13
#define SIGALRM		14
#define SIGTERM		15
#define SIGSTKFLT	16
#define SIGCHLD		17
#define SIGCONT		18
#define SIGSTOP		19
#define SIGTSTP		20
#define SIGTTIN		21
#define SIGTTOU		22
#define SIGURG		23
#define SIGXCPU		24
#define SIGXFSZ		25
#define SIGVTALRM	26
#define SIGPROF		27
#define SIGWINCH	28
#define SIGIO		29
#define SIGPOLL		SIGIO
/*
#define SIGLOST		29
*/
#define SIGPWR		30
#define SIGSYS		31
#define	SIGUNUSED	31

/* These should not be considered constants from userland. */
#define SIGRTMIN	32
#define SIGRTMAX	_NSIG

/*
 * SA_FLAGS values:
 *
 * SA_ONSTACK indicates that a registered stack_t will be used.
 * SA_RESTART flag to get restarting signals (which were the default long ago)
 * SA_NOCLDSTOP flag to turn off SIGCHLD when children stop.
 * SA_RESETHAND clears the handler when the signal is delivered.
 * SA_NOCLDWAIT flag on SIGCHLD to inhibit zombies.
 * SA_NODEFER prevents the current signal from being masked in the handler.
 *
 * SA_ONESHOT and SA_NOMASK are the historical Linux names for the Single
 * Unix names RESETHAND and NODEFER respectively.
 */
#define SA_NOCLDSTOP	0x00000001
#define SA_NOCLDWAIT	0x00000002
#define SA_SIGINFO	0x00000004
#define SA_ONSTACK	0x08000000
#define SA_RESTART	0x10000000
#define SA_NODEFER	0x40000000
#define SA_RESETHAND	0x80000000

#define SA_NOMASK	SA_NODEFER
#define SA_ONESHOT	SA_RESETHAND

#define SA_RESTORER	0x04000000

/*
 * sigaltstack controls
 */
#define SS_ONSTACK	1
#define SS_DISABLE	2

#define MINSIGSTKSZ	2048
#define SIGSTKSZ	8192

# ifndef __ASSEMBLY__
# include <linux/types.h>
# include <asm-generic/signal-defs.h>

/* Avoid too many header ordering problems. */
struct siginfo;

#  ifdef __KERNEL__
/*
 * Most things should be clean enough to redefine this at will, if care
 * is taken to make libc match.
 */
#  define _NSIG		64
#  define _NSIG_BPW	32
#  define _NSIG_WORDS	(_NSIG / _NSIG_BPW)

typedef unsigned long old_sigset_t; /* at least 32 bits */

typedef struct {
	unsigned long sig[_NSIG_WORDS];
} sigset_t;

struct old_sigaction {
	__sighandler_t sa_handler;
	old_sigset_t sa_mask;
	unsigned long sa_flags;
	void (*sa_restorer)(void);
};

struct sigaction {
	__sighandler_t sa_handler;
	unsigned long sa_flags;
	void (*sa_restorer)(void);
	sigset_t sa_mask; /* mask last for extensibility */
};

struct k_sigaction {
	struct sigaction sa;
};

#  include <asm/sigcontext.h>
#  undef __HAVE_ARCH_SIG_BITOPS

#  define ptrace_signal_deliver(regs, cookie) do { } while (0)

#  else /* !__KERNEL__ */

/* Here we must cater to libcs that poke about in kernel headers. */

#  define NSIG		32
typedef unsigned long sigset_t;

struct sigaction {
	union {
	__sighandler_t _sa_handler;
	void (*_sa_sigaction)(int, struct siginfo *, void *);
	} _u;
	sigset_t sa_mask;
	unsigned long sa_flags;
	void (*sa_restorer)(void);
};

#  define sa_handler	_u._sa_handler
#  define sa_sigaction	_u._sa_sigaction

#  endif /* __KERNEL__ */

typedef struct sigaltstack {
	void *ss_sp;
	int ss_flags;
	size_t ss_size;
} stack_t;

# endif /* __ASSEMBLY__ */
#endif /* _ASM_MICROBLAZE_SIGNAL_H */
