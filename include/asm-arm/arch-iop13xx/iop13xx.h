#ifndef _IOP13XX_HW_H_
#define _IOP13XX_HW_H_

#ifndef __ASSEMBLY__
/* The ATU offsets can change based on the strapping */
extern u32 iop13xx_atux_pmmr_offset;
extern u32 iop13xx_atue_pmmr_offset;
void iop13xx_init_irq(void);
void iop13xx_map_io(void);
void iop13xx_platform_init(void);
void iop13xx_add_tpmi_devices(void);
void iop13xx_init_irq(void);

/* CPUID CP6 R0 Page 0 */
static inline int iop13xx_cpu_id(void)
{
	int id;
	asm volatile("mrc p6, 0, %0, c0, c0, 0":"=r" (id));
	return id;
}

#endif

/*
 * IOP13XX I/O and Mem space regions for PCI autoconfiguration
 */
#define IOP13XX_MAX_RAM_SIZE    0x80000000UL  /* 2GB */
#define IOP13XX_PCI_OFFSET	 IOP13XX_MAX_RAM_SIZE

/* PCI MAP
 * bus range		cpu phys	cpu virt	note
 * 0x0000.0000 + 2GB	(n/a)		(n/a)		inbound, 1:1 mapping with Physical RAM
 * 0x8000.0000 + 928M	0x1.8000.0000   (ioremap)	PCIX outbound memory window
 * 0x8000.0000 + 928M	0x2.8000.0000   (ioremap)	PCIE outbound memory window
 *
 * IO MAP
 * 0x1000 + 64K	0x0.fffb.1000	0xfec6.1000	PCIX outbound i/o window
 * 0x1000 + 64K	0x0.fffd.1000	0xfed7.1000	PCIE outbound i/o window
 */
#define IOP13XX_PCIX_IO_WINDOW_SIZE   0x10000UL
#define IOP13XX_PCIX_LOWER_IO_PA      0xfffb0000UL
#define IOP13XX_PCIX_LOWER_IO_VA      0xfec60000UL
#define IOP13XX_PCIX_LOWER_IO_BA      0x0UL /* OIOTVR */
#define IOP13XX_PCIX_IO_BUS_OFFSET    0x1000UL
#define IOP13XX_PCIX_UPPER_IO_PA      (IOP13XX_PCIX_LOWER_IO_PA +\
				       IOP13XX_PCIX_IO_WINDOW_SIZE - 1)
#define IOP13XX_PCIX_UPPER_IO_VA      (IOP13XX_PCIX_LOWER_IO_VA +\
				       IOP13XX_PCIX_IO_WINDOW_SIZE - 1)
#define IOP13XX_PCIX_IO_PHYS_TO_VIRT(addr) (u32) ((u32) addr -\
					   (IOP13XX_PCIX_LOWER_IO_PA\
					   - IOP13XX_PCIX_LOWER_IO_VA))

#define IOP13XX_PCIX_MEM_PHYS_OFFSET  0x100000000ULL
#define IOP13XX_PCIX_MEM_WINDOW_SIZE  0x3a000000UL
#define IOP13XX_PCIX_LOWER_MEM_BA     (PHYS_OFFSET + IOP13XX_PCI_OFFSET)
#define IOP13XX_PCIX_LOWER_MEM_PA     (IOP13XX_PCIX_MEM_PHYS_OFFSET +\
				       IOP13XX_PCIX_LOWER_MEM_BA)
#define IOP13XX_PCIX_UPPER_MEM_PA     (IOP13XX_PCIX_LOWER_MEM_PA +\
				       IOP13XX_PCIX_MEM_WINDOW_SIZE - 1)
#define IOP13XX_PCIX_UPPER_MEM_BA     (IOP13XX_PCIX_LOWER_MEM_BA +\
				       IOP13XX_PCIX_MEM_WINDOW_SIZE - 1)

#define IOP13XX_PCIX_MEM_COOKIE        0x80000000UL
#define IOP13XX_PCIX_LOWER_MEM_RA      IOP13XX_PCIX_MEM_COOKIE
#define IOP13XX_PCIX_UPPER_MEM_RA      (IOP13XX_PCIX_LOWER_MEM_RA +\
					IOP13XX_PCIX_MEM_WINDOW_SIZE - 1)
#define IOP13XX_PCIX_MEM_OFFSET        (IOP13XX_PCIX_MEM_COOKIE -\
					IOP13XX_PCIX_LOWER_MEM_BA)

