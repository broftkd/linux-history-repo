/*
	Copyright (C) 2004 - 2009 rt2x00 SourceForge Project
	<http://rt2x00.serialmonkey.com>

	This program is free software; you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation; either version 2 of the License, or
	(at your option) any later version.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with this program; if not, write to the
	Free Software Foundation, Inc.,
	59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

/*
	Module: rt2800usb
	Abstract: Data structures and registers for the rt2800usb module.
	Supported chipsets: RT2800U.
 */

#ifndef RT2800USB_H
#define RT2800USB_H

struct rt2800_ops {
	void (*register_read)(struct rt2x00_dev *rt2x00dev,
			      const unsigned int offset, u32 *value);
	void (*register_write)(struct rt2x00_dev *rt2x00dev,
			       const unsigned int offset, u32 value);
	void (*register_write_lock)(struct rt2x00_dev *rt2x00dev,
				    const unsigned int offset, u32 value);

	void (*register_multiread)(struct rt2x00_dev *rt2x00dev,
				   const unsigned int offset,
				   void *value, const u32 length);
	void (*register_multiwrite)(struct rt2x00_dev *rt2x00dev,
				    const unsigned int offset,
				    const void *value, const u32 length);

	int (*regbusy_read)(struct rt2x00_dev *rt2x00dev,
			    const unsigned int offset,
			    const struct rt2x00_field32 field, u32 *reg);
};

static inline void rt2800_register_read(struct rt2x00_dev *rt2x00dev,
					const unsigned int offset,
					u32 *value)
{
	const struct rt2800_ops *rt2800ops = rt2x00dev->priv;

	rt2800ops->register_read(rt2x00dev, offset, value);
}

static inline void rt2800_register_write(struct rt2x00_dev *rt2x00dev,
					 const unsigned int offset,
					 u32 value)
{
	const struct rt2800_ops *rt2800ops = rt2x00dev->priv;

	rt2800ops->register_write(rt2x00dev, offset, value);
}

static inline void rt2800_register_write_lock(struct rt2x00_dev *rt2x00dev,
					      const unsigned int offset,
					      u32 value)
{
	const struct rt2800_ops *rt2800ops = rt2x00dev->priv;

	rt2800ops->register_write_lock(rt2x00dev, offset, value);
}

static inline void rt2800_register_multiread(struct rt2x00_dev *rt2x00dev,
					     const unsigned int offset,
					     void *value, const u32 length)
{
	const struct rt2800_ops *rt2800ops = rt2x00dev->priv;

	rt2800ops->register_multiread(rt2x00dev, offset, value, length);
}

static inline void rt2800_register_multiwrite(struct rt2x00_dev *rt2x00dev,
					      const unsigned int offset,
					      void *value, const u32 length)
{
	const struct rt2800_ops *rt2800ops = rt2x00dev->priv;

	rt2800ops->register_multiwrite(rt2x00dev, offset, value, length);
}

static inline int rt2800_regbusy_read(struct rt2x00_dev *rt2x00dev,
				      const unsigned int offset,
				      struct rt2x00_field32 field,
				      u32 *reg)
{
	const struct rt2800_ops *rt2800ops = rt2x00dev->priv;

	return rt2800ops->regbusy_read(rt2x00dev, offset, field, reg);
}

/*
 * RF chip defines.
 *
 * RF2820 2.4G 2T3R
 * RF2850 2.4G/5G 2T3R
 * RF2720 2.4G 1T2R
 * RF2750 2.4G/5G 1T2R
 * RF3020 2.4G 1T1R
 * RF2020 2.4G B/G
 * RF3021 2.4G 1T2R
 * RF3022 2.4G 2T2R
 * RF3052 2.4G 2T2R
 */
#define RF2820				0x0001
#define RF2850				0x0002
#define RF2720				0x0003
#define RF2750				0x0004
#define RF3020				0x0005
#define RF2020				0x0006
#define RF3021				0x0007
#define RF3022				0x0008
#define RF3052				0x0009

/*
 * RT2870 version
 */
#define RT2860C_VERSION			0x28600100
#define RT2860D_VERSION			0x28600101
#define RT2880E_VERSION			0x28720200
#define RT2883_VERSION			0x28830300
#define RT3070_VERSION			0x30700200

/*
 * Signal information.
 * Default offset is required for RSSI <-> dBm conversion.
 */
#define DEFAULT_RSSI_OFFSET		120 /* FIXME */

/*
 * Register layout information.
 */
#define CSR_REG_BASE			0x1000
#define CSR_REG_SIZE			0x0800
#define EEPROM_BASE			0x0000
#define EEPROM_SIZE			0x0110
#define BBP_BASE			0x0000
#define BBP_SIZE			0x0080
#define RF_BASE				0x0004
#define RF_SIZE				0x0010

/*
 * Number of TX queues.
 */
#define NUM_TX_QUEUES			4

/*
 * USB registers.
 */

/*
 * INT_SOURCE_CSR: Interrupt source register.
 * Write one to clear corresponding bit.
 * TX_FIFO_STATUS: FIFO Statistics is full, sw should read 0x171c
 */
#define INT_SOURCE_CSR			0x0200
#define INT_SOURCE_CSR_RXDELAYINT	FIELD32(0x00000001)
#define INT_SOURCE_CSR_TXDELAYINT	FIELD32(0x00000002)
#define INT_SOURCE_CSR_RX_DONE		FIELD32(0x00000004)
#define INT_SOURCE_CSR_AC0_DMA_DONE	FIELD32(0x00000008)
#define INT_SOURCE_CSR_AC1_DMA_DONE	FIELD32(0x00000010)
#define INT_SOURCE_CSR_AC2_DMA_DONE	FIELD32(0x00000020)
#define INT_SOURCE_CSR_AC3_DMA_DONE	FIELD32(0x00000040)
#define INT_SOURCE_CSR_HCCA_DMA_DONE	FIELD32(0x00000080)
#define INT_SOURCE_CSR_MGMT_DMA_DONE	FIELD32(0x00000100)
#define INT_SOURCE_CSR_MCU_COMMAND	FIELD32(0x00000200)
#define INT_SOURCE_CSR_RXTX_COHERENT	FIELD32(0x00000400)
#define INT_SOURCE_CSR_TBTT		FIELD32(0x00000800)
#define INT_SOURCE_CSR_PRE_TBTT		FIELD32(0x00001000)
#define INT_SOURCE_CSR_TX_FIFO_STATUS	FIELD32(0x00002000)
#define INT_SOURCE_CSR_AUTO_WAKEUP	FIELD32(0x00004000)
#define INT_SOURCE_CSR_GPTIMER		FIELD32(0x00008000)
#define INT_SOURCE_CSR_RX_COHERENT	FIELD32(0x00010000)
#define INT_SOURCE_CSR_TX_COHERENT	FIELD32(0x00020000)

/*
 * INT_MASK_CSR: Interrupt MASK register. 1: the interrupt is mask OFF.
 */
#define INT_MASK_CSR			0x0204
#define INT_MASK_CSR_RXDELAYINT		FIELD32(0x00000001)
#define INT_MASK_CSR_TXDELAYINT		FIELD32(0x00000002)
#define INT_MASK_CSR_RX_DONE		FIELD32(0x00000004)
#define INT_MASK_CSR_AC0_DMA_DONE	FIELD32(0x00000008)
#define INT_MASK_CSR_AC1_DMA_DONE	FIELD32(0x00000010)
#define INT_MASK_CSR_AC2_DMA_DONE	FIELD32(0x00000020)
#define INT_MASK_CSR_AC3_DMA_DONE	FIELD32(0x00000040)
#define INT_MASK_CSR_HCCA_DMA_DONE	FIELD32(0x00000080)
#define INT_MASK_CSR_MGMT_DMA_DONE	FIELD32(0x00000100)
#define INT_MASK_CSR_MCU_COMMAND	FIELD32(0x00000200)
#define INT_MASK_CSR_RXTX_COHERENT	FIELD32(0x00000400)
#define INT_MASK_CSR_TBTT		FIELD32(0x00000800)
#define INT_MASK_CSR_PRE_TBTT		FIELD32(0x00001000)
#define INT_MASK_CSR_TX_FIFO_STATUS	FIELD32(0x00002000)
#define INT_MASK_CSR_AUTO_WAKEUP	FIELD32(0x00004000)
#define INT_MASK_CSR_GPTIMER		FIELD32(0x00008000)
#define INT_MASK_CSR_RX_COHERENT	FIELD32(0x00010000)
#define INT_MASK_CSR_TX_COHERENT	FIELD32(0x00020000)

/*
 * WPDMA_GLO_CFG
 */
#define WPDMA_GLO_CFG 			0x0208
#define WPDMA_GLO_CFG_ENABLE_TX_DMA	FIELD32(0x00000001)
#define WPDMA_GLO_CFG_TX_DMA_BUSY    	FIELD32(0x00000002)
#define WPDMA_GLO_CFG_ENABLE_RX_DMA	FIELD32(0x00000004)
#define WPDMA_GLO_CFG_RX_DMA_BUSY	FIELD32(0x00000008)
#define WPDMA_GLO_CFG_WP_DMA_BURST_SIZE	FIELD32(0x00000030)
#define WPDMA_GLO_CFG_TX_WRITEBACK_DONE	FIELD32(0x00000040)
#define WPDMA_GLO_CFG_BIG_ENDIAN	FIELD32(0x00000080)
#define WPDMA_GLO_CFG_RX_HDR_SCATTER	FIELD32(0x0000ff00)
#define WPDMA_GLO_CFG_HDR_SEG_LEN	FIELD32(0xffff0000)

/*
 * WPDMA_RST_IDX
 */
#define WPDMA_RST_IDX 			0x020c
#define WPDMA_RST_IDX_DTX_IDX0		FIELD32(0x00000001)
#define WPDMA_RST_IDX_DTX_IDX1		FIELD32(0x00000002)
#define WPDMA_RST_IDX_DTX_IDX2		FIELD32(0x00000004)
#define WPDMA_RST_IDX_DTX_IDX3		FIELD32(0x00000008)
#define WPDMA_RST_IDX_DTX_IDX4		FIELD32(0x00000010)
#define WPDMA_RST_IDX_DTX_IDX5		FIELD32(0x00000020)
#define WPDMA_RST_IDX_DRX_IDX0		FIELD32(0x00010000)

/*
 * DELAY_INT_CFG
 */
#define DELAY_INT_CFG			0x0210
#define DELAY_INT_CFG_RXMAX_PTIME	FIELD32(0x000000ff)
#define DELAY_INT_CFG_RXMAX_PINT	FIELD32(0x00007f00)
#define DELAY_INT_CFG_RXDLY_INT_EN	FIELD32(0x00008000)
#define DELAY_INT_CFG_TXMAX_PTIME	FIELD32(0x00ff0000)
#define DELAY_INT_CFG_TXMAX_PINT	FIELD32(0x7f000000)
#define DELAY_INT_CFG_TXDLY_INT_EN	FIELD32(0x80000000)

/*
 * WMM_AIFSN_CFG: Aifsn for each EDCA AC
 * AIFSN0: AC_BE
 * AIFSN1: AC_BK
 * AIFSN1: AC_VI
 * AIFSN1: AC_VO
 */
#define WMM_AIFSN_CFG			0x0214
#define WMM_AIFSN_CFG_AIFSN0		FIELD32(0x0000000f)
#define WMM_AIFSN_CFG_AIFSN1		FIELD32(0x000000f0)
#define WMM_AIFSN_CFG_AIFSN2		FIELD32(0x00000f00)
#define WMM_AIFSN_CFG_AIFSN3		FIELD32(0x0000f000)

/*
 * WMM_CWMIN_CSR: CWmin for each EDCA AC
 * CWMIN0: AC_BE
 * CWMIN1: AC_BK
 * CWMIN1: AC_VI
 * CWMIN1: AC_VO
 */
