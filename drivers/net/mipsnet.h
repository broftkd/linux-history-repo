//
// <COPYRIGHT CLASS="1B" YEAR="2005">
// Unpublished work (c) MIPS Technologies, Inc.  All rights reserved.
// Unpublished rights reserved under the copyright laws of the U.S.A. and
//  other countries.
//
// PROPRIETARY / SECRET CONFIDENTIAL INFORMATION OF MIPS TECHNOLOGIES, INC.
// FOR INTERNAL USE ONLY.
//
// Under no circumstances (contract or otherwise) may this information be
// disclosed to, or copied, modified or used by anyone other than employees
// or contractors of MIPS Technologies having a need to know.
// </COPYRIGHT>
//
//++
// File: MIPS_Net.h
//
// Description:
//   The definition of the emulated MIPSNET device's interface.
//
// Notes: This include file needs to work from a Linux device drivers.
//
//--
//

#ifndef __MIPSNET_H
#define __MIPSNET_H

/*
 *  Id of this Net device, as seen by the core.
 */
#define MIPS_NET_DEV_ID ((uint64_t)           \
	                     ((uint64_t)'M'<< 0)| \
	                     ((uint64_t)'I'<< 8)| \
	                     ((uint64_t)'P'<<16)| \
	                     ((uint64_t)'S'<<24)| \
	                     ((uint64_t)'N'<<32)| \
	                     ((uint64_t)'E'<<40)| \
	                     ((uint64_t)'T'<<48)| \
	                     ((uint64_t)'0'<<56))

/*
 * Net status/control block as seen by sw in the core.
 * (Why not use bit fields? can't be bothered with cross-platform struct
 *  packing.)
 */
typedef struct _net_control_block {
	/// dev info for probing
	///  reads as MIPSNET%d where %d is some form of version
	uint64_t devId;		/*0x00 */

	/*
	 * read only busy flag.
	 * Set and cleared by the Net Device to indicate that an rx or a tx
	 * is in progress.
	 */
	uint32_t busy;		/*0x08 */

	/*
	 * Set by the Net Device.
	 * The device will set it once data has been received.
	 * The value is the number of bytes that should be read from
	 * rxDataBuffer.  The value will decrease till 0 until all the data
	 * from rxDataBuffer has been read.
	 */
	uint32_t rxDataCount;	/*0x0c */
#define MIPSNET_MAX_RXTX_DATACOUNT (1<<16)

	/*
	 * Settable from the MIPS core, cleared by the Net Device.
	 * The core should set the number of bytes it wants to send,
	 *   then it should write those bytes of data to txDataBuffer.
	 * The device will clear txDataCount has been processed (not necessarily sent).
	 */
	uint32_t txDataCount;	/*0x10 */

	/*
	 * Interrupt control
	 *
	 * Used to clear the interrupted generated by this dev.
	 * Write a 1 to clear the interrupt. (except bit31).
	 *
	 * Bit0 is set if it was a tx-done interrupt.
	 * Bit1 is set when new rx-data is available.
	 *      Until this bit is cleared there will be no other RXs.
	 *
	 * Bit31 is used for testing, it clears after a read.
	 *    Writing 1 to this bit will cause an interrupt to be generated.
	 *    To clear the test interrupt, write 0 to this register.
	 */
	uint32_t interruptControl;	/*0x14 */
#define MIPSNET_INTCTL_TXDONE     ((uint32_t)(1<< 0))
#define MIPSNET_INTCTL_RXDONE     ((uint32_t)(1<< 1))
#define MIPSNET_INTCTL_TESTBIT    ((uint32_t)(1<<31))
#define MIPSNET_INTCTL_ALLSOURCES (MIPSNET_INTCTL_TXDONE|MIPSNET_INTCTL_RXDONE|MIPSNET_INTCTL_TESTBIT)

	/*
	 * Readonly core-specific interrupt info for the device to signal the core.
	 * The meaning of the contents of this field might change.
	 */
	/*###\todo: the whole memIntf interrupt scheme is messy: the device should have
	 *  no control what so ever of what VPE/register set is being used.
	 *  The MemIntf should only expose interrupt lines, and something in the
	 *  config should be responsible for the line<->core/vpe bindings.
	 */
	uint32_t interruptInfo;	/*0x18 */

	/*
	 *  This is where the received data is read out.
	 *  There is more data to read until rxDataReady is 0.
	 *  Only 1 byte at this regs offset is used.
	 */
	uint32_t rxDataBuffer;	/*0x1c */

	/*
	 * This is where the data to transmit is written.
	 * Data should be written for the amount specified in the txDataCount register.
	 *  Only 1 byte at this regs offset is used.
	 */
	uint32_t txDataBuffer;	/*0x20 */
} MIPS_T_NetControl;

#define MIPSNET_IO_EXTENT 0x40	/* being generous */

#define field_offset(field) ((int)&((MIPS_T_NetControl*)(0))->field)

#endif /* __MIPSNET_H */
