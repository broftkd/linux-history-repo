/*
 * Copyright (C) 2005 - 2008 ServerEngines
 * All rights reserved.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License version 2
 * as published by the Free Software Foundation.  The full GNU General
 * Public License is included in this distribution in the file called COPYING.
 *
 * Contact Information:
 * linux-drivers@serverengines.com
 *
 * ServerEngines
 * 209 N. Fair Oaks Ave
 * Sunnyvale, CA 94085
 */
/*
 * Autogenerated by srcgen version: 0127
 */
#ifndef __doorbells_amap_h__
#define __doorbells_amap_h__

/* The TX/RDMA send queue doorbell. */
struct BE_SQ_DB_AMAP {
	u8 cid[11];		/* DWORD 0 */
	u8 rsvd0[5];	/* DWORD 0 */
	u8 numPosted[14];	/* DWORD 0 */
	u8 rsvd1[2];	/* DWORD 0 */
} __packed;
struct SQ_DB_AMAP {
	u32 dw[1];
};

/* The receive queue doorbell. */
struct BE_RQ_DB_AMAP {
	u8 rq[10];		/* DWORD 0 */
	u8 rsvd0[13];	/* DWORD 0 */
	u8 Invalidate;	/* DWORD 0 */
	u8 numPosted[8];	/* DWORD 0 */
} __packed;
struct RQ_DB_AMAP {
	u32 dw[1];
};

/*
 * The CQ/EQ doorbell. Software MUST set reserved fields in this
 * descriptor to zero, otherwise (CEV) hardware will not execute the
 * doorbell (flagging a bad_db_qid error instead).
 */
struct BE_CQ_DB_AMAP {
	u8 qid[10];		/* DWORD 0 */
	u8 rsvd0[4];	/* DWORD 0 */
	u8 rearm;		/* DWORD 0 */
	u8 event;		/* DWORD 0 */
	u8 num_popped[13];	/* DWORD 0 */
	u8 rsvd1[3];	/* DWORD 0 */
} __packed;
struct CQ_DB_AMAP {
	u32 dw[1];
};

struct BE_TPM_RQ_DB_AMAP {
	u8 qid[10];		/* DWORD 0 */
	u8 rsvd0[6];	/* DWORD 0 */
	u8 numPosted[11];	/* DWORD 0 */
	u8 mss_cnt[5];	/* DWORD 0 */
} __packed;
struct TPM_RQ_DB_AMAP {
	u32 dw[1];
};

/*
 * Post WRB Queue Doorbell Register used by the host Storage stack
 * to notify the controller of a posted Work Request Block
 */
struct BE_WRB_POST_DB_AMAP {
	u8 wrb_cid[10];	/* DWORD 0 */
	u8 rsvd0[6];	/* DWORD 0 */
	u8 wrb_index[8];	/* DWORD 0 */
	u8 numberPosted[8];	/* DWORD 0 */
} __packed;
struct WRB_POST_DB_AMAP {
	u32 dw[1];
};

/*
 * Update Default PDU Queue Doorbell Register used to communicate
 * to the controller that the driver has stopped processing the queue
 * and where in the queue it stopped, this is
 * a CQ Entry Type. Used by storage driver.
 */
struct BE_DEFAULT_PDU_DB_AMAP {
	u8 qid[10];		/* DWORD 0 */
	u8 rsvd0[4];	/* DWORD 0 */
	u8 rearm;		/* DWORD 0 */
	u8 event;		/* DWORD 0 */
	u8 cqproc[14];	/* DWORD 0 */
	u8 rsvd1[2];	/* DWORD 0 */
} __packed;
struct DEFAULT_PDU_DB_AMAP {
	u32 dw[1];
};

/* Management Command and Controller default fragment ring */
struct BE_MCC_DB_AMAP {
	u8 rid[11];		/* DWORD 0 */
	u8 rsvd0[5];	/* DWORD 0 */
	u8 numPosted[14];	/* DWORD 0 */
	u8 rsvd1[2];	/* DWORD 0 */
} __packed;
struct MCC_DB_AMAP {
	u32 dw[1];
};

/*
 * Used for bootstrapping the Host interface. This register is
 * used for driver communication with the MPU when no MCC Rings exist.
 * The software must write this register twice to post any MCC
 * command. First, it writes the register with hi=1 and the upper bits of
 * the  physical address for the MCC_MAILBOX structure.  Software must poll
 * the ready bit until this is acknowledged.  Then, sotware writes the
 * register with hi=0 with the lower bits in the address.  It must
 * poll the ready bit until the MCC command is complete.  Upon completion,
 * the MCC_MAILBOX will contain a valid completion queue  entry.
 */
struct BE_MPU_MAILBOX_DB_AMAP {
	u8 ready;		/* DWORD 0 */
	u8 hi;		/* DWORD 0 */
	u8 address[30];	/* DWORD 0 */
} __packed;
struct MPU_MAILBOX_DB_AMAP {
	u32 dw[1];
};

/*
 *  This is the protection domain doorbell register map. Note that
 *  while this map shows doorbells for all Blade Engine supported
 *  protocols, not all of these may be valid in a given function or
 *  protection domain. It is the responsibility of the application
 *  accessing the doorbells to know which are valid. Each doorbell
 *  occupies 32 bytes of space, but unless otherwise specified,
 *  only the first 4 bytes should be written.  There are 32 instances
 *  of these doorbells for the host and 31 virtual machines respectively.
 *  The host and VMs will only map the doorbell pages belonging to its
 *  protection domain. It will not be able to touch the doorbells for
 *  another VM. The doorbells are the only registers directly accessible
 *  by a virtual machine. Similarly, there are 511 additional
 *  doorbells for RDMA protection domains. PD 0 for RDMA shares
 *  the same physical protection domain doorbell page as ETH/iSCSI.
 *
 */
struct BE_PROTECTION_DOMAIN_DBMAP_AMAP {
	u8 rsvd0[512];	/* DWORD 0 */
	struct BE_SQ_DB_AMAP rdma_sq_db;
	u8 rsvd1[7][32];	/* DWORD 17 */
	struct BE_WRB_POST_DB_AMAP iscsi_wrb_post_db;
	u8 rsvd2[7][32];	/* DWORD 25 */
	struct BE_SQ_DB_AMAP etx_sq_db;
	u8 rsvd3[7][32];	/* DWORD 33 */
	struct BE_RQ_DB_AMAP rdma_rq_db;
	u8 rsvd4[7][32];	/* DWORD 41 */
	struct BE_DEFAULT_PDU_DB_AMAP iscsi_default_pdu_db;
	u8 rsvd5[7][32];	/* DWORD 49 */
	struct BE_TPM_RQ_DB_AMAP tpm_rq_db;
	u8 rsvd6[7][32];	/* DWORD 57 */
	struct BE_RQ_DB_AMAP erx_rq_db;
	u8 rsvd7[7][32];	/* DWORD 65 */
	struct BE_CQ_DB_AMAP cq_db;
	u8 rsvd8[7][32];	/* DWORD 73 */
	struct BE_MCC_DB_AMAP mpu_mcc_db;
	u8 rsvd9[7][32];	/* DWORD 81 */
	struct BE_MPU_MAILBOX_DB_AMAP mcc_bootstrap_db;
	u8 rsvd10[935][32];	/* DWORD 89 */
} __packed;
struct PROTECTION_DOMAIN_DBMAP_AMAP {
	u32 dw[1024];
};

#endif /* __doorbells_amap_h__ */