/* PCI-E ranges */
#define IOP13XX_PCIE_IO_WINDOW_SIZE   	 0x10000UL
#define IOP13XX_PCIE_LOWER_IO_PA      	 0xfffd0000UL
#define IOP13XX_PCIE_LOWER_IO_VA      	 0xfed70000UL
#define IOP13XX_PCIE_LOWER_IO_BA      	 0x0UL  /* OIOTVR */
#define IOP13XX_PCIE_IO_BUS_OFFSET	 0x1000UL
#define IOP13XX_PCIE_UPPER_IO_PA      	 (IOP13XX_PCIE_LOWER_IO_PA +\
					 IOP13XX_PCIE_IO_WINDOW_SIZE - 1)
#define IOP13XX_PCIE_UPPER_IO_VA      	 (IOP13XX_PCIE_LOWER_IO_VA +\
					 IOP13XX_PCIE_IO_WINDOW_SIZE - 1)
#define IOP13XX_PCIE_UPPER_IO_BA      	 (IOP13XX_PCIE_LOWER_IO_BA +\
					 IOP13XX_PCIE_IO_WINDOW_SIZE - 1)
#define IOP13XX_PCIE_IO_PHYS_TO_VIRT(addr) (u32) ((u32) addr -\
					   (IOP13XX_PCIE_LOWER_IO_PA\
					   - IOP13XX_PCIE_LOWER_IO_VA))

#define IOP13XX_PCIE_MEM_PHYS_OFFSET  	 0x200000000ULL
#define IOP13XX_PCIE_MEM_WINDOW_SIZE  	 0x3a000000UL
#define IOP13XX_PCIE_LOWER_MEM_BA     	 (PHYS_OFFSET + IOP13XX_PCI_OFFSET)
#define IOP13XX_PCIE_LOWER_MEM_PA     	 (IOP13XX_PCIE_MEM_PHYS_OFFSET +\
					 IOP13XX_PCIE_LOWER_MEM_BA)
#define IOP13XX_PCIE_UPPER_MEM_PA     	 (IOP13XX_PCIE_LOWER_MEM_PA +\
					 IOP13XX_PCIE_MEM_WINDOW_SIZE - 1)
#define IOP13XX_PCIE_UPPER_MEM_BA     	 (IOP13XX_PCIE_LOWER_MEM_BA +\
					 IOP13XX_PCIE_MEM_WINDOW_SIZE - 1)

/* All 0xc000.0000 - 0xfdff.ffff addresses belong to PCIe */
#define IOP13XX_PCIE_MEM_COOKIE       	 0xc0000000UL
#define IOP13XX_PCIE_LOWER_MEM_RA     	 IOP13XX_PCIE_MEM_COOKIE
#define IOP13XX_PCIE_UPPER_MEM_RA     	 (IOP13XX_PCIE_LOWER_MEM_RA +\
					 IOP13XX_PCIE_MEM_WINDOW_SIZE - 1)
#define IOP13XX_PCIE_MEM_OFFSET       	 (IOP13XX_PCIE_MEM_COOKIE -\
					 IOP13XX_PCIE_LOWER_MEM_BA)

/* PBI Ranges */
#define IOP13XX_PBI_LOWER_MEM_PA	  0xf0000000UL
#define IOP13XX_PBI_MEM_WINDOW_SIZE	  0x04000000UL
#define IOP13XX_PBI_MEM_COOKIE		  0xfa000000UL
#define IOP13XX_PBI_LOWER_MEM_RA	  IOP13XX_PBI_MEM_COOKIE
#define IOP13XX_PBI_UPPER_MEM_RA	  (IOP13XX_PBI_LOWER_MEM_RA +\
					  IOP13XX_PBI_MEM_WINDOW_SIZE - 1)

/*
 * IOP13XX chipset registers
 */
#define IOP13XX_PMMR_PHYS_MEM_BASE	   0xffd80000UL  /* PMMR phys. address */
#define IOP13XX_PMMR_VIRT_MEM_BASE	   0xfee80000UL  /* PMMR phys. address */
#define IOP13XX_PMMR_MEM_WINDOW_SIZE	   0x80000
#define IOP13XX_PMMR_UPPER_MEM_VA	   (IOP13XX_PMMR_VIRT_MEM_BASE +\
					   IOP13XX_PMMR_MEM_WINDOW_SIZE - 1)
#define IOP13XX_PMMR_UPPER_MEM_PA	   (IOP13XX_PMMR_PHYS_MEM_BASE +\
					   IOP13XX_PMMR_MEM_WINDOW_SIZE - 1)
#define IOP13XX_PMMR_VIRT_TO_PHYS(addr)   (u32) ((u32) addr +\
					   (IOP13XX_PMMR_PHYS_MEM_BASE\
					   - IOP13XX_PMMR_VIRT_MEM_BASE))
#define IOP13XX_PMMR_PHYS_TO_VIRT(addr)   (u32) ((u32) addr -\
					   (IOP13XX_PMMR_PHYS_MEM_BASE\
					   - IOP13XX_PMMR_VIRT_MEM_BASE))