#define WMM_CWMIN_CFG			0x0218
#define WMM_CWMIN_CFG_CWMIN0		FIELD32(0x0000000f)
#define WMM_CWMIN_CFG_CWMIN1		FIELD32(0x000000f0)
#define WMM_CWMIN_CFG_CWMIN2		FIELD32(0x00000f00)
#define WMM_CWMIN_CFG_CWMIN3		FIELD32(0x0000f000)

/*
 * WMM_CWMAX_CSR: CWmax for each EDCA AC
 * CWMAX0: AC_BE
 * CWMAX1: AC_BK
 * CWMAX1: AC_VI
 * CWMAX1: AC_VO
 */
#define WMM_CWMAX_CFG			0x021c
#define WMM_CWMAX_CFG_CWMAX0		FIELD32(0x0000000f)
#define WMM_CWMAX_CFG_CWMAX1		FIELD32(0x000000f0)
#define WMM_CWMAX_CFG_CWMAX2		FIELD32(0x00000f00)
#define WMM_CWMAX_CFG_CWMAX3		FIELD32(0x0000f000)

/*
 * AC_TXOP0: AC_BK/AC_BE TXOP register
 * AC0TXOP: AC_BK in unit of 32us
 * AC1TXOP: AC_BE in unit of 32us
 */
#define WMM_TXOP0_CFG			0x0220
#define WMM_TXOP0_CFG_AC0TXOP		FIELD32(0x0000ffff)
#define WMM_TXOP0_CFG_AC1TXOP		FIELD32(0xffff0000)

/*
 * AC_TXOP1: AC_VO/AC_VI TXOP register
 * AC2TXOP: AC_VI in unit of 32us
 * AC3TXOP: AC_VO in unit of 32us
 */
#define WMM_TXOP1_CFG			0x0224
#define WMM_TXOP1_CFG_AC2TXOP		FIELD32(0x0000ffff)
#define WMM_TXOP1_CFG_AC3TXOP		FIELD32(0xffff0000)

/*
 * GPIO_CTRL_CFG:
 */
#define GPIO_CTRL_CFG			0x0228
#define GPIO_CTRL_CFG_BIT0		FIELD32(0x00000001)
#define GPIO_CTRL_CFG_BIT1		FIELD32(0x00000002)
#define GPIO_CTRL_CFG_BIT2		FIELD32(0x00000004)
#define GPIO_CTRL_CFG_BIT3		FIELD32(0x00000008)
#define GPIO_CTRL_CFG_BIT4		FIELD32(0x00000010)
#define GPIO_CTRL_CFG_BIT5		FIELD32(0x00000020)
#define GPIO_CTRL_CFG_BIT6		FIELD32(0x00000040)
#define GPIO_CTRL_CFG_BIT7		FIELD32(0x00000080)
#define GPIO_CTRL_CFG_BIT8		FIELD32(0x00000100)

/*
 * MCU_CMD_CFG
 */
#define MCU_CMD_CFG			0x022c

/*
 * AC_BK register offsets
 */
#define TX_BASE_PTR0			0x0230
#define TX_MAX_CNT0			0x0234
#define TX_CTX_IDX0			0x0238
#define TX_DTX_IDX0			0x023c

/*
 * AC_BE register offsets
 */
#define TX_BASE_PTR1			0x0240
#define TX_MAX_CNT1			0x0244
#define TX_CTX_IDX1			0x0248
#define TX_DTX_IDX1			0x024c

/*
 * AC_VI register offsets
 */
#define TX_BASE_PTR2			0x0250
#define TX_MAX_CNT2			0x0254
#define TX_CTX_IDX2			0x0258
#define TX_DTX_IDX2			0x025c

/*
 * AC_VO register offsets
 */
#define TX_BASE_PTR3			0x0260
#define TX_MAX_CNT3			0x0264
#define TX_CTX_IDX3			0x0268
#define TX_DTX_IDX3			0x026c

/*
 * HCCA register offsets
 */
#define TX_BASE_PTR4			0x0270
#define TX_MAX_CNT4			0x0274
#define TX_CTX_IDX4			0x0278
#define TX_DTX_IDX4			0x027c

/*
 * MGMT register offsets
 */
#define TX_BASE_PTR5			0x0280
#define TX_MAX_CNT5			0x0284
#define TX_CTX_IDX5			0x0288
#define TX_DTX_IDX5			0x028c

/*
 * RX register offsets
 */
#define RX_BASE_PTR			0x0290
#define RX_MAX_CNT			0x0294
#define RX_CRX_IDX			0x0298
#define RX_DRX_IDX			0x029c

/*
 * USB_DMA_CFG
 * RX_BULK_AGG_TIMEOUT: Rx Bulk Aggregation TimeOut in unit of 33ns.
 * RX_BULK_AGG_LIMIT: Rx Bulk Aggregation Limit in unit of 256 bytes.
 * PHY_CLEAR: phy watch dog enable.
 * TX_CLEAR: Clear USB DMA TX path.
 * TXOP_HALT: Halt TXOP count down when TX buffer is full.
 * RX_BULK_AGG_EN: Enable Rx Bulk Aggregation.
 * RX_BULK_EN: Enable USB DMA Rx.
 * TX_BULK_EN: Enable USB DMA Tx.
 * EP_OUT_VALID: OUT endpoint data valid.
 * RX_BUSY: USB DMA RX FSM busy.
 * TX_BUSY: USB DMA TX FSM busy.
 */
#define USB_DMA_CFG			0x02a0
#define USB_DMA_CFG_RX_BULK_AGG_TIMEOUT	FIELD32(0x000000ff)
#define USB_DMA_CFG_RX_BULK_AGG_LIMIT	FIELD32(0x0000ff00)
#define USB_DMA_CFG_PHY_CLEAR		FIELD32(0x00010000)
#define USB_DMA_CFG_TX_CLEAR		FIELD32(0x00080000)
#define USB_DMA_CFG_TXOP_HALT		FIELD32(0x00100000)
#define USB_DMA_CFG_RX_BULK_AGG_EN	FIELD32(0x00200000)
#define USB_DMA_CFG_RX_BULK_EN		FIELD32(0x00400000)
#define USB_DMA_CFG_TX_BULK_EN		FIELD32(0x00800000)
#define USB_DMA_CFG_EP_OUT_VALID	FIELD32(0x3f000000)
#define USB_DMA_CFG_RX_BUSY		FIELD32(0x40000000)
#define USB_DMA_CFG_TX_BUSY		FIELD32(0x80000000)

/*
 * USB_CYC_CFG
 */
#define USB_CYC_CFG			0x02a4
#define USB_CYC_CFG_CLOCK_CYCLE		FIELD32(0x000000ff)

/*
 * PBF_SYS_CTRL
 * HOST_RAM_WRITE: enable Host program ram write selection
 */
#define PBF_SYS_CTRL			0x0400
#define PBF_SYS_CTRL_READY		FIELD32(0x00000080)
#define PBF_SYS_CTRL_HOST_RAM_WRITE	FIELD32(0x00010000)

/*
 * HOST-MCU shared memory
 */
#define HOST_CMD_CSR			0x0404
#define HOST_CMD_CSR_HOST_COMMAND	FIELD32(0x000000ff)

/*
 * PBF registers
 * Most are for debug. Driver doesn't touch PBF register.
 */
#define PBF_CFG				0x0408
#define PBF_MAX_PCNT			0x040c
#define PBF_CTRL			0x0410
#define PBF_INT_STA			0x0414
#define PBF_INT_ENA			0x0418

/*
 * BCN_OFFSET0:
 */
#define BCN_OFFSET0			0x042c
#define BCN_OFFSET0_BCN0		FIELD32(0x000000ff)
#define BCN_OFFSET0_BCN1		FIELD32(0x0000ff00)
#define BCN_OFFSET0_BCN2		FIELD32(0x00ff0000)
#define BCN_OFFSET0_BCN3		FIELD32(0xff000000)

/*
 * BCN_OFFSET1:
 */
#define BCN_OFFSET1			0x0430
#define BCN_OFFSET1_BCN4		FIELD32(0x000000ff)
#define BCN_OFFSET1_BCN5		FIELD32(0x0000ff00)
#define BCN_OFFSET1_BCN6		FIELD32(0x00ff0000)
#define BCN_OFFSET1_BCN7		FIELD32(0xff000000)

/*
 * PBF registers
 * Most are for debug. Driver doesn't touch PBF register.
 */
#define TXRXQ_PCNT			0x0438
#define PBF_DBG				0x043c

/*
 * RF registers
 */
#define	RF_CSR_CFG			0x0500
#define RF_CSR_CFG_DATA			FIELD32(0x000000ff)
#define RF_CSR_CFG_REGNUM		FIELD32(0x00001f00)
#define RF_CSR_CFG_WRITE		FIELD32(0x00010000)
#define RF_CSR_CFG_BUSY			FIELD32(0x00020000)

/*
 * MAC Control/Status Registers(CSR).
 * Some values are set in TU, whereas 1 TU == 1024 us.
 */

/*
 * MAC_CSR0: ASIC revision number.
 * ASIC_REV: 0
 * ASIC_VER: 2870
 */
#define MAC_CSR0			0x1000
#define MAC_CSR0_ASIC_REV		FIELD32(0x0000ffff)
#define MAC_CSR0_ASIC_VER		FIELD32(0xffff0000)

/*
 * MAC_SYS_CTRL:
 */
#define MAC_SYS_CTRL			0x1004
#define MAC_SYS_CTRL_RESET_CSR		FIELD32(0x00000001)
#define MAC_SYS_CTRL_RESET_BBP		FIELD32(0x00000002)
#define MAC_SYS_CTRL_ENABLE_TX		FIELD32(0x00000004)
#define MAC_SYS_CTRL_ENABLE_RX		FIELD32(0x00000008)
#define MAC_SYS_CTRL_CONTINUOUS_TX	FIELD32(0x00000010)
#define MAC_SYS_CTRL_LOOPBACK		FIELD32(0x00000020)
#define MAC_SYS_CTRL_WLAN_HALT		FIELD32(0x00000040)
#define MAC_SYS_CTRL_RX_TIMESTAMP	FIELD32(0x00000080)

/*
 * MAC_ADDR_DW0: STA MAC register 0
 */
#define MAC_ADDR_DW0			0x1008
#define MAC_ADDR_DW0_BYTE0		FIELD32(0x000000ff)
#define MAC_ADDR_DW0_BYTE1		FIELD32(0x0000ff00)
#define MAC_ADDR_DW0_BYTE2		FIELD32(0x00ff0000)
#define MAC_ADDR_DW0_BYTE3		FIELD32(0xff000000)

/*
 * MAC_ADDR_DW1: STA MAC register 1
 * UNICAST_TO_ME_MASK:
 * Used to mask off bits from byte 5 of the MAC address
 * to determine the UNICAST_TO_ME bit for RX frames.
 * The full mask is complemented by BSS_ID_MASK:
 *    MASK = BSS_ID_MASK & UNICAST_TO_ME_MASK
 */
#define MAC_ADDR_DW1			0x100c
#define MAC_ADDR_DW1_BYTE4		FIELD32(0x000000ff)
#define MAC_ADDR_DW1_BYTE5		FIELD32(0x0000ff00)
#define MAC_ADDR_DW1_UNICAST_TO_ME_MASK	FIELD32(0x00ff0000)

/*
 * MAC_BSSID_DW0: BSSID register 0
 */
#define MAC_BSSID_DW0			0x1010
#define MAC_BSSID_DW0_BYTE0		FIELD32(0x000000ff)
#define MAC_BSSID_DW0_BYTE1		FIELD32(0x0000ff00)
#define MAC_BSSID_DW0_BYTE2		FIELD32(0x00ff0000)
#define MAC_BSSID_DW0_BYTE3		FIELD32(0xff000000)

/*
 * MAC_BSSID_DW1: BSSID register 1
 * BSS_ID_MASK:
 *     0: 1-BSSID mode (BSS index = 0)
 *     1: 2-BSSID mode (BSS index: Byte5, bit 0)
 *     2: 4-BSSID mode (BSS index: byte5, bit 0 - 1)
 *     3: 8-BSSID mode (BSS index: byte5, bit 0 - 2)
 * This mask is used to mask off bits 0, 1 and 2 of byte 5 of the
 * BSSID. This will make sure that those bits will be ignored
 * when determining the MY_BSS of RX frames.
 */
