/*
 * Copyright (c) 1996, 2003 VIA Networking Technologies, Inc.
 * All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 *
 * File: tether.h
 *
 * Purpose:
 *
 * Author: Tevin Chen
 *
 * Date: Jan. 28, 1997
 *
 */



#ifndef __TETHER_H__
#define __TETHER_H__


#if !defined(__TTYPE_H__)
#include "ttype.h"
#endif




/*---------------------  Export Definitions -------------------------*/
//
// constants
//
#define U_ETHER_ADDR_LEN    6           // Ethernet address length
#define U_TYPE_LEN          2           //
#define U_CRC_LEN           4           //
#define U_HEADER_LEN        (U_ETHER_ADDR_LEN * 2 + U_TYPE_LEN)
#define U_ETHER_ADDR_STR_LEN (U_ETHER_ADDR_LEN * 2 + 1)
                                        // Ethernet address string length

#define MIN_DATA_LEN        46          // min data length
#define MAX_DATA_LEN        1500        // max data length

#define MIN_PACKET_LEN      (MIN_DATA_LEN + U_HEADER_LEN)
                                        // 60
                                        // min total packet length (tx)
#define MAX_PACKET_LEN      (MAX_DATA_LEN + U_HEADER_LEN)
                                        // 1514
                                        // max total packet length (tx)

#define MAX_LOOKAHEAD_SIZE  MAX_PACKET_LEN

#define U_MULTI_ADDR_LEN    8           // multicast address length


#ifdef __BIG_ENDIAN

#define TYPE_PKT_IP         0x0800      //
#define TYPE_PKT_ARP        0x0806      //
#define TYPE_PKT_RARP       0x8035      //
#define TYPE_PKT_IPX	    0x8137	    //
#define TYPE_PKT_802_1x     0x888e
#define TYPE_PKT_PreAuth    0x88C7

#define TYPE_PKT_PING_M_REQ 0x8011      // master reguest
#define TYPE_PKT_PING_S_GNT 0x8022      // slave grant
#define TYPE_PKT_PING_M     0x8077      // pingpong master packet
#define TYPE_PKT_PING_S     0x8088      // pingpong slave packet
#define TYPE_PKT_WOL_M_REQ  0x8033      // WOL waker request
#define TYPE_PKT_WOL_S_GNT  0x8044      // WOL sleeper grant
#define TYPE_MGMT_PROBE_RSP 0x5000
#define TYPE_PKT_VNT_DIAG   0x8011      // Diag Pkt
#define TYPE_PKT_VNT_PER    0x8888      // Diag PER Pkt
//
// wFrameCtl field in the S802_11Header
//
// NOTE....
//   in network byte order, high byte is going first
#define FC_TODS             0x0001
#define FC_FROMDS           0x0002
#define FC_MOREFRAG         0x0004
#define FC_RETRY            0x0008
#define FC_POWERMGT         0x0010
#define FC_MOREDATA         0x0020
#define FC_WEP              0x0040
#define TYPE_802_11_ATIM    0x9000

#define TYPE_802_11_DATA    0x0800
#define TYPE_802_11_CTL     0x0400
#define TYPE_802_11_MGMT    0x0000
#define TYPE_802_11_MASK    0x0C00
#define TYPE_SUBTYPE_MASK   0xFC00
#define TYPE_802_11_NODATA  0x4000
#define TYPE_DATE_NULL      0x4800

#define TYPE_CTL_PSPOLL     0xa400
#define TYPE_CTL_RTS        0xb400
#define TYPE_CTL_CTS        0xc400
#define TYPE_CTL_ACK        0xd400


//#define WEP_IV_MASK         0xFFFFFF00

#else //if LITTLE_ENDIAN
//
// wType field in the SEthernetHeader
//
// NOTE....
//   in network byte order, high byte is going first
#define TYPE_PKT_IP         0x0008      //
#define TYPE_PKT_ARP        0x0608      //
#define TYPE_PKT_RARP       0x3580      //
#define TYPE_PKT_IPX	    0x3781	    //

#define TYPE_PKT_802_1x     0x8e88
#define TYPE_PKT_PreAuth    0xC788