#define IOP13XX_REG_ADDR32(reg)     	   (IOP13XX_PMMR_VIRT_MEM_BASE + (reg))
#define IOP13XX_REG_ADDR16(reg)     	   (IOP13XX_PMMR_VIRT_MEM_BASE + (reg))
#define IOP13XX_REG_ADDR8(reg)      	   (IOP13XX_PMMR_VIRT_MEM_BASE + (reg))
#define IOP13XX_REG_ADDR32_PHYS(reg)      (IOP13XX_PMMR_PHYS_MEM_BASE + (reg))
#define IOP13XX_REG_ADDR16_PHYS(reg)      (IOP13XX_PMMR_PHYS_MEM_BASE + (reg))
#define IOP13XX_REG_ADDR8_PHYS(reg)       (IOP13XX_PMMR_PHYS_MEM_BASE + (reg))
#define IOP13XX_PMMR_SIZE		   0x00080000

/*=================== Defines for Platform Devices =====================*/
#define IOP13XX_UART0_PHYS  (IOP13XX_PMMR_PHYS_MEM_BASE | 0x00002300)
#define IOP13XX_UART1_PHYS  (IOP13XX_PMMR_PHYS_MEM_BASE | 0x00002340)
#define IOP13XX_UART0_VIRT  (IOP13XX_PMMR_VIRT_MEM_BASE | 0x00002300)
#define IOP13XX_UART1_VIRT  (IOP13XX_PMMR_VIRT_MEM_BASE | 0x00002340)

#define IOP13XX_I2C0_PHYS   (IOP13XX_PMMR_PHYS_MEM_BASE | 0x00002500)
#define IOP13XX_I2C1_PHYS   (IOP13XX_PMMR_PHYS_MEM_BASE | 0x00002520)
#define IOP13XX_I2C2_PHYS   (IOP13XX_PMMR_PHYS_MEM_BASE | 0x00002540)
#define IOP13XX_I2C0_VIRT   (IOP13XX_PMMR_VIRT_MEM_BASE | 0x00002500)
#define IOP13XX_I2C1_VIRT   (IOP13XX_PMMR_VIRT_MEM_BASE | 0x00002520)
#define IOP13XX_I2C2_VIRT   (IOP13XX_PMMR_VIRT_MEM_BASE | 0x00002540)

/* ATU selection flags */
/* IOP13XX_INIT_ATU_DEFAULT = Rely on CONFIG_IOP13XX_ATU* */
#define IOP13XX_INIT_ATU_DEFAULT     (0)
#define IOP13XX_INIT_ATU_ATUX	      (1 << 0)
#define IOP13XX_INIT_ATU_ATUE	      (1 << 1)
#define IOP13XX_INIT_ATU_NONE	      (1 << 2)

/* UART selection flags */
/* IOP13XX_INIT_UART_DEFAULT = Rely on CONFIG_IOP13XX_UART* */
#define IOP13XX_INIT_UART_DEFAULT    (0)
#define IOP13XX_INIT_UART_0	      (1 << 0)
#define IOP13XX_INIT_UART_1	      (1 << 1)

/* I2C selection flags */
/* IOP13XX_INIT_I2C_DEFAULT = Rely on CONFIG_IOP13XX_I2C* */
#define IOP13XX_INIT_I2C_DEFAULT     (0)
#define IOP13XX_INIT_I2C_0	      (1 << 0)
#define IOP13XX_INIT_I2C_1	      (1 << 1)
#define IOP13XX_INIT_I2C_2	      (1 << 2)

#define IQ81340_NUM_UART     2
#define IQ81340_NUM_I2C      3
#define IQ81340_NUM_PHYS_MAP_FLASH 1
#define IQ81340_MAX_PLAT_DEVICES (IQ81340_NUM_UART +\
				IQ81340_NUM_I2C +\
				IQ81340_NUM_PHYS_MAP_FLASH)

/*========================== PMMR offsets for key registers ============*/
#define IOP13XX_ATU0_PMMR_OFFSET   	0x00048000
#define IOP13XX_ATU1_PMMR_OFFSET   	0x0004c000
#define IOP13XX_ATU2_PMMR_OFFSET   	0x0004d000
#define IOP13XX_ADMA0_PMMR_OFFSET  	0x00000000
#define IOP13XX_ADMA1_PMMR_OFFSET  	0x00000200
#define IOP13XX_ADMA2_PMMR_OFFSET  	0x00000400
#define IOP13XX_PBI_PMMR_OFFSET    	0x00001580
#define IOP13XX_ESSR0_PMMR_OFFSET  	0x00002188
#define IOP13XX_ESSR0			IOP13XX_REG_ADDR32(0x00002188)