#define MAC_BSSID_DW1			0x1014
#define MAC_BSSID_DW1_BYTE4		FIELD32(0x000000ff)
#define MAC_BSSID_DW1_BYTE5		FIELD32(0x0000ff00)
#define MAC_BSSID_DW1_BSS_ID_MASK	FIELD32(0x00030000)
#define MAC_BSSID_DW1_BSS_BCN_NUM	FIELD32(0x001c0000)

/*
 * MAX_LEN_CFG: Maximum frame length register.
 * MAX_MPDU: rt2860b max 16k bytes
 * MAX_PSDU: Maximum PSDU length
 *	(power factor) 0:2^13, 1:2^14, 2:2^15, 3:2^16
 */
#define MAX_LEN_CFG			0x1018
#define MAX_LEN_CFG_MAX_MPDU		FIELD32(0x00000fff)
#define MAX_LEN_CFG_MAX_PSDU		FIELD32(0x00003000)
#define MAX_LEN_CFG_MIN_PSDU		FIELD32(0x0000c000)
#define MAX_LEN_CFG_MIN_MPDU		FIELD32(0x000f0000)

/*
 * BBP_CSR_CFG: BBP serial control register
 * VALUE: Register value to program into BBP
 * REG_NUM: Selected BBP register
 * READ_CONTROL: 0 write BBP, 1 read BBP
 * BUSY: ASIC is busy executing BBP commands
 * BBP_PAR_DUR: 0 4 MAC clocks, 1 8 MAC clocks
 * BBP_RW_MODE: 0 serial, 1 paralell
 */
#define BBP_CSR_CFG			0x101c
#define BBP_CSR_CFG_VALUE		FIELD32(0x000000ff)
#define BBP_CSR_CFG_REGNUM		FIELD32(0x0000ff00)
#define BBP_CSR_CFG_READ_CONTROL	FIELD32(0x00010000)
#define BBP_CSR_CFG_BUSY		FIELD32(0x00020000)
#define BBP_CSR_CFG_BBP_PAR_DUR		FIELD32(0x00040000)
#define BBP_CSR_CFG_BBP_RW_MODE		FIELD32(0x00080000)

/*
 * RF_CSR_CFG0: RF control register
 * REGID_AND_VALUE: Register value to program into RF
 * BITWIDTH: Selected RF register
 * STANDBYMODE: 0 high when standby, 1 low when standby
 * SEL: 0 RF_LE0 activate, 1 RF_LE1 activate
 * BUSY: ASIC is busy executing RF commands
 */
#define RF_CSR_CFG0			0x1020
#define RF_CSR_CFG0_REGID_AND_VALUE	FIELD32(0x00ffffff)
#define RF_CSR_CFG0_BITWIDTH		FIELD32(0x1f000000)
#define RF_CSR_CFG0_REG_VALUE_BW	FIELD32(0x1fffffff)
#define RF_CSR_CFG0_STANDBYMODE		FIELD32(0x20000000)
#define RF_CSR_CFG0_SEL			FIELD32(0x40000000)
#define RF_CSR_CFG0_BUSY		FIELD32(0x80000000)

/*
 * RF_CSR_CFG1: RF control register
 * REGID_AND_VALUE: Register value to program into RF
 * RFGAP: Gap between BB_CONTROL_RF and RF_LE
 *        0: 3 system clock cycle (37.5usec)
 *        1: 5 system clock cycle (62.5usec)
 */
#define RF_CSR_CFG1			0x1024
#define RF_CSR_CFG1_REGID_AND_VALUE	FIELD32(0x00ffffff)
#define RF_CSR_CFG1_RFGAP		FIELD32(0x1f000000)

/*
 * RF_CSR_CFG2: RF control register
 * VALUE: Register value to program into RF
 * RFGAP: Gap between BB_CONTROL_RF and RF_LE
 *        0: 3 system clock cycle (37.5usec)
 *        1: 5 system clock cycle (62.5usec)
 */
#define RF_CSR_CFG2			0x1028
#define RF_CSR_CFG2_VALUE		FIELD32(0x00ffffff)

/*
 * LED_CFG: LED control
 * color LED's:
 *   0: off
 *   1: blinking upon TX2
 *   2: periodic slow blinking
 *   3: always on
 * LED polarity:
 *   0: active low
 *   1: active high
 */
#define LED_CFG				0x102c
#define LED_CFG_ON_PERIOD		FIELD32(0x000000ff)
#define LED_CFG_OFF_PERIOD		FIELD32(0x0000ff00)
#define LED_CFG_SLOW_BLINK_PERIOD	FIELD32(0x003f0000)
#define LED_CFG_R_LED_MODE		FIELD32(0x03000000)
#define LED_CFG_G_LED_MODE		FIELD32(0x0c000000)
#define LED_CFG_Y_LED_MODE		FIELD32(0x30000000)
#define LED_CFG_LED_POLAR		FIELD32(0x40000000)

/*
 * XIFS_TIME_CFG: MAC timing
 * CCKM_SIFS_TIME: unit 1us. Applied after CCK RX/TX
 * OFDM_SIFS_TIME: unit 1us. Applied after OFDM RX/TX
 * OFDM_XIFS_TIME: unit 1us. Applied after OFDM RX
 *	when MAC doesn't reference BBP signal BBRXEND
 * EIFS: unit 1us
 * BB_RXEND_ENABLE: reference RXEND signal to begin XIFS defer
 *
 */
#define XIFS_TIME_CFG			0x1100
#define XIFS_TIME_CFG_CCKM_SIFS_TIME	FIELD32(0x000000ff)
#define XIFS_TIME_CFG_OFDM_SIFS_TIME	FIELD32(0x0000ff00)
#define XIFS_TIME_CFG_OFDM_XIFS_TIME	FIELD32(0x000f0000)
#define XIFS_TIME_CFG_EIFS		FIELD32(0x1ff00000)
#define XIFS_TIME_CFG_BB_RXEND_ENABLE	FIELD32(0x20000000)

/*
 * BKOFF_SLOT_CFG:
 */
#define BKOFF_SLOT_CFG			0x1104
#define BKOFF_SLOT_CFG_SLOT_TIME	FIELD32(0x000000ff)
#define BKOFF_SLOT_CFG_CC_DELAY_TIME	FIELD32(0x0000ff00)

/*
 * NAV_TIME_CFG:
 */
#define NAV_TIME_CFG			0x1108
#define NAV_TIME_CFG_SIFS		FIELD32(0x000000ff)
#define NAV_TIME_CFG_SLOT_TIME		FIELD32(0x0000ff00)
#define NAV_TIME_CFG_EIFS		FIELD32(0x01ff0000)
#define NAV_TIME_ZERO_SIFS		FIELD32(0x02000000)

/*
 * CH_TIME_CFG: count as channel busy
 */
#define CH_TIME_CFG     	        0x110c

/*
 * PBF_LIFE_TIMER: TX/RX MPDU timestamp timer (free run) Unit: 1us
 */
#define PBF_LIFE_TIMER     	        0x1110

/*
 * BCN_TIME_CFG:
 * BEACON_INTERVAL: in unit of 1/16 TU
 * TSF_TICKING: Enable TSF auto counting
 * TSF_SYNC: Enable TSF sync, 00: disable, 01: infra mode, 10: ad-hoc mode
 * BEACON_GEN: Enable beacon generator
 */
#define BCN_TIME_CFG			0x1114
#define BCN_TIME_CFG_BEACON_INTERVAL	FIELD32(0x0000ffff)
#define BCN_TIME_CFG_TSF_TICKING	FIELD32(0x00010000)
#define BCN_TIME_CFG_TSF_SYNC		FIELD32(0x00060000)
#define BCN_TIME_CFG_TBTT_ENABLE	FIELD32(0x00080000)
#define BCN_TIME_CFG_BEACON_GEN		FIELD32(0x00100000)
#define BCN_TIME_CFG_TX_TIME_COMPENSATE	FIELD32(0xf0000000)

/*
 * TBTT_SYNC_CFG:
 */
#define TBTT_SYNC_CFG			0x1118

/*
 * TSF_TIMER_DW0: Local lsb TSF timer, read-only
 */
#define TSF_TIMER_DW0			0x111c
#define TSF_TIMER_DW0_LOW_WORD		FIELD32(0xffffffff)

/*
 * TSF_TIMER_DW1: Local msb TSF timer, read-only
 */
#define TSF_TIMER_DW1			0x1120
#define TSF_TIMER_DW1_HIGH_WORD		FIELD32(0xffffffff)

/*
 * TBTT_TIMER: TImer remains till next TBTT, read-only
 */
#define TBTT_TIMER			0x1124

/*
 * INT_TIMER_CFG:
 */
#define INT_TIMER_CFG			0x1128

/*
 * INT_TIMER_EN: GP-timer and pre-tbtt Int enable
 */
#define INT_TIMER_EN			0x112c

/*
 * CH_IDLE_STA: channel idle time
 */
#define CH_IDLE_STA			0x1130

/*
 * CH_BUSY_STA: channel busy time
 */
#define CH_BUSY_STA			0x1134

/*
 * MAC_STATUS_CFG:
 * BBP_RF_BUSY: When set to 0, BBP and RF are stable.
 *	if 1 or higher one of the 2 registers is busy.
 */
#define MAC_STATUS_CFG			0x1200
#define MAC_STATUS_CFG_BBP_RF_BUSY	FIELD32(0x00000003)

/*
 * PWR_PIN_CFG:
 */
#define PWR_PIN_CFG			0x1204

/*
 * AUTOWAKEUP_CFG: Manual power control / status register
 * TBCN_BEFORE_WAKE: ForceWake has high privilege than PutToSleep when both set
 * AUTOWAKE: 0:sleep, 1:awake
 */
#define AUTOWAKEUP_CFG			0x1208
#define AUTOWAKEUP_CFG_AUTO_LEAD_TIME	FIELD32(0x000000ff)
#define AUTOWAKEUP_CFG_TBCN_BEFORE_WAKE	FIELD32(0x00007f00)
#define AUTOWAKEUP_CFG_AUTOWAKE		FIELD32(0x00008000)

/*
 * EDCA_AC0_CFG:
 */
#define EDCA_AC0_CFG			0x1300
#define EDCA_AC0_CFG_TX_OP		FIELD32(0x000000ff)
#define EDCA_AC0_CFG_AIFSN		FIELD32(0x00000f00)
#define EDCA_AC0_CFG_CWMIN		FIELD32(0x0000f000)
#define EDCA_AC0_CFG_CWMAX		FIELD32(0x000f0000)

/*
 * EDCA_AC1_CFG:
 */
#define EDCA_AC1_CFG			0x1304
#define EDCA_AC1_CFG_TX_OP		FIELD32(0x000000ff)
#define EDCA_AC1_CFG_AIFSN		FIELD32(0x00000f00)
#define EDCA_AC1_CFG_CWMIN		FIELD32(0x0000f000)
#define EDCA_AC1_CFG_CWMAX		FIELD32(0x000f0000)

/*
 * EDCA_AC2_CFG:
 */
#define EDCA_AC2_CFG			0x1308
#define EDCA_AC2_CFG_TX_OP		FIELD32(0x000000ff)
#define EDCA_AC2_CFG_AIFSN		FIELD32(0x00000f00)
#define EDCA_AC2_CFG_CWMIN		FIELD32(0x0000f000)
#define EDCA_AC2_CFG_CWMAX		FIELD32(0x000f0000)

/*
 * EDCA_AC3_CFG:
 */
#define EDCA_AC3_CFG			0x130c
#define EDCA_AC3_CFG_TX_OP		FIELD32(0x000000ff)
#define EDCA_AC3_CFG_AIFSN		FIELD32(0x00000f00)
#define EDCA_AC3_CFG_CWMIN		FIELD32(0x0000f000)
#define EDCA_AC3_CFG_CWMAX		FIELD32(0x000f0000)

