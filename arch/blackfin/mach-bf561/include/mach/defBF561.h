
/*
 * File:         include/asm-blackfin/mach-bf561/defBF561.h
 * Based on:
 * Author:
 *
 * Created:
 * Description:
 * SYSTEM MMR REGISTER AND MEMORY MAP FOR ADSP-BF561
 * Rev:
 *
 * Modified:
 *
 * Bugs:         Enter bugs at http://blackfin.uclinux.org/
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; see the file COPYING.
 * If not, write to the Free Software Foundation,
 * 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#ifndef _DEF_BF561_H
#define _DEF_BF561_H
/*
#if !defined(__ADSPBF561__)
#warning defBF561.h should only be included for BF561 chip.
#endif
*/
/* include all Core registers and bit definitions */
#include <asm/def_LPBlackfin.h>

/*********************************************************************************** */
/* System MMR Register Map */
/*********************************************************************************** */

/* Clock and System Control (0xFFC00000 - 0xFFC000FF) */

#define PLL_CTL                0xFFC00000	/* PLL Control register (16-bit) */
#define PLL_DIV			        0xFFC00004	/* PLL Divide Register (16-bit) */
#define VR_CTL			        0xFFC00008	/* Voltage Regulator Control Register (16-bit) */
#define PLL_STAT               0xFFC0000C	/* PLL Status register (16-bit) */
#define PLL_LOCKCNT            0xFFC00010	/* PLL Lock Count register (16-bit) */
#define CHIPID                 0xFFC00014       /* Chip ID Register */

/* For MMR's that are reserved on Core B, set up defines to better integrate with other ports */
#define SWRST                   SICA_SWRST
#define SYSCR                   SICA_SYSCR
#define DOUBLE_FAULT            (DOUBLE_FAULT_B|DOUBLE_FAULT_A)
#define RESET_DOUBLE            (SWRST_DBL_FAULT_B|SWRST_DBL_FAULT_A)
#define RESET_WDOG              (SWRST_WDT_B|SWRST_WDT_A)
#define RESET_SOFTWARE          (SWRST_OCCURRED)

/* System Reset and Interrupt Controller registers for core A (0xFFC0 0100-0xFFC0 01FF) */
#define SICA_SWRST              0xFFC00100	/* Software Reset register */
#define SICA_SYSCR              0xFFC00104	/* System Reset Configuration register */
#define SICA_RVECT              0xFFC00108	/* SIC Reset Vector Address Register */
#define SICA_IMASK              0xFFC0010C	/* SIC Interrupt Mask register 0 - hack to fix old tests */
#define SICA_IMASK0             0xFFC0010C	/* SIC Interrupt Mask register 0 */
#define SICA_IMASK1             0xFFC00110	/* SIC Interrupt Mask register 1 */
#define SICA_IAR0               0xFFC00124	/* SIC Interrupt Assignment Register 0 */
#define SICA_IAR1               0xFFC00128	/* SIC Interrupt Assignment Register 1 */
#define SICA_IAR2               0xFFC0012C	/* SIC Interrupt Assignment Register 2 */
#define SICA_IAR3               0xFFC00130	/* SIC Interrupt Assignment Register 3 */
#define SICA_IAR4               0xFFC00134	/* SIC Interrupt Assignment Register 4 */
#define SICA_IAR5               0xFFC00138	/* SIC Interrupt Assignment Register 5 */
#define SICA_IAR6               0xFFC0013C	/* SIC Interrupt Assignment Register 6 */
#define SICA_IAR7               0xFFC00140	/* SIC Interrupt Assignment Register 7 */
#define SICA_ISR0               0xFFC00114	/* SIC Interrupt Status register 0 */
#define SICA_ISR1               0xFFC00118	/* SIC Interrupt Status register 1 */
#define SICA_IWR0               0xFFC0011C	/* SIC Interrupt Wakeup-Enable register 0 */
#define SICA_IWR1               0xFFC00120	/* SIC Interrupt Wakeup-Enable register 1 */

/* System Reset and Interrupt Controller registers for Core B (0xFFC0 1100-0xFFC0 11FF) */
#define SICB_SWRST              0xFFC01100	/* reserved */
#define SICB_SYSCR              0xFFC01104	/* reserved */
#define SICB_RVECT              0xFFC01108	/* SIC Reset Vector Address Register */
#define SICB_IMASK0             0xFFC0110C	/* SIC Interrupt Mask register 0 */
#define SICB_IMASK1             0xFFC01110	/* SIC Interrupt Mask register 1 */
#define SICB_IAR0               0xFFC01124	/* SIC Interrupt Assignment Register 0 */
#define SICB_IAR1               0xFFC01128	/* SIC Interrupt Assignment Register 1 */
#define SICB_IAR2               0xFFC0112C	/* SIC Interrupt Assignment Register 2 */
#define SICB_IAR3               0xFFC01130	/* SIC Interrupt Assignment Register 3 */
#define SICB_IAR4               0xFFC01134	/* SIC Interrupt Assignment Register 4 */
#define SICB_IAR5               0xFFC01138	/* SIC Interrupt Assignment Register 5 */
#define SICB_IAR6               0xFFC0113C	/* SIC Interrupt Assignment Register 6 */
#define SICB_IAR7               0xFFC01140	/* SIC Interrupt Assignment Register 7 */
#define SICB_ISR0               0xFFC01114	/* SIC Interrupt Status register 0 */
#define SICB_ISR1               0xFFC01118	/* SIC Interrupt Status register 1 */
#define SICB_IWR0               0xFFC0111C	/* SIC Interrupt Wakeup-Enable register 0 */
#define SICB_IWR1               0xFFC01120	/* SIC Interrupt Wakeup-Enable register 1 */

/* Watchdog Timer registers for Core A (0xFFC0 0200-0xFFC0 02FF) */
#define WDOGA_CTL 				0xFFC00200	/* Watchdog Control register */
#define WDOGA_CNT 				0xFFC00204	/* Watchdog Count register */
#define WDOGA_STAT 				0xFFC00208	/* Watchdog Status register */

/* Watchdog Timer registers for Core B (0xFFC0 1200-0xFFC0 12FF) */
#define WDOGB_CTL 				0xFFC01200	/* Watchdog Control register */
#define WDOGB_CNT 				0xFFC01204	/* Watchdog Count register */
#define WDOGB_STAT 				0xFFC01208	/* Watchdog Status register */

/* UART Controller (0xFFC00400 - 0xFFC004FF) */

/*
 * Because include/linux/serial_reg.h have defined UART_*,
 * So we define blackfin uart regs to BFIN_UART0_*.
 */
#define BFIN_UART_THR			0xFFC00400  /* Transmit Holding register */
#define BFIN_UART_RBR			0xFFC00400  /* Receive Buffer register */
#define BFIN_UART_DLL			0xFFC00400  /* Divisor Latch (Low-Byte) */
#define BFIN_UART_IER			0xFFC00404  /* Interrupt Enable Register */
#define BFIN_UART_DLH			0xFFC00404  /* Divisor Latch (High-Byte) */
#define BFIN_UART_IIR			0xFFC00408  /* Interrupt Identification Register */
#define BFIN_UART_LCR			0xFFC0040C  /* Line Control Register */
#define BFIN_UART_MCR			0xFFC00410  /* Modem Control Register */
#define BFIN_UART_LSR			0xFFC00414  /* Line Status Register */
#define BFIN_UART_MSR			0xFFC00418  /* Modem Status Register */
#define BFIN_UART_SCR			0xFFC0041C  /* SCR Scratch Register */
#define BFIN_UART_GCTL			0xFFC00424  /* Global Control Register */

/* SPI Controller (0xFFC00500 - 0xFFC005FF) */
#define SPI0_REGBASE          		0xFFC00500
#define SPI_CTL               		0xFFC00500	/* SPI Control Register */
#define SPI_FLG               		0xFFC00504	/* SPI Flag register */
#define SPI_STAT              		0xFFC00508	/* SPI Status register */
#define SPI_TDBR              		0xFFC0050C	/* SPI Transmit Data Buffer Register */
#define SPI_RDBR              		0xFFC00510	/* SPI Receive Data Buffer Register */
#define SPI_BAUD              		0xFFC00514	/* SPI Baud rate Register */
#define SPI_SHADOW            		0xFFC00518	/* SPI_RDBR Shadow Register */

/* Timer 0-7 registers (0xFFC0 0600-0xFFC0 06FF) */
#define TIMER0_CONFIG 				0xFFC00600	/* Timer0 Configuration register */
#define TIMER0_COUNTER 				0xFFC00604	/* Timer0 Counter register */
#define TIMER0_PERIOD 				0xFFC00608	/* Timer0 Period register */
#define TIMER0_WIDTH 				0xFFC0060C	/* Timer0 Width register */

#define TIMER1_CONFIG 				0xFFC00610	/* Timer1 Configuration register */
#define TIMER1_COUNTER 				0xFFC00614	/* Timer1 Counter register */
#define TIMER1_PERIOD 				0xFFC00618	/* Timer1 Period register */
#define TIMER1_WIDTH 				0xFFC0061C	/* Timer1 Width register */

#define TIMER2_CONFIG 				0xFFC00620	/* Timer2 Configuration register */
#define TIMER2_COUNTER 				0xFFC00624	/* Timer2 Counter register */
#define TIMER2_PERIOD 				0xFFC00628	/* Timer2 Period register */
#define TIMER2_WIDTH 				0xFFC0062C	/* Timer2 Width register */

#define TIMER3_CONFIG 				0xFFC00630	/* Timer3 Configuration register */
#define TIMER3_COUNTER 				0xFFC00634	/* Timer3 Counter register */
#define TIMER3_PERIOD 				0xFFC00638	/* Timer3 Period register */
#define TIMER3_WIDTH 				0xFFC0063C	/* Timer3 Width register */

#define TIMER4_CONFIG 				0xFFC00640	/* Timer4 Configuration register */
#define TIMER4_COUNTER 				0xFFC00644	/* Timer4 Counter register */
#define TIMER4_PERIOD 				0xFFC00648	/* Timer4 Period register */
#define TIMER4_WIDTH 				0xFFC0064C	/* Timer4 Width register */

#define TIMER5_CONFIG 				0xFFC00650	/* Timer5 Configuration register */
#define TIMER5_COUNTER 				0xFFC00654	/* Timer5 Counter register */
#define TIMER5_PERIOD 				0xFFC00658	/* Timer5 Period register */
#define TIMER5_WIDTH 				0xFFC0065C	/* Timer5 Width register */

#define TIMER6_CONFIG 				0xFFC00660	/* Timer6 Configuration register */
#define TIMER6_COUNTER 				0xFFC00664	/* Timer6 Counter register */
#define TIMER6_PERIOD 				0xFFC00668	/* Timer6 Period register */
#define TIMER6_WIDTH 				0xFFC0066C	/* Timer6 Width register */

#define TIMER7_CONFIG 				0xFFC00670	/* Timer7 Configuration register */
#define TIMER7_COUNTER 				0xFFC00674	/* Timer7 Counter register */
#define TIMER7_PERIOD 				0xFFC00678	/* Timer7 Period register */
#define TIMER7_WIDTH 				0xFFC0067C	/* Timer7 Width register */

#define TMRS8_ENABLE 				0xFFC00680	/* Timer Enable Register */
#define TMRS8_DISABLE 				0xFFC00684	/* Timer Disable register */
#define TMRS8_STATUS 				0xFFC00688	/* Timer Status register */

/* Timer registers 8-11 (0xFFC0 1600-0xFFC0 16FF) */
#define TIMER8_CONFIG 				0xFFC01600	/* Timer8 Configuration register */
#define TIMER8_COUNTER 				0xFFC01604	/* Timer8 Counter register */
#define TIMER8_PERIOD 				0xFFC01608	/* Timer8 Period register */
#define TIMER8_WIDTH 				0xFFC0160C	/* Timer8 Width register */

#define TIMER9_CONFIG 				0xFFC01610	/* Timer9 Configuration register */
#define TIMER9_COUNTER 				0xFFC01614	/* Timer9 Counter register */
#define TIMER9_PERIOD 				0xFFC01618	/* Timer9 Period register */
#define TIMER9_WIDTH 				0xFFC0161C	/* Timer9 Width register */

#define TIMER10_CONFIG 				0xFFC01620	/* Timer10 Configuration register */
#define TIMER10_COUNTER 			0xFFC01624	/* Timer10 Counter register */
#define TIMER10_PERIOD 				0xFFC01628	/* Timer10 Period register */
#define TIMER10_WIDTH 				0xFFC0162C	/* Timer10 Width register */

#define TIMER11_CONFIG 				0xFFC01630	/* Timer11 Configuration register */
#define TIMER11_COUNTER 			0xFFC01634	/* Timer11 Counter register */
#define TIMER11_PERIOD 				0xFFC01638	/* Timer11 Period register */
#define TIMER11_WIDTH 				0xFFC0163C	/* Timer11 Width register */

#define TMRS4_ENABLE 				0xFFC01640	/* Timer Enable Register */
#define TMRS4_DISABLE 				0xFFC01644	/* Timer Disable register */
#define TMRS4_STATUS 				0xFFC01648	/* Timer Status register */

/* Programmable Flag 0 registers (0xFFC0 0700-0xFFC0 07FF) */
#define FIO0_FLAG_D 				0xFFC00700	/* Flag Data register */
#define FIO0_FLAG_C 				0xFFC00704	/* Flag Clear register */
#define FIO0_FLAG_S 				0xFFC00708	/* Flag Set register */
#define FIO0_FLAG_T 				0xFFC0070C	/* Flag Toggle register */
#define FIO0_MASKA_D 				0xFFC00710	/* Flag Mask Interrupt A Data register */
#define FIO0_MASKA_C 				0xFFC00714	/* Flag Mask Interrupt A Clear register */
#define FIO0_MASKA_S 				0xFFC00718	/* Flag Mask Interrupt A Set register */
#define FIO0_MASKA_T 				0xFFC0071C	/* Flag Mask Interrupt A Toggle register */
#define FIO0_MASKB_D 				0xFFC00720	/* Flag Mask Interrupt B Data register */
#define FIO0_MASKB_C 				0xFFC00724	/* Flag Mask Interrupt B Clear register */
#define FIO0_MASKB_S 				0xFFC00728	/* Flag Mask Interrupt B Set register */
#define FIO0_MASKB_T 				0xFFC0072C	/* Flag Mask Interrupt B Toggle register */
#define FIO0_DIR 					0xFFC00730	/* Flag Direction register */
#define FIO0_POLAR 					0xFFC00734	/* Flag Polarity register */
#define FIO0_EDGE 					0xFFC00738	/* Flag Interrupt Sensitivity register */
#define FIO0_BOTH 					0xFFC0073C	/* Flag Set on Both Edges register */
#define FIO0_INEN 					0xFFC00740	/* Flag Input Enable register */

/* Programmable Flag 1 registers (0xFFC0 1500-0xFFC0 15FF) */
#define FIO1_FLAG_D 				0xFFC01500	/* Flag Data register (mask used to directly */
#define FIO1_FLAG_C 				0xFFC01504	/* Flag Clear register */
#define FIO1_FLAG_S 				0xFFC01508	/* Flag Set register */
#define FIO1_FLAG_T 				0xFFC0150C	/* Flag Toggle register (mask used to */
#define FIO1_MASKA_D 				0xFFC01510	/* Flag Mask Interrupt A Data register */
#define FIO1_MASKA_C 				0xFFC01514	/* Flag Mask Interrupt A Clear register */
#define FIO1_MASKA_S 				0xFFC01518	/* Flag Mask Interrupt A Set register */
#define FIO1_MASKA_T 				0xFFC0151C	/* Flag Mask Interrupt A Toggle register */
#define FIO1_MASKB_D 				0xFFC01520	/* Flag Mask Interrupt B Data register */
#define FIO1_MASKB_C 				0xFFC01524	/* Flag Mask Interrupt B Clear register */
#define FIO1_MASKB_S 				0xFFC01528	/* Flag Mask Interrupt B Set register */
#define FIO1_MASKB_T 				0xFFC0152C	/* Flag Mask Interrupt B Toggle register */
#define FIO1_DIR 					0xFFC01530	/* Flag Direction register */
#define FIO1_POLAR 					0xFFC01534	/* Flag Polarity register */
#define FIO1_EDGE 					0xFFC01538	/* Flag Interrupt Sensitivity register */
#define FIO1_BOTH 					0xFFC0153C	/* Flag Set on Both Edges register */
#define FIO1_INEN 					0xFFC01540	/* Flag Input Enable register */

/* Programmable Flag registers (0xFFC0 1700-0xFFC0 17FF) */
#define FIO2_FLAG_D 				0xFFC01700	/* Flag Data register (mask used to directly */
#define FIO2_FLAG_C 				0xFFC01704	/* Flag Clear register */
#define FIO2_FLAG_S 				0xFFC01708	/* Flag Set register */
#define FIO2_FLAG_T 				0xFFC0170C	/* Flag Toggle register (mask used to */
#define FIO2_MASKA_D 				0xFFC01710	/* Flag Mask Interrupt A Data register */
#define FIO2_MASKA_C 				0xFFC01714	/* Flag Mask Interrupt A Clear register */
#define FIO2_MASKA_S 				0xFFC01718	/* Flag Mask Interrupt A Set register */
#define FIO2_MASKA_T 				0xFFC0171C	/* Flag Mask Interrupt A Toggle register */
#define FIO2_MASKB_D 				0xFFC01720	/* Flag Mask Interrupt B Data register */
#define FIO2_MASKB_C 				0xFFC01724	/* Flag Mask Interrupt B Clear register */
#define FIO2_MASKB_S 				0xFFC01728	/* Flag Mask Interrupt B Set register */
#define FIO2_MASKB_T 				0xFFC0172C	/* Flag Mask Interrupt B Toggle register */
#define FIO2_DIR 					0xFFC01730	/* Flag Direction register */
#define FIO2_POLAR 					0xFFC01734	/* Flag Polarity register */
#define FIO2_EDGE 					0xFFC01738	/* Flag Interrupt Sensitivity register */
#define FIO2_BOTH 					0xFFC0173C	/* Flag Set on Both Edges register */
#define FIO2_INEN 					0xFFC01740	/* Flag Input Enable register */