#define IOP13XX_ESSR0_IFACE_MASK   	0x00004000  /* Interface PCI-X / PCI-E */
#define IOP13XX_CONTROLLER_ONLY    	(1 << 14)
#define IOP13XX_INTERFACE_SEL_PCIX 	(1 << 15)

#define IOP13XX_PMON_PMMR_OFFSET	0x0001A000
#define IOP13XX_PMON_BASE		(IOP13XX_PMMR_VIRT_MEM_BASE +\
					IOP13XX_PMON_PMMR_OFFSET)
#define IOP13XX_PMON_PHYSBASE		(IOP13XX_PMMR_PHYS_MEM_BASE +\
					IOP13XX_PMON_PMMR_OFFSET)

#define IOP13XX_PMON_CMD0		(IOP13XX_PMON_BASE + 0x0)
#define IOP13XX_PMON_EVR0		(IOP13XX_PMON_BASE + 0x4)
#define IOP13XX_PMON_STS0		(IOP13XX_PMON_BASE + 0x8)
#define IOP13XX_PMON_DATA0		(IOP13XX_PMON_BASE + 0xC)

#define IOP13XX_PMON_CMD3		(IOP13XX_PMON_BASE + 0x30)
#define IOP13XX_PMON_EVR3		(IOP13XX_PMON_BASE + 0x34)
#define IOP13XX_PMON_STS3		(IOP13XX_PMON_BASE + 0x38)
#define IOP13XX_PMON_DATA3		(IOP13XX_PMON_BASE + 0x3C)

#define IOP13XX_PMON_CMD7		(IOP13XX_PMON_BASE + 0x70)
#define IOP13XX_PMON_EVR7		(IOP13XX_PMON_BASE + 0x74)
#define IOP13XX_PMON_STS7		(IOP13XX_PMON_BASE + 0x78)
#define IOP13XX_PMON_DATA7		(IOP13XX_PMON_BASE + 0x7C)

#define IOP13XX_PMONEN			(IOP13XX_PMMR_VIRT_MEM_BASE + 0x4E040)
#define IOP13XX_PMONSTAT		(IOP13XX_PMMR_VIRT_MEM_BASE + 0x4E044)

/*================================ATU===================================*/
#define IOP13XX_ATUX_OFFSET(ofs)	IOP13XX_REG_ADDR32(\
					iop13xx_atux_pmmr_offset + (ofs))

#define IOP13XX_ATUX_DID		IOP13XX_REG_ADDR16(\
					iop13xx_atux_pmmr_offset + 0x2)

#define IOP13XX_ATUX_ATUCMD		IOP13XX_REG_ADDR16(\
					iop13xx_atux_pmmr_offset + 0x4)
#define IOP13XX_ATUX_ATUSR		IOP13XX_REG_ADDR16(\
					iop13xx_atux_pmmr_offset + 0x6)

#define IOP13XX_ATUX_IABAR0   		IOP13XX_ATUX_OFFSET(0x10)
#define IOP13XX_ATUX_IAUBAR0  		IOP13XX_ATUX_OFFSET(0x14)
#define IOP13XX_ATUX_IABAR1   		IOP13XX_ATUX_OFFSET(0x18)
#define IOP13XX_ATUX_IAUBAR1  		IOP13XX_ATUX_OFFSET(0x1c)
#define IOP13XX_ATUX_IABAR2   		IOP13XX_ATUX_OFFSET(0x20)
#define IOP13XX_ATUX_IAUBAR2  		IOP13XX_ATUX_OFFSET(0x24)
#define IOP13XX_ATUX_IALR0    		IOP13XX_ATUX_OFFSET(0x40)
#define IOP13XX_ATUX_IATVR0   		IOP13XX_ATUX_OFFSET(0x44)
#define IOP13XX_ATUX_IAUTVR0  		IOP13XX_ATUX_OFFSET(0x48)
#define IOP13XX_ATUX_IALR1    		IOP13XX_ATUX_OFFSET(0x4c)
#define IOP13XX_ATUX_IATVR1   		IOP13XX_ATUX_OFFSET(0x50)
#define IOP13XX_ATUX_IAUTVR1  		IOP13XX_ATUX_OFFSET(0x54)
#define IOP13XX_ATUX_IALR2    		IOP13XX_ATUX_OFFSET(0x58)
#define IOP13XX_ATUX_IATVR2   		IOP13XX_ATUX_OFFSET(0x5c)
#define IOP13XX_ATUX_IAUTVR2  		IOP13XX_ATUX_OFFSET(0x60)
#define IOP13XX_ATUX_ATUCR    		IOP13XX_ATUX_OFFSET(0x70)
#define IOP13XX_ATUX_PCSR     		IOP13XX_ATUX_OFFSET(0x74)
#define IOP13XX_ATUX_ATUISR   		IOP13XX_ATUX_OFFSET(0x78)
#define IOP13XX_ATUX_PCIXSR   		IOP13XX_ATUX_OFFSET(0xD4)
#define IOP13XX_ATUX_IABAR3   		IOP13XX_ATUX_OFFSET(0x200)
#define IOP13XX_ATUX_IAUBAR3  		IOP13XX_ATUX_OFFSET(0x204)
#define IOP13XX_ATUX_IALR3    		IOP13XX_ATUX_OFFSET(0x208)
#define IOP13XX_ATUX_IATVR3   		IOP13XX_ATUX_OFFSET(0x20c)
#define IOP13XX_ATUX_IAUTVR3  		IOP13XX_ATUX_OFFSET(0x210)