#define TYPE_PKT_PING_M_REQ 0x1180      // master reguest
#define TYPE_PKT_PING_S_GNT 0x2280      // slave grant
#define TYPE_PKT_PING_M     0x7780      // pingpong master packet
#define TYPE_PKT_PING_S     0x8880      // pingpong slave packet
#define TYPE_PKT_WOL_M_REQ  0x3380      // WOL waker request
#define TYPE_PKT_WOL_S_GNT  0x4480      // WOL sleeper grant
#define TYPE_MGMT_PROBE_RSP 0x0050
#define TYPE_PKT_VNT_DIAG   0x1180      // Diag Pkt
#define TYPE_PKT_VNT_PER    0x8888      // Diag PER Pkt
//
// wFrameCtl field in the S802_11Header
//
// NOTE....
//   in network byte order, high byte is going first
#define FC_TODS             0x0100
#define FC_FROMDS           0x0200
#define FC_MOREFRAG         0x0400
#define FC_RETRY            0x0800
#define FC_POWERMGT         0x1000
#define FC_MOREDATA         0x2000
#define FC_WEP              0x4000
#define TYPE_802_11_ATIM    0x0090

#define TYPE_802_11_DATA    0x0008
#define TYPE_802_11_CTL     0x0004
#define TYPE_802_11_MGMT    0x0000
#define TYPE_802_11_MASK    0x000C
#define TYPE_SUBTYPE_MASK   0x00FC
#define TYPE_802_11_NODATA  0x0040
#define TYPE_DATE_NULL      0x0048

#define TYPE_CTL_PSPOLL     0x00a4
#define TYPE_CTL_RTS        0x00b4
#define TYPE_CTL_CTS        0x00c4
#define TYPE_CTL_ACK        0x00d4


//#define WEP_IV_MASK         0x00FFFFFF

#endif //#ifdef __BIG_ENDIAN

#define WEP_IV_MASK         0x00FFFFFF

/*---------------------  Export Types  ------------------------------*/
//
// Ethernet packet
//
typedef struct tagSEthernetHeader {
    BYTE    abyDstAddr[U_ETHER_ADDR_LEN];
    BYTE    abySrcAddr[U_ETHER_ADDR_LEN];
    WORD    wType;
}__attribute__ ((__packed__))
SEthernetHeader, DEF* PSEthernetHeader;


//
// 802_3 packet
//
typedef struct tagS802_3Header {
    BYTE    abyDstAddr[U_ETHER_ADDR_LEN];
    BYTE    abySrcAddr[U_ETHER_ADDR_LEN];
    WORD    wLen;
}__attribute__ ((__packed__))
S802_3Header, DEF* PS802_3Header;

//
// 802_11 packet
//
typedef struct tagS802_11Header {
    WORD    wFrameCtl;
    WORD    wDurationID;
    BYTE    abyAddr1[U_ETHER_ADDR_LEN];
    BYTE    abyAddr2[U_ETHER_ADDR_LEN];
    BYTE    abyAddr3[U_ETHER_ADDR_LEN];
    WORD    wSeqCtl;
    BYTE    abyAddr4[U_ETHER_ADDR_LEN];
}__attribute__ ((__packed__))
S802_11Header, DEF* PS802_11Header;

/*---------------------  Export Macros ------------------------------*/
// Frame type macro

#define IS_MULTICAST_ADDRESS(pbyEtherAddr)          \
    ((*(PBYTE)(pbyEtherAddr) & 0x01) == 1)

#define IS_BROADCAST_ADDRESS(pbyEtherAddr) (        \
    (*(PDWORD)(pbyEtherAddr) == 0xFFFFFFFFL) &&     \
    (*(PWORD)((PBYTE)(pbyEtherAddr) + 4) == 0xFFFF) \
)

#define IS_NULL_ADDRESS(pbyEtherAddr) (             \
    (*(PDWORD)(pbyEtherAddr) == 0L) &&              \
    (*(PWORD)((PBYTE)(pbyEtherAddr) + 4) == 0)      \
)

#define IS_ETH_ADDRESS_EQUAL(pbyAddr1, pbyAddr2) (  \
    (*(PDWORD)(pbyAddr1) == *(PDWORD)(pbyAddr2)) && \
    (*(PWORD)((PBYTE)(pbyAddr1) + 4) ==             \
    *(PWORD)((PBYTE)(pbyAddr2) + 4))                \
)

/*---------------------  Export Classes  ----------------------------*/

/*---------------------  Export Variables  --------------------------*/

/*---------------------  Export Functions  --------------------------*/
#ifdef __cplusplus
extern "C" {                            /* Assume C declarations for C++ */
#endif /* __cplusplus */


BYTE ETHbyGetHashIndexByCrc32(PBYTE pbyMultiAddr);
//BYTE ETHbyGetHashIndexByCrc(PBYTE pbyMultiAddr);
BOOL ETHbIsBufferCrc32Ok(PBYTE pbyBuffer, UINT cbFrameLength);


#ifdef __cplusplus
}                                       /* End of extern "C" { */
#endif /* __cplusplus */




#endif // __TETHER_H__



