#ifndef _ASM_GENERIC_BUG_H
#define _ASM_GENERIC_BUG_H

#include <linux/compiler.h>

#ifdef CONFIG_BUG
#ifndef HAVE_ARCH_BUG
#define BUG() do { \
	printk("BUG: failure at %s:%d/%s()!\n", __FILE__, __LINE__, __FUNCTION__); \
	panic("BUG!"); \
} while (0)
#endif

#ifndef HAVE_ARCH_BUG_ON
#define BUG_ON(condition) do { if (unlikely((condition)!=0)) BUG(); } while(0)
#endif

#ifndef HAVE_ARCH_WARN_ON
#define WARN_ON(condition) ({						\
	typeof(condition) __ret_warn_on = (condition);			\
	if (unlikely(__ret_warn_on)) {					\
		printk("BUG: warning at %s:%d/%s()\n", __FILE__,	\
			__LINE__, __FUNCTION__);			\
		dump_stack();						\
	}								\
	unlikely(__ret_warn_on);					\
})
#endif

#else /* !CONFIG_BUG */
#ifndef HAVE_ARCH_BUG
#define BUG()
#endif

#ifndef HAVE_ARCH_BUG_ON
#define BUG_ON(condition) do { if (condition) ; } while(0)
#endif

#ifndef HAVE_ARCH_WARN_ON
#define WARN_ON(condition) unlikely((condition))
#endif
#endif

#define WARN_ON_ONCE(condition)	({			\
	static int __warn_once = 1;			\
	typeof(condition) __ret_warn_once = (condition);\
							\
	if (likely(__warn_once))			\
		if (WARN_ON(__ret_warn_once)) 		\
			__warn_once = 0;		\
	unlikely(__ret_warn_once);			\
})

#ifdef CONFIG_SMP
# define WARN_ON_SMP(x)			WARN_ON(x)
#else
# define WARN_ON_SMP(x)			do { } while (0)
#endif

#endif