/* SPORT0 Controller (0xFFC00800 - 0xFFC008FF) */
#define SPORT0_TCR1     	 	0xFFC00800	/* SPORT0 Transmit Configuration 1 Register */
#define SPORT0_TCR2      	 	0xFFC00804	/* SPORT0 Transmit Configuration 2 Register */
#define SPORT0_TCLKDIV        		0xFFC00808	/* SPORT0 Transmit Clock Divider */
#define SPORT0_TFSDIV          		0xFFC0080C	/* SPORT0 Transmit Frame Sync Divider */
#define SPORT0_TX	             	0xFFC00810	/* SPORT0 TX Data Register */
#define SPORT0_RX	            	0xFFC00818	/* SPORT0 RX Data Register */
#define SPORT0_RCR1      	 		0xFFC00820	/* SPORT0 Transmit Configuration 1 Register */
#define SPORT0_RCR2      	 		0xFFC00824	/* SPORT0 Transmit Configuration 2 Register */
#define SPORT0_RCLKDIV        		0xFFC00828	/* SPORT0 Receive Clock Divider */
#define SPORT0_RFSDIV          		0xFFC0082C	/* SPORT0 Receive Frame Sync Divider */
#define SPORT0_STAT            		0xFFC00830	/* SPORT0 Status Register */
#define SPORT0_CHNL            		0xFFC00834	/* SPORT0 Current Channel Register */
#define SPORT0_MCMC1           		0xFFC00838	/* SPORT0 Multi-Channel Configuration Register 1 */
#define SPORT0_MCMC2           		0xFFC0083C	/* SPORT0 Multi-Channel Configuration Register 2 */
#define SPORT0_MTCS0           		0xFFC00840	/* SPORT0 Multi-Channel Transmit Select Register 0 */
#define SPORT0_MTCS1           		0xFFC00844	/* SPORT0 Multi-Channel Transmit Select Register 1 */
#define SPORT0_MTCS2           		0xFFC00848	/* SPORT0 Multi-Channel Transmit Select Register 2 */
#define SPORT0_MTCS3           		0xFFC0084C	/* SPORT0 Multi-Channel Transmit Select Register 3 */
#define SPORT0_MRCS0           		0xFFC00850	/* SPORT0 Multi-Channel Receive Select Register 0 */
#define SPORT0_MRCS1           		0xFFC00854	/* SPORT0 Multi-Channel Receive Select Register 1 */
#define SPORT0_MRCS2           		0xFFC00858	/* SPORT0 Multi-Channel Receive Select Register 2 */
#define SPORT0_MRCS3           		0xFFC0085C	/* SPORT0 Multi-Channel Receive Select Register 3 */

/* SPORT1 Controller (0xFFC00900 - 0xFFC009FF) */
#define SPORT1_TCR1     	 		0xFFC00900	/* SPORT1 Transmit Configuration 1 Register */
#define SPORT1_TCR2      	 		0xFFC00904	/* SPORT1 Transmit Configuration 2 Register */
#define SPORT1_TCLKDIV        		0xFFC00908	/* SPORT1 Transmit Clock Divider */
#define SPORT1_TFSDIV          		0xFFC0090C	/* SPORT1 Transmit Frame Sync Divider */
#define SPORT1_TX	             	0xFFC00910	/* SPORT1 TX Data Register */
#define SPORT1_RX	            	0xFFC00918	/* SPORT1 RX Data Register */
#define SPORT1_RCR1      	 		0xFFC00920	/* SPORT1 Transmit Configuration 1 Register */
#define SPORT1_RCR2      	 		0xFFC00924	/* SPORT1 Transmit Configuration 2 Register */
#define SPORT1_RCLKDIV        		0xFFC00928	/* SPORT1 Receive Clock Divider */
#define SPORT1_RFSDIV          		0xFFC0092C	/* SPORT1 Receive Frame Sync Divider */
#define SPORT1_STAT            		0xFFC00930	/* SPORT1 Status Register */
#define SPORT1_CHNL            		0xFFC00934	/* SPORT1 Current Channel Register */
#define SPORT1_MCMC1           		0xFFC00938	/* SPORT1 Multi-Channel Configuration Register 1 */
#define SPORT1_MCMC2           		0xFFC0093C	/* SPORT1 Multi-Channel Configuration Register 2 */
#define SPORT1_MTCS0           		0xFFC00940	/* SPORT1 Multi-Channel Transmit Select Register 0 */
#define SPORT1_MTCS1           		0xFFC00944	/* SPORT1 Multi-Channel Transmit Select Register 1 */
#define SPORT1_MTCS2           		0xFFC00948	/* SPORT1 Multi-Channel Transmit Select Register 2 */
#define SPORT1_MTCS3           		0xFFC0094C	/* SPORT1 Multi-Channel Transmit Select Register 3 */
#define SPORT1_MRCS0           		0xFFC00950	/* SPORT1 Multi-Channel Receive Select Register 0 */
#define SPORT1_MRCS1           		0xFFC00954	/* SPORT1 Multi-Channel Receive Select Register 1 */
#define SPORT1_MRCS2           		0xFFC00958	/* SPORT1 Multi-Channel Receive Select Register 2 */
#define SPORT1_MRCS3           		0xFFC0095C	/* SPORT1 Multi-Channel Receive Select Register 3 */

/* Asynchronous Memory Controller - External Bus Interface Unit  */
#define EBIU_AMGCTL					0xFFC00A00	/* Asynchronous Memory Global Control Register */
#define EBIU_AMBCTL0				0xFFC00A04	/* Asynchronous Memory Bank Control Register 0 */
#define EBIU_AMBCTL1				0xFFC00A08	/* Asynchronous Memory Bank Control Register 1 */

/* SDRAM Controller External Bus Interface Unit (0xFFC00A00 - 0xFFC00AFF) */
#define EBIU_SDGCTL					0xFFC00A10	/* SDRAM Global Control Register */
#define EBIU_SDBCTL					0xFFC00A14	/* SDRAM Bank Control Register */
#define EBIU_SDRRC 					0xFFC00A18	/* SDRAM Refresh Rate Control Register */
#define EBIU_SDSTAT					0xFFC00A1C	/* SDRAM Status Register */

/* Parallel Peripheral Interface (PPI) 0 registers (0xFFC0 1000-0xFFC0 10FF) */
#define PPI0_CONTROL 				0xFFC01000	/* PPI0 Control register */
#define PPI0_STATUS 				0xFFC01004	/* PPI0 Status register */
#define PPI0_COUNT 					0xFFC01008	/* PPI0 Transfer Count register */
#define PPI0_DELAY 					0xFFC0100C	/* PPI0 Delay Count register */
#define PPI0_FRAME 					0xFFC01010	/* PPI0 Frame Length register */

/*Parallel Peripheral Interface (PPI) 1 registers (0xFFC0 1300-0xFFC0 13FF) */
#define PPI1_CONTROL 				0xFFC01300	/* PPI1 Control register */
#define PPI1_STATUS 				0xFFC01304	/* PPI1 Status register */
#define PPI1_COUNT 					0xFFC01308	/* PPI1 Transfer Count register */
#define PPI1_DELAY 					0xFFC0130C	/* PPI1 Delay Count register */
#define PPI1_FRAME 					0xFFC01310	/* PPI1 Frame Length register */

/*DMA traffic control registers */
#define	DMA1_TC_PER  0xFFC01B0C	/* Traffic control periods */
#define	DMA1_TC_CNT  0xFFC01B10	/* Traffic control current counts */
#define	DMA2_TC_PER  0xFFC00B0C	/* Traffic control periods */
#define	DMA2_TC_CNT  0xFFC00B10	/* Traffic control current counts        */

/* DMA1 Controller registers (0xFFC0 1C00-0xFFC0 1FFF) */
#define DMA1_0_CONFIG 0xFFC01C08	/* DMA1 Channel 0 Configuration register */
#define DMA1_0_NEXT_DESC_PTR 0xFFC01C00	/* DMA1 Channel 0 Next Descripter Ptr Reg */
#define DMA1_0_START_ADDR 0xFFC01C04	/* DMA1 Channel 0 Start Address */
#define DMA1_0_X_COUNT 0xFFC01C10	/* DMA1 Channel 0 Inner Loop Count */
#define DMA1_0_Y_COUNT 0xFFC01C18	/* DMA1 Channel 0 Outer Loop Count */
#define DMA1_0_X_MODIFY 0xFFC01C14	/* DMA1 Channel 0 Inner Loop Addr Increment */
#define DMA1_0_Y_MODIFY 0xFFC01C1C	/* DMA1 Channel 0 Outer Loop Addr Increment */
#define DMA1_0_CURR_DESC_PTR 0xFFC01C20	/* DMA1 Channel 0 Current Descriptor Pointer */
#define DMA1_0_CURR_ADDR 0xFFC01C24	/* DMA1 Channel 0 Current Address Pointer */
#define DMA1_0_CURR_X_COUNT 0xFFC01C30	/* DMA1 Channel 0 Current Inner Loop Count */
#define DMA1_0_CURR_Y_COUNT 0xFFC01C38	/* DMA1 Channel 0 Current Outer Loop Count */
#define DMA1_0_IRQ_STATUS 0xFFC01C28	/* DMA1 Channel 0 Interrupt/Status Register */
#define DMA1_0_PERIPHERAL_MAP 0xFFC01C2C	/* DMA1 Channel 0 Peripheral Map Register */

#define DMA1_1_CONFIG 0xFFC01C48	/* DMA1 Channel 1 Configuration register */
#define DMA1_1_NEXT_DESC_PTR 0xFFC01C40	/* DMA1 Channel 1 Next Descripter Ptr Reg */
#define DMA1_1_START_ADDR 0xFFC01C44	/* DMA1 Channel 1 Start Address */
#define DMA1_1_X_COUNT 0xFFC01C50	/* DMA1 Channel 1 Inner Loop Count */
#define DMA1_1_Y_COUNT 0xFFC01C58	/* DMA1 Channel 1 Outer Loop Count */
#define DMA1_1_X_MODIFY 0xFFC01C54	/* DMA1 Channel 1 Inner Loop Addr Increment */
#define DMA1_1_Y_MODIFY 0xFFC01C5C	/* DMA1 Channel 1 Outer Loop Addr Increment */
#define DMA1_1_CURR_DESC_PTR 0xFFC01C60	/* DMA1 Channel 1 Current Descriptor Pointer */
#define DMA1_1_CURR_ADDR 0xFFC01C64	/* DMA1 Channel 1 Current Address Pointer */
#define DMA1_1_CURR_X_COUNT 0xFFC01C70	/* DMA1 Channel 1 Current Inner Loop Count */
#define DMA1_1_CURR_Y_COUNT 0xFFC01C78	/* DMA1 Channel 1 Current Outer Loop Count */
#define DMA1_1_IRQ_STATUS 0xFFC01C68	/* DMA1 Channel 1 Interrupt/Status Register */
#define DMA1_1_PERIPHERAL_MAP 0xFFC01C6C	/* DMA1 Channel 1 Peripheral Map Register */

#define DMA1_2_CONFIG 0xFFC01C88	/* DMA1 Channel 2 Configuration register */
#define DMA1_2_NEXT_DESC_PTR 0xFFC01C80	/* DMA1 Channel 2 Next Descripter Ptr Reg */
#define DMA1_2_START_ADDR 0xFFC01C84	/* DMA1 Channel 2 Start Address */
#define DMA1_2_X_COUNT 0xFFC01C90	/* DMA1 Channel 2 Inner Loop Count */
#define DMA1_2_Y_COUNT 0xFFC01C98	/* DMA1 Channel 2 Outer Loop Count */
#define DMA1_2_X_MODIFY 0xFFC01C94	/* DMA1 Channel 2 Inner Loop Addr Increment */
#define DMA1_2_Y_MODIFY 0xFFC01C9C	/* DMA1 Channel 2 Outer Loop Addr Increment */
#define DMA1_2_CURR_DESC_PTR 0xFFC01CA0	/* DMA1 Channel 2 Current Descriptor Pointer */
#define DMA1_2_CURR_ADDR 0xFFC01CA4	/* DMA1 Channel 2 Current Address Pointer */
#define DMA1_2_CURR_X_COUNT 0xFFC01CB0	/* DMA1 Channel 2 Current Inner Loop Count */
#define DMA1_2_CURR_Y_COUNT 0xFFC01CB8	/* DMA1 Channel 2 Current Outer Loop Count */
#define DMA1_2_IRQ_STATUS 0xFFC01CA8	/* DMA1 Channel 2 Interrupt/Status Register */
#define DMA1_2_PERIPHERAL_MAP 0xFFC01CAC	/* DMA1 Channel 2 Peripheral Map Register */

#define DMA1_3_CONFIG 0xFFC01CC8	/* DMA1 Channel 3 Configuration register */
#define DMA1_3_NEXT_DESC_PTR 0xFFC01CC0	/* DMA1 Channel 3 Next Descripter Ptr Reg */
#define DMA1_3_START_ADDR 0xFFC01CC4	/* DMA1 Channel 3 Start Address */
#define DMA1_3_X_COUNT 0xFFC01CD0	/* DMA1 Channel 3 Inner Loop Count */
#define DMA1_3_Y_COUNT 0xFFC01CD8	/* DMA1 Channel 3 Outer Loop Count */
#define DMA1_3_X_MODIFY 0xFFC01CD4	/* DMA1 Channel 3 Inner Loop Addr Increment */
#define DMA1_3_Y_MODIFY 0xFFC01CDC	/* DMA1 Channel 3 Outer Loop Addr Increment */
#define DMA1_3_CURR_DESC_PTR 0xFFC01CE0	/* DMA1 Channel 3 Current Descriptor Pointer */
#define DMA1_3_CURR_ADDR 0xFFC01CE4	/* DMA1 Channel 3 Current Address Pointer */
#define DMA1_3_CURR_X_COUNT 0xFFC01CF0	/* DMA1 Channel 3 Current Inner Loop Count */
#define DMA1_3_CURR_Y_COUNT 0xFFC01CF8	/* DMA1 Channel 3 Current Outer Loop Count */
#define DMA1_3_IRQ_STATUS 0xFFC01CE8	/* DMA1 Channel 3 Interrupt/Status Register */
#define DMA1_3_PERIPHERAL_MAP 0xFFC01CEC	/* DMA1 Channel 3 Peripheral Map Register */

#define DMA1_4_CONFIG 0xFFC01D08	/* DMA1 Channel 4 Configuration register */
#define DMA1_4_NEXT_DESC_PTR 0xFFC01D00	/* DMA1 Channel 4 Next Descripter Ptr Reg */
#define DMA1_4_START_ADDR 0xFFC01D04	/* DMA1 Channel 4 Start Address */
#define DMA1_4_X_COUNT 0xFFC01D10	/* DMA1 Channel 4 Inner Loop Count */
#define DMA1_4_Y_COUNT 0xFFC01D18	/* DMA1 Channel 4 Outer Loop Count */
#define DMA1_4_X_MODIFY 0xFFC01D14	/* DMA1 Channel 4 Inner Loop Addr Increment */
#define DMA1_4_Y_MODIFY 0xFFC01D1C	/* DMA1 Channel 4 Outer Loop Addr Increment */
#define DMA1_4_CURR_DESC_PTR 0xFFC01D20	/* DMA1 Channel 4 Current Descriptor Pointer */
#define DMA1_4_CURR_ADDR 0xFFC01D24	/* DMA1 Channel 4 Current Address Pointer */
#define DMA1_4_CURR_X_COUNT 0xFFC01D30	/* DMA1 Channel 4 Current Inner Loop Count */
#define DMA1_4_CURR_Y_COUNT 0xFFC01D38	/* DMA1 Channel 4 Current Outer Loop Count */
#define DMA1_4_IRQ_STATUS 0xFFC01D28	/* DMA1 Channel 4 Interrupt/Status Register */
#define DMA1_4_PERIPHERAL_MAP 0xFFC01D2C	/* DMA1 Channel 4 Peripheral Map Register */

#define DMA1_5_CONFIG 0xFFC01D48	/* DMA1 Channel 5 Configuration register */
#define DMA1_5_NEXT_DESC_PTR 0xFFC01D40	/* DMA1 Channel 5 Next Descripter Ptr Reg */
#define DMA1_5_START_ADDR 0xFFC01D44	/* DMA1 Channel 5 Start Address */
#define DMA1_5_X_COUNT 0xFFC01D50	/* DMA1 Channel 5 Inner Loop Count */
#define DMA1_5_Y_COUNT 0xFFC01D58	/* DMA1 Channel 5 Outer Loop Count */
#define DMA1_5_X_MODIFY 0xFFC01D54	/* DMA1 Channel 5 Inner Loop Addr Increment */
#define DMA1_5_Y_MODIFY 0xFFC01D5C	/* DMA1 Channel 5 Outer Loop Addr Increment */
#define DMA1_5_CURR_DESC_PTR 0xFFC01D60	/* DMA1 Channel 5 Current Descriptor Pointer */
#define DMA1_5_CURR_ADDR 0xFFC01D64	/* DMA1 Channel 5 Current Address Pointer */
#define DMA1_5_CURR_X_COUNT 0xFFC01D70	/* DMA1 Channel 5 Current Inner Loop Count */
#define DMA1_5_CURR_Y_COUNT 0xFFC01D78	/* DMA1 Channel 5 Current Outer Loop Count */
#define DMA1_5_IRQ_STATUS 0xFFC01D68	/* DMA1 Channel 5 Interrupt/Status Register */
#define DMA1_5_PERIPHERAL_MAP 0xFFC01D6C	/* DMA1 Channel 5 Peripheral Map Register */

#define DMA1_6_CONFIG 0xFFC01D88	/* DMA1 Channel 6 Configuration register */
#define DMA1_6_NEXT_DESC_PTR 0xFFC01D80	/* DMA1 Channel 6 Next Descripter Ptr Reg */
#define DMA1_6_START_ADDR 0xFFC01D84	/* DMA1 Channel 6 Start Address */
#define DMA1_6_X_COUNT 0xFFC01D90	/* DMA1 Channel 6 Inner Loop Count */
#define DMA1_6_Y_COUNT 0xFFC01D98	/* DMA1 Channel 6 Outer Loop Count */
#define DMA1_6_X_MODIFY 0xFFC01D94	/* DMA1 Channel 6 Inner Loop Addr Increment */
#define DMA1_6_Y_MODIFY 0xFFC01D9C	/* DMA1 Channel 6 Outer Loop Addr Increment */
#define DMA1_6_CURR_DESC_PTR 0xFFC01DA0	/* DMA1 Channel 6 Current Descriptor Pointer */
#define DMA1_6_CURR_ADDR 0xFFC01DA4	/* DMA1 Channel 6 Current Address Pointer */
#define DMA1_6_CURR_X_COUNT 0xFFC01DB0	/* DMA1 Channel 6 Current Inner Loop Count */
#define DMA1_6_CURR_Y_COUNT 0xFFC01DB8	/* DMA1 Channel 6 Current Outer Loop Count */
#define DMA1_6_IRQ_STATUS 0xFFC01DA8	/* DMA1 Channel 6 Interrupt/Status Register */
#define DMA1_6_PERIPHERAL_MAP 0xFFC01DAC	/* DMA1 Channel 6 Peripheral Map Register */