/*
 * EDCA_TID_AC_MAP:
 */
#define EDCA_TID_AC_MAP			0x1310

/*
 * TX_PWR_CFG_0:
 */
#define TX_PWR_CFG_0			0x1314
#define TX_PWR_CFG_0_1MBS		FIELD32(0x0000000f)
#define TX_PWR_CFG_0_2MBS		FIELD32(0x000000f0)
#define TX_PWR_CFG_0_55MBS		FIELD32(0x00000f00)
#define TX_PWR_CFG_0_11MBS		FIELD32(0x0000f000)
#define TX_PWR_CFG_0_6MBS		FIELD32(0x000f0000)
#define TX_PWR_CFG_0_9MBS		FIELD32(0x00f00000)
#define TX_PWR_CFG_0_12MBS		FIELD32(0x0f000000)
#define TX_PWR_CFG_0_18MBS		FIELD32(0xf0000000)

/*
 * TX_PWR_CFG_1:
 */
#define TX_PWR_CFG_1			0x1318
#define TX_PWR_CFG_1_24MBS		FIELD32(0x0000000f)
#define TX_PWR_CFG_1_36MBS		FIELD32(0x000000f0)
#define TX_PWR_CFG_1_48MBS		FIELD32(0x00000f00)
#define TX_PWR_CFG_1_54MBS		FIELD32(0x0000f000)
#define TX_PWR_CFG_1_MCS0		FIELD32(0x000f0000)
#define TX_PWR_CFG_1_MCS1		FIELD32(0x00f00000)
#define TX_PWR_CFG_1_MCS2		FIELD32(0x0f000000)
#define TX_PWR_CFG_1_MCS3		FIELD32(0xf0000000)

/*
 * TX_PWR_CFG_2:
 */
#define TX_PWR_CFG_2			0x131c
#define TX_PWR_CFG_2_MCS4		FIELD32(0x0000000f)
#define TX_PWR_CFG_2_MCS5		FIELD32(0x000000f0)
#define TX_PWR_CFG_2_MCS6		FIELD32(0x00000f00)
#define TX_PWR_CFG_2_MCS7		FIELD32(0x0000f000)
#define TX_PWR_CFG_2_MCS8		FIELD32(0x000f0000)
#define TX_PWR_CFG_2_MCS9		FIELD32(0x00f00000)
#define TX_PWR_CFG_2_MCS10		FIELD32(0x0f000000)
#define TX_PWR_CFG_2_MCS11		FIELD32(0xf0000000)

/*
 * TX_PWR_CFG_3:
 */
#define TX_PWR_CFG_3			0x1320
#define TX_PWR_CFG_3_MCS12		FIELD32(0x0000000f)
#define TX_PWR_CFG_3_MCS13		FIELD32(0x000000f0)
#define TX_PWR_CFG_3_MCS14		FIELD32(0x00000f00)
#define TX_PWR_CFG_3_MCS15		FIELD32(0x0000f000)
#define TX_PWR_CFG_3_UKNOWN1		FIELD32(0x000f0000)
#define TX_PWR_CFG_3_UKNOWN2		FIELD32(0x00f00000)
#define TX_PWR_CFG_3_UKNOWN3		FIELD32(0x0f000000)
#define TX_PWR_CFG_3_UKNOWN4		FIELD32(0xf0000000)

/*
 * TX_PWR_CFG_4:
 */
#define TX_PWR_CFG_4			0x1324
#define TX_PWR_CFG_4_UKNOWN5		FIELD32(0x0000000f)
#define TX_PWR_CFG_4_UKNOWN6		FIELD32(0x000000f0)
#define TX_PWR_CFG_4_UKNOWN7		FIELD32(0x00000f00)
#define TX_PWR_CFG_4_UKNOWN8		FIELD32(0x0000f000)

/*
 * TX_PIN_CFG:
 */
#define TX_PIN_CFG			0x1328
#define TX_PIN_CFG_PA_PE_A0_EN		FIELD32(0x00000001)
#define TX_PIN_CFG_PA_PE_G0_EN		FIELD32(0x00000002)
#define TX_PIN_CFG_PA_PE_A1_EN		FIELD32(0x00000004)
#define TX_PIN_CFG_PA_PE_G1_EN		FIELD32(0x00000008)
#define TX_PIN_CFG_PA_PE_A0_POL		FIELD32(0x00000010)
#define TX_PIN_CFG_PA_PE_G0_POL		FIELD32(0x00000020)
#define TX_PIN_CFG_PA_PE_A1_POL		FIELD32(0x00000040)
#define TX_PIN_CFG_PA_PE_G1_POL		FIELD32(0x00000080)
#define TX_PIN_CFG_LNA_PE_A0_EN		FIELD32(0x00000100)
#define TX_PIN_CFG_LNA_PE_G0_EN		FIELD32(0x00000200)
#define TX_PIN_CFG_LNA_PE_A1_EN		FIELD32(0x00000400)
#define TX_PIN_CFG_LNA_PE_G1_EN		FIELD32(0x00000800)
#define TX_PIN_CFG_LNA_PE_A0_POL	FIELD32(0x00001000)
#define TX_PIN_CFG_LNA_PE_G0_POL	FIELD32(0x00002000)
#define TX_PIN_CFG_LNA_PE_A1_POL	FIELD32(0x00004000)
#define TX_PIN_CFG_LNA_PE_G1_POL	FIELD32(0x00008000)
#define TX_PIN_CFG_RFTR_EN		FIELD32(0x00010000)
#define TX_PIN_CFG_RFTR_POL		FIELD32(0x00020000)
#define TX_PIN_CFG_TRSW_EN		FIELD32(0x00040000)
#define TX_PIN_CFG_TRSW_POL		FIELD32(0x00080000)

/*
 * TX_BAND_CFG: 0x1 use upper 20MHz, 0x0 use lower 20MHz
 */
#define TX_BAND_CFG			0x132c
#define TX_BAND_CFG_HT40_PLUS		FIELD32(0x00000001)
#define TX_BAND_CFG_A			FIELD32(0x00000002)
#define TX_BAND_CFG_BG			FIELD32(0x00000004)

/*
 * TX_SW_CFG0:
 */
#define TX_SW_CFG0			0x1330

/*
 * TX_SW_CFG1:
 */
#define TX_SW_CFG1			0x1334

/*
 * TX_SW_CFG2:
 */
#define TX_SW_CFG2			0x1338

/*
 * TXOP_THRES_CFG:
 */
#define TXOP_THRES_CFG			0x133c

/*
 * TXOP_CTRL_CFG:
 */
#define TXOP_CTRL_CFG			0x1340

/*
 * TX_RTS_CFG:
 * RTS_THRES: unit:byte
 * RTS_FBK_EN: enable rts rate fallback
 */
#define TX_RTS_CFG			0x1344
#define TX_RTS_CFG_AUTO_RTS_RETRY_LIMIT	FIELD32(0x000000ff)
#define TX_RTS_CFG_RTS_THRES		FIELD32(0x00ffff00)
#define TX_RTS_CFG_RTS_FBK_EN		FIELD32(0x01000000)

/*
 * TX_TIMEOUT_CFG:
 * MPDU_LIFETIME: expiration time = 2^(9+MPDU LIFE TIME) us
 * RX_ACK_TIMEOUT: unit:slot. Used for TX procedure
 * TX_OP_TIMEOUT: TXOP timeout value for TXOP truncation.
 *                it is recommended that:
 *                (SLOT_TIME) > (TX_OP_TIMEOUT) > (RX_ACK_TIMEOUT)
 */
#define TX_TIMEOUT_CFG			0x1348
#define TX_TIMEOUT_CFG_MPDU_LIFETIME	FIELD32(0x000000f0)
#define TX_TIMEOUT_CFG_RX_ACK_TIMEOUT	FIELD32(0x0000ff00)
#define TX_TIMEOUT_CFG_TX_OP_TIMEOUT	FIELD32(0x00ff0000)

/*
 * TX_RTY_CFG:
 * SHORT_RTY_LIMIT: short retry limit
 * LONG_RTY_LIMIT: long retry limit
 * LONG_RTY_THRE: Long retry threshoold
 * NON_AGG_RTY_MODE: Non-Aggregate MPDU retry mode
 *                   0:expired by retry limit, 1: expired by mpdu life timer
 * AGG_RTY_MODE: Aggregate MPDU retry mode
 *               0:expired by retry limit, 1: expired by mpdu life timer
 * TX_AUTO_FB_ENABLE: Tx retry PHY rate auto fallback enable
 */
#define TX_RTY_CFG			0x134c
#define TX_RTY_CFG_SHORT_RTY_LIMIT	FIELD32(0x000000ff)
#define TX_RTY_CFG_LONG_RTY_LIMIT	FIELD32(0x0000ff00)
#define TX_RTY_CFG_LONG_RTY_THRE	FIELD32(0x0fff0000)
#define TX_RTY_CFG_NON_AGG_RTY_MODE	FIELD32(0x10000000)
#define TX_RTY_CFG_AGG_RTY_MODE		FIELD32(0x20000000)
#define TX_RTY_CFG_TX_AUTO_FB_ENABLE	FIELD32(0x40000000)

/*
 * TX_LINK_CFG:
 * REMOTE_MFB_LIFETIME: remote MFB life time. unit: 32us
 * MFB_ENABLE: TX apply remote MFB 1:enable
 * REMOTE_UMFS_ENABLE: remote unsolicit  MFB enable
 *                     0: not apply remote remote unsolicit (MFS=7)
 * TX_MRQ_EN: MCS request TX enable
 * TX_RDG_EN: RDG TX enable
 * TX_CF_ACK_EN: Piggyback CF-ACK enable
 * REMOTE_MFB: remote MCS feedback
 * REMOTE_MFS: remote MCS feedback sequence number
 */
#define TX_LINK_CFG			0x1350
#define TX_LINK_CFG_REMOTE_MFB_LIFETIME	FIELD32(0x000000ff)
#define TX_LINK_CFG_MFB_ENABLE		FIELD32(0x00000100)
#define TX_LINK_CFG_REMOTE_UMFS_ENABLE	FIELD32(0x00000200)
#define TX_LINK_CFG_TX_MRQ_EN		FIELD32(0x00000400)
#define TX_LINK_CFG_TX_RDG_EN		FIELD32(0x00000800)
#define TX_LINK_CFG_TX_CF_ACK_EN	FIELD32(0x00001000)
#define TX_LINK_CFG_REMOTE_MFB		FIELD32(0x00ff0000)
#define TX_LINK_CFG_REMOTE_MFS		FIELD32(0xff000000)

/*
 * HT_FBK_CFG0:
 */
#define HT_FBK_CFG0			0x1354
#define HT_FBK_CFG0_HTMCS0FBK		FIELD32(0x0000000f)
#define HT_FBK_CFG0_HTMCS1FBK		FIELD32(0x000000f0)
#define HT_FBK_CFG0_HTMCS2FBK		FIELD32(0x00000f00)
#define HT_FBK_CFG0_HTMCS3FBK		FIELD32(0x0000f000)
#define HT_FBK_CFG0_HTMCS4FBK		FIELD32(0x000f0000)
#define HT_FBK_CFG0_HTMCS5FBK		FIELD32(0x00f00000)
#define HT_FBK_CFG0_HTMCS6FBK		FIELD32(0x0f000000)
#define HT_FBK_CFG0_HTMCS7FBK		FIELD32(0xf0000000)

/*
 * HT_FBK_CFG1:
 */
#define HT_FBK_CFG1			0x1358
#define HT_FBK_CFG1_HTMCS8FBK		FIELD32(0x0000000f)
#define HT_FBK_CFG1_HTMCS9FBK		FIELD32(0x000000f0)
#define HT_FBK_CFG1_HTMCS10FBK		FIELD32(0x00000f00)
#define HT_FBK_CFG1_HTMCS11FBK		FIELD32(0x0000f000)
#define HT_FBK_CFG1_HTMCS12FBK		FIELD32(0x000f0000)
#define HT_FBK_CFG1_HTMCS13FBK		FIELD32(0x00f00000)
#define HT_FBK_CFG1_HTMCS14FBK		FIELD32(0x0f000000)
#define HT_FBK_CFG1_HTMCS15FBK		FIELD32(0xf0000000)