#define IOP13XX_ATUX_OIOBAR   		IOP13XX_ATUX_OFFSET(0x300)
#define IOP13XX_ATUX_OIOWTVR  		IOP13XX_ATUX_OFFSET(0x304)
#define IOP13XX_ATUX_OUMBAR0  		IOP13XX_ATUX_OFFSET(0x308)
#define IOP13XX_ATUX_OUMWTVR0 		IOP13XX_ATUX_OFFSET(0x30c)
#define IOP13XX_ATUX_OUMBAR1  		IOP13XX_ATUX_OFFSET(0x310)
#define IOP13XX_ATUX_OUMWTVR1 		IOP13XX_ATUX_OFFSET(0x314)
#define IOP13XX_ATUX_OUMBAR2  		IOP13XX_ATUX_OFFSET(0x318)
#define IOP13XX_ATUX_OUMWTVR2 		IOP13XX_ATUX_OFFSET(0x31c)
#define IOP13XX_ATUX_OUMBAR3  		IOP13XX_ATUX_OFFSET(0x320)
#define IOP13XX_ATUX_OUMWTVR3 		IOP13XX_ATUX_OFFSET(0x324)
#define IOP13XX_ATUX_OUDMABAR 		IOP13XX_ATUX_OFFSET(0x328)
#define IOP13XX_ATUX_OUMSIBAR 		IOP13XX_ATUX_OFFSET(0x32c)
#define IOP13XX_ATUX_OCCAR    		IOP13XX_ATUX_OFFSET(0x330)
#define IOP13XX_ATUX_OCCDR    		IOP13XX_ATUX_OFFSET(0x334)

#define IOP13XX_ATUX_ATUCR_OUT_EN		(1 << 1)
#define IOP13XX_ATUX_PCSR_CENTRAL_RES		(1 << 25)
#define IOP13XX_ATUX_PCSR_P_RSTOUT		(1 << 21)
#define IOP13XX_ATUX_PCSR_OUT_Q_BUSY		(1 << 15)
#define IOP13XX_ATUX_PCSR_IN_Q_BUSY		(1 << 14)
#define IOP13XX_ATUX_PCSR_FREQ_OFFSET		(16)

#define IOP13XX_ATUX_STAT_PCI_IFACE_ERR	(1 << 18)
#define IOP13XX_ATUX_STAT_VPD_ADDR		(1 << 17)
#define IOP13XX_ATUX_STAT_INT_PAR_ERR		(1 << 16)
#define IOP13XX_ATUX_STAT_CFG_WRITE		(1 << 15)
#define IOP13XX_ATUX_STAT_ERR_COR		(1 << 14)
#define IOP13XX_ATUX_STAT_TX_SCEM		(1 << 13)
#define IOP13XX_ATUX_STAT_REC_SCEM		(1 << 12)
#define IOP13XX_ATUX_STAT_POWER_TRAN	 	(1 << 11)
#define IOP13XX_ATUX_STAT_TX_SERR		(1 << 10)
#define IOP13XX_ATUX_STAT_DET_PAR_ERR	 	(1 << 9	)
#define IOP13XX_ATUX_STAT_BIST			(1 << 8	)
#define IOP13XX_ATUX_STAT_INT_REC_MABORT 	(1 << 7	)
#define IOP13XX_ATUX_STAT_REC_SERR		(1 << 4	)
#define IOP13XX_ATUX_STAT_EXT_REC_MABORT 	(1 << 3	)
#define IOP13XX_ATUX_STAT_EXT_REC_TABORT 	(1 << 2	)
#define IOP13XX_ATUX_STAT_EXT_SIG_TABORT 	(1 << 1	)
#define IOP13XX_ATUX_STAT_MASTER_DATA_PAR	(1 << 0	)

#define IOP13XX_ATUX_PCIXSR_BUS_NUM	(8)
#define IOP13XX_ATUX_PCIXSR_DEV_NUM	(3)
#define IOP13XX_ATUX_PCIXSR_FUNC_NUM	(0)

#define IOP13XX_ATUX_IALR_DISABLE  	0x00000001
#define IOP13XX_ATUX_OUMBAR_ENABLE 	0x80000000