#define DMA1_7_CONFIG 0xFFC01DC8	/* DMA1 Channel 7 Configuration register */
#define DMA1_7_NEXT_DESC_PTR 0xFFC01DC0	/* DMA1 Channel 7 Next Descripter Ptr Reg */
#define DMA1_7_START_ADDR 0xFFC01DC4	/* DMA1 Channel 7 Start Address */
#define DMA1_7_X_COUNT 0xFFC01DD0	/* DMA1 Channel 7 Inner Loop Count */
#define DMA1_7_Y_COUNT 0xFFC01DD8	/* DMA1 Channel 7 Outer Loop Count */
#define DMA1_7_X_MODIFY 0xFFC01DD4	/* DMA1 Channel 7 Inner Loop Addr Increment */
#define DMA1_7_Y_MODIFY 0xFFC01DDC	/* DMA1 Channel 7 Outer Loop Addr Increment */
#define DMA1_7_CURR_DESC_PTR 0xFFC01DE0	/* DMA1 Channel 7 Current Descriptor Pointer */
#define DMA1_7_CURR_ADDR 0xFFC01DE4	/* DMA1 Channel 7 Current Address Pointer */
#define DMA1_7_CURR_X_COUNT 0xFFC01DF0	/* DMA1 Channel 7 Current Inner Loop Count */
#define DMA1_7_CURR_Y_COUNT 0xFFC01DF8	/* DMA1 Channel 7 Current Outer Loop Count */
#define DMA1_7_IRQ_STATUS 0xFFC01DE8	/* DMA1 Channel 7 Interrupt/Status Register */
#define DMA1_7_PERIPHERAL_MAP 0xFFC01DEC	/* DMA1 Channel 7 Peripheral Map Register */

#define DMA1_8_CONFIG 0xFFC01E08	/* DMA1 Channel 8 Configuration register */
#define DMA1_8_NEXT_DESC_PTR 0xFFC01E00	/* DMA1 Channel 8 Next Descripter Ptr Reg */
#define DMA1_8_START_ADDR 0xFFC01E04	/* DMA1 Channel 8 Start Address */
#define DMA1_8_X_COUNT 0xFFC01E10	/* DMA1 Channel 8 Inner Loop Count */
#define DMA1_8_Y_COUNT 0xFFC01E18	/* DMA1 Channel 8 Outer Loop Count */
#define DMA1_8_X_MODIFY 0xFFC01E14	/* DMA1 Channel 8 Inner Loop Addr Increment */
#define DMA1_8_Y_MODIFY 0xFFC01E1C	/* DMA1 Channel 8 Outer Loop Addr Increment */
#define DMA1_8_CURR_DESC_PTR 0xFFC01E20	/* DMA1 Channel 8 Current Descriptor Pointer */
#define DMA1_8_CURR_ADDR 0xFFC01E24	/* DMA1 Channel 8 Current Address Pointer */
#define DMA1_8_CURR_X_COUNT 0xFFC01E30	/* DMA1 Channel 8 Current Inner Loop Count */
#define DMA1_8_CURR_Y_COUNT 0xFFC01E38	/* DMA1 Channel 8 Current Outer Loop Count */
#define DMA1_8_IRQ_STATUS 0xFFC01E28	/* DMA1 Channel 8 Interrupt/Status Register */
#define DMA1_8_PERIPHERAL_MAP 0xFFC01E2C	/* DMA1 Channel 8 Peripheral Map Register */

#define DMA1_9_CONFIG 0xFFC01E48	/* DMA1 Channel 9 Configuration register */
#define DMA1_9_NEXT_DESC_PTR 0xFFC01E40	/* DMA1 Channel 9 Next Descripter Ptr Reg */
#define DMA1_9_START_ADDR 0xFFC01E44	/* DMA1 Channel 9 Start Address */
#define DMA1_9_X_COUNT 0xFFC01E50	/* DMA1 Channel 9 Inner Loop Count */
#define DMA1_9_Y_COUNT 0xFFC01E58	/* DMA1 Channel 9 Outer Loop Count */
#define DMA1_9_X_MODIFY 0xFFC01E54	/* DMA1 Channel 9 Inner Loop Addr Increment */
#define DMA1_9_Y_MODIFY 0xFFC01E5C	/* DMA1 Channel 9 Outer Loop Addr Increment */
#define DMA1_9_CURR_DESC_PTR 0xFFC01E60	/* DMA1 Channel 9 Current Descriptor Pointer */
#define DMA1_9_CURR_ADDR 0xFFC01E64	/* DMA1 Channel 9 Current Address Pointer */
#define DMA1_9_CURR_X_COUNT 0xFFC01E70	/* DMA1 Channel 9 Current Inner Loop Count */
#define DMA1_9_CURR_Y_COUNT 0xFFC01E78	/* DMA1 Channel 9 Current Outer Loop Count */
#define DMA1_9_IRQ_STATUS 0xFFC01E68	/* DMA1 Channel 9 Interrupt/Status Register */
#define DMA1_9_PERIPHERAL_MAP 0xFFC01E6C	/* DMA1 Channel 9 Peripheral Map Register */

#define DMA1_10_CONFIG 0xFFC01E88	/* DMA1 Channel 10 Configuration register */
#define DMA1_10_NEXT_DESC_PTR 0xFFC01E80	/* DMA1 Channel 10 Next Descripter Ptr Reg */
#define DMA1_10_START_ADDR 0xFFC01E84	/* DMA1 Channel 10 Start Address */
#define DMA1_10_X_COUNT 0xFFC01E90	/* DMA1 Channel 10 Inner Loop Count */
#define DMA1_10_Y_COUNT 0xFFC01E98	/* DMA1 Channel 10 Outer Loop Count */
#define DMA1_10_X_MODIFY 0xFFC01E94	/* DMA1 Channel 10 Inner Loop Addr Increment */
#define DMA1_10_Y_MODIFY 0xFFC01E9C	/* DMA1 Channel 10 Outer Loop Addr Increment */
#define DMA1_10_CURR_DESC_PTR 0xFFC01EA0	/* DMA1 Channel 10 Current Descriptor Pointer */
#define DMA1_10_CURR_ADDR 0xFFC01EA4	/* DMA1 Channel 10 Current Address Pointer */
#define DMA1_10_CURR_X_COUNT 0xFFC01EB0	/* DMA1 Channel 10 Current Inner Loop Count */
#define DMA1_10_CURR_Y_COUNT 0xFFC01EB8	/* DMA1 Channel 10 Current Outer Loop Count */
#define DMA1_10_IRQ_STATUS 0xFFC01EA8	/* DMA1 Channel 10 Interrupt/Status Register */
#define DMA1_10_PERIPHERAL_MAP 0xFFC01EAC	/* DMA1 Channel 10 Peripheral Map Register */

#define DMA1_11_CONFIG 0xFFC01EC8	/* DMA1 Channel 11 Configuration register */
#define DMA1_11_NEXT_DESC_PTR 0xFFC01EC0	/* DMA1 Channel 11 Next Descripter Ptr Reg */
#define DMA1_11_START_ADDR 0xFFC01EC4	/* DMA1 Channel 11 Start Address */
#define DMA1_11_X_COUNT 0xFFC01ED0	/* DMA1 Channel 11 Inner Loop Count */
#define DMA1_11_Y_COUNT 0xFFC01ED8	/* DMA1 Channel 11 Outer Loop Count */
#define DMA1_11_X_MODIFY 0xFFC01ED4	/* DMA1 Channel 11 Inner Loop Addr Increment */
#define DMA1_11_Y_MODIFY 0xFFC01EDC	/* DMA1 Channel 11 Outer Loop Addr Increment */
#define DMA1_11_CURR_DESC_PTR 0xFFC01EE0	/* DMA1 Channel 11 Current Descriptor Pointer */
#define DMA1_11_CURR_ADDR 0xFFC01EE4	/* DMA1 Channel 11 Current Address Pointer */
#define DMA1_11_CURR_X_COUNT 0xFFC01EF0	/* DMA1 Channel 11 Current Inner Loop Count */
#define DMA1_11_CURR_Y_COUNT 0xFFC01EF8	/* DMA1 Channel 11 Current Outer Loop Count */
#define DMA1_11_IRQ_STATUS 0xFFC01EE8	/* DMA1 Channel 11 Interrupt/Status Register */
#define DMA1_11_PERIPHERAL_MAP 0xFFC01EEC	/* DMA1 Channel 11 Peripheral Map Register */

/* Memory DMA1 Controller registers (0xFFC0 1E80-0xFFC0 1FFF) */
#define MDMA1_D0_CONFIG 0xFFC01F08	/*MemDMA1 Stream 0 Destination Configuration */
#define MDMA1_D0_NEXT_DESC_PTR 0xFFC01F00	/*MemDMA1 Stream 0 Destination Next Descriptor Ptr Reg */
#define MDMA1_D0_START_ADDR 0xFFC01F04	/*MemDMA1 Stream 0 Destination Start Address */
#define MDMA1_D0_X_COUNT 0xFFC01F10	/*MemDMA1 Stream 0 Destination Inner-Loop Count */
#define MDMA1_D0_Y_COUNT 0xFFC01F18	/*MemDMA1 Stream 0 Destination Outer-Loop Count */
#define MDMA1_D0_X_MODIFY 0xFFC01F14	/*MemDMA1 Stream 0 Dest Inner-Loop Address-Increment */
#define MDMA1_D0_Y_MODIFY 0xFFC01F1C	/*MemDMA1 Stream 0 Dest Outer-Loop Address-Increment */
#define MDMA1_D0_CURR_DESC_PTR 0xFFC01F20	/*MemDMA1 Stream 0 Dest Current Descriptor Ptr reg */
#define MDMA1_D0_CURR_ADDR 0xFFC01F24	/*MemDMA1 Stream 0 Destination Current Address */
#define MDMA1_D0_CURR_X_COUNT 0xFFC01F30	/*MemDMA1 Stream 0 Dest Current Inner-Loop Count */
#define MDMA1_D0_CURR_Y_COUNT 0xFFC01F38	/*MemDMA1 Stream 0 Dest Current Outer-Loop Count */
#define MDMA1_D0_IRQ_STATUS 0xFFC01F28	/*MemDMA1 Stream 0 Destination Interrupt/Status */
#define MDMA1_D0_PERIPHERAL_MAP 0xFFC01F2C	/*MemDMA1 Stream 0 Destination Peripheral Map */

#define MDMA1_S0_CONFIG 0xFFC01F48	/*MemDMA1 Stream 0 Source Configuration */
#define MDMA1_S0_NEXT_DESC_PTR 0xFFC01F40	/*MemDMA1 Stream 0 Source Next Descriptor Ptr Reg */
#define MDMA1_S0_START_ADDR 0xFFC01F44	/*MemDMA1 Stream 0 Source Start Address */
#define MDMA1_S0_X_COUNT 0xFFC01F50	/*MemDMA1 Stream 0 Source Inner-Loop Count */
#define MDMA1_S0_Y_COUNT 0xFFC01F58	/*MemDMA1 Stream 0 Source Outer-Loop Count */
#define MDMA1_S0_X_MODIFY 0xFFC01F54	/*MemDMA1 Stream 0 Source Inner-Loop Address-Increment */
#define MDMA1_S0_Y_MODIFY 0xFFC01F5C	/*MemDMA1 Stream 0 Source Outer-Loop Address-Increment */
#define MDMA1_S0_CURR_DESC_PTR 0xFFC01F60	/*MemDMA1 Stream 0 Source Current Descriptor Ptr reg */
#define MDMA1_S0_CURR_ADDR 0xFFC01F64	/*MemDMA1 Stream 0 Source Current Address */
#define MDMA1_S0_CURR_X_COUNT 0xFFC01F70	/*MemDMA1 Stream 0 Source Current Inner-Loop Count */
#define MDMA1_S0_CURR_Y_COUNT 0xFFC01F78	/*MemDMA1 Stream 0 Source Current Outer-Loop Count */
#define MDMA1_S0_IRQ_STATUS 0xFFC01F68	/*MemDMA1 Stream 0 Source Interrupt/Status */
#define MDMA1_S0_PERIPHERAL_MAP 0xFFC01F6C	/*MemDMA1 Stream 0 Source Peripheral Map */

#define MDMA1_D1_CONFIG 0xFFC01F88	/*MemDMA1 Stream 1 Destination Configuration */
#define MDMA1_D1_NEXT_DESC_PTR 0xFFC01F80	/*MemDMA1 Stream 1 Destination Next Descriptor Ptr Reg */
#define MDMA1_D1_START_ADDR 0xFFC01F84	/*MemDMA1 Stream 1 Destination Start Address */
#define MDMA1_D1_X_COUNT 0xFFC01F90	/*MemDMA1 Stream 1 Destination Inner-Loop Count */
#define MDMA1_D1_Y_COUNT 0xFFC01F98	/*MemDMA1 Stream 1 Destination Outer-Loop Count */
#define MDMA1_D1_X_MODIFY 0xFFC01F94	/*MemDMA1 Stream 1 Dest Inner-Loop Address-Increment */
#define MDMA1_D1_Y_MODIFY 0xFFC01F9C	/*MemDMA1 Stream 1 Dest Outer-Loop Address-Increment */
#define MDMA1_D1_CURR_DESC_PTR 0xFFC01FA0	/*MemDMA1 Stream 1 Dest Current Descriptor Ptr reg */
#define MDMA1_D1_CURR_ADDR 0xFFC01FA4	/*MemDMA1 Stream 1 Dest Current Address */
#define MDMA1_D1_CURR_X_COUNT 0xFFC01FB0	/*MemDMA1 Stream 1 Dest Current Inner-Loop Count */
#define MDMA1_D1_CURR_Y_COUNT 0xFFC01FB8	/*MemDMA1 Stream 1 Dest Current Outer-Loop Count */
#define MDMA1_D1_IRQ_STATUS 0xFFC01FA8	/*MemDMA1 Stream 1 Dest Interrupt/Status */
#define MDMA1_D1_PERIPHERAL_MAP 0xFFC01FAC	/*MemDMA1 Stream 1 Dest Peripheral Map */

#define MDMA1_S1_CONFIG 0xFFC01FC8	/*MemDMA1 Stream 1 Source Configuration */
#define MDMA1_S1_NEXT_DESC_PTR 0xFFC01FC0	/*MemDMA1 Stream 1 Source Next Descriptor Ptr Reg */
#define MDMA1_S1_START_ADDR 0xFFC01FC4	/*MemDMA1 Stream 1 Source Start Address */
#define MDMA1_S1_X_COUNT 0xFFC01FD0	/*MemDMA1 Stream 1 Source Inner-Loop Count */
#define MDMA1_S1_Y_COUNT 0xFFC01FD8	/*MemDMA1 Stream 1 Source Outer-Loop Count */
#define MDMA1_S1_X_MODIFY 0xFFC01FD4	/*MemDMA1 Stream 1 Source Inner-Loop Address-Increment */
#define MDMA1_S1_Y_MODIFY 0xFFC01FDC	/*MemDMA1 Stream 1 Source Outer-Loop Address-Increment */
#define MDMA1_S1_CURR_DESC_PTR 0xFFC01FE0	/*MemDMA1 Stream 1 Source Current Descriptor Ptr reg */
#define MDMA1_S1_CURR_ADDR 0xFFC01FE4	/*MemDMA1 Stream 1 Source Current Address */
#define MDMA1_S1_CURR_X_COUNT 0xFFC01FF0	/*MemDMA1 Stream 1 Source Current Inner-Loop Count */
#define MDMA1_S1_CURR_Y_COUNT 0xFFC01FF8	/*MemDMA1 Stream 1 Source Current Outer-Loop Count */
#define MDMA1_S1_IRQ_STATUS 0xFFC01FE8	/*MemDMA1 Stream 1 Source Interrupt/Status */
#define MDMA1_S1_PERIPHERAL_MAP 0xFFC01FEC	/*MemDMA1 Stream 1 Source Peripheral Map */

/* DMA2 Controller registers (0xFFC0 0C00-0xFFC0 0DFF) */
#define DMA2_0_CONFIG 0xFFC00C08	/* DMA2 Channel 0 Configuration register */
#define DMA2_0_NEXT_DESC_PTR 0xFFC00C00	/* DMA2 Channel 0 Next Descripter Ptr Reg */
#define DMA2_0_START_ADDR 0xFFC00C04	/* DMA2 Channel 0 Start Address */
#define DMA2_0_X_COUNT 0xFFC00C10	/* DMA2 Channel 0 Inner Loop Count */
#define DMA2_0_Y_COUNT 0xFFC00C18	/* DMA2 Channel 0 Outer Loop Count */
#define DMA2_0_X_MODIFY 0xFFC00C14	/* DMA2 Channel 0 Inner Loop Addr Increment */
#define DMA2_0_Y_MODIFY 0xFFC00C1C	/* DMA2 Channel 0 Outer Loop Addr Increment */
#define DMA2_0_CURR_DESC_PTR 0xFFC00C20	/* DMA2 Channel 0 Current Descriptor Pointer */
#define DMA2_0_CURR_ADDR 0xFFC00C24	/* DMA2 Channel 0 Current Address Pointer */
#define DMA2_0_CURR_X_COUNT 0xFFC00C30	/* DMA2 Channel 0 Current Inner Loop Count */
#define DMA2_0_CURR_Y_COUNT 0xFFC00C38	/* DMA2 Channel 0 Current Outer Loop Count */
#define DMA2_0_IRQ_STATUS 0xFFC00C28	/* DMA2 Channel 0 Interrupt/Status Register */
#define DMA2_0_PERIPHERAL_MAP 0xFFC00C2C	/* DMA2 Channel 0 Peripheral Map Register */