/*
 * LG_FBK_CFG0:
 */
#define LG_FBK_CFG0			0x135c
#define LG_FBK_CFG0_OFDMMCS0FBK		FIELD32(0x0000000f)
#define LG_FBK_CFG0_OFDMMCS1FBK		FIELD32(0x000000f0)
#define LG_FBK_CFG0_OFDMMCS2FBK		FIELD32(0x00000f00)
#define LG_FBK_CFG0_OFDMMCS3FBK		FIELD32(0x0000f000)
#define LG_FBK_CFG0_OFDMMCS4FBK		FIELD32(0x000f0000)
#define LG_FBK_CFG0_OFDMMCS5FBK		FIELD32(0x00f00000)
#define LG_FBK_CFG0_OFDMMCS6FBK		FIELD32(0x0f000000)
#define LG_FBK_CFG0_OFDMMCS7FBK		FIELD32(0xf0000000)

/*
 * LG_FBK_CFG1:
 */
#define LG_FBK_CFG1			0x1360
#define LG_FBK_CFG0_CCKMCS0FBK		FIELD32(0x0000000f)
#define LG_FBK_CFG0_CCKMCS1FBK		FIELD32(0x000000f0)
#define LG_FBK_CFG0_CCKMCS2FBK		FIELD32(0x00000f00)
#define LG_FBK_CFG0_CCKMCS3FBK		FIELD32(0x0000f000)

/*
 * CCK_PROT_CFG: CCK Protection
 * PROTECT_RATE: Protection control frame rate for CCK TX(RTS/CTS/CFEnd)
 * PROTECT_CTRL: Protection control frame type for CCK TX
 *               0:none, 1:RTS/CTS, 2:CTS-to-self
 * PROTECT_NAV: TXOP protection type for CCK TX
 *              0:none, 1:ShortNAVprotect, 2:LongNAVProtect
 * TX_OP_ALLOW_CCK: CCK TXOP allowance, 0:disallow
 * TX_OP_ALLOW_OFDM: CCK TXOP allowance, 0:disallow
 * TX_OP_ALLOW_MM20: CCK TXOP allowance, 0:disallow
 * TX_OP_ALLOW_MM40: CCK TXOP allowance, 0:disallow
 * TX_OP_ALLOW_GF20: CCK TXOP allowance, 0:disallow
 * TX_OP_ALLOW_GF40: CCK TXOP allowance, 0:disallow
 * RTS_TH_EN: RTS threshold enable on CCK TX
 */
#define CCK_PROT_CFG			0x1364
#define CCK_PROT_CFG_PROTECT_RATE	FIELD32(0x0000ffff)
#define CCK_PROT_CFG_PROTECT_CTRL	FIELD32(0x00030000)
#define CCK_PROT_CFG_PROTECT_NAV	FIELD32(0x000c0000)
#define CCK_PROT_CFG_TX_OP_ALLOW_CCK	FIELD32(0x00100000)
#define CCK_PROT_CFG_TX_OP_ALLOW_OFDM	FIELD32(0x00200000)
#define CCK_PROT_CFG_TX_OP_ALLOW_MM20	FIELD32(0x00400000)
#define CCK_PROT_CFG_TX_OP_ALLOW_MM40	FIELD32(0x00800000)
#define CCK_PROT_CFG_TX_OP_ALLOW_GF20	FIELD32(0x01000000)
#define CCK_PROT_CFG_TX_OP_ALLOW_GF40	FIELD32(0x02000000)
#define CCK_PROT_CFG_RTS_TH_EN		FIELD32(0x04000000)

/*
 * OFDM_PROT_CFG: OFDM Protection
 */
#define OFDM_PROT_CFG			0x1368
#define OFDM_PROT_CFG_PROTECT_RATE	FIELD32(0x0000ffff)
#define OFDM_PROT_CFG_PROTECT_CTRL	FIELD32(0x00030000)
#define OFDM_PROT_CFG_PROTECT_NAV	FIELD32(0x000c0000)
#define OFDM_PROT_CFG_TX_OP_ALLOW_CCK	FIELD32(0x00100000)
#define OFDM_PROT_CFG_TX_OP_ALLOW_OFDM	FIELD32(0x00200000)
#define OFDM_PROT_CFG_TX_OP_ALLOW_MM20	FIELD32(0x00400000)
#define OFDM_PROT_CFG_TX_OP_ALLOW_MM40	FIELD32(0x00800000)
#define OFDM_PROT_CFG_TX_OP_ALLOW_GF20	FIELD32(0x01000000)
#define OFDM_PROT_CFG_TX_OP_ALLOW_GF40	FIELD32(0x02000000)
#define OFDM_PROT_CFG_RTS_TH_EN		FIELD32(0x04000000)

/*
 * MM20_PROT_CFG: MM20 Protection
 */
#define MM20_PROT_CFG			0x136c
#define MM20_PROT_CFG_PROTECT_RATE	FIELD32(0x0000ffff)
#define MM20_PROT_CFG_PROTECT_CTRL	FIELD32(0x00030000)
#define MM20_PROT_CFG_PROTECT_NAV	FIELD32(0x000c0000)
#define MM20_PROT_CFG_TX_OP_ALLOW_CCK	FIELD32(0x00100000)
#define MM20_PROT_CFG_TX_OP_ALLOW_OFDM	FIELD32(0x00200000)
#define MM20_PROT_CFG_TX_OP_ALLOW_MM20	FIELD32(0x00400000)
#define MM20_PROT_CFG_TX_OP_ALLOW_MM40	FIELD32(0x00800000)
#define MM20_PROT_CFG_TX_OP_ALLOW_GF20	FIELD32(0x01000000)
#define MM20_PROT_CFG_TX_OP_ALLOW_GF40	FIELD32(0x02000000)
#define MM20_PROT_CFG_RTS_TH_EN		FIELD32(0x04000000)

/*
 * MM40_PROT_CFG: MM40 Protection
 */
#define MM40_PROT_CFG			0x1370
#define MM40_PROT_CFG_PROTECT_RATE	FIELD32(0x0000ffff)
#define MM40_PROT_CFG_PROTECT_CTRL	FIELD32(0x00030000)
#define MM40_PROT_CFG_PROTECT_NAV	FIELD32(0x000c0000)
#define MM40_PROT_CFG_TX_OP_ALLOW_CCK	FIELD32(0x00100000)
#define MM40_PROT_CFG_TX_OP_ALLOW_OFDM	FIELD32(0x00200000)
#define MM40_PROT_CFG_TX_OP_ALLOW_MM20	FIELD32(0x00400000)
#define MM40_PROT_CFG_TX_OP_ALLOW_MM40	FIELD32(0x00800000)
#define MM40_PROT_CFG_TX_OP_ALLOW_GF20	FIELD32(0x01000000)
#define MM40_PROT_CFG_TX_OP_ALLOW_GF40	FIELD32(0x02000000)
#define MM40_PROT_CFG_RTS_TH_EN		FIELD32(0x04000000)

/*
 * GF20_PROT_CFG: GF20 Protection
 */
#define GF20_PROT_CFG			0x1374
#define GF20_PROT_CFG_PROTECT_RATE	FIELD32(0x0000ffff)
#define GF20_PROT_CFG_PROTECT_CTRL	FIELD32(0x00030000)
#define GF20_PROT_CFG_PROTECT_NAV	FIELD32(0x000c0000)
#define GF20_PROT_CFG_TX_OP_ALLOW_CCK	FIELD32(0x00100000)
#define GF20_PROT_CFG_TX_OP_ALLOW_OFDM	FIELD32(0x00200000)
#define GF20_PROT_CFG_TX_OP_ALLOW_MM20	FIELD32(0x00400000)
#define GF20_PROT_CFG_TX_OP_ALLOW_MM40	FIELD32(0x00800000)
#define GF20_PROT_CFG_TX_OP_ALLOW_GF20	FIELD32(0x01000000)
#define GF20_PROT_CFG_TX_OP_ALLOW_GF40	FIELD32(0x02000000)
#define GF20_PROT_CFG_RTS_TH_EN		FIELD32(0x04000000)

/*
 * GF40_PROT_CFG: GF40 Protection
 */
#define GF40_PROT_CFG			0x1378
#define GF40_PROT_CFG_PROTECT_RATE	FIELD32(0x0000ffff)
#define GF40_PROT_CFG_PROTECT_CTRL	FIELD32(0x00030000)
#define GF40_PROT_CFG_PROTECT_NAV	FIELD32(0x000c0000)
#define GF40_PROT_CFG_TX_OP_ALLOW_CCK	FIELD32(0x00100000)
#define GF40_PROT_CFG_TX_OP_ALLOW_OFDM	FIELD32(0x00200000)
#define GF40_PROT_CFG_TX_OP_ALLOW_MM20	FIELD32(0x00400000)
#define GF40_PROT_CFG_TX_OP_ALLOW_MM40	FIELD32(0x00800000)
#define GF40_PROT_CFG_TX_OP_ALLOW_GF20	FIELD32(0x01000000)
#define GF40_PROT_CFG_TX_OP_ALLOW_GF40	FIELD32(0x02000000)
#define GF40_PROT_CFG_RTS_TH_EN		FIELD32(0x04000000)

/*
 * EXP_CTS_TIME:
 */
#define EXP_CTS_TIME			0x137c

/*
 * EXP_ACK_TIME:
 */
#define EXP_ACK_TIME			0x1380

/*
 * RX_FILTER_CFG: RX configuration register.
 */
#define RX_FILTER_CFG			0x1400
#define RX_FILTER_CFG_DROP_CRC_ERROR	FIELD32(0x00000001)
#define RX_FILTER_CFG_DROP_PHY_ERROR	FIELD32(0x00000002)
#define RX_FILTER_CFG_DROP_NOT_TO_ME	FIELD32(0x00000004)
#define RX_FILTER_CFG_DROP_NOT_MY_BSSD	FIELD32(0x00000008)
#define RX_FILTER_CFG_DROP_VER_ERROR	FIELD32(0x00000010)
#define RX_FILTER_CFG_DROP_MULTICAST	FIELD32(0x00000020)
#define RX_FILTER_CFG_DROP_BROADCAST	FIELD32(0x00000040)
#define RX_FILTER_CFG_DROP_DUPLICATE	FIELD32(0x00000080)
#define RX_FILTER_CFG_DROP_CF_END_ACK	FIELD32(0x00000100)
#define RX_FILTER_CFG_DROP_CF_END	FIELD32(0x00000200)
#define RX_FILTER_CFG_DROP_ACK		FIELD32(0x00000400)
#define RX_FILTER_CFG_DROP_CTS		FIELD32(0x00000800)
#define RX_FILTER_CFG_DROP_RTS		FIELD32(0x00001000)
#define RX_FILTER_CFG_DROP_PSPOLL	FIELD32(0x00002000)
#define RX_FILTER_CFG_DROP_BA		FIELD32(0x00004000)
#define RX_FILTER_CFG_DROP_BAR		FIELD32(0x00008000)
#define RX_FILTER_CFG_DROP_CNTL		FIELD32(0x00010000)

/*
 * AUTO_RSP_CFG:
 * AUTORESPONDER: 0: disable, 1: enable
 * BAC_ACK_POLICY: 0:long, 1:short preamble
 * CTS_40_MMODE: Response CTS 40MHz duplicate mode
 * CTS_40_MREF: Response CTS 40MHz duplicate mode
 * AR_PREAMBLE: Auto responder preamble 0:long, 1:short preamble
 * DUAL_CTS_EN: Power bit value in control frame
 * ACK_CTS_PSM_BIT:Power bit value in control frame
 */