#define IOP13XX_ATUE_OFFSET(ofs)	IOP13XX_REG_ADDR32(\
					iop13xx_atue_pmmr_offset + (ofs))

#define IOP13XX_ATUE_DID		IOP13XX_REG_ADDR16(\
					iop13xx_atue_pmmr_offset + 0x2)
#define IOP13XX_ATUE_ATUCMD		IOP13XX_REG_ADDR16(\
					iop13xx_atue_pmmr_offset + 0x4)
#define IOP13XX_ATUE_ATUSR		IOP13XX_REG_ADDR16(\
					iop13xx_atue_pmmr_offset + 0x6)

#define IOP13XX_ATUE_IABAR0		IOP13XX_ATUE_OFFSET(0x10)
#define IOP13XX_ATUE_IAUBAR0		IOP13XX_ATUE_OFFSET(0x14)
#define IOP13XX_ATUE_IABAR1		IOP13XX_ATUE_OFFSET(0x18)
#define IOP13XX_ATUE_IAUBAR1		IOP13XX_ATUE_OFFSET(0x1c)
#define IOP13XX_ATUE_IABAR2		IOP13XX_ATUE_OFFSET(0x20)
#define IOP13XX_ATUE_IAUBAR2		IOP13XX_ATUE_OFFSET(0x24)
#define IOP13XX_ATUE_IALR0		IOP13XX_ATUE_OFFSET(0x40)
#define IOP13XX_ATUE_IATVR0		IOP13XX_ATUE_OFFSET(0x44)
#define IOP13XX_ATUE_IAUTVR0		IOP13XX_ATUE_OFFSET(0x48)
#define IOP13XX_ATUE_IALR1		IOP13XX_ATUE_OFFSET(0x4c)
#define IOP13XX_ATUE_IATVR1		IOP13XX_ATUE_OFFSET(0x50)
#define IOP13XX_ATUE_IAUTVR1		IOP13XX_ATUE_OFFSET(0x54)
#define IOP13XX_ATUE_IALR2		IOP13XX_ATUE_OFFSET(0x58)
#define IOP13XX_ATUE_IATVR2		IOP13XX_ATUE_OFFSET(0x5c)
#define IOP13XX_ATUE_IAUTVR2		IOP13XX_ATUE_OFFSET(0x60)
#define IOP13XX_ATUE_PE_LSTS		IOP13XX_REG_ADDR16(\
					iop13xx_atue_pmmr_offset + 0xe2)
#define IOP13XX_ATUE_OIOWTVR		IOP13XX_ATUE_OFFSET(0x304)
#define IOP13XX_ATUE_OUMBAR0		IOP13XX_ATUE_OFFSET(0x308)
#define IOP13XX_ATUE_OUMWTVR0		IOP13XX_ATUE_OFFSET(0x30c)
#define IOP13XX_ATUE_OUMBAR1		IOP13XX_ATUE_OFFSET(0x310)
#define IOP13XX_ATUE_OUMWTVR1		IOP13XX_ATUE_OFFSET(0x314)
#define IOP13XX_ATUE_OUMBAR2		IOP13XX_ATUE_OFFSET(0x318)
#define IOP13XX_ATUE_OUMWTVR2		IOP13XX_ATUE_OFFSET(0x31c)
#define IOP13XX_ATUE_OUMBAR3		IOP13XX_ATUE_OFFSET(0x320)
#define IOP13XX_ATUE_OUMWTVR3		IOP13XX_ATUE_OFFSET(0x324)

#define IOP13XX_ATUE_ATUCR		IOP13XX_ATUE_OFFSET(0x70)
#define IOP13XX_ATUE_PCSR		IOP13XX_ATUE_OFFSET(0x74)
#define IOP13XX_ATUE_ATUISR		IOP13XX_ATUE_OFFSET(0x78)
#define IOP13XX_ATUE_OIOBAR		IOP13XX_ATUE_OFFSET(0x300)
#define IOP13XX_ATUE_OCCAR		IOP13XX_ATUE_OFFSET(0x32c)
#define IOP13XX_ATUE_OCCDR		IOP13XX_ATUE_OFFSET(0x330)

#define IOP13XX_ATUE_PIE_STS		IOP13XX_ATUE_OFFSET(0x384)
#define IOP13XX_ATUE_PIE_MSK		IOP13XX_ATUE_OFFSET(0x388)

#define IOP13XX_ATUE_ATUCR_IVM		(1 << 6)
#define IOP13XX_ATUE_ATUCR_OUT_EN	(1 << 1)
#define IOP13XX_ATUE_OCCAR_BUS_NUM	(24)
#define IOP13XX_ATUE_OCCAR_DEV_NUM	(19)
#define IOP13XX_ATUE_OCCAR_FUNC_NUM	(16)
#define IOP13XX_ATUE_OCCAR_EXT_REG	(8)
#define IOP13XX_ATUE_OCCAR_REG		(2)