#define DMA2_1_CONFIG 0xFFC00C48	/* DMA2 Channel 1 Configuration register */
#define DMA2_1_NEXT_DESC_PTR 0xFFC00C40	/* DMA2 Channel 1 Next Descripter Ptr Reg */
#define DMA2_1_START_ADDR 0xFFC00C44	/* DMA2 Channel 1 Start Address */
#define DMA2_1_X_COUNT 0xFFC00C50	/* DMA2 Channel 1 Inner Loop Count */
#define DMA2_1_Y_COUNT 0xFFC00C58	/* DMA2 Channel 1 Outer Loop Count */
#define DMA2_1_X_MODIFY 0xFFC00C54	/* DMA2 Channel 1 Inner Loop Addr Increment */
#define DMA2_1_Y_MODIFY 0xFFC00C5C	/* DMA2 Channel 1 Outer Loop Addr Increment */
#define DMA2_1_CURR_DESC_PTR 0xFFC00C60	/* DMA2 Channel 1 Current Descriptor Pointer */
#define DMA2_1_CURR_ADDR 0xFFC00C64	/* DMA2 Channel 1 Current Address Pointer */
#define DMA2_1_CURR_X_COUNT 0xFFC00C70	/* DMA2 Channel 1 Current Inner Loop Count */
#define DMA2_1_CURR_Y_COUNT 0xFFC00C78	/* DMA2 Channel 1 Current Outer Loop Count */
#define DMA2_1_IRQ_STATUS 0xFFC00C68	/* DMA2 Channel 1 Interrupt/Status Register */
#define DMA2_1_PERIPHERAL_MAP 0xFFC00C6C	/* DMA2 Channel 1 Peripheral Map Register */

#define DMA2_2_CONFIG 0xFFC00C88	/* DMA2 Channel 2 Configuration register */
#define DMA2_2_NEXT_DESC_PTR 0xFFC00C80	/* DMA2 Channel 2 Next Descripter Ptr Reg */
#define DMA2_2_START_ADDR 0xFFC00C84	/* DMA2 Channel 2 Start Address */
#define DMA2_2_X_COUNT 0xFFC00C90	/* DMA2 Channel 2 Inner Loop Count */
#define DMA2_2_Y_COUNT 0xFFC00C98	/* DMA2 Channel 2 Outer Loop Count */
#define DMA2_2_X_MODIFY 0xFFC00C94	/* DMA2 Channel 2 Inner Loop Addr Increment */
#define DMA2_2_Y_MODIFY 0xFFC00C9C	/* DMA2 Channel 2 Outer Loop Addr Increment */
#define DMA2_2_CURR_DESC_PTR 0xFFC00CA0	/* DMA2 Channel 2 Current Descriptor Pointer */
#define DMA2_2_CURR_ADDR 0xFFC00CA4	/* DMA2 Channel 2 Current Address Pointer */
#define DMA2_2_CURR_X_COUNT 0xFFC00CB0	/* DMA2 Channel 2 Current Inner Loop Count */
#define DMA2_2_CURR_Y_COUNT 0xFFC00CB8	/* DMA2 Channel 2 Current Outer Loop Count */
#define DMA2_2_IRQ_STATUS 0xFFC00CA8	/* DMA2 Channel 2 Interrupt/Status Register */
#define DMA2_2_PERIPHERAL_MAP 0xFFC00CAC	/* DMA2 Channel 2 Peripheral Map Register */

#define DMA2_3_CONFIG 0xFFC00CC8	/* DMA2 Channel 3 Configuration register */
#define DMA2_3_NEXT_DESC_PTR 0xFFC00CC0	/* DMA2 Channel 3 Next Descripter Ptr Reg */
#define DMA2_3_START_ADDR 0xFFC00CC4	/* DMA2 Channel 3 Start Address */
#define DMA2_3_X_COUNT 0xFFC00CD0	/* DMA2 Channel 3 Inner Loop Count */
#define DMA2_3_Y_COUNT 0xFFC00CD8	/* DMA2 Channel 3 Outer Loop Count */
#define DMA2_3_X_MODIFY 0xFFC00CD4	/* DMA2 Channel 3 Inner Loop Addr Increment */
#define DMA2_3_Y_MODIFY 0xFFC00CDC	/* DMA2 Channel 3 Outer Loop Addr Increment */
#define DMA2_3_CURR_DESC_PTR 0xFFC00CE0	/* DMA2 Channel 3 Current Descriptor Pointer */
#define DMA2_3_CURR_ADDR 0xFFC00CE4	/* DMA2 Channel 3 Current Address Pointer */
#define DMA2_3_CURR_X_COUNT 0xFFC00CF0	/* DMA2 Channel 3 Current Inner Loop Count */
#define DMA2_3_CURR_Y_COUNT 0xFFC00CF8	/* DMA2 Channel 3 Current Outer Loop Count */
#define DMA2_3_IRQ_STATUS 0xFFC00CE8	/* DMA2 Channel 3 Interrupt/Status Register */
#define DMA2_3_PERIPHERAL_MAP 0xFFC00CEC	/* DMA2 Channel 3 Peripheral Map Register */

#define DMA2_4_CONFIG 0xFFC00D08	/* DMA2 Channel 4 Configuration register */
#define DMA2_4_NEXT_DESC_PTR 0xFFC00D00	/* DMA2 Channel 4 Next Descripter Ptr Reg */
#define DMA2_4_START_ADDR 0xFFC00D04	/* DMA2 Channel 4 Start Address */
#define DMA2_4_X_COUNT 0xFFC00D10	/* DMA2 Channel 4 Inner Loop Count */
#define DMA2_4_Y_COUNT 0xFFC00D18	/* DMA2 Channel 4 Outer Loop Count */
#define DMA2_4_X_MODIFY 0xFFC00D14	/* DMA2 Channel 4 Inner Loop Addr Increment */
#define DMA2_4_Y_MODIFY 0xFFC00D1C	/* DMA2 Channel 4 Outer Loop Addr Increment */
#define DMA2_4_CURR_DESC_PTR 0xFFC00D20	/* DMA2 Channel 4 Current Descriptor Pointer */
#define DMA2_4_CURR_ADDR 0xFFC00D24	/* DMA2 Channel 4 Current Address Pointer */
#define DMA2_4_CURR_X_COUNT 0xFFC00D30	/* DMA2 Channel 4 Current Inner Loop Count */
#define DMA2_4_CURR_Y_COUNT 0xFFC00D38	/* DMA2 Channel 4 Current Outer Loop Count */
#define DMA2_4_IRQ_STATUS 0xFFC00D28	/* DMA2 Channel 4 Interrupt/Status Register */
#define DMA2_4_PERIPHERAL_MAP 0xFFC00D2C	/* DMA2 Channel 4 Peripheral Map Register */

#define DMA2_5_CONFIG 0xFFC00D48	/* DMA2 Channel 5 Configuration register */
#define DMA2_5_NEXT_DESC_PTR 0xFFC00D40	/* DMA2 Channel 5 Next Descripter Ptr Reg */
#define DMA2_5_START_ADDR 0xFFC00D44	/* DMA2 Channel 5 Start Address */
#define DMA2_5_X_COUNT 0xFFC00D50	/* DMA2 Channel 5 Inner Loop Count */
#define DMA2_5_Y_COUNT 0xFFC00D58	/* DMA2 Channel 5 Outer Loop Count */
#define DMA2_5_X_MODIFY 0xFFC00D54	/* DMA2 Channel 5 Inner Loop Addr Increment */
#define DMA2_5_Y_MODIFY 0xFFC00D5C	/* DMA2 Channel 5 Outer Loop Addr Increment */
#define DMA2_5_CURR_DESC_PTR 0xFFC00D60	/* DMA2 Channel 5 Current Descriptor Pointer */
#define DMA2_5_CURR_ADDR 0xFFC00D64	/* DMA2 Channel 5 Current Address Pointer */
#define DMA2_5_CURR_X_COUNT 0xFFC00D70	/* DMA2 Channel 5 Current Inner Loop Count */
#define DMA2_5_CURR_Y_COUNT 0xFFC00D78	/* DMA2 Channel 5 Current Outer Loop Count */
#define DMA2_5_IRQ_STATUS 0xFFC00D68	/* DMA2 Channel 5 Interrupt/Status Register */
#define DMA2_5_PERIPHERAL_MAP 0xFFC00D6C	/* DMA2 Channel 5 Peripheral Map Register */

#define DMA2_6_CONFIG 0xFFC00D88	/* DMA2 Channel 6 Configuration register */
#define DMA2_6_NEXT_DESC_PTR 0xFFC00D80	/* DMA2 Channel 6 Next Descripter Ptr Reg */
#define DMA2_6_START_ADDR 0xFFC00D84	/* DMA2 Channel 6 Start Address */
#define DMA2_6_X_COUNT 0xFFC00D90	/* DMA2 Channel 6 Inner Loop Count */
#define DMA2_6_Y_COUNT 0xFFC00D98	/* DMA2 Channel 6 Outer Loop Count */
#define DMA2_6_X_MODIFY 0xFFC00D94	/* DMA2 Channel 6 Inner Loop Addr Increment */
#define DMA2_6_Y_MODIFY 0xFFC00D9C	/* DMA2 Channel 6 Outer Loop Addr Increment */
#define DMA2_6_CURR_DESC_PTR 0xFFC00DA0	/* DMA2 Channel 6 Current Descriptor Pointer */
#define DMA2_6_CURR_ADDR 0xFFC00DA4	/* DMA2 Channel 6 Current Address Pointer */
#define DMA2_6_CURR_X_COUNT 0xFFC00DB0	/* DMA2 Channel 6 Current Inner Loop Count */
#define DMA2_6_CURR_Y_COUNT 0xFFC00DB8	/* DMA2 Channel 6 Current Outer Loop Count */
#define DMA2_6_IRQ_STATUS 0xFFC00DA8	/* DMA2 Channel 6 Interrupt/Status Register */
#define DMA2_6_PERIPHERAL_MAP 0xFFC00DAC	/* DMA2 Channel 6 Peripheral Map Register */

#define DMA2_7_CONFIG 0xFFC00DC8	/* DMA2 Channel 7 Configuration register */
#define DMA2_7_NEXT_DESC_PTR 0xFFC00DC0	/* DMA2 Channel 7 Next Descripter Ptr Reg */
#define DMA2_7_START_ADDR 0xFFC00DC4	/* DMA2 Channel 7 Start Address */
#define DMA2_7_X_COUNT 0xFFC00DD0	/* DMA2 Channel 7 Inner Loop Count */
#define DMA2_7_Y_COUNT 0xFFC00DD8	/* DMA2 Channel 7 Outer Loop Count */
#define DMA2_7_X_MODIFY 0xFFC00DD4	/* DMA2 Channel 7 Inner Loop Addr Increment */
#define DMA2_7_Y_MODIFY 0xFFC00DDC	/* DMA2 Channel 7 Outer Loop Addr Increment */
#define DMA2_7_CURR_DESC_PTR 0xFFC00DE0	/* DMA2 Channel 7 Current Descriptor Pointer */
#define DMA2_7_CURR_ADDR 0xFFC00DE4	/* DMA2 Channel 7 Current Address Pointer */
#define DMA2_7_CURR_X_COUNT 0xFFC00DF0	/* DMA2 Channel 7 Current Inner Loop Count */
#define DMA2_7_CURR_Y_COUNT 0xFFC00DF8	/* DMA2 Channel 7 Current Outer Loop Count */
#define DMA2_7_IRQ_STATUS 0xFFC00DE8	/* DMA2 Channel 7 Interrupt/Status Register */
#define DMA2_7_PERIPHERAL_MAP 0xFFC00DEC	/* DMA2 Channel 7 Peripheral Map Register */

#define DMA2_8_CONFIG 0xFFC00E08	/* DMA2 Channel 8 Configuration register */
#define DMA2_8_NEXT_DESC_PTR 0xFFC00E00	/* DMA2 Channel 8 Next Descripter Ptr Reg */
#define DMA2_8_START_ADDR 0xFFC00E04	/* DMA2 Channel 8 Start Address */
#define DMA2_8_X_COUNT 0xFFC00E10	/* DMA2 Channel 8 Inner Loop Count */
#define DMA2_8_Y_COUNT 0xFFC00E18	/* DMA2 Channel 8 Outer Loop Count */
#define DMA2_8_X_MODIFY 0xFFC00E14	/* DMA2 Channel 8 Inner Loop Addr Increment */
#define DMA2_8_Y_MODIFY 0xFFC00E1C	/* DMA2 Channel 8 Outer Loop Addr Increment */
#define DMA2_8_CURR_DESC_PTR 0xFFC00E20	/* DMA2 Channel 8 Current Descriptor Pointer */
#define DMA2_8_CURR_ADDR 0xFFC00E24	/* DMA2 Channel 8 Current Address Pointer */
#define DMA2_8_CURR_X_COUNT 0xFFC00E30	/* DMA2 Channel 8 Current Inner Loop Count */
#define DMA2_8_CURR_Y_COUNT 0xFFC00E38	/* DMA2 Channel 8 Current Outer Loop Count */
#define DMA2_8_IRQ_STATUS 0xFFC00E28	/* DMA2 Channel 8 Interrupt/Status Register */
#define DMA2_8_PERIPHERAL_MAP 0xFFC00E2C	/* DMA2 Channel 8 Peripheral Map Register */

#define DMA2_9_CONFIG 0xFFC00E48	/* DMA2 Channel 9 Configuration register */
#define DMA2_9_NEXT_DESC_PTR 0xFFC00E40	/* DMA2 Channel 9 Next Descripter Ptr Reg */
#define DMA2_9_START_ADDR 0xFFC00E44	/* DMA2 Channel 9 Start Address */
#define DMA2_9_X_COUNT 0xFFC00E50	/* DMA2 Channel 9 Inner Loop Count */
#define DMA2_9_Y_COUNT 0xFFC00E58	/* DMA2 Channel 9 Outer Loop Count */
#define DMA2_9_X_MODIFY 0xFFC00E54	/* DMA2 Channel 9 Inner Loop Addr Increment */
#define DMA2_9_Y_MODIFY 0xFFC00E5C	/* DMA2 Channel 9 Outer Loop Addr Increment */
#define DMA2_9_CURR_DESC_PTR 0xFFC00E60	/* DMA2 Channel 9 Current Descriptor Pointer */
#define DMA2_9_CURR_ADDR 0xFFC00E64	/* DMA2 Channel 9 Current Address Pointer */
#define DMA2_9_CURR_X_COUNT 0xFFC00E70	/* DMA2 Channel 9 Current Inner Loop Count */
#define DMA2_9_CURR_Y_COUNT 0xFFC00E78	/* DMA2 Channel 9 Current Outer Loop Count */
#define DMA2_9_IRQ_STATUS 0xFFC00E68	/* DMA2 Channel 9 Interrupt/Status Register */
#define DMA2_9_PERIPHERAL_MAP 0xFFC00E6C	/* DMA2 Channel 9 Peripheral Map Register */

#define DMA2_10_CONFIG 0xFFC00E88	/* DMA2 Channel 10 Configuration register */
#define DMA2_10_NEXT_DESC_PTR 0xFFC00E80	/* DMA2 Channel 10 Next Descripter Ptr Reg */
#define DMA2_10_START_ADDR 0xFFC00E84	/* DMA2 Channel 10 Start Address */
#define DMA2_10_X_COUNT 0xFFC00E90	/* DMA2 Channel 10 Inner Loop Count */
#define DMA2_10_Y_COUNT 0xFFC00E98	/* DMA2 Channel 10 Outer Loop Count */
#define DMA2_10_X_MODIFY 0xFFC00E94	/* DMA2 Channel 10 Inner Loop Addr Increment */
#define DMA2_10_Y_MODIFY 0xFFC00E9C	/* DMA2 Channel 10 Outer Loop Addr Increment */
#define DMA2_10_CURR_DESC_PTR 0xFFC00EA0	/* DMA2 Channel 10 Current Descriptor Pointer */
#define DMA2_10_CURR_ADDR 0xFFC00EA4	/* DMA2 Channel 10 Current Address Pointer */
#define DMA2_10_CURR_X_COUNT 0xFFC00EB0	/* DMA2 Channel 10 Current Inner Loop Count */
#define DMA2_10_CURR_Y_COUNT 0xFFC00EB8	/* DMA2 Channel 10 Current Outer Loop Count */
#define DMA2_10_IRQ_STATUS 0xFFC00EA8	/* DMA2 Channel 10 Interrupt/Status Register */
#define DMA2_10_PERIPHERAL_MAP 0xFFC00EAC	/* DMA2 Channel 10 Peripheral Map Register */

#define DMA2_11_CONFIG 0xFFC00EC8	/* DMA2 Channel 11 Configuration register */
#define DMA2_11_NEXT_DESC_PTR 0xFFC00EC0	/* DMA2 Channel 11 Next Descripter Ptr Reg */
#define DMA2_11_START_ADDR 0xFFC00EC4	/* DMA2 Channel 11 Start Address */
#define DMA2_11_X_COUNT 0xFFC00ED0	/* DMA2 Channel 11 Inner Loop Count */
#define DMA2_11_Y_COUNT 0xFFC00ED8	/* DMA2 Channel 11 Outer Loop Count */
#define DMA2_11_X_MODIFY 0xFFC00ED4	/* DMA2 Channel 11 Inner Loop Addr Increment */
#define DMA2_11_Y_MODIFY 0xFFC00EDC	/* DMA2 Channel 11 Outer Loop Addr Increment */
#define DMA2_11_CURR_DESC_PTR 0xFFC00EE0	/* DMA2 Channel 11 Current Descriptor Pointer */
#define DMA2_11_CURR_ADDR 0xFFC00EE4	/* DMA2 Channel 11 Current Address Pointer */
#define DMA2_11_CURR_X_COUNT 0xFFC00EF0	/* DMA2 Channel 11 Current Inner Loop Count */
#define DMA2_11_CURR_Y_COUNT 0xFFC00EF8	/* DMA2 Channel 11 Current Outer Loop Count */
#define DMA2_11_IRQ_STATUS 0xFFC00EE8	/* DMA2 Channel 11 Interrupt/Status Register */
#define DMA2_11_PERIPHERAL_MAP 0xFFC00EEC	/* DMA2 Channel 11 Peripheral Map Register */

