/**
 * @file op_x86_model.h
 * interface to x86 model-specific MSR operations
 *
 * @remark Copyright 2002 OProfile authors
 * @remark Read the file COPYING
 *
 * @author Graydon Hoare
 */

#ifndef OP_X86_MODEL_H
#define OP_X86_MODEL_H

#define CTR_IS_RESERVED(msrs, c) (msrs->counters[(c)].addr ? 1 : 0)
#define CTRL_IS_RESERVED(msrs, c) (msrs->controls[(c)].addr ? 1 : 0)
#define CTRL_SET_ACTIVE(n) (n |= (1<<22))
#define CTRL_SET_ENABLE(val) (val |= 1<<20)
#define CTRL_SET_INACTIVE(n) (n &= ~(1<<22))
#define CTRL_SET_KERN(val, k) (val |= ((k & 1) << 17))
#define CTRL_SET_UM(val, m) (val |= (m << 8))
#define CTRL_SET_USR(val, u) (val |= ((u & 1) << 16))

struct op_saved_msr {
	unsigned int high;
	unsigned int low;
};

struct op_msr {
	unsigned long addr;
	struct op_saved_msr saved;
};

struct op_msrs {
	struct op_msr *counters;
	struct op_msr *controls;
};

struct pt_regs;

/* The model vtable abstracts the differences between
 * various x86 CPU models' perfctr support.
 */
struct op_x86_model_spec {
	unsigned int	num_counters;
	unsigned int	num_controls;
	int		(*init)(struct oprofile_operations *ops);
	void		(*exit)(void);
	void		(*fill_in_addresses)(struct op_msrs * const msrs);
	void		(*setup_ctrs)(struct op_msrs const * const msrs);
	int		(*check_ctrs)(struct pt_regs * const regs,
				      struct op_msrs const * const msrs);
	void		(*start)(struct op_msrs const * const msrs);
	void		(*stop)(struct op_msrs const * const msrs);
	void		(*shutdown)(struct op_msrs const * const msrs);
};

extern struct op_x86_model_spec const op_ppro_spec;
extern struct op_x86_model_spec const op_p4_spec;
extern struct op_x86_model_spec const op_p4_ht2_spec;
extern struct op_x86_model_spec const op_amd_spec;
extern struct op_x86_model_spec op_arch_perfmon_spec;

#endif /* OP_X86_MODEL_H */