#define IOP13XX_ATUE_PCSR_BUS_NUM	(24)
#define IOP13XX_ATUE_PCSR_DEV_NUM	(19)
#define IOP13XX_ATUE_PCSR_FUNC_NUM	(16)
#define IOP13XX_ATUE_PCSR_OUT_Q_BUSY	(1 << 15)
#define IOP13XX_ATUE_PCSR_IN_Q_BUSY	(1 << 14)
#define IOP13XX_ATUE_PCSR_END_POINT	(1 << 13)
#define IOP13XX_ATUE_PCSR_LLRB_BUSY	(1 << 12)

#define IOP13XX_ATUE_PCSR_BUS_NUM_MASK		(0xff)
#define IOP13XX_ATUE_PCSR_DEV_NUM_MASK		(0x1f)
#define IOP13XX_ATUE_PCSR_FUNC_NUM_MASK	(0x7)

#define IOP13XX_ATUE_PCSR_CORE_RESET		(8)
#define IOP13XX_ATUE_PCSR_FUNC_NUM		(16)

#define IOP13XX_ATUE_LSTS_TRAINING		(1 << 11)
#define IOP13XX_ATUE_STAT_SLOT_PWR_MSG		(1 << 28)
#define IOP13XX_ATUE_STAT_PME			(1 << 27)
#define IOP13XX_ATUE_STAT_HOT_PLUG_MSG		(1 << 26)
#define IOP13XX_ATUE_STAT_IVM			(1 << 25)
#define IOP13XX_ATUE_STAT_BIST			(1 << 24)
#define IOP13XX_ATUE_STAT_CFG_WRITE		(1 << 18)
#define IOP13XX_ATUE_STAT_VPD_ADDR		(1 << 17)
#define IOP13XX_ATUE_STAT_POWER_TRAN		(1 << 16)
#define IOP13XX_ATUE_STAT_HALT_ON_ERROR	(1 << 13)
#define IOP13XX_ATUE_STAT_ROOT_SYS_ERR		(1 << 12)
#define IOP13XX_ATUE_STAT_ROOT_ERR_MSG		(1 << 11)
#define IOP13XX_ATUE_STAT_PCI_IFACE_ERR	(1 << 10)
#define IOP13XX_ATUE_STAT_ERR_COR		(1 << 9	)
#define IOP13XX_ATUE_STAT_ERR_UNCOR		(1 << 8	)
#define IOP13XX_ATUE_STAT_CRS			(1 << 7	)
#define IOP13XX_ATUE_STAT_LNK_DWN		(1 << 6	)
#define IOP13XX_ATUE_STAT_INT_REC_MABORT	(1 << 5	)
#define IOP13XX_ATUE_STAT_DET_PAR_ERR		(1 << 4	)
#define IOP13XX_ATUE_STAT_EXT_REC_MABORT	(1 << 3	)
#define IOP13XX_ATUE_STAT_SIG_TABORT		(1 << 2	)
#define IOP13XX_ATUE_STAT_EXT_REC_TABORT	(1 << 1	)
#define IOP13XX_ATUE_STAT_MASTER_DATA_PAR	(1 << 0	)

#define IOP13XX_ATUE_ESTAT_REC_UNSUPPORTED_COMP_REQ	(1 << 31)
#define IOP13XX_ATUE_ESTAT_REC_COMPLETER_ABORT		(1 << 30)
#define IOP13XX_ATUE_ESTAT_TX_POISONED_TLP		(1 << 29)
#define IOP13XX_ATUE_ESTAT_TX_PAR_ERR			(1 << 28)
#define IOP13XX_ATUE_ESTAT_REC_UNSUPPORTED_REQ		(1 << 20)
#define IOP13XX_ATUE_ESTAT_REC_ECRC_ERR		(1 << 19)
#define IOP13XX_ATUE_ESTAT_REC_MALFORMED_TLP		(1 << 18)
#define IOP13XX_ATUE_ESTAT_TX_RECEIVER_OVERFLOW	(1 << 17)
#define IOP13XX_ATUE_ESTAT_REC_UNEXPECTED_COMP		(1 << 16)
#define IOP13XX_ATUE_ESTAT_INT_COMP_ABORT		(1 << 15)
#define IOP13XX_ATUE_ESTAT_COMP_TIMEOUT		(1 << 14)
#define IOP13XX_ATUE_ESTAT_FLOW_CONTROL_ERR		(1 << 13)
#define IOP13XX_ATUE_ESTAT_REC_POISONED_TLP		(1 << 12)
#define IOP13XX_ATUE_ESTAT_DATA_LNK_ERR		(1 << 4	)
#define IOP13XX_ATUE_ESTAT_TRAINING_ERR		(1 << 0	)