/* Memory DMA2 Controller registers (0xFFC0 0E80-0xFFC0 0FFF) */
#define MDMA2_D0_CONFIG 0xFFC00F08	/*MemDMA2 Stream 0 Destination Configuration register */
#define MDMA2_D0_NEXT_DESC_PTR 0xFFC00F00	/*MemDMA2 Stream 0 Destination Next Descriptor Ptr Reg */
#define MDMA2_D0_START_ADDR 0xFFC00F04	/*MemDMA2 Stream 0 Destination Start Address */
#define MDMA2_D0_X_COUNT 0xFFC00F10	/*MemDMA2 Stream 0 Dest Inner-Loop Count register */
#define MDMA2_D0_Y_COUNT 0xFFC00F18	/*MemDMA2 Stream 0 Dest Outer-Loop Count register */
#define MDMA2_D0_X_MODIFY 0xFFC00F14	/*MemDMA2 Stream 0 Dest Inner-Loop Address-Increment */
#define MDMA2_D0_Y_MODIFY 0xFFC00F1C	/*MemDMA2 Stream 0 Dest Outer-Loop Address-Increment */
#define MDMA2_D0_CURR_DESC_PTR 0xFFC00F20	/*MemDMA2 Stream 0 Dest Current Descriptor Ptr reg */
#define MDMA2_D0_CURR_ADDR 0xFFC00F24	/*MemDMA2 Stream 0 Destination Current Address */
#define MDMA2_D0_CURR_X_COUNT 0xFFC00F30	/*MemDMA2 Stream 0 Dest Current Inner-Loop Count reg */
#define MDMA2_D0_CURR_Y_COUNT 0xFFC00F38	/*MemDMA2 Stream 0 Dest Current Outer-Loop Count reg */
#define MDMA2_D0_IRQ_STATUS 0xFFC00F28	/*MemDMA2 Stream 0 Dest Interrupt/Status Register */
#define MDMA2_D0_PERIPHERAL_MAP 0xFFC00F2C	/*MemDMA2 Stream 0 Destination Peripheral Map register */

#define MDMA2_S0_CONFIG 0xFFC00F48	/*MemDMA2 Stream 0 Source Configuration register */
#define MDMA2_S0_NEXT_DESC_PTR 0xFFC00F40	/*MemDMA2 Stream 0 Source Next Descriptor Ptr Reg */
#define MDMA2_S0_START_ADDR 0xFFC00F44	/*MemDMA2 Stream 0 Source Start Address */
#define MDMA2_S0_X_COUNT 0xFFC00F50	/*MemDMA2 Stream 0 Source Inner-Loop Count register */
#define MDMA2_S0_Y_COUNT 0xFFC00F58	/*MemDMA2 Stream 0 Source Outer-Loop Count register */
#define MDMA2_S0_X_MODIFY 0xFFC00F54	/*MemDMA2 Stream 0 Src Inner-Loop Addr-Increment reg */
#define MDMA2_S0_Y_MODIFY 0xFFC00F5C	/*MemDMA2 Stream 0 Src Outer-Loop Addr-Increment reg */
#define MDMA2_S0_CURR_DESC_PTR 0xFFC00F60	/*MemDMA2 Stream 0 Source Current Descriptor Ptr reg */
#define MDMA2_S0_CURR_ADDR 0xFFC00F64	/*MemDMA2 Stream 0 Source Current Address */
#define MDMA2_S0_CURR_X_COUNT 0xFFC00F70	/*MemDMA2 Stream 0 Src Current Inner-Loop Count reg */
#define MDMA2_S0_CURR_Y_COUNT 0xFFC00F78	/*MemDMA2 Stream 0 Src Current Outer-Loop Count reg */
#define MDMA2_S0_IRQ_STATUS 0xFFC00F68	/*MemDMA2 Stream 0 Source Interrupt/Status Register */
#define MDMA2_S0_PERIPHERAL_MAP 0xFFC00F6C	/*MemDMA2 Stream 0 Source Peripheral Map register */

#define MDMA2_D1_CONFIG 0xFFC00F88	/*MemDMA2 Stream 1 Destination Configuration register */
#define MDMA2_D1_NEXT_DESC_PTR 0xFFC00F80	/*MemDMA2 Stream 1 Destination Next Descriptor Ptr Reg */
#define MDMA2_D1_START_ADDR 0xFFC00F84	/*MemDMA2 Stream 1 Destination Start Address */
#define MDMA2_D1_X_COUNT 0xFFC00F90	/*MemDMA2 Stream 1 Dest Inner-Loop Count register */
#define MDMA2_D1_Y_COUNT 0xFFC00F98	/*MemDMA2 Stream 1 Dest Outer-Loop Count register */
#define MDMA2_D1_X_MODIFY 0xFFC00F94	/*MemDMA2 Stream 1 Dest Inner-Loop Address-Increment */
#define MDMA2_D1_Y_MODIFY 0xFFC00F9C	/*MemDMA2 Stream 1 Dest Outer-Loop Address-Increment */
#define MDMA2_D1_CURR_DESC_PTR 0xFFC00FA0	/*MemDMA2 Stream 1 Destination Current Descriptor Ptr */
#define MDMA2_D1_CURR_ADDR 0xFFC00FA4	/*MemDMA2 Stream 1 Destination Current Address reg */
#define MDMA2_D1_CURR_X_COUNT 0xFFC00FB0	/*MemDMA2 Stream 1 Dest Current Inner-Loop Count reg */
#define MDMA2_D1_CURR_Y_COUNT 0xFFC00FB8	/*MemDMA2 Stream 1 Dest Current Outer-Loop Count reg */
#define MDMA2_D1_IRQ_STATUS 0xFFC00FA8	/*MemDMA2 Stream 1 Destination Interrupt/Status Reg */
#define MDMA2_D1_PERIPHERAL_MAP 0xFFC00FAC	/*MemDMA2 Stream 1 Destination Peripheral Map register */

#define MDMA2_S1_CONFIG 0xFFC00FC8	/*MemDMA2 Stream 1 Source Configuration register */
#define MDMA2_S1_NEXT_DESC_PTR 0xFFC00FC0	/*MemDMA2 Stream 1 Source Next Descriptor Ptr Reg */
#define MDMA2_S1_START_ADDR 0xFFC00FC4	/*MemDMA2 Stream 1 Source Start Address */
#define MDMA2_S1_X_COUNT 0xFFC00FD0	/*MemDMA2 Stream 1 Source Inner-Loop Count register */
#define MDMA2_S1_Y_COUNT 0xFFC00FD8	/*MemDMA2 Stream 1 Source Outer-Loop Count register */
#define MDMA2_S1_X_MODIFY 0xFFC00FD4	/*MemDMA2 Stream 1 Src Inner-Loop Address-Increment */
#define MDMA2_S1_Y_MODIFY 0xFFC00FDC	/*MemDMA2 Stream 1 Source Outer-Loop Address-Increment */
#define MDMA2_S1_CURR_DESC_PTR 0xFFC00FE0	/*MemDMA2 Stream 1 Source Current Descriptor Ptr reg */
#define MDMA2_S1_CURR_ADDR 0xFFC00FE4	/*MemDMA2 Stream 1 Source Current Address */
#define MDMA2_S1_CURR_X_COUNT 0xFFC00FF0	/*MemDMA2 Stream 1 Source Current Inner-Loop Count */
#define MDMA2_S1_CURR_Y_COUNT 0xFFC00FF8	/*MemDMA2 Stream 1 Source Current Outer-Loop Count */
#define MDMA2_S1_IRQ_STATUS 0xFFC00FE8	/*MemDMA2 Stream 1 Source Interrupt/Status Register */
#define MDMA2_S1_PERIPHERAL_MAP 0xFFC00FEC	/*MemDMA2 Stream 1 Source Peripheral Map register */

/* Internal Memory DMA Registers (0xFFC0_1800 - 0xFFC0_19FF) */
#define IMDMA_D0_CONFIG 0xFFC01808	/*IMDMA Stream 0 Destination Configuration */
#define IMDMA_D0_NEXT_DESC_PTR 0xFFC01800	/*IMDMA Stream 0 Destination Next Descriptor Ptr Reg */
#define IMDMA_D0_START_ADDR 0xFFC01804	/*IMDMA Stream 0 Destination Start Address */
#define IMDMA_D0_X_COUNT 0xFFC01810	/*IMDMA Stream 0 Destination Inner-Loop Count */
#define IMDMA_D0_Y_COUNT 0xFFC01818	/*IMDMA Stream 0 Destination Outer-Loop Count */
#define IMDMA_D0_X_MODIFY 0xFFC01814	/*IMDMA Stream 0 Dest Inner-Loop Address-Increment */
#define IMDMA_D0_Y_MODIFY 0xFFC0181C	/*IMDMA Stream 0 Dest Outer-Loop Address-Increment */
#define IMDMA_D0_CURR_DESC_PTR 0xFFC01820	/*IMDMA Stream 0 Destination Current Descriptor Ptr */
#define IMDMA_D0_CURR_ADDR 0xFFC01824	/*IMDMA Stream 0 Destination Current Address */
#define IMDMA_D0_CURR_X_COUNT 0xFFC01830	/*IMDMA Stream 0 Destination Current Inner-Loop Count */
#define IMDMA_D0_CURR_Y_COUNT 0xFFC01838	/*IMDMA Stream 0 Destination Current Outer-Loop Count */
#define IMDMA_D0_IRQ_STATUS 0xFFC01828	/*IMDMA Stream 0 Destination Interrupt/Status */

#define IMDMA_S0_CONFIG 0xFFC01848	/*IMDMA Stream 0 Source Configuration */
#define IMDMA_S0_NEXT_DESC_PTR 0xFFC01840	/*IMDMA Stream 0 Source Next Descriptor Ptr Reg */
#define IMDMA_S0_START_ADDR 0xFFC01844	/*IMDMA Stream 0 Source Start Address */
#define IMDMA_S0_X_COUNT 0xFFC01850	/*IMDMA Stream 0 Source Inner-Loop Count */
#define IMDMA_S0_Y_COUNT 0xFFC01858	/*IMDMA Stream 0 Source Outer-Loop Count */
#define IMDMA_S0_X_MODIFY 0xFFC01854	/*IMDMA Stream 0 Source Inner-Loop Address-Increment */
#define IMDMA_S0_Y_MODIFY 0xFFC0185C	/*IMDMA Stream 0 Source Outer-Loop Address-Increment */
#define IMDMA_S0_CURR_DESC_PTR 0xFFC01860	/*IMDMA Stream 0 Source Current Descriptor Ptr reg */
#define IMDMA_S0_CURR_ADDR 0xFFC01864	/*IMDMA Stream 0 Source Current Address */
#define IMDMA_S0_CURR_X_COUNT 0xFFC01870	/*IMDMA Stream 0 Source Current Inner-Loop Count */
#define IMDMA_S0_CURR_Y_COUNT 0xFFC01878	/*IMDMA Stream 0 Source Current Outer-Loop Count */
#define IMDMA_S0_IRQ_STATUS 0xFFC01868	/*IMDMA Stream 0 Source Interrupt/Status */

#define IMDMA_D1_CONFIG 0xFFC01888	/*IMDMA Stream 1 Destination Configuration */
#define IMDMA_D1_NEXT_DESC_PTR 0xFFC01880	/*IMDMA Stream 1 Destination Next Descriptor Ptr Reg */
#define IMDMA_D1_START_ADDR 0xFFC01884	/*IMDMA Stream 1 Destination Start Address */
#define IMDMA_D1_X_COUNT 0xFFC01890	/*IMDMA Stream 1 Destination Inner-Loop Count */
#define IMDMA_D1_Y_COUNT 0xFFC01898	/*IMDMA Stream 1 Destination Outer-Loop Count */
#define IMDMA_D1_X_MODIFY 0xFFC01894	/*IMDMA Stream 1 Dest Inner-Loop Address-Increment */
#define IMDMA_D1_Y_MODIFY 0xFFC0189C	/*IMDMA Stream 1 Dest Outer-Loop Address-Increment */
#define IMDMA_D1_CURR_DESC_PTR 0xFFC018A0	/*IMDMA Stream 1 Destination Current Descriptor Ptr */
#define IMDMA_D1_CURR_ADDR 0xFFC018A4	/*IMDMA Stream 1 Destination Current Address */
#define IMDMA_D1_CURR_X_COUNT 0xFFC018B0	/*IMDMA Stream 1 Destination Current Inner-Loop Count */
#define IMDMA_D1_CURR_Y_COUNT 0xFFC018B8	/*IMDMA Stream 1 Destination Current Outer-Loop Count */
#define IMDMA_D1_IRQ_STATUS 0xFFC018A8	/*IMDMA Stream 1 Destination Interrupt/Status */

#define IMDMA_S1_CONFIG 0xFFC018C8	/*IMDMA Stream 1 Source Configuration */
#define IMDMA_S1_NEXT_DESC_PTR 0xFFC018C0	/*IMDMA Stream 1 Source Next Descriptor Ptr Reg */
#define IMDMA_S1_START_ADDR 0xFFC018C4	/*IMDMA Stream 1 Source Start Address */
#define IMDMA_S1_X_COUNT 0xFFC018D0	/*IMDMA Stream 1 Source Inner-Loop Count */
#define IMDMA_S1_Y_COUNT 0xFFC018D8	/*IMDMA Stream 1 Source Outer-Loop Count */
#define IMDMA_S1_X_MODIFY 0xFFC018D4	/*IMDMA Stream 1 Source Inner-Loop Address-Increment */
#define IMDMA_S1_Y_MODIFY 0xFFC018DC	/*IMDMA Stream 1 Source Outer-Loop Address-Increment */
#define IMDMA_S1_CURR_DESC_PTR 0xFFC018E0	/*IMDMA Stream 1 Source Current Descriptor Ptr reg */
#define IMDMA_S1_CURR_ADDR 0xFFC018E4	/*IMDMA Stream 1 Source Current Address */
#define IMDMA_S1_CURR_X_COUNT 0xFFC018F0	/*IMDMA Stream 1 Source Current Inner-Loop Count */
#define IMDMA_S1_CURR_Y_COUNT 0xFFC018F8	/*IMDMA Stream 1 Source Current Outer-Loop Count */
#define IMDMA_S1_IRQ_STATUS 0xFFC018E8	/*IMDMA Stream 1 Source Interrupt/Status */

/*********************************************************************************** */
/* System MMR Register Bits */
/******************************************************************************* */

/* ********************* PLL AND RESET MASKS ************************ */

/* PLL_CTL Masks */
#define PLL_CLKIN              0x00000000	/* Pass CLKIN to PLL */
#define PLL_CLKIN_DIV2         0x00000001	/* Pass CLKIN/2 to PLL */
#define PLL_OFF                0x00000002	/* Shut off PLL clocks */
#define STOPCK_OFF             0x00000008	/* Core clock off */
#define PDWN                   0x00000020	/* Put the PLL in a Deep Sleep state */
#define BYPASS                 0x00000100	/* Bypass the PLL */

/* CHIPID Masks */
#define CHIPID_VERSION         0xF0000000
#define CHIPID_FAMILY          0x0FFFF000
#define CHIPID_MANUFACTURE     0x00000FFE

/* VR_CTL Masks																	*/
#define	FREQ			0x0003	/* Switching Oscillator Frequency For Regulator	*/
#define	HIBERNATE		0x0000	/* Powerdown/Bypass On-Board Regulation	*/
#define	FREQ_333		0x0001	/* Switching Frequency Is 333 kHz */
#define	FREQ_667		0x0002	/* Switching Frequency Is 667 kHz */
#define	FREQ_1000		0x0003	/* Switching Frequency Is 1 MHz */

#define	GAIN			0x000C	/* Voltage Level Gain	*/
#define	GAIN_5			0x0000	/* GAIN = 5*/
#define	GAIN_10			0x0004	/* GAIN = 1*/
#define	GAIN_20			0x0008	/* GAIN = 2*/
#define	GAIN_50			0x000C	/* GAIN = 5*/

#define	VLEV			0x00F0	/* Internal Voltage Level */
#define	VLEV_085 		0x0060	/* VLEV = 0.85 V (-5% - +10% Accuracy) */
#define	VLEV_090		0x0070	/* VLEV = 0.90 V (-5% - +10% Accuracy) */
#define	VLEV_095		0x0080	/* VLEV = 0.95 V (-5% - +10% Accuracy) */
#define	VLEV_100		0x0090	/* VLEV = 1.00 V (-5% - +10% Accuracy) */
#define	VLEV_105		0x00A0	/* VLEV = 1.05 V (-5% - +10% Accuracy) */
#define	VLEV_110		0x00B0	/* VLEV = 1.10 V (-5% - +10% Accuracy) */
#define	VLEV_115		0x00C0	/* VLEV = 1.15 V (-5% - +10% Accuracy) */
#define	VLEV_120		0x00D0	/* VLEV = 1.20 V (-5% - +10% Accuracy) */
#define	VLEV_125		0x00E0	/* VLEV = 1.25 V (-5% - +10% Accuracy) */
#define	VLEV_130		0x00F0	/* VLEV = 1.30 V (-5% - +10% Accuracy) */

#define	WAKE			0x0100	/* Enable RTC/Reset Wakeup From Hibernate */
#define	SCKELOW			0x8000	/* Do Not Drive SCKE High During Reset After Hibernate */

/* PLL_DIV Masks */
#define SCLK_DIV(x)  (x)	/* SCLK = VCO / x */

#define CSEL			0x30		/* Core Select */
#define SSEL			0xf		/* System Select */
#define CCLK_DIV1              0x00000000	/* CCLK = VCO / 1 */
#define CCLK_DIV2              0x00000010	/* CCLK = VCO / 2 */
#define CCLK_DIV4              0x00000020	/* CCLK = VCO / 4 */
#define CCLK_DIV8              0x00000030	/* CCLK = VCO / 8 */

/* PLL_STAT Masks																	*/
#define ACTIVE_PLLENABLED	0x0001	/* Processor In Active Mode With PLL Enabled    */
#define	FULL_ON				0x0002	/* Processor In Full On Mode                                    */
#define ACTIVE_PLLDISABLED	0x0004	/* Processor In Active Mode With PLL Disabled   */
#define	PLL_LOCKED			0x0020	/* PLL_LOCKCNT Has Been Reached                                 */

/* SICA_SYSCR Masks */
#define COREB_SRAM_INIT		0x0020