#define AUTO_RSP_CFG			0x1404
#define AUTO_RSP_CFG_AUTORESPONDER	FIELD32(0x00000001)
#define AUTO_RSP_CFG_BAC_ACK_POLICY	FIELD32(0x00000002)
#define AUTO_RSP_CFG_CTS_40_MMODE	FIELD32(0x00000004)
#define AUTO_RSP_CFG_CTS_40_MREF	FIELD32(0x00000008)
#define AUTO_RSP_CFG_AR_PREAMBLE	FIELD32(0x00000010)
#define AUTO_RSP_CFG_DUAL_CTS_EN	FIELD32(0x00000040)
#define AUTO_RSP_CFG_ACK_CTS_PSM_BIT	FIELD32(0x00000080)

/*
 * LEGACY_BASIC_RATE:
 */
#define LEGACY_BASIC_RATE		0x1408

/*
 * HT_BASIC_RATE:
 */
#define HT_BASIC_RATE			0x140c

/*
 * HT_CTRL_CFG:
 */
#define HT_CTRL_CFG			0x1410

/*
 * SIFS_COST_CFG:
 */
#define SIFS_COST_CFG			0x1414

/*
 * RX_PARSER_CFG:
 * Set NAV for all received frames
 */
#define RX_PARSER_CFG			0x1418

/*
 * TX_SEC_CNT0:
 */
#define TX_SEC_CNT0			0x1500

/*
 * RX_SEC_CNT0:
 */
#define RX_SEC_CNT0			0x1504

/*
 * CCMP_FC_MUTE:
 */
#define CCMP_FC_MUTE			0x1508

/*
 * TXOP_HLDR_ADDR0:
 */
#define TXOP_HLDR_ADDR0			0x1600

/*
 * TXOP_HLDR_ADDR1:
 */
#define TXOP_HLDR_ADDR1			0x1604

/*
 * TXOP_HLDR_ET:
 */
#define TXOP_HLDR_ET			0x1608

/*
 * QOS_CFPOLL_RA_DW0:
 */
#define QOS_CFPOLL_RA_DW0		0x160c

/*
 * QOS_CFPOLL_RA_DW1:
 */
#define QOS_CFPOLL_RA_DW1		0x1610

/*
 * QOS_CFPOLL_QC:
 */
#define QOS_CFPOLL_QC			0x1614

/*
 * RX_STA_CNT0: RX PLCP error count & RX CRC error count
 */
#define RX_STA_CNT0			0x1700
#define RX_STA_CNT0_CRC_ERR		FIELD32(0x0000ffff)
#define RX_STA_CNT0_PHY_ERR		FIELD32(0xffff0000)

/*
 * RX_STA_CNT1: RX False CCA count & RX LONG frame count
 */
#define RX_STA_CNT1			0x1704
#define RX_STA_CNT1_FALSE_CCA		FIELD32(0x0000ffff)
#define RX_STA_CNT1_PLCP_ERR		FIELD32(0xffff0000)

/*
 * RX_STA_CNT2:
 */
#define RX_STA_CNT2			0x1708
#define RX_STA_CNT2_RX_DUPLI_COUNT	FIELD32(0x0000ffff)
#define RX_STA_CNT2_RX_FIFO_OVERFLOW	FIELD32(0xffff0000)

/*
 * TX_STA_CNT0: TX Beacon count
 */
#define TX_STA_CNT0			0x170c
#define TX_STA_CNT0_TX_FAIL_COUNT	FIELD32(0x0000ffff)
#define TX_STA_CNT0_TX_BEACON_COUNT	FIELD32(0xffff0000)

/*
 * TX_STA_CNT1: TX tx count
 */
#define TX_STA_CNT1			0x1710
#define TX_STA_CNT1_TX_SUCCESS		FIELD32(0x0000ffff)
#define TX_STA_CNT1_TX_RETRANSMIT	FIELD32(0xffff0000)

/*
 * TX_STA_CNT2: TX tx count
 */
#define TX_STA_CNT2			0x1714
#define TX_STA_CNT2_TX_ZERO_LEN_COUNT	FIELD32(0x0000ffff)
#define TX_STA_CNT2_TX_UNDER_FLOW_COUNT	FIELD32(0xffff0000)

/*
 * TX_STA_FIFO: TX Result for specific PID status fifo register
 */
#define TX_STA_FIFO			0x1718
#define TX_STA_FIFO_VALID		FIELD32(0x00000001)
#define TX_STA_FIFO_PID_TYPE		FIELD32(0x0000001e)
#define TX_STA_FIFO_TX_SUCCESS		FIELD32(0x00000020)
#define TX_STA_FIFO_TX_AGGRE		FIELD32(0x00000040)
#define TX_STA_FIFO_TX_ACK_REQUIRED	FIELD32(0x00000080)
#define TX_STA_FIFO_WCID		FIELD32(0x0000ff00)
#define TX_STA_FIFO_SUCCESS_RATE	FIELD32(0xffff0000)

/*
 * TX_AGG_CNT: Debug counter
 */
#define TX_AGG_CNT			0x171c
#define TX_AGG_CNT_NON_AGG_TX_COUNT	FIELD32(0x0000ffff)
#define TX_AGG_CNT_AGG_TX_COUNT		FIELD32(0xffff0000)

/*
 * TX_AGG_CNT0:
 */
#define TX_AGG_CNT0			0x1720
#define TX_AGG_CNT0_AGG_SIZE_1_COUNT	FIELD32(0x0000ffff)
#define TX_AGG_CNT0_AGG_SIZE_2_COUNT	FIELD32(0xffff0000)

/*
 * TX_AGG_CNT1:
 */
#define TX_AGG_CNT1			0x1724
#define TX_AGG_CNT1_AGG_SIZE_3_COUNT	FIELD32(0x0000ffff)
#define TX_AGG_CNT1_AGG_SIZE_4_COUNT	FIELD32(0xffff0000)

/*
 * TX_AGG_CNT2:
 */
#define TX_AGG_CNT2			0x1728
#define TX_AGG_CNT2_AGG_SIZE_5_COUNT	FIELD32(0x0000ffff)
#define TX_AGG_CNT2_AGG_SIZE_6_COUNT	FIELD32(0xffff0000)

/*
 * TX_AGG_CNT3:
 */
#define TX_AGG_CNT3			0x172c
#define TX_AGG_CNT3_AGG_SIZE_7_COUNT	FIELD32(0x0000ffff)
#define TX_AGG_CNT3_AGG_SIZE_8_COUNT	FIELD32(0xffff0000)

/*
 * TX_AGG_CNT4:
 */
#define TX_AGG_CNT4			0x1730
#define TX_AGG_CNT4_AGG_SIZE_9_COUNT	FIELD32(0x0000ffff)
#define TX_AGG_CNT4_AGG_SIZE_10_COUNT	FIELD32(0xffff0000)

/*
 * TX_AGG_CNT5:
 */
#define TX_AGG_CNT5			0x1734
#define TX_AGG_CNT5_AGG_SIZE_11_COUNT	FIELD32(0x0000ffff)
#define TX_AGG_CNT5_AGG_SIZE_12_COUNT	FIELD32(0xffff0000)

/*
 * TX_AGG_CNT6:
 */
#define TX_AGG_CNT6			0x1738
#define TX_AGG_CNT6_AGG_SIZE_13_COUNT	FIELD32(0x0000ffff)
#define TX_AGG_CNT6_AGG_SIZE_14_COUNT	FIELD32(0xffff0000)

/*
 * TX_AGG_CNT7:
 */
#define TX_AGG_CNT7			0x173c
#define TX_AGG_CNT7_AGG_SIZE_15_COUNT	FIELD32(0x0000ffff)
#define TX_AGG_CNT7_AGG_SIZE_16_COUNT	FIELD32(0xffff0000)

/*
 * MPDU_DENSITY_CNT:
 * TX_ZERO_DEL: TX zero length delimiter count
 * RX_ZERO_DEL: RX zero length delimiter count
 */
#define MPDU_DENSITY_CNT		0x1740
#define MPDU_DENSITY_CNT_TX_ZERO_DEL	FIELD32(0x0000ffff)
#define MPDU_DENSITY_CNT_RX_ZERO_DEL	FIELD32(0xffff0000)

/*
 * Security key table memory.
 * MAC_WCID_BASE: 8-bytes (use only 6 bytes) * 256 entry
 * PAIRWISE_KEY_TABLE_BASE: 32-byte * 256 entry
 * MAC_IVEIV_TABLE_BASE: 8-byte * 256-entry
 * MAC_WCID_ATTRIBUTE_BASE: 4-byte * 256-entry
 * SHARED_KEY_TABLE_BASE: 32 bytes * 32-entry
 * SHARED_KEY_MODE_BASE: 4 bits * 32-entry
 */
#define MAC_WCID_BASE			0x1800
#define PAIRWISE_KEY_TABLE_BASE		0x4000
#define MAC_IVEIV_TABLE_BASE		0x6000
#define MAC_WCID_ATTRIBUTE_BASE		0x6800
#define SHARED_KEY_TABLE_BASE		0x6c00
#define SHARED_KEY_MODE_BASE		0x7000

#define MAC_WCID_ENTRY(__idx) \
	( MAC_WCID_BASE + ((__idx) * sizeof(struct mac_wcid_entry)) )
#define PAIRWISE_KEY_ENTRY(__idx) \
	( PAIRWISE_KEY_TABLE_BASE + ((__idx) * sizeof(struct hw_key_entry)) )
#define MAC_IVEIV_ENTRY(__idx) \
	( MAC_IVEIV_TABLE_BASE + ((__idx) & sizeof(struct mac_iveiv_entry)) )
#define MAC_WCID_ATTR_ENTRY(__idx) \
	( MAC_WCID_ATTRIBUTE_BASE + ((__idx) * sizeof(u32)) )
#define SHARED_KEY_ENTRY(__idx) \
	( SHARED_KEY_TABLE_BASE + ((__idx) * sizeof(struct hw_key_entry)) )
#define SHARED_KEY_MODE_ENTRY(__idx) \
	( SHARED_KEY_MODE_BASE + ((__idx) * sizeof(u32)) )

struct mac_wcid_entry {
	u8 mac[6];
	u8 reserved[2];
} __attribute__ ((packed));

struct hw_key_entry {
	u8 key[16];
	u8 tx_mic[8];
	u8 rx_mic[8];
} __attribute__ ((packed));

struct mac_iveiv_entry {
	u8 iv[8];
} __attribute__ ((packed));

/*
 * MAC_WCID_ATTRIBUTE:
 */
#define MAC_WCID_ATTRIBUTE_KEYTAB	FIELD32(0x00000001)
#define MAC_WCID_ATTRIBUTE_CIPHER	FIELD32(0x0000000e)
#define MAC_WCID_ATTRIBUTE_BSS_IDX	FIELD32(0x00000070)
#define MAC_WCID_ATTRIBUTE_RX_WIUDF	FIELD32(0x00000380)

/*
 * SHARED_KEY_MODE:
 */
#define SHARED_KEY_MODE_BSS0_KEY0	FIELD32(0x00000007)
#define SHARED_KEY_MODE_BSS0_KEY1	FIELD32(0x00000070)
#define SHARED_KEY_MODE_BSS0_KEY2	FIELD32(0x00000700)
#define SHARED_KEY_MODE_BSS0_KEY3	FIELD32(0x00007000)
#define SHARED_KEY_MODE_BSS1_KEY0	FIELD32(0x00070000)
#define SHARED_KEY_MODE_BSS1_KEY1	FIELD32(0x00700000)
#define SHARED_KEY_MODE_BSS1_KEY2	FIELD32(0x07000000)
#define SHARED_KEY_MODE_BSS1_KEY3	FIELD32(0x70000000)

/*
 * HOST-MCU communication
 */

/*
 * H2M_MAILBOX_CSR: Host-to-MCU Mailbox.
 */