#define IOP13XX_ATUE_IALR_DISABLE   		(0x00000001)
#define IOP13XX_ATUE_OUMBAR_ENABLE  		(0x80000000)
#define IOP13XX_ATU_OUMBAR_FUNC_NUM  		(28)
#define IOP13XX_ATU_OUMBAR_FUNC_NUM_MASK  	(0x7)
/*=======================================================================*/

/*==============================ADMA UNITS===============================*/
#define IOP13XX_ADMA_PHYS_BASE(chan)	IOP13XX_REG_ADDR32_PHYS((chan << 9))
#define IOP13XX_ADMA_UPPER_PA(chan)	(IOP13XX_ADMA_PHYS_BASE(chan) + 0xc0)
#define IOP13XX_ADMA_OFFSET(chan, ofs)	IOP13XX_REG_ADDR32((chan << 9) + (ofs))

#define IOP13XX_ADMA_ACCR(chan)      IOP13XX_ADMA_OFFSET(chan, 0x0)
#define IOP13XX_ADMA_ACSR(chan)      IOP13XX_ADMA_OFFSET(chan, 0x4)
#define IOP13XX_ADMA_ADAR(chan)      IOP13XX_ADMA_OFFSET(chan, 0x8)
#define IOP13XX_ADMA_IIPCR(chan)     IOP13XX_ADMA_OFFSET(chan, 0x18)
#define IOP13XX_ADMA_IIPAR(chan)     IOP13XX_ADMA_OFFSET(chan, 0x1c)
#define IOP13XX_ADMA_IIPUAR(chan)    IOP13XX_ADMA_OFFSET(chan, 0x20)
#define IOP13XX_ADMA_ANDAR(chan)     IOP13XX_ADMA_OFFSET(chan, 0x24)
#define IOP13XX_ADMA_ADCR(chan)      IOP13XX_ADMA_OFFSET(chan, 0x28)
#define IOP13XX_ADMA_CARMD(chan)     IOP13XX_ADMA_OFFSET(chan, 0x2c)
#define IOP13XX_ADMA_ABCR(chan)      IOP13XX_ADMA_OFFSET(chan, 0x30)
#define IOP13XX_ADMA_DLADR(chan)     IOP13XX_ADMA_OFFSET(chan, 0x34)
#define IOP13XX_ADMA_DUADR(chan)     IOP13XX_ADMA_OFFSET(chan, 0x38)
#define IOP13XX_ADMA_SLAR(src, chan) IOP13XX_ADMA_OFFSET(chan, 0x3c + (src <<3))
#define IOP13XX_ADMA_SUAR(src, chan) IOP13XX_ADMA_OFFSET(chan, 0x40 + (src <<3))

/*==============================XSI BRIDGE===============================*/
#define IOP13XX_XBG_BECSR		IOP13XX_REG_ADDR32(0x178c)
#define IOP13XX_XBG_BERAR		IOP13XX_REG_ADDR32(0x1790)
#define IOP13XX_XBG_BERUAR		IOP13XX_REG_ADDR32(0x1794)
#define is_atue_occdr_error(x) 	((__raw_readl(IOP13XX_XBG_BERAR) == \
					IOP13XX_PMMR_VIRT_TO_PHYS(\
					IOP13XX_ATUE_OCCDR))\
					&& (__raw_readl(IOP13XX_XBG_BECSR) & 1))
#define is_atux_occdr_error(x) 	((__raw_readl(IOP13XX_XBG_BERAR) == \
					IOP13XX_PMMR_VIRT_TO_PHYS(\
					IOP13XX_ATUX_OCCDR))\
					&& (__raw_readl(IOP13XX_XBG_BECSR) & 1))
/*=======================================================================*/

#define IOP13XX_PBI_OFFSET(ofs) IOP13XX_REG_ADDR32(IOP13XX_PBI_PMMR_OFFSET +\
							(ofs))

#define IOP13XX_PBI_CR	       		IOP13XX_PBI_OFFSET(0x0)
#define IOP13XX_PBI_SR	       		IOP13XX_PBI_OFFSET(0x4)
#define IOP13XX_PBI_BAR0      		IOP13XX_PBI_OFFSET(0x8)
#define IOP13XX_PBI_LR0       		IOP13XX_PBI_OFFSET(0xc)
#define IOP13XX_PBI_BAR1      		IOP13XX_PBI_OFFSET(0x10)
#define IOP13XX_PBI_LR1       		IOP13XX_PBI_OFFSET(0x14)

#define IOP13XX_PROCESSOR_FREQ		IOP13XX_REG_ADDR32(0x2180)
#endif /* _IOP13XX_HW_H_ */