/* SWRST Mask */
#define SYSTEM_RESET           0x0007	/* Initiates a system software reset */
#define DOUBLE_FAULT_A         0x0008	/* Core A Double Fault Causes Reset */
#define DOUBLE_FAULT_B         0x0010	/* Core B Double Fault Causes Reset */
#define SWRST_DBL_FAULT_A      0x0800	/* SWRST Core A Double Fault */
#define SWRST_DBL_FAULT_B      0x1000	/* SWRST Core B Double Fault */
#define SWRST_WDT_B		       0x2000	/* SWRST Watchdog B */
#define SWRST_WDT_A		       0x4000	/* SWRST Watchdog A */
#define SWRST_OCCURRED         0x8000	/* SWRST Status */

/* *************  SYSTEM INTERRUPT CONTROLLER MASKS ***************** */

/* SICu_IARv Masks	 */
/* u = A or B */
/* v = 0 to 7 */
/* w = 0 or 1 */

/* Per_number = 0 to 63 */
/* IVG_number = 7 to 15   */
#define Peripheral_IVG(Per_number, IVG_number)    \
    ((IVG_number) - 7) << (((Per_number) % 8) * 4)	/* Peripheral #Per_number assigned IVG #IVG_number  */
    /* Usage: r0.l = lo(Peripheral_IVG(62, 10)); */
    /*        r0.h = hi(Peripheral_IVG(62, 10)); */

/* SICx_IMASKw Masks */
/* masks are 32 bit wide, so two writes reguired for "64 bit" wide registers  */
#define SIC_UNMASK_ALL         0x00000000	/* Unmask all peripheral interrupts */
#define SIC_MASK_ALL           0xFFFFFFFF	/* Mask all peripheral interrupts */
#define SIC_MASK(x)	       (1 << (x))	/* Mask Peripheral #x interrupt */
#define SIC_UNMASK(x) (0xFFFFFFFF ^ (1 << (x)))	/* Unmask Peripheral #x interrupt */

/* SIC_IWR Masks */
#define IWR_DISABLE_ALL        0x00000000	/* Wakeup Disable all peripherals */
#define IWR_ENABLE_ALL         0xFFFFFFFF	/* Wakeup Enable all peripherals */
/* x = pos 0 to 31, for 32-63 use value-32 */
#define IWR_ENABLE(x)	       (1 << (x))	/* Wakeup Enable Peripheral #x */
#define IWR_DISABLE(x) (0xFFFFFFFF ^ (1 << (x)))	/* Wakeup Disable Peripheral #x */

/* ***************************** UART CONTROLLER MASKS ********************** */

/* UART_LCR Register */

#define DLAB	0x80
#define SB      0x40
#define STP      0x20
#define EPS     0x10
#define PEN	0x08
#define STB	0x04
#define WLS(x)	((x-5) & 0x03)

#define DLAB_P	0x07
#define SB_P	0x06
#define STP_P	0x05
#define EPS_P	0x04
#define PEN_P	0x03
#define STB_P	0x02
#define WLS_P1	0x01
#define WLS_P0	0x00

/* UART_MCR Register */
#define LOOP_ENA	0x10
#define LOOP_ENA_P	0x04

/* UART_LSR Register */
#define TEMT	0x40
#define THRE	0x20
#define BI	0x10
#define FE	0x08
#define PE	0x04
#define OE	0x02
#define DR	0x01

#define TEMP_P	0x06
#define THRE_P	0x05
#define BI_P	0x04
#define FE_P	0x03
#define PE_P	0x02
#define OE_P	0x01
#define DR_P	0x00

/* UART_IER Register */
#define ELSI	0x04
#define ETBEI	0x02
#define ERBFI	0x01

#define ELSI_P	0x02
#define ETBEI_P	0x01
#define ERBFI_P	0x00

/* UART_IIR Register */
#define STATUS(x)	((x << 1) & 0x06)
#define NINT		0x01
#define STATUS_P1	0x02
#define STATUS_P0	0x01
#define NINT_P		0x00
#define IIR_TX_READY    0x02	/* UART_THR empty                               */
#define IIR_RX_READY    0x04	/* Receive data ready                           */
#define IIR_LINE_CHANGE 0x06	/* Receive line status                          */
#define IIR_STATUS	0x06

/* UART_GCTL Register */
#define FFE	0x20
#define FPE	0x10
#define RPOLC	0x08
#define TPOLC	0x04
#define IREN	0x02
#define UCEN	0x01

#define FFE_P	0x05
#define FPE_P	0x04
#define RPOLC_P	0x03
#define TPOLC_P	0x02
#define IREN_P	0x01
#define UCEN_P	0x00

/* **********  SERIAL PORT MASKS  ********************** */

/* SPORTx_TCR1 Masks */
#define TSPEN    0x0001		/* TX enable  */
#define ITCLK    0x0002		/* Internal TX Clock Select  */
#define TDTYPE   0x000C		/* TX Data Formatting Select */
#define TLSBIT   0x0010		/* TX Bit Order */
#define ITFS     0x0200		/* Internal TX Frame Sync Select  */
#define TFSR     0x0400		/* TX Frame Sync Required Select  */
#define DITFS    0x0800		/* Data Independent TX Frame Sync Select  */
#define LTFS     0x1000		/* Low TX Frame Sync Select  */
#define LATFS    0x2000		/* Late TX Frame Sync Select  */
#define TCKFE    0x4000		/* TX Clock Falling Edge Select  */

/* SPORTx_TCR2 Masks */
#define SLEN	    0x001F	/*TX Word Length  */
#define TXSE        0x0100	/*TX Secondary Enable */
#define TSFSE       0x0200	/*TX Stereo Frame Sync Enable */
#define TRFST       0x0400	/*TX Right-First Data Order  */

/* SPORTx_RCR1 Masks */
#define RSPEN    0x0001		/* RX enable  */
#define IRCLK    0x0002		/* Internal RX Clock Select  */
#define RDTYPE   0x000C		/* RX Data Formatting Select */
#define RULAW    0x0008		/* u-Law enable  */
#define RALAW    0x000C		/* A-Law enable  */
#define RLSBIT   0x0010		/* RX Bit Order */
#define IRFS     0x0200		/* Internal RX Frame Sync Select  */
#define RFSR     0x0400		/* RX Frame Sync Required Select  */
#define LRFS     0x1000		/* Low RX Frame Sync Select  */
#define LARFS    0x2000		/* Late RX Frame Sync Select  */
#define RCKFE    0x4000		/* RX Clock Falling Edge Select  */

/* SPORTx_RCR2 Masks */
#define SLEN	    0x001F	/*RX Word Length  */
#define RXSE        0x0100	/*RX Secondary Enable */
#define RSFSE       0x0200	/*RX Stereo Frame Sync Enable */
#define RRFST       0x0400	/*Right-First Data Order  */

/*SPORTx_STAT Masks */
#define RXNE		0x0001	/*RX FIFO Not Empty Status */
#define RUVF	    	0x0002	/*RX Underflow Status */
#define ROVF		0x0004	/*RX Overflow Status */
#define TXF		0x0008	/*TX FIFO Full Status */
#define TUVF         	0x0010	/*TX Underflow Status */
#define TOVF         	0x0020	/*TX Overflow Status */
#define TXHRE        	0x0040	/*TX Hold Register Empty */

/*SPORTx_MCMC1 Masks */
#define SP_WSIZE		0x0000F000	/*Multichannel Window Size Field */
#define SP_WOFF		0x000003FF	/*Multichannel Window Offset Field */

/*SPORTx_MCMC2 Masks */
#define MCCRM		0x00000003	/*Multichannel Clock Recovery Mode */
#define MCDTXPE		0x00000004	/*Multichannel DMA Transmit Packing */
#define MCDRXPE		0x00000008	/*Multichannel DMA Receive Packing */
#define MCMEN		0x00000010	/*Multichannel Frame Mode Enable */
#define FSDR		0x00000080	/*Multichannel Frame Sync to Data Relationship */
#define MFD		0x0000F000	/*Multichannel Frame Delay    */

/*  *********  PARALLEL PERIPHERAL INTERFACE (PPI) MASKS ****************   */

/*  PPI_CONTROL Masks         */
#define PORT_EN              0x00000001	/* PPI Port Enable  */
#define PORT_DIR             0x00000002	/* PPI Port Direction       */
#define XFR_TYPE             0x0000000C	/* PPI Transfer Type  */
#define PORT_CFG             0x00000030	/* PPI Port Configuration */
#define FLD_SEL              0x00000040	/* PPI Active Field Select */
#define PACK_EN              0x00000080	/* PPI Packing Mode */
#define DMA32                0x00000100	/* PPI 32-bit DMA Enable */
#define SKIP_EN              0x00000200	/* PPI Skip Element Enable */
#define SKIP_EO              0x00000400	/* PPI Skip Even/Odd Elements */
#define DLENGTH              0x00003800	/* PPI Data Length  */
#define DLEN_8		     0x0	/* PPI Data Length mask for DLEN=8 */
#define DLEN(x)	(((x-9) & 0x07) << 11)	/* PPI Data Length (only works for x=10-->x=16) */
#define POL                  0x0000C000	/* PPI Signal Polarities       */

/* PPI_STATUS Masks */
#define FLD	             0x00000400	/* Field Indicator   */
#define FT_ERR	             0x00000800	/* Frame Track Error */
#define OVR	             0x00001000	/* FIFO Overflow Error */
#define UNDR	             0x00002000	/* FIFO Underrun Error */
#define ERR_DET	      	     0x00004000	/* Error Detected Indicator */
#define ERR_NCOR	     0x00008000	/* Error Not Corrected Indicator */

/* **********  DMA CONTROLLER MASKS  *********************8 */

/* DMAx_CONFIG, MDMA_yy_CONFIG, IMDMA_yy_CONFIG Masks */
#define DMAEN	        0x00000001	/* Channel Enable */
#define WNR	   	0x00000002	/* Channel Direction (W/R*) */
#define WDSIZE_8	0x00000000	/* Word Size 8 bits */
#define WDSIZE_16	0x00000004	/* Word Size 16 bits */
#define WDSIZE_32	0x00000008	/* Word Size 32 bits */
#define DMA2D	        0x00000010	/* 2D/1D* Mode */
#define RESTART         0x00000020	/* Restart */
#define DI_SEL	        0x00000040	/* Data Interrupt Select */
#define DI_EN	        0x00000080	/* Data Interrupt Enable */
#define NDSIZE_0		0x0000	/* Next Descriptor Size = 0 (Stop/Autobuffer)   */
#define NDSIZE_1		0x0100	/* Next Descriptor Size = 1                                             */
#define NDSIZE_2		0x0200	/* Next Descriptor Size = 2                                             */
#define NDSIZE_3		0x0300	/* Next Descriptor Size = 3                                             */
#define NDSIZE_4		0x0400	/* Next Descriptor Size = 4                                             */
#define NDSIZE_5		0x0500	/* Next Descriptor Size = 5                                             */
#define NDSIZE_6		0x0600	/* Next Descriptor Size = 6                                             */
#define NDSIZE_7		0x0700	/* Next Descriptor Size = 7                                             */
#define NDSIZE_8		0x0800	/* Next Descriptor Size = 8                                             */
#define NDSIZE_9		0x0900	/* Next Descriptor Size = 9                                             */
#define NDSIZE	        0x00000900	/* Next Descriptor Size */
#define DMAFLOW	        0x00007000	/* Flow Control */
#define DMAFLOW_STOP		0x0000	/* Stop Mode */
#define DMAFLOW_AUTO		0x1000	/* Autobuffer Mode */
#define DMAFLOW_ARRAY		0x4000	/* Descriptor Array Mode */
#define DMAFLOW_SMALL		0x6000	/* Small Model Descriptor List Mode */
#define DMAFLOW_LARGE		0x7000	/* Large Model Descriptor List Mode */

#define DMAEN_P	            	0	/* Channel Enable */
#define WNR_P	            	1	/* Channel Direction (W/R*) */
#define DMA2D_P	        	4	/* 2D/1D* Mode */
#define RESTART_P	      	5	/* Restart */
#define DI_SEL_P	     	6	/* Data Interrupt Select */
#define DI_EN_P	            	7	/* Data Interrupt Enable */

/* DMAx_IRQ_STATUS, MDMA_yy_IRQ_STATUS, IMDMA_yy_IRQ_STATUS Masks */

#define DMA_DONE		0x00000001	/* DMA Done Indicator */
#define DMA_ERR	        	0x00000002	/* DMA Error Indicator */
#define DFETCH	            	0x00000004	/* Descriptor Fetch Indicator */
#define DMA_RUN	            	0x00000008	/* DMA Running Indicator */

#define DMA_DONE_P	    	0	/* DMA Done Indicator */
#define DMA_ERR_P     		1	/* DMA Error Indicator */
#define DFETCH_P     		2	/* Descriptor Fetch Indicator */
#define DMA_RUN_P     		3	/* DMA Running Indicator */

/* DMAx_PERIPHERAL_MAP, MDMA_yy_PERIPHERAL_MAP, IMDMA_yy_PERIPHERAL_MAP Masks */

#define CTYPE	            0x00000040	/* DMA Channel Type Indicator */
#define CTYPE_P             6	/* DMA Channel Type Indicator BIT POSITION */
#define PCAP8	            0x00000080	/* DMA 8-bit Operation Indicator   */
#define PCAP16	            0x00000100	/* DMA 16-bit Operation Indicator */
#define PCAP32	            0x00000200	/* DMA 32-bit Operation Indicator */
#define PCAPWR	            0x00000400	/* DMA Write Operation Indicator */
#define PCAPRD	            0x00000800	/* DMA Read Operation Indicator */
#define PMAP	            0x00007000	/* DMA Peripheral Map Field */

/*  *************  GENERAL PURPOSE TIMER MASKS  ******************** */

/* PWM Timer bit definitions */

/* TIMER_ENABLE Register */
#define TIMEN0	0x0001
#define TIMEN1	0x0002
#define TIMEN2	0x0004
#define TIMEN3	0x0008
#define TIMEN4	0x0010
#define TIMEN5	0x0020
#define TIMEN6	0x0040
#define TIMEN7	0x0080
#define TIMEN8	0x0001
#define TIMEN9	0x0002
#define TIMEN10	0x0004
#define TIMEN11	0x0008

#define TIMEN0_P	0x00
#define TIMEN1_P	0x01
#define TIMEN2_P	0x02
#define TIMEN3_P	0x03
#define TIMEN4_P	0x04
#define TIMEN5_P	0x05
#define TIMEN6_P	0x06
#define TIMEN7_P	0x07
#define TIMEN8_P	0x00
#define TIMEN9_P	0x01
#define TIMEN10_P	0x02
#define TIMEN11_P	0x03

/* TIMER_DISABLE Register */
#define TIMDIS0		0x0001
#define TIMDIS1		0x0002
#define TIMDIS2		0x0004
#define TIMDIS3		0x0008
#define TIMDIS4		0x0010
#define TIMDIS5		0x0020
#define TIMDIS6		0x0040
#define TIMDIS7		0x0080
#define TIMDIS8		0x0001
#define TIMDIS9		0x0002
#define TIMDIS10	0x0004
#define TIMDIS11	0x0008

#define TIMDIS0_P	0x00
#define TIMDIS1_P	0x01
#define TIMDIS2_P	0x02
#define TIMDIS3_P	0x03
#define TIMDIS4_P	0x04
#define TIMDIS5_P	0x05
#define TIMDIS6_P	0x06
#define TIMDIS7_P	0x07
#define TIMDIS8_P	0x00
#define TIMDIS9_P	0x01
#define TIMDIS10_P	0x02
#define TIMDIS11_P	0x03

/* TIMER_STATUS Register */
#define TIMIL0		0x00000001
#define TIMIL1		0x00000002
#define TIMIL2		0x00000004
#define TIMIL3		0x00000008
#define TIMIL4		0x00010000
#define TIMIL5		0x00020000
#define TIMIL6		0x00040000
#define TIMIL7		0x00080000
#define TIMIL8		0x0001
#define TIMIL9		0x0002
#define TIMIL10		0x0004
#define TIMIL11		0x0008
#define TOVF_ERR0	0x00000010
#define TOVF_ERR1	0x00000020
#define TOVF_ERR2	0x00000040
#define TOVF_ERR3	0x00000080
#define TOVF_ERR4	0x00100000
#define TOVF_ERR5	0x00200000
#define TOVF_ERR6	0x00400000
#define TOVF_ERR7	0x00800000
#define TOVF_ERR8	0x0010
#define TOVF_ERR9	0x0020
#define TOVF_ERR10	0x0040
#define TOVF_ERR11	0x0080
#define TRUN0		0x00001000
#define TRUN1		0x00002000
#define TRUN2		0x00004000
#define TRUN3		0x00008000
#define TRUN4		0x10000000
#define TRUN5		0x20000000
#define TRUN6		0x40000000
#define TRUN7		0x80000000
#define TRUN8		0x1000
#define TRUN9		0x2000
#define TRUN10		0x4000
#define TRUN11		0x8000

#define TIMIL0_P	0x00
#define TIMIL1_P	0x01
#define TIMIL2_P	0x02
#define TIMIL3_P	0x03
#define TIMIL4_P	0x10
#define TIMIL5_P	0x11
#define TIMIL6_P	0x12
#define TIMIL7_P	0x13
#define TIMIL8_P	0x00
#define TIMIL9_P	0x01
#define TIMIL10_P	0x02
#define TIMIL11_P	0x03
#define TOVF_ERR0_P	0x04
#define TOVF_ERR1_P	0x05
#define TOVF_ERR2_P	0x06
#define TOVF_ERR3_P	0x07
#define TOVF_ERR4_P	0x14
#define TOVF_ERR5_P	0x15
#define TOVF_ERR6_P	0x16
#define TOVF_ERR7_P	0x17
#define TOVF_ERR8_P	0x04
#define TOVF_ERR9_P	0x05
#define TOVF_ERR10_P	0x06
#define TOVF_ERR11_P	0x07
#define TRUN0_P		0x0C
#define TRUN1_P		0x0D
#define TRUN2_P		0x0E
#define TRUN3_P		0x0F
#define TRUN4_P		0x1C
#define TRUN5_P		0x1D
#define TRUN6_P		0x1E
#define TRUN7_P		0x1F
#define TRUN8_P		0x0C
#define TRUN9_P		0x0D
#define TRUN10_P	0x0E
#define TRUN11_P	0x0F