#define H2M_MAILBOX_CSR			0x7010
#define H2M_MAILBOX_CSR_ARG0		FIELD32(0x000000ff)
#define H2M_MAILBOX_CSR_ARG1		FIELD32(0x0000ff00)
#define H2M_MAILBOX_CSR_CMD_TOKEN	FIELD32(0x00ff0000)
#define H2M_MAILBOX_CSR_OWNER		FIELD32(0xff000000)

/*
 * H2M_MAILBOX_CID:
 */
#define H2M_MAILBOX_CID			0x7014
#define H2M_MAILBOX_CID_CMD0		FIELD32(0x000000ff)
#define H2M_MAILBOX_CID_CMD1		FIELD32(0x0000ff00)
#define H2M_MAILBOX_CID_CMD2		FIELD32(0x00ff0000)
#define H2M_MAILBOX_CID_CMD3		FIELD32(0xff000000)

/*
 * H2M_MAILBOX_STATUS:
 */
#define H2M_MAILBOX_STATUS		0x701c

/*
 * H2M_INT_SRC:
 */
#define H2M_INT_SRC			0x7024

/*
 * H2M_BBP_AGENT:
 */
#define H2M_BBP_AGENT			0x7028

/*
 * MCU_LEDCS: LED control for MCU Mailbox.
 */
#define MCU_LEDCS_LED_MODE		FIELD8(0x1f)
#define MCU_LEDCS_POLARITY		FIELD8(0x01)

/*
 * HW_CS_CTS_BASE:
 * Carrier-sense CTS frame base address.
 * It's where mac stores carrier-sense frame for carrier-sense function.
 */
#define HW_CS_CTS_BASE			0x7700

/*
 * HW_DFS_CTS_BASE:
 * FS CTS frame base address. It's where mac stores CTS frame for DFS.
 */
#define HW_DFS_CTS_BASE			0x7780

/*
 * TXRX control registers - base address 0x3000
 */

/*
 * TXRX_CSR1:
 * rt2860b  UNKNOWN reg use R/O Reg Addr 0x77d0 first..
 */
#define TXRX_CSR1			0x77d0

/*
 * HW_DEBUG_SETTING_BASE:
 * since NULL frame won't be that long (256 byte)
 * We steal 16 tail bytes to save debugging settings
 */
#define HW_DEBUG_SETTING_BASE		0x77f0
#define HW_DEBUG_SETTING_BASE2		0x7770

/*
 * HW_BEACON_BASE
 * In order to support maximum 8 MBSS and its maximum length
 * is 512 bytes for each beacon
 * Three section discontinue memory segments will be used.
 * 1. The original region for BCN 0~3
 * 2. Extract memory from FCE table for BCN 4~5
 * 3. Extract memory from Pair-wise key table for BCN 6~7
 *    It occupied those memory of wcid 238~253 for BCN 6
 *    and wcid 222~237 for BCN 7
 *
 * IMPORTANT NOTE: Not sure why legacy driver does this,
 * but HW_BEACON_BASE7 is 0x0200 bytes below HW_BEACON_BASE6.
 */
#define HW_BEACON_BASE0			0x7800
#define HW_BEACON_BASE1			0x7a00
#define HW_BEACON_BASE2			0x7c00
#define HW_BEACON_BASE3			0x7e00
#define HW_BEACON_BASE4			0x7200
#define HW_BEACON_BASE5			0x7400
#define HW_BEACON_BASE6			0x5dc0
#define HW_BEACON_BASE7			0x5bc0

#define HW_BEACON_OFFSET(__index) \
	( ((__index) < 4) ? ( HW_BEACON_BASE0 + (__index * 0x0200) ) : \
	  (((__index) < 6) ? ( HW_BEACON_BASE4 + ((__index - 4) * 0x0200) ) : \
	  (HW_BEACON_BASE6 - ((__index - 6) * 0x0200))) )

/*
 * 8051 firmware image.
 */
#define FIRMWARE_RT2870			"rt2870.bin"
#define FIRMWARE_IMAGE_BASE		0x3000

/*
 * BBP registers.
 * The wordsize of the BBP is 8 bits.
 */

/*
 * BBP 1: TX Antenna
 */
#define BBP1_TX_POWER			FIELD8(0x07)
#define BBP1_TX_ANTENNA			FIELD8(0x18)

/*
 * BBP 3: RX Antenna
 */
#define BBP3_RX_ANTENNA			FIELD8(0x18)
#define BBP3_HT40_PLUS			FIELD8(0x20)

/*
 * BBP 4: Bandwidth
 */
#define BBP4_TX_BF			FIELD8(0x01)
#define BBP4_BANDWIDTH			FIELD8(0x18)

/*
 * RFCSR registers
 * The wordsize of the RFCSR is 8 bits.
 */

/*
 * RFCSR 6:
 */
#define RFCSR6_R			FIELD8(0x03)

/*
 * RFCSR 7:
 */
#define RFCSR7_RF_TUNING		FIELD8(0x01)

/*
 * RFCSR 12:
 */
#define RFCSR12_TX_POWER		FIELD8(0x1f)

/*
 * RFCSR 22:
 */
#define RFCSR22_BASEBAND_LOOPBACK	FIELD8(0x01)

/*
 * RFCSR 23:
 */
#define RFCSR23_FREQ_OFFSET		FIELD8(0x7f)

/*
 * RFCSR 30:
 */
#define RFCSR30_RF_CALIBRATION		FIELD8(0x80)

/*
 * RF registers
 */

/*
 * RF 2
 */
#define RF2_ANTENNA_RX2			FIELD32(0x00000040)
#define RF2_ANTENNA_TX1			FIELD32(0x00004000)
#define RF2_ANTENNA_RX1			FIELD32(0x00020000)

/*
 * RF 3
 */
#define RF3_TXPOWER_G			FIELD32(0x00003e00)
#define RF3_TXPOWER_A_7DBM_BOOST	FIELD32(0x00000200)
#define RF3_TXPOWER_A			FIELD32(0x00003c00)

/*
 * RF 4
 */
#define RF4_TXPOWER_G			FIELD32(0x000007c0)
#define RF4_TXPOWER_A_7DBM_BOOST	FIELD32(0x00000040)
#define RF4_TXPOWER_A			FIELD32(0x00000780)
#define RF4_FREQ_OFFSET			FIELD32(0x001f8000)
#define RF4_HT40			FIELD32(0x00200000)

/*
 * EEPROM content.
 * The wordsize of the EEPROM is 16 bits.
 */

/*
 * EEPROM Version
 */
#define EEPROM_VERSION			0x0001
#define EEPROM_VERSION_FAE		FIELD16(0x00ff)
#define EEPROM_VERSION_VERSION		FIELD16(0xff00)

/*
 * HW MAC address.
 */
#define EEPROM_MAC_ADDR_0		0x0002
#define EEPROM_MAC_ADDR_BYTE0		FIELD16(0x00ff)
#define EEPROM_MAC_ADDR_BYTE1		FIELD16(0xff00)
#define EEPROM_MAC_ADDR_1		0x0003
#define EEPROM_MAC_ADDR_BYTE2		FIELD16(0x00ff)
#define EEPROM_MAC_ADDR_BYTE3		FIELD16(0xff00)
#define EEPROM_MAC_ADDR_2		0x0004
#define EEPROM_MAC_ADDR_BYTE4		FIELD16(0x00ff)
#define EEPROM_MAC_ADDR_BYTE5		FIELD16(0xff00)

/*
 * EEPROM ANTENNA config
 * RXPATH: 1: 1R, 2: 2R, 3: 3R
 * TXPATH: 1: 1T, 2: 2T
 */
#define	EEPROM_ANTENNA			0x001a
#define EEPROM_ANTENNA_RXPATH		FIELD16(0x000f)
#define EEPROM_ANTENNA_TXPATH		FIELD16(0x00f0)
#define EEPROM_ANTENNA_RF_TYPE		FIELD16(0x0f00)

/*
 * EEPROM NIC config
 * CARDBUS_ACCEL: 0 - enable, 1 - disable
 */
#define	EEPROM_NIC			0x001b
#define EEPROM_NIC_HW_RADIO		FIELD16(0x0001)
#define EEPROM_NIC_DYNAMIC_TX_AGC	FIELD16(0x0002)
#define EEPROM_NIC_EXTERNAL_LNA_BG	FIELD16(0x0004)
#define EEPROM_NIC_EXTERNAL_LNA_A	FIELD16(0x0008)
#define EEPROM_NIC_CARDBUS_ACCEL	FIELD16(0x0010)
#define EEPROM_NIC_BW40M_SB_BG		FIELD16(0x0020)
#define EEPROM_NIC_BW40M_SB_A		FIELD16(0x0040)
#define EEPROM_NIC_WPS_PBC		FIELD16(0x0080)
#define EEPROM_NIC_BW40M_BG		FIELD16(0x0100)
#define EEPROM_NIC_BW40M_A		FIELD16(0x0200)

/*
 * EEPROM frequency
 */
#define	EEPROM_FREQ			0x001d
#define EEPROM_FREQ_OFFSET		FIELD16(0x00ff)
#define EEPROM_FREQ_LED_MODE		FIELD16(0x7f00)
#define EEPROM_FREQ_LED_POLARITY	FIELD16(0x1000)

/*
 * EEPROM LED
 * POLARITY_RDY_G: Polarity RDY_G setting.
 * POLARITY_RDY_A: Polarity RDY_A setting.
 * POLARITY_ACT: Polarity ACT setting.
 * POLARITY_GPIO_0: Polarity GPIO0 setting.
 * POLARITY_GPIO_1: Polarity GPIO1 setting.
 * POLARITY_GPIO_2: Polarity GPIO2 setting.
 * POLARITY_GPIO_3: Polarity GPIO3 setting.
 * POLARITY_GPIO_4: Polarity GPIO4 setting.
 * LED_MODE: Led mode.
 */
#define EEPROM_LED1			0x001e
#define EEPROM_LED2			0x001f
#define EEPROM_LED3			0x0020
#define EEPROM_LED_POLARITY_RDY_BG	FIELD16(0x0001)
#define EEPROM_LED_POLARITY_RDY_A	FIELD16(0x0002)
#define EEPROM_LED_POLARITY_ACT		FIELD16(0x0004)
#define EEPROM_LED_POLARITY_GPIO_0	FIELD16(0x0008)
#define EEPROM_LED_POLARITY_GPIO_1	FIELD16(0x0010)
#define EEPROM_LED_POLARITY_GPIO_2	FIELD16(0x0020)
#define EEPROM_LED_POLARITY_GPIO_3	FIELD16(0x0040)
#define EEPROM_LED_POLARITY_GPIO_4	FIELD16(0x0080)
#define EEPROM_LED_LED_MODE		FIELD16(0x1f00)

/*
 * EEPROM LNA
 */
#define EEPROM_LNA			0x0022
#define EEPROM_LNA_BG			FIELD16(0x00ff)
#define EEPROM_LNA_A0			FIELD16(0xff00)

/*
 * EEPROM RSSI BG offset
 */
#define EEPROM_RSSI_BG			0x0023
#define EEPROM_RSSI_BG_OFFSET0		FIELD16(0x00ff)
#define EEPROM_RSSI_BG_OFFSET1		FIELD16(0xff00)

/*
 * EEPROM RSSI BG2 offset
 */
#define EEPROM_RSSI_BG2			0x0024
#define EEPROM_RSSI_BG2_OFFSET2		FIELD16(0x00ff)
#define EEPROM_RSSI_BG2_LNA_A1		FIELD16(0xff00)

/*
 * EEPROM RSSI A offset
 */
#define EEPROM_RSSI_A			0x0025
#define EEPROM_RSSI_A_OFFSET0		FIELD16(0x00ff)
#define EEPROM_RSSI_A_OFFSET1		FIELD16(0xff00)

/*
 * EEPROM RSSI A2 offset
 */
#define EEPROM_RSSI_A2			0x0026
#define EEPROM_RSSI_A2_OFFSET2		FIELD16(0x00ff)
#define EEPROM_RSSI_A2_LNA_A2		FIELD16(0xff00)