/* Alternate Deprecated Macros Provided For Backwards Code Compatibility */
#define TOVL_ERR0 TOVF_ERR0
#define TOVL_ERR1 TOVF_ERR1
#define TOVL_ERR2 TOVF_ERR2
#define TOVL_ERR3 TOVF_ERR3
#define TOVL_ERR4 TOVF_ERR4
#define TOVL_ERR5 TOVF_ERR5
#define TOVL_ERR6 TOVF_ERR6
#define TOVL_ERR7 TOVF_ERR7
#define TOVL_ERR8 TOVF_ERR8
#define TOVL_ERR9 TOVF_ERR9
#define TOVL_ERR10 TOVF_ERR10
#define TOVL_ERR11 TOVF_ERR11
#define TOVL_ERR0_P TOVF_ERR0_P
#define TOVL_ERR1_P TOVF_ERR1_P
#define TOVL_ERR2_P TOVF_ERR2_P
#define TOVL_ERR3_P TOVF_ERR3_P
#define TOVL_ERR4_P TOVF_ERR4_P
#define TOVL_ERR5_P TOVF_ERR5_P
#define TOVL_ERR6_P TOVF_ERR6_P
#define TOVL_ERR7_P TOVF_ERR7_P
#define TOVL_ERR8_P TOVF_ERR8_P
#define TOVL_ERR9_P TOVF_ERR9_P
#define TOVL_ERR10_P TOVF_ERR10_P
#define TOVL_ERR11_P TOVF_ERR11_P

/* TIMERx_CONFIG Registers */
#define PWM_OUT		0x0001
#define WDTH_CAP	0x0002
#define EXT_CLK		0x0003
#define PULSE_HI	0x0004
#define PERIOD_CNT	0x0008
#define IRQ_ENA		0x0010
#define TIN_SEL		0x0020
#define OUT_DIS		0x0040
#define CLK_SEL		0x0080
#define TOGGLE_HI	0x0100
#define EMU_RUN		0x0200
#define ERR_TYP(x)	((x & 0x03) << 14)

#define TMODE_P0		0x00
#define TMODE_P1		0x01
#define PULSE_HI_P		0x02
#define PERIOD_CNT_P		0x03
#define IRQ_ENA_P		0x04
#define TIN_SEL_P		0x05
#define OUT_DIS_P		0x06
#define CLK_SEL_P		0x07
#define TOGGLE_HI_P		0x08
#define EMU_RUN_P		0x09
#define ERR_TYP_P0		0x0E
#define ERR_TYP_P1		0x0F

/*/ ******************   PROGRAMMABLE FLAG MASKS  ********************* */

/*  General Purpose IO (0xFFC00700 - 0xFFC007FF)  Masks */
#define PF0         0x0001
#define PF1         0x0002
#define PF2         0x0004
#define PF3         0x0008
#define PF4         0x0010
#define PF5         0x0020
#define PF6         0x0040
#define PF7         0x0080
#define PF8         0x0100
#define PF9         0x0200
#define PF10        0x0400
#define PF11        0x0800
#define PF12        0x1000
#define PF13        0x2000
#define PF14        0x4000
#define PF15        0x8000

/*  General Purpose IO (0xFFC00700 - 0xFFC007FF)  BIT POSITIONS */
#define PF0_P         0
#define PF1_P         1
#define PF2_P         2
#define PF3_P         3
#define PF4_P         4
#define PF5_P         5
#define PF6_P         6
#define PF7_P         7
#define PF8_P         8
#define PF9_P         9
#define PF10_P        10
#define PF11_P        11
#define PF12_P        12
#define PF13_P        13
#define PF14_P        14
#define PF15_P        15

/* ***********  SERIAL PERIPHERAL INTERFACE (SPI) MASKS  **************** */

/* SPI_CTL Masks */
#define TIMOD                  0x00000003	/* Transfer initiation mode and interrupt generation */
#define SZ                     0x00000004	/* Send Zero (=0) or last (=1) word when TDBR empty. */
#define GM                     0x00000008	/* When RDBR full, get more (=1) data or discard (=0) incoming Data */
#define PSSE                   0x00000010	/* Enable (=1) Slave-Select input for Master. */
#define EMISO                  0x00000020	/* Enable (=1) MISO pin as an output. */
#define SIZE                   0x00000100	/* Word length (0 => 8 bits, 1 => 16 bits) */
#define LSBF                   0x00000200	/* Data format (0 => MSB sent/received first 1 => LSB sent/received first) */
#define CPHA                   0x00000400	/* Clock phase (0 => SPICLK starts toggling in middle of xfer, 1 => SPICLK toggles at the beginning of xfer. */
#define CPOL                   0x00000800	/* Clock polarity (0 => active-high, 1 => active-low) */
#define MSTR                   0x00001000	/* Configures SPI as master (=1) or slave (=0) */
#define WOM                    0x00002000	/* Open drain (=1) data output enable (for MOSI and MISO) */
#define SPE                    0x00004000	/* SPI module enable (=1), disable (=0) */

/* SPI_FLG Masks */
#define FLS1                   0x00000002	/* Enables (=1) SPI_FLOUT1 as flag output for SPI Slave-select */
#define FLS2                   0x00000004	/* Enables (=1) SPI_FLOUT2 as flag output for SPI Slave-select */
#define FLS3                   0x00000008	/* Enables (=1) SPI_FLOUT3 as flag output for SPI Slave-select */
#define FLS4                   0x00000010	/* Enables (=1) SPI_FLOUT4 as flag output for SPI Slave-select */
#define FLS5                   0x00000020	/* Enables (=1) SPI_FLOUT5 as flag output for SPI Slave-select */
#define FLS6                   0x00000040	/* Enables (=1) SPI_FLOUT6 as flag output for SPI Slave-select */
#define FLS7                   0x00000080	/* Enables (=1) SPI_FLOUT7 as flag output for SPI Slave-select */
#define FLG1                   0x00000200	/* Activates (=0) SPI_FLOUT1 as flag output for SPI Slave-select  */
#define FLG2                   0x00000400	/* Activates (=0) SPI_FLOUT2 as flag output for SPI Slave-select */
#define FLG3                   0x00000800	/* Activates (=0) SPI_FLOUT3 as flag output for SPI Slave-select  */
#define FLG4                   0x00001000	/* Activates (=0) SPI_FLOUT4 as flag output for SPI Slave-select  */
#define FLG5                   0x00002000	/* Activates (=0) SPI_FLOUT5 as flag output for SPI Slave-select  */
#define FLG6                   0x00004000	/* Activates (=0) SPI_FLOUT6 as flag output for SPI Slave-select  */
#define FLG7                   0x00008000	/* Activates (=0) SPI_FLOUT7 as flag output for SPI Slave-select */

/* SPI_FLG Bit Positions */
#define FLS1_P                 0x00000001	/* Enables (=1) SPI_FLOUT1 as flag output for SPI Slave-select */
#define FLS2_P                 0x00000002	/* Enables (=1) SPI_FLOUT2 as flag output for SPI Slave-select */
#define FLS3_P                 0x00000003	/* Enables (=1) SPI_FLOUT3 as flag output for SPI Slave-select */
#define FLS4_P                 0x00000004	/* Enables (=1) SPI_FLOUT4 as flag output for SPI Slave-select */
#define FLS5_P                 0x00000005	/* Enables (=1) SPI_FLOUT5 as flag output for SPI Slave-select */
#define FLS6_P                 0x00000006	/* Enables (=1) SPI_FLOUT6 as flag output for SPI Slave-select */
#define FLS7_P                 0x00000007	/* Enables (=1) SPI_FLOUT7 as flag output for SPI Slave-select */
#define FLG1_P                 0x00000009	/* Activates (=0) SPI_FLOUT1 as flag output for SPI Slave-select  */
#define FLG2_P                 0x0000000A	/* Activates (=0) SPI_FLOUT2 as flag output for SPI Slave-select */
#define FLG3_P                 0x0000000B	/* Activates (=0) SPI_FLOUT3 as flag output for SPI Slave-select  */
#define FLG4_P                 0x0000000C	/* Activates (=0) SPI_FLOUT4 as flag output for SPI Slave-select  */
#define FLG5_P                 0x0000000D	/* Activates (=0) SPI_FLOUT5 as flag output for SPI Slave-select  */
#define FLG6_P                 0x0000000E	/* Activates (=0) SPI_FLOUT6 as flag output for SPI Slave-select  */
#define FLG7_P                 0x0000000F	/* Activates (=0) SPI_FLOUT7 as flag output for SPI Slave-select */

/* SPI_STAT Masks */
#define SPIF                   0x00000001	/* Set (=1) when SPI single-word transfer complete */
#define MODF                   0x00000002	/* Set (=1) in a master device when some other device tries to become master */
#define TXE                    0x00000004	/* Set (=1) when transmission occurs with no new data in SPI_TDBR */
#define TXS                    0x00000008	/* SPI_TDBR Data Buffer Status (0=Empty, 1=Full) */
#define RBSY                   0x00000010	/* Set (=1) when data is received with RDBR full */
#define RXS                    0x00000020	/* SPI_RDBR Data Buffer Status (0=Empty, 1=Full)  */
#define TXCOL                  0x00000040	/* When set (=1), corrupt data may have been transmitted  */

/* *********************  ASYNCHRONOUS MEMORY CONTROLLER MASKS  ************* */

/* AMGCTL Masks */
#define AMCKEN			0x0001	/* Enable CLKOUT */
#define AMBEN_B0		0x0002	/* Enable Asynchronous Memory Bank 0 only */
#define AMBEN_B0_B1		0x0004	/* Enable Asynchronous Memory Banks 0 & 1 only */
#define AMBEN_B0_B1_B2	0x0006	/* Enable Asynchronous Memory Banks 0, 1, and 2 */
#define AMBEN_ALL		0x0008	/* Enable Asynchronous Memory Banks (all) 0, 1, 2, and 3 */
#define B0_PEN			0x0010	/* Enable 16-bit packing Bank 0  */
#define B1_PEN			0x0020	/* Enable 16-bit packing Bank 1  */
#define B2_PEN			0x0040	/* Enable 16-bit packing Bank 2  */
#define B3_PEN			0x0080	/* Enable 16-bit packing Bank 3  */

/* AMGCTL Bit Positions */
#define AMCKEN_P		0x00000000	/* Enable CLKOUT */
#define AMBEN_P0		0x00000001	/* Asynchronous Memory Enable, 000 - banks 0-3 disabled, 001 - Bank 0 enabled */
#define AMBEN_P1		0x00000002	/* Asynchronous Memory Enable, 010 - banks 0&1 enabled,  011 - banks 0-3 enabled */
#define AMBEN_P2		0x00000003	/* Asynchronous Memory Enable, 1xx - All banks (bank 0, 1, 2, and 3) enabled */
#define B0_PEN_P			0x004	/* Enable 16-bit packing Bank 0  */
#define B1_PEN_P			0x005	/* Enable 16-bit packing Bank 1  */
#define B2_PEN_P			0x006	/* Enable 16-bit packing Bank 2  */
#define B3_PEN_P			0x007	/* Enable 16-bit packing Bank 3  */

/* AMBCTL0 Masks */
#define B0RDYEN	0x00000001	/* Bank 0 RDY Enable, 0=disable, 1=enable */
#define B0RDYPOL 0x00000002	/* Bank 0 RDY Active high, 0=active low, 1=active high */
#define B0TT_1	0x00000004	/* Bank 0 Transition Time from Read to Write = 1 cycle */
#define B0TT_2	0x00000008	/* Bank 0 Transition Time from Read to Write = 2 cycles */
#define B0TT_3	0x0000000C	/* Bank 0 Transition Time from Read to Write = 3 cycles */
#define B0TT_4	0x00000000	/* Bank 0 Transition Time from Read to Write = 4 cycles */
#define B0ST_1	0x00000010	/* Bank 0 Setup Time from AOE asserted to Read/Write asserted=1 cycle */
#define B0ST_2	0x00000020	/* Bank 0 Setup Time from AOE asserted to Read/Write asserted=2 cycles */
#define B0ST_3	0x00000030	/* Bank 0 Setup Time from AOE asserted to Read/Write asserted=3 cycles */
#define B0ST_4	0x00000000	/* Bank 0 Setup Time from AOE asserted to Read/Write asserted=4 cycles */
#define B0HT_1	0x00000040	/* Bank 0 Hold Time from Read/Write deasserted to AOE deasserted = 1 cycle */
#define B0HT_2	0x00000080	/* Bank 0 Hold Time from Read/Write deasserted to AOE deasserted = 2 cycles */
#define B0HT_3	0x000000C0	/* Bank 0 Hold Time from Read/Write deasserted to AOE deasserted = 3 cycles */
#define B0HT_0	0x00000000	/* Bank 0 Hold Time from Read/Write deasserted to AOE deasserted = 0 cycles */
#define B0RAT_1			0x00000100	/* Bank 0 Read Access Time = 1 cycle */
#define B0RAT_2			0x00000200	/* Bank 0 Read Access Time = 2 cycles */
#define B0RAT_3			0x00000300	/* Bank 0 Read Access Time = 3 cycles */
#define B0RAT_4			0x00000400	/* Bank 0 Read Access Time = 4 cycles */
#define B0RAT_5			0x00000500	/* Bank 0 Read Access Time = 5 cycles */
#define B0RAT_6			0x00000600	/* Bank 0 Read Access Time = 6 cycles */
#define B0RAT_7			0x00000700	/* Bank 0 Read Access Time = 7 cycles */
#define B0RAT_8			0x00000800	/* Bank 0 Read Access Time = 8 cycles */
#define B0RAT_9			0x00000900	/* Bank 0 Read Access Time = 9 cycles */
#define B0RAT_10		0x00000A00	/* Bank 0 Read Access Time = 10 cycles */
#define B0RAT_11		0x00000B00	/* Bank 0 Read Access Time = 11 cycles */
#define B0RAT_12		0x00000C00	/* Bank 0 Read Access Time = 12 cycles */
#define B0RAT_13		0x00000D00	/* Bank 0 Read Access Time = 13 cycles */
#define B0RAT_14		0x00000E00	/* Bank 0 Read Access Time = 14 cycles */
#define B0RAT_15		0x00000F00	/* Bank 0 Read Access Time = 15 cycles */
#define B0WAT_1			0x00001000	/* Bank 0 Write Access Time = 1 cycle */
#define B0WAT_2			0x00002000	/* Bank 0 Write Access Time = 2 cycles */
#define B0WAT_3			0x00003000	/* Bank 0 Write Access Time = 3 cycles */
#define B0WAT_4			0x00004000	/* Bank 0 Write Access Time = 4 cycles */
#define B0WAT_5			0x00005000	/* Bank 0 Write Access Time = 5 cycles */
#define B0WAT_6			0x00006000	/* Bank 0 Write Access Time = 6 cycles */
#define B0WAT_7			0x00007000	/* Bank 0 Write Access Time = 7 cycles */
#define B0WAT_8			0x00008000	/* Bank 0 Write Access Time = 8 cycles */
#define B0WAT_9			0x00009000	/* Bank 0 Write Access Time = 9 cycles */
#define B0WAT_10		0x0000A000	/* Bank 0 Write Access Time = 10 cycles */
#define B0WAT_11		0x0000B000	/* Bank 0 Write Access Time = 11 cycles */
#define B0WAT_12		0x0000C000	/* Bank 0 Write Access Time = 12 cycles */
#define B0WAT_13		0x0000D000	/* Bank 0 Write Access Time = 13 cycles */
#define B0WAT_14		0x0000E000	/* Bank 0 Write Access Time = 14 cycles */
#define B0WAT_15		0x0000F000	/* Bank 0 Write Access Time = 15 cycles */
#define B1RDYEN			0x00010000	/* Bank 1 RDY enable, 0=disable, 1=enable */
#define B1RDYPOL		0x00020000	/* Bank 1 RDY Active high, 0=active low, 1=active high */
#define B1TT_1			0x00040000	/* Bank 1 Transition Time from Read to Write = 1 cycle */
#define B1TT_2			0x00080000	/* Bank 1 Transition Time from Read to Write = 2 cycles */
#define B1TT_3			0x000C0000	/* Bank 1 Transition Time from Read to Write = 3 cycles */
#define B1TT_4			0x00000000	/* Bank 1 Transition Time from Read to Write = 4 cycles */
#define B1ST_1			0x00100000	/* Bank 1 Setup Time from AOE asserted to Read or Write asserted = 1 cycle */
#define B1ST_2			0x00200000	/* Bank 1 Setup Time from AOE asserted to Read or Write asserted = 2 cycles */
#define B1ST_3			0x00300000	/* Bank 1 Setup Time from AOE asserted to Read or Write asserted = 3 cycles */
#define B1ST_4			0x00000000	/* Bank 1 Setup Time from AOE asserted to Read or Write asserted = 4 cycles */
#define B1HT_1			0x00400000	/* Bank 1 Hold Time from Read or Write deasserted to AOE deasserted = 1 cycle */
#define B1HT_2			0x00800000	/* Bank 1 Hold Time from Read or Write deasserted to AOE deasserted = 2 cycles */
#define B1HT_3			0x00C00000	/* Bank 1 Hold Time from Read or Write deasserted to AOE deasserted = 3 cycles */
#define B1HT_0			0x00000000	/* Bank 1 Hold Time from Read or Write deasserted to AOE deasserted = 0 cycles */
#define B1RAT_1			0x01000000	/* Bank 1 Read Access Time = 1 cycle */
#define B1RAT_2			0x02000000	/* Bank 1 Read Access Time = 2 cycles */
#define B1RAT_3			0x03000000	/* Bank 1 Read Access Time = 3 cycles */
#define B1RAT_4			0x04000000	/* Bank 1 Read Access Time = 4 cycles */
#define B1RAT_5			0x05000000	/* Bank 1 Read Access Time = 5 cycles */
#define B1RAT_6			0x06000000	/* Bank 1 Read Access Time = 6 cycles */
#define B1RAT_7			0x07000000	/* Bank 1 Read Access Time = 7 cycles */
#define B1RAT_8			0x08000000	/* Bank 1 Read Access Time = 8 cycles */
#define B1RAT_9			0x09000000	/* Bank 1 Read Access Time = 9 cycles */
#define B1RAT_10		0x0A000000	/* Bank 1 Read Access Time = 10 cycles */
#define B1RAT_11		0x0B000000	/* Bank 1 Read Access Time = 11 cycles */
#define B1RAT_12		0x0C000000	/* Bank 1 Read Access Time = 12 cycles */
#define B1RAT_13		0x0D000000	/* Bank 1 Read Access Time = 13 cycles */
#define B1RAT_14		0x0E000000	/* Bank 1 Read Access Time = 14 cycles */
#define B1RAT_15		0x0F000000	/* Bank 1 Read Access Time = 15 cycles */
#define B1WAT_1			0x10000000	/* Bank 1 Write Access Time = 1 cycle */
#define B1WAT_2			0x20000000	/* Bank 1 Write Access Time = 2 cycles */
#define B1WAT_3			0x30000000	/* Bank 1 Write Access Time = 3 cycles */
#define B1WAT_4			0x40000000	/* Bank 1 Write Access Time = 4 cycles */
#define B1WAT_5			0x50000000	/* Bank 1 Write Access Time = 5 cycles */
#define B1WAT_6			0x60000000	/* Bank 1 Write Access Time = 6 cycles */
#define B1WAT_7			0x70000000	/* Bank 1 Write Access Time = 7 cycles */
#define B1WAT_8			0x80000000	/* Bank 1 Write Access Time = 8 cycles */
#define B1WAT_9			0x90000000	/* Bank 1 Write Access Time = 9 cycles */
#define B1WAT_10		0xA0000000	/* Bank 1 Write Access Time = 10 cycles */
#define B1WAT_11		0xB0000000	/* Bank 1 Write Access Time = 11 cycles */
#define B1WAT_12		0xC0000000	/* Bank 1 Write Access Time = 12 cycles */
#define B1WAT_13		0xD0000000	/* Bank 1 Write Access Time = 13 cycles */
#define B1WAT_14		0xE0000000	/* Bank 1 Write Access Time = 14 cycles */
#define B1WAT_15		0xF0000000	/* Bank 1 Write Access Time = 15 cycles */

/* AMBCTL1 Masks */
#define B2RDYEN			0x00000001	/* Bank 2 RDY Enable, 0=disable, 1=enable */
#define B2RDYPOL		0x00000002	/* Bank 2 RDY Active high, 0=active low, 1=active high */
#define B2TT_1			0x00000004	/* Bank 2 Transition Time from Read to Write = 1 cycle */
#define B2TT_2			0x00000008	/* Bank 2 Transition Time from Read to Write = 2 cycles */
#define B2TT_3			0x0000000C	/* Bank 2 Transition Time from Read to Write = 3 cycles */
#define B2TT_4			0x00000000	/* Bank 2 Transition Time from Read to Write = 4 cycles */
#define B2ST_1			0x00000010	/* Bank 2 Setup Time from AOE asserted to Read or Write asserted = 1 cycle */
#define B2ST_2			0x00000020	/* Bank 2 Setup Time from AOE asserted to Read or Write asserted = 2 cycles */
#define B2ST_3			0x00000030	/* Bank 2 Setup Time from AOE asserted to Read or Write asserted = 3 cycles */
#define B2ST_4			0x00000000	/* Bank 2 Setup Time from AOE asserted to Read or Write asserted = 4 cycles */
#define B2HT_1			0x00000040	/* Bank 2 Hold Time from Read or Write deasserted to AOE deasserted = 1 cycle */
#define B2HT_2			0x00000080	/* Bank 2 Hold Time from Read or Write deasserted to AOE deasserted = 2 cycles */
#define B2HT_3			0x000000C0	/* Bank 2 Hold Time from Read or Write deasserted to AOE deasserted = 3 cycles */
#define B2HT_0			0x00000000	/* Bank 2 Hold Time from Read or Write deasserted to AOE deasserted = 0 cycles */
#define B2RAT_1			0x00000100	/* Bank 2 Read Access Time = 1 cycle */
#define B2RAT_2			0x00000200	/* Bank 2 Read Access Time = 2 cycles */
#define B2RAT_3			0x00000300	/* Bank 2 Read Access Time = 3 cycles */
#define B2RAT_4			0x00000400	/* Bank 2 Read Access Time = 4 cycles */
#define B2RAT_5			0x00000500	/* Bank 2 Read Access Time = 5 cycles */
#define B2RAT_6			0x00000600	/* Bank 2 Read Access Time = 6 cycles */
#define B2RAT_7			0x00000700	/* Bank 2 Read Access Time = 7 cycles */
#define B2RAT_8			0x00000800	/* Bank 2 Read Access Time = 8 cycles */
#define B2RAT_9			0x00000900	/* Bank 2 Read Access Time = 9 cycles */
#define B2RAT_10		0x00000A00	/* Bank 2 Read Access Time = 10 cycles */
#define B2RAT_11		0x00000B00	/* Bank 2 Read Access Time = 11 cycles */
#define B2RAT_12		0x00000C00	/* Bank 2 Read Access Time = 12 cycles */
#define B2RAT_13		0x00000D00	/* Bank 2 Read Access Time = 13 cycles */
#define B2RAT_14		0x00000E00	/* Bank 2 Read Access Time = 14 cycles */
#define B2RAT_15		0x00000F00	/* Bank 2 Read Access Time = 15 cycles */
#define B2WAT_1			0x00001000	/* Bank 2 Write Access Time = 1 cycle */
#define B2WAT_2			0x00002000	/* Bank 2 Write Access Time = 2 cycles */
#define B2WAT_3			0x00003000	/* Bank 2 Write Access Time = 3 cycles */
#define B2WAT_4			0x00004000	/* Bank 2 Write Access Time = 4 cycles */
#define B2WAT_5			0x00005000	/* Bank 2 Write Access Time = 5 cycles */
#define B2WAT_6			0x00006000	/* Bank 2 Write Access Time = 6 cycles */
#define B2WAT_7			0x00007000	/* Bank 2 Write Access Time = 7 cycles */
#define B2WAT_8			0x00008000	/* Bank 2 Write Access Time = 8 cycles */
#define B2WAT_9			0x00009000	/* Bank 2 Write Access Time = 9 cycles */
#define B2WAT_10		0x0000A000	/* Bank 2 Write Access Time = 10 cycles */
#define B2WAT_11		0x0000B000	/* Bank 2 Write Access Time = 11 cycles */
#define B2WAT_12		0x0000C000	/* Bank 2 Write Access Time = 12 cycles */
#define B2WAT_13		0x0000D000	/* Bank 2 Write Access Time = 13 cycles */
#define B2WAT_14		0x0000E000	/* Bank 2 Write Access Time = 14 cycles */
#define B2WAT_15		0x0000F000	/* Bank 2 Write Access Time = 15 cycles */
#define B3RDYEN			0x00010000	/* Bank 3 RDY enable, 0=disable, 1=enable */
#define B3RDYPOL		0x00020000	/* Bank 3 RDY Active high, 0=active low, 1=active high */
#define B3TT_1			0x00040000	/* Bank 3 Transition Time from Read to Write = 1 cycle */
#define B3TT_2			0x00080000	/* Bank 3 Transition Time from Read to Write = 2 cycles */
#define B3TT_3			0x000C0000	/* Bank 3 Transition Time from Read to Write = 3 cycles */
#define B3TT_4			0x00000000	/* Bank 3 Transition Time from Read to Write = 4 cycles */
#define B3ST_1			0x00100000	/* Bank 3 Setup Time from AOE asserted to Read or Write asserted = 1 cycle */
#define B3ST_2			0x00200000	/* Bank 3 Setup Time from AOE asserted to Read or Write asserted = 2 cycles */
#define B3ST_3			0x00300000	/* Bank 3 Setup Time from AOE asserted to Read or Write asserted = 3 cycles */
#define B3ST_4			0x00000000	/* Bank 3 Setup Time from AOE asserted to Read or Write asserted = 4 cycles */
#define B3HT_1			0x00400000	/* Bank 3 Hold Time from Read or Write deasserted to AOE deasserted = 1 cycle */
#define B3HT_2			0x00800000	/* Bank 3 Hold Time from Read or Write deasserted to AOE deasserted = 2 cycles */
#define B3HT_3			0x00C00000	/* Bank 3 Hold Time from Read or Write deasserted to AOE deasserted = 3 cycles */
#define B3HT_0			0x00000000	/* Bank 3 Hold Time from Read or Write deasserted to AOE deasserted = 0 cycles */
#define B3RAT_1			0x01000000	/* Bank 3 Read Access Time = 1 cycle */
#define B3RAT_2			0x02000000	/* Bank 3 Read Access Time = 2 cycles */
#define B3RAT_3			0x03000000	/* Bank 3 Read Access Time = 3 cycles */
#define B3RAT_4			0x04000000	/* Bank 3 Read Access Time = 4 cycles */
#define B3RAT_5			0x05000000	/* Bank 3 Read Access Time = 5 cycles */
#define B3RAT_6			0x06000000	/* Bank 3 Read Access Time = 6 cycles */
#define B3RAT_7			0x07000000	/* Bank 3 Read Access Time = 7 cycles */
#define B3RAT_8			0x08000000	/* Bank 3 Read Access Time = 8 cycles */
#define B3RAT_9			0x09000000	/* Bank 3 Read Access Time = 9 cycles */
#define B3RAT_10		0x0A000000	/* Bank 3 Read Access Time = 10 cycles */
#define B3RAT_11		0x0B000000	/* Bank 3 Read Access Time = 11 cycles */
#define B3RAT_12		0x0C000000	/* Bank 3 Read Access Time = 12 cycles */
#define B3RAT_13		0x0D000000	/* Bank 3 Read Access Time = 13 cycles */
#define B3RAT_14		0x0E000000	/* Bank 3 Read Access Time = 14 cycles */
#define B3RAT_15		0x0F000000	/* Bank 3 Read Access Time = 15 cycles */
#define B3WAT_1			0x10000000	/* Bank 3 Write Access Time = 1 cycle */
#define B3WAT_2			0x20000000	/* Bank 3 Write Access Time = 2 cycles */
#define B3WAT_3			0x30000000	/* Bank 3 Write Access Time = 3 cycles */
#define B3WAT_4			0x40000000	/* Bank 3 Write Access Time = 4 cycles */
#define B3WAT_5			0x50000000	/* Bank 3 Write Access Time = 5 cycles */
#define B3WAT_6			0x60000000	/* Bank 3 Write Access Time = 6 cycles */
#define B3WAT_7			0x70000000	/* Bank 3 Write Access Time = 7 cycles */
#define B3WAT_8			0x80000000	/* Bank 3 Write Access Time = 8 cycles */
#define B3WAT_9			0x90000000	/* Bank 3 Write Access Time = 9 cycles */
#define B3WAT_10		0xA0000000	/* Bank 3 Write Access Time = 10 cycles */
#define B3WAT_11		0xB0000000	/* Bank 3 Write Access Time = 11 cycles */
#define B3WAT_12		0xC0000000	/* Bank 3 Write Access Time = 12 cycles */
#define B3WAT_13		0xD0000000	/* Bank 3 Write Access Time = 13 cycles */
#define B3WAT_14		0xE0000000	/* Bank 3 Write Access Time = 14 cycles */
#define B3WAT_15		0xF0000000	/* Bank 3 Write Access Time = 15 cycles */

/* **********************  SDRAM CONTROLLER MASKS  *************************** */

/* EBIU_SDGCTL Masks */
#define SCTLE			0x00000001	/* Enable SCLK[0], /SRAS, /SCAS, /SWE, SDQM[3:0] */
#define CL_2			0x00000008	/* SDRAM CAS latency = 2 cycles */
#define CL_3			0x0000000C	/* SDRAM CAS latency = 3 cycles */
#define PFE			0x00000010	/* Enable SDRAM prefetch */
#define PFP			0x00000020	/* Prefetch has priority over AMC requests */
#define TRAS_1			0x00000040	/* SDRAM tRAS = 1 cycle */
#define TRAS_2			0x00000080	/* SDRAM tRAS = 2 cycles */
#define TRAS_3			0x000000C0	/* SDRAM tRAS = 3 cycles */
#define TRAS_4			0x00000100	/* SDRAM tRAS = 4 cycles */
#define TRAS_5			0x00000140	/* SDRAM tRAS = 5 cycles */
#define TRAS_6			0x00000180	/* SDRAM tRAS = 6 cycles */
#define TRAS_7			0x000001C0	/* SDRAM tRAS = 7 cycles */
#define TRAS_8			0x00000200	/* SDRAM tRAS = 8 cycles */
#define TRAS_9			0x00000240	/* SDRAM tRAS = 9 cycles */
#define TRAS_10			0x00000280	/* SDRAM tRAS = 10 cycles */
#define TRAS_11			0x000002C0	/* SDRAM tRAS = 11 cycles */
#define TRAS_12			0x00000300	/* SDRAM tRAS = 12 cycles */
#define TRAS_13			0x00000340	/* SDRAM tRAS = 13 cycles */
#define TRAS_14			0x00000380	/* SDRAM tRAS = 14 cycles */
#define TRAS_15			0x000003C0	/* SDRAM tRAS = 15 cycles */
#define TRP_1			0x00000800	/* SDRAM tRP = 1 cycle */
#define TRP_2			0x00001000	/* SDRAM tRP = 2 cycles */
#define TRP_3			0x00001800	/* SDRAM tRP = 3 cycles */
#define TRP_4			0x00002000	/* SDRAM tRP = 4 cycles */
#define TRP_5			0x00002800	/* SDRAM tRP = 5 cycles */
#define TRP_6			0x00003000	/* SDRAM tRP = 6 cycles */
#define TRP_7			0x00003800	/* SDRAM tRP = 7 cycles */
#define TRCD_1			0x00008000	/* SDRAM tRCD = 1 cycle */
#define TRCD_2			0x00010000	/* SDRAM tRCD = 2 cycles */
#define TRCD_3			0x00018000	/* SDRAM tRCD = 3 cycles */
#define TRCD_4			0x00020000	/* SDRAM tRCD = 4 cycles */
#define TRCD_5			0x00028000	/* SDRAM tRCD = 5 cycles */
#define TRCD_6			0x00030000	/* SDRAM tRCD = 6 cycles */
#define TRCD_7			0x00038000	/* SDRAM tRCD = 7 cycles */
#define TWR_1			0x00080000	/* SDRAM tWR = 1 cycle */
#define TWR_2			0x00100000	/* SDRAM tWR = 2 cycles */
#define TWR_3			0x00180000	/* SDRAM tWR = 3 cycles */
#define PUPSD			0x00200000	/*Power-up start delay */
#define PSM			0x00400000	/* SDRAM power-up sequence = Precharge, mode register set, 8 CBR refresh cycles */
#define PSS				0x00800000	/* enable SDRAM power-up sequence on next SDRAM access */
#define SRFS			0x01000000	/* Start SDRAM self-refresh mode */
#define EBUFE			0x02000000	/* Enable external buffering timing */
#define FBBRW			0x04000000	/* Fast back-to-back read write enable */
#define EMREN			0x10000000	/* Extended mode register enable */
#define TCSR			0x20000000	/* Temp compensated self refresh value 85 deg C */
#define CDDBG			0x40000000	/* Tristate SDRAM controls during bus grant */

/* EBIU_SDBCTL Masks */
#define EB0_E				0x00000001	/* Enable SDRAM external bank 0 */
#define EB0_SZ_16			0x00000000	/* SDRAM external bank size = 16MB */
#define EB0_SZ_32			0x00000002	/* SDRAM external bank size = 32MB */
#define EB0_SZ_64			0x00000004	/* SDRAM external bank size = 64MB */
#define EB0_SZ_128			0x00000006	/* SDRAM external bank size = 128MB */
#define EB0_CAW_8			0x00000000	/* SDRAM external bank column address width = 8 bits */
#define EB0_CAW_9			0x00000010	/* SDRAM external bank column address width = 9 bits */
#define EB0_CAW_10			0x00000020	/* SDRAM external bank column address width = 9 bits */
#define EB0_CAW_11			0x00000030	/* SDRAM external bank column address width = 9 bits */

#define EB1_E				0x00000100	/* Enable SDRAM external bank 1 */
#define EB1__SZ_16			0x00000000	/* SDRAM external bank size = 16MB */
#define EB1__SZ_32			0x00000200	/* SDRAM external bank size = 32MB */
#define EB1__SZ_64			0x00000400	/* SDRAM external bank size = 64MB */
#define EB1__SZ_128			0x00000600	/* SDRAM external bank size = 128MB */
#define EB1__CAW_8			0x00000000	/* SDRAM external bank column address width = 8 bits */
#define EB1__CAW_9			0x00001000	/* SDRAM external bank column address width = 9 bits */
#define EB1__CAW_10			0x00002000	/* SDRAM external bank column address width = 9 bits */
#define EB1__CAW_11			0x00003000	/* SDRAM external bank column address width = 9 bits */

#define EB2__E				0x00010000	/* Enable SDRAM external bank 2 */
#define EB2__SZ_16			0x00000000	/* SDRAM external bank size = 16MB */
#define EB2__SZ_32			0x00020000	/* SDRAM external bank size = 32MB */
#define EB2__SZ_64			0x00040000	/* SDRAM external bank size = 64MB */
#define EB2__SZ_128			0x00060000	/* SDRAM external bank size = 128MB */
#define EB2__CAW_8			0x00000000	/* SDRAM external bank column address width = 8 bits */
#define EB2__CAW_9			0x00100000	/* SDRAM external bank column address width = 9 bits */
#define EB2__CAW_10			0x00200000	/* SDRAM external bank column address width = 9 bits */
#define EB2__CAW_11			0x00300000	/* SDRAM external bank column address width = 9 bits */

#define EB3__E				0x01000000	/* Enable SDRAM external bank 3 */
#define EB3__SZ_16			0x00000000	/* SDRAM external bank size = 16MB */
#define EB3__SZ_32			0x02000000	/* SDRAM external bank size = 32MB */
#define EB3__SZ_64			0x04000000	/* SDRAM external bank size = 64MB */
#define EB3__SZ_128			0x06000000	/* SDRAM external bank size = 128MB */
#define EB3__CAW_8			0x00000000	/* SDRAM external bank column address width = 8 bits */
#define EB3__CAW_9			0x10000000	/* SDRAM external bank column address width = 9 bits */
#define EB3__CAW_10			0x20000000	/* SDRAM external bank column address width = 9 bits */
#define EB3__CAW_11			0x30000000	/* SDRAM external bank column address width = 9 bits */

/* EBIU_SDSTAT Masks */
#define SDCI			0x00000001	/* SDRAM controller is idle  */
#define SDSRA			0x00000002	/* SDRAM SDRAM self refresh is active */
#define SDPUA			0x00000004	/* SDRAM power up active  */
#define SDRS			0x00000008	/* SDRAM is in reset state */
#define SDEASE		    0x00000010	/* SDRAM EAB sticky error status - W1C */
#define BGSTAT			0x00000020	/* Bus granted */

#endif				/* _DEF_BF561_H */