/*
 * EEPROM TXpower delta: 20MHZ AND 40 MHZ use different power.
 *	This is delta in 40MHZ.
 * VALUE: Tx Power dalta value (MAX=4)
 * TYPE: 1: Plus the delta value, 0: minus the delta value
 * TXPOWER: Enable:
 */
#define EEPROM_TXPOWER_DELTA		0x0028
#define EEPROM_TXPOWER_DELTA_VALUE	FIELD16(0x003f)
#define EEPROM_TXPOWER_DELTA_TYPE	FIELD16(0x0040)
#define EEPROM_TXPOWER_DELTA_TXPOWER	FIELD16(0x0080)

/*
 * EEPROM TXPOWER 802.11BG
 */
#define	EEPROM_TXPOWER_BG1		0x0029
#define	EEPROM_TXPOWER_BG2		0x0030
#define EEPROM_TXPOWER_BG_SIZE		7
#define EEPROM_TXPOWER_BG_1		FIELD16(0x00ff)
#define EEPROM_TXPOWER_BG_2		FIELD16(0xff00)

/*
 * EEPROM TXPOWER 802.11A
 */
#define EEPROM_TXPOWER_A1		0x003c
#define EEPROM_TXPOWER_A2		0x0053
#define EEPROM_TXPOWER_A_SIZE		6
#define EEPROM_TXPOWER_A_1		FIELD16(0x00ff)
#define EEPROM_TXPOWER_A_2		FIELD16(0xff00)

/*
 * EEPROM TXpower byrate: 20MHZ power
 */
#define EEPROM_TXPOWER_BYRATE		0x006f

/*
 * EEPROM BBP.
 */
#define	EEPROM_BBP_START		0x0078
#define EEPROM_BBP_SIZE			16
#define EEPROM_BBP_VALUE		FIELD16(0x00ff)
#define EEPROM_BBP_REG_ID		FIELD16(0xff00)

/*
 * MCU mailbox commands.
 */
#define MCU_SLEEP			0x30
#define MCU_WAKEUP			0x31
#define MCU_RADIO_OFF			0x35
#define MCU_CURRENT			0x36
#define MCU_LED				0x50
#define MCU_LED_STRENGTH		0x51
#define MCU_LED_1			0x52
#define MCU_LED_2			0x53
#define MCU_LED_3			0x54
#define MCU_RADAR			0x60
#define MCU_BOOT_SIGNAL			0x72
#define MCU_BBP_SIGNAL			0x80
#define MCU_POWER_SAVE			0x83

/*
 * MCU mailbox tokens
 */
#define TOKEN_WAKUP			3

/*
 * DMA descriptor defines.
 */
#define TXD_DESC_SIZE			( 4 * sizeof(__le32) )
#define TXINFO_DESC_SIZE		( 1 * sizeof(__le32) )
#define TXWI_DESC_SIZE			( 4 * sizeof(__le32) )
#define RXD_DESC_SIZE			( 1 * sizeof(__le32) )
#define RXWI_DESC_SIZE			( 4 * sizeof(__le32) )

/*
 * TX descriptor format for TX, PRIO and Beacon Ring.
 */

/*
 * Word0
 */
#define TXD_W0_SD_PTR0			FIELD32(0xffffffff)

/*
 * Word1
 */
#define TXD_W1_SD_LEN1			FIELD32(0x00003fff)
#define TXD_W1_LAST_SEC1		FIELD32(0x00004000)
#define TXD_W1_BURST			FIELD32(0x00008000)
#define TXD_W1_SD_LEN0			FIELD32(0x3fff0000)
#define TXD_W1_LAST_SEC0		FIELD32(0x40000000)
#define TXD_W1_DMA_DONE			FIELD32(0x80000000)

/*
 * Word2
 */
#define TXD_W2_SD_PTR1			FIELD32(0xffffffff)

/*
 * Word3
 * WIV: Wireless Info Valid. 1: Driver filled WI, 0: DMA needs to copy WI
 * QSEL: Select on-chip FIFO ID for 2nd-stage output scheduler.
 *       0:MGMT, 1:HCCA 2:EDCA
 */
#define TXD_W3_WIV			FIELD32(0x01000000)
#define TXD_W3_QSEL			FIELD32(0x06000000)
#define TXD_W3_TCO			FIELD32(0x20000000)
#define TXD_W3_UCO			FIELD32(0x40000000)
#define TXD_W3_ICO			FIELD32(0x80000000)

/*
 * TX Info structure
 */

/*
 * Word0
 * WIV: Wireless Info Valid. 1: Driver filled WI,  0: DMA needs to copy WI
 * QSEL: Select on-chip FIFO ID for 2nd-stage output scheduler.
 *       0:MGMT, 1:HCCA 2:EDCA
 * USB_DMA_NEXT_VALID: Used ONLY in USB bulk Aggregation, NextValid
 * DMA_TX_BURST: used ONLY in USB bulk Aggregation.
 *               Force USB DMA transmit frame from current selected endpoint
 */
#define TXINFO_W0_USB_DMA_TX_PKT_LEN	FIELD32(0x0000ffff)
#define TXINFO_W0_WIV			FIELD32(0x01000000)
#define TXINFO_W0_QSEL			FIELD32(0x06000000)
#define TXINFO_W0_SW_USE_LAST_ROUND	FIELD32(0x08000000)
#define TXINFO_W0_USB_DMA_NEXT_VALID	FIELD32(0x40000000)
#define TXINFO_W0_USB_DMA_TX_BURST	FIELD32(0x80000000)

/*
 * TX WI structure
 */

/*
 * Word0
 * FRAG: 1 To inform TKIP engine this is a fragment.
 * MIMO_PS: The remote peer is in dynamic MIMO-PS mode
 * TX_OP: 0:HT TXOP rule , 1:PIFS TX ,2:Backoff, 3:sifs
 * BW: Channel bandwidth 20MHz or 40 MHz
 * STBC: 1: STBC support MCS =0-7, 2,3 : RESERVED
 */
#define TXWI_W0_FRAG			FIELD32(0x00000001)
#define TXWI_W0_MIMO_PS			FIELD32(0x00000002)
#define TXWI_W0_CF_ACK			FIELD32(0x00000004)
#define TXWI_W0_TS			FIELD32(0x00000008)
#define TXWI_W0_AMPDU			FIELD32(0x00000010)
#define TXWI_W0_MPDU_DENSITY		FIELD32(0x000000e0)
#define TXWI_W0_TX_OP			FIELD32(0x00000300)
#define TXWI_W0_MCS			FIELD32(0x007f0000)
#define TXWI_W0_BW			FIELD32(0x00800000)
#define TXWI_W0_SHORT_GI		FIELD32(0x01000000)
#define TXWI_W0_STBC			FIELD32(0x06000000)
#define TXWI_W0_IFS			FIELD32(0x08000000)
#define TXWI_W0_PHYMODE			FIELD32(0xc0000000)

/*
 * Word1
 */
#define TXWI_W1_ACK			FIELD32(0x00000001)
#define TXWI_W1_NSEQ			FIELD32(0x00000002)
#define TXWI_W1_BW_WIN_SIZE		FIELD32(0x000000fc)
#define TXWI_W1_WIRELESS_CLI_ID		FIELD32(0x0000ff00)
#define TXWI_W1_MPDU_TOTAL_BYTE_COUNT	FIELD32(0x0fff0000)
#define TXWI_W1_PACKETID		FIELD32(0xf0000000)

/*
 * Word2
 */
#define TXWI_W2_IV			FIELD32(0xffffffff)

/*
 * Word3
 */
#define TXWI_W3_EIV			FIELD32(0xffffffff)

/*
 * RX descriptor format for RX Ring.
 */

/*
 * Word0
 * UNICAST_TO_ME: This RX frame is unicast to me.
 * MULTICAST: This is a multicast frame.
 * BROADCAST: This is a broadcast frame.
 * MY_BSS: this frame belongs to the same BSSID.
 * CRC_ERROR: CRC error.
 * CIPHER_ERROR: 0: decryption okay, 1:ICV error, 2:MIC error, 3:KEY not valid.
 * AMSDU: rx with 802.3 header, not 802.11 header.
 */

#define RXD_W0_BA			FIELD32(0x00000001)
#define RXD_W0_DATA			FIELD32(0x00000002)
#define RXD_W0_NULLDATA			FIELD32(0x00000004)
#define RXD_W0_FRAG			FIELD32(0x00000008)
#define RXD_W0_UNICAST_TO_ME		FIELD32(0x00000010)
#define RXD_W0_MULTICAST		FIELD32(0x00000020)
#define RXD_W0_BROADCAST		FIELD32(0x00000040)
#define RXD_W0_MY_BSS			FIELD32(0x00000080)
#define RXD_W0_CRC_ERROR		FIELD32(0x00000100)
#define RXD_W0_CIPHER_ERROR		FIELD32(0x00000600)
#define RXD_W0_AMSDU			FIELD32(0x00000800)
#define RXD_W0_HTC			FIELD32(0x00001000)
#define RXD_W0_RSSI			FIELD32(0x00002000)
#define RXD_W0_L2PAD			FIELD32(0x00004000)
#define RXD_W0_AMPDU			FIELD32(0x00008000)
#define RXD_W0_DECRYPTED		FIELD32(0x00010000)
#define RXD_W0_PLCP_RSSI		FIELD32(0x00020000)
#define RXD_W0_CIPHER_ALG		FIELD32(0x00040000)
#define RXD_W0_LAST_AMSDU		FIELD32(0x00080000)
#define RXD_W0_PLCP_SIGNAL		FIELD32(0xfff00000)

/*
 * RX WI structure
 */

/*
 * Word0
 */
#define RXWI_W0_WIRELESS_CLI_ID		FIELD32(0x000000ff)
#define RXWI_W0_KEY_INDEX		FIELD32(0x00000300)
#define RXWI_W0_BSSID			FIELD32(0x00001c00)
#define RXWI_W0_UDF			FIELD32(0x0000e000)
#define RXWI_W0_MPDU_TOTAL_BYTE_COUNT	FIELD32(0x0fff0000)
#define RXWI_W0_TID			FIELD32(0xf0000000)

/*
 * Word1
 */
#define RXWI_W1_FRAG			FIELD32(0x0000000f)
#define RXWI_W1_SEQUENCE		FIELD32(0x0000fff0)
#define RXWI_W1_MCS			FIELD32(0x007f0000)
#define RXWI_W1_BW			FIELD32(0x00800000)
#define RXWI_W1_SHORT_GI		FIELD32(0x01000000)
#define RXWI_W1_STBC			FIELD32(0x06000000)
#define RXWI_W1_PHYMODE			FIELD32(0xc0000000)

/*
 * Word2
 */
#define RXWI_W2_RSSI0			FIELD32(0x000000ff)
#define RXWI_W2_RSSI1			FIELD32(0x0000ff00)
#define RXWI_W2_RSSI2			FIELD32(0x00ff0000)

/*
 * Word3
 */
#define RXWI_W3_SNR0			FIELD32(0x000000ff)
#define RXWI_W3_SNR1			FIELD32(0x0000ff00)

/*
 * Macros for converting txpower from EEPROM to mac80211 value
 * and from mac80211 value to register value.
 */
#define MIN_G_TXPOWER	0
#define MIN_A_TXPOWER	-7
#define MAX_G_TXPOWER	31
#define MAX_A_TXPOWER	15
#define DEFAULT_TXPOWER	5

#define TXPOWER_G_FROM_DEV(__txpower) \
	((__txpower) > MAX_G_TXPOWER) ? DEFAULT_TXPOWER : (__txpower)

#define TXPOWER_G_TO_DEV(__txpower) \
	clamp_t(char, __txpower, MIN_G_TXPOWER, MAX_G_TXPOWER)

#define TXPOWER_A_FROM_DEV(__txpower) \
	((__txpower) > MAX_A_TXPOWER) ? DEFAULT_TXPOWER : (__txpower)

#define TXPOWER_A_TO_DEV(__txpower) \
	clamp_t(char, __txpower, MIN_A_TXPOWER, MAX_A_TXPOWER)

#endif /* RT2800USB_H */
