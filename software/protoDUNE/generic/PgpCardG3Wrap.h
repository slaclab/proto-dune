//---------------------------------------------------------------------------------
// Title         : Kernel Module For PGP To PCI Bridge Card
// Project       : PGP To PCI-E Bridge Card
//---------------------------------------------------------------------------------
// File          : PgpCardG3Wrap.h
// Author        : Ryan Herbst, rherbst@slac.stanford.edu
// Created       : 05/18/2010
//---------------------------------------------------------------------------------
//
//---------------------------------------------------------------------------------
// This file is part of 'SLAC Generic DAQ Software'.
// It is subject to the license terms in the LICENSE.txt file found in the 
// top-level directory of this distribution and at: 
//    https://confluence.slac.stanford.edu/display/ppareg/LICENSE.html. 
// No part of 'SLAC Generic DAQ Software', including this file, 
// may be copied, modified, propagated, or distributed except according to 
// the terms contained in the LICENSE.txt file.
//---------------------------------------------------------------------------------
// Modification history:
// 05/18/2010: created.
//---------------------------------------------------------------------------------
#ifndef __PGP_CARD_WRAP_G3_H__
#define __PGP_CARD_WRAP_G3_H__

#include <linux/types.h>
#include "PgpCardG3Mod.h"

/////////////////////////////////////////////////////////////////////////////
// Send Frame, size in dwords
// int pgpcard_send(int fd, void *buf, size_t count, uint lane, uint vc);

// Receive Frame, size in dwords, return in dwords
// int pgpcard_recv(int fd, void *buf, size_t maxSize, uint *lane, uint *vc, uint *eofe, uint *fifoErr, uint *lengthErr);

// Send PGP OP-Code
// int pgpcard_sendOpCode(int fd, uint opCode);

// Read Status
// int pgpcard_status(int fd, PgpCardStatus *status);

// Reset Counters
// int pgpcard_rstCount(int fd);

// Set/Clear Loopback For Lane
// int pgpcard_setLoop(int fd, uint lane);
// int pgpcard_clrLoop(int fd, uint lane);

// Set/Clear RX Reset For Lane
// int pgpcard_setRxReset(int fd, uint lane);
// int pgpcard_clrRxReset(int fd, uint lane);

// Set/Clear TX Reset For Lane
// int pgpcard_setTxReset(int fd, uint lane);
// int pgpcard_clrTxReset(int fd, uint lane);

// Set EVR Run Code
// int pgpcard_setEvrRunCode(int fd, uint lane uint runCode)

// Set EVR Run Code
// int pgpcard_setEvrAcceptCode(int fd, uint lane uint acceptCode)

// Set EVR Run Delay
// int pgpcard_setEvrRunDelay(int fd, uint lane, uint runDelay)

// Set EVR Run Delay
// int pgpcard_setEvrAcceptDelay(int fd, uint lane, uint acceptDelay)

// Enable/Disable EVR 
// int pgpcard_enableEvr(int fd)
// int pgpcard_disableEvr(int fd)

// Set/Clear EVR Reset
// int pgpcard_setEvrRst(int fd)
// int pgpcard_clrEvrRst(int fd)

// Set/Clear EVR PLL Reset
// int pgpcard_setEvrPllRst(int fd)
// int pgpcard_clrEvrPllRst(int fd)

// Set EVR Virtual Channel Masking
// int pgpcard_evrMask(int fd, uint mask) {

// Set debug
// int pgpcard_setDebug(int fd, uint level);

// Dump Debug
// int pgpcard_dumpDebug(int fd);
/////////////////////////////////////////////////////////////////////////////

// Send Frame, size in dwords
inline int pgpcard_send(int fd, void *buf, size_t size, uint lane, uint vc) {
   PgpCardTx pgpCardTx;

   pgpCardTx.model   = (sizeof(buf));
   pgpCardTx.cmd     = IOCTL_Normal_Write;
   pgpCardTx.pgpVc   = vc;
   pgpCardTx.pgpLane = lane;
   pgpCardTx.size    = size;
   pgpCardTx.data    = (__u32*)buf;

   return(write(fd,&pgpCardTx,sizeof(PgpCardTx)));
}

// Receive Frame, size in dwords, return in dwords
inline int pgpcard_recv(int fd, void *buf, size_t maxSize, uint *lane, uint *vc, uint *eofe, uint *fifoErr, uint *lengthErr) {
   PgpCardRx pgpCardRx;
   int       ret;

   pgpCardRx.maxSize = maxSize;
   pgpCardRx.data    = (__u32*)buf;
   pgpCardRx.model   = sizeof(buf);

   ret = read(fd,&pgpCardRx,sizeof(PgpCardRx));

   *lane      = pgpCardRx.pgpLane;
   *vc        = pgpCardRx.pgpVc;
   *eofe      = pgpCardRx.eofe;
   *fifoErr   = pgpCardRx.fifoErr;
   *lengthErr = pgpCardRx.lengthErr;

   return(ret);
}

// Send PGP OP-Code
inline int pgpcard_sendOpCode(int fd, uint opCode){
   PgpCardTx  t;

   t.model = sizeof(PgpCardTx*);
   t.cmd   = IOCTL_Pgp_OpCode;;
   t.data  = (__u32*) (opCode&0xFF);
   return(write(fd, &t, sizeof(PgpCardTx)));   
}

// Read Status
inline int pgpcard_status(int fd, PgpCardStatus *status) {

   // the buffer is a PgpCardTx on the way in and a PgpCardStatus on the way out
   __u8*      c = (__u8*) status;  // this adheres to strict aliasing rules
   PgpCardTx* p = (PgpCardTx*) c;

   p->model = sizeof(p);
   p->cmd   = IOCTL_Read_Status;
   p->data  = (__u32*)status;
   return(write(fd, p, sizeof(PgpCardStatus)));
}

// Reset Counters
inline int pgpcard_rstCount(int fd) {
   PgpCardTx  t;

   t.model = sizeof(PgpCardTx*);
   t.cmd   = IOCTL_Count_Reset;
   t.data  = (__u32*)0;
   return(write(fd, &t, sizeof(PgpCardTx)));
}

// Set/Clear RX Reset For Lane
inline int pgpcard_setRxReset(int fd, uint lane) {
   PgpCardTx  t;

   t.model = sizeof(PgpCardTx*);
   t.cmd   = IOCTL_Set_Rx_Reset;;
   t.data  = (__u32*) lane;
   return(write(fd, &t, sizeof(PgpCardTx)));
}

inline int pgpcard_clrRxReset(int fd, uint lane){
   PgpCardTx  t;

   t.model = sizeof(PgpCardTx*);
   t.cmd   = IOCTL_Clr_Rx_Reset;
   t.data  = (__u32*) lane;
   return(write(fd, &t, sizeof(PgpCardTx)));
}

// Set/Clear TX Reset For Lane
inline int pgpcard_setTxReset(int fd, uint lane) {
   PgpCardTx  t;

   t.model = sizeof(PgpCardTx*);
   t.cmd   = IOCTL_Set_Tx_Reset;;
   t.data  = (__u32*) lane;
   return(write(fd, &t, sizeof(PgpCardTx)));

}

inline int pgpcard_clrTxReset(int fd, uint lane) {
   PgpCardTx  t;

   t.model = sizeof(PgpCardTx*);
   t.cmd   = IOCTL_Clr_Tx_Reset;
   t.data  = (__u32*) lane;
   return(write(fd, &t, sizeof(PgpCardTx)));
}

// Set/Clear Loopback For Lane
inline int pgpcard_setLoop(int fd, uint lane) {
   PgpCardTx  t;

   t.model = sizeof(PgpCardTx*);
   t.cmd   = IOCTL_Set_Loop;
   t.data  = (__u32*) lane;
   return(write(fd, &t, sizeof(PgpCardTx)));
}

inline int pgpcard_clrLoop(int fd, uint lane) {
   PgpCardTx  t;

   t.model = sizeof(PgpCardTx*);
   t.cmd   = IOCTL_Clr_Loop;
   t.data  = (__u32*) lane;
   return(write(fd, &t, sizeof(PgpCardTx)));
}

// Set EVR Run Code
inline int pgpcard_setEvrRunCode(int fd, uint lane, uint runDelay) {
   PgpCardTx  t;

   t.model = sizeof(PgpCardTx*);
   // Determine command
   switch ( lane ) {   
      case 0x0:
         t.cmd   = IOCTL_Evr_RunCode0;
         break;      
      case 0x1:
         t.cmd   = IOCTL_Evr_RunCode1;
         break;      
      case 0x2:
         t.cmd   = IOCTL_Evr_RunCode2;
         break;      
      case 0x3:
         t.cmd   = IOCTL_Evr_RunCode3;
         break; 
      case 0x4:
         t.cmd   = IOCTL_Evr_RunCode4;
         break;      
      case 0x5:
         t.cmd   = IOCTL_Evr_RunCode5;
         break;      
      case 0x6:
         t.cmd   = IOCTL_Evr_RunCode6;
         break;      
      case 0x7:
         t.cmd   = IOCTL_Evr_RunCode7;
         break;          
      default:
         t.cmd   = IOCTL_NOP;
         break;
   }   
   t.data  = (__u32*) runDelay;
   return(write(fd, &t, sizeof(PgpCardTx)));     
}

// Set EVR Accept Code
inline int pgpcard_setEvrAcceptCode(int fd, uint lane, uint runDelay) {
   PgpCardTx  t;

   t.model = sizeof(PgpCardTx*);
   // Determine command
   switch ( lane ) {   
      case 0x0:
         t.cmd   = IOCTL_Evr_AcceptCode0;
         break;      
      case 0x1:
         t.cmd   = IOCTL_Evr_AcceptCode1;
         break;      
      case 0x2:
         t.cmd   = IOCTL_Evr_AcceptCode2;
         break;      
      case 0x3:
         t.cmd   = IOCTL_Evr_AcceptCode3;
         break; 
      case 0x4:
         t.cmd   = IOCTL_Evr_AcceptCode4;
         break;      
      case 0x5:
         t.cmd   = IOCTL_Evr_AcceptCode5;
         break;      
      case 0x6:
         t.cmd   = IOCTL_Evr_AcceptCode6;
         break;      
      case 0x7:
         t.cmd   = IOCTL_Evr_AcceptCode7;
         break;          
      default:
         t.cmd   = IOCTL_NOP;
         break;
   }   
   t.data  = (__u32*) runDelay;
   return(write(fd, &t, sizeof(PgpCardTx)));   
}

// Set EVR Run Delay
inline int pgpcard_setEvrRunDelay(int fd, uint lane, uint runDelay) {
   PgpCardTx  t;

   t.model = sizeof(PgpCardTx*);
   // Determine command
   switch ( lane ) {   
      case 0x0:
         t.cmd   = IOCTL_Evr_RunDelay0;
         break;      
      case 0x1:
         t.cmd   = IOCTL_Evr_RunDelay1;
         break;      
      case 0x2:
         t.cmd   = IOCTL_Evr_RunDelay2;
         break;      
      case 0x3:
         t.cmd   = IOCTL_Evr_RunDelay3;
         break; 
      case 0x4:
         t.cmd   = IOCTL_Evr_RunDelay4;
         break;      
      case 0x5:
         t.cmd   = IOCTL_Evr_RunDelay5;
         break;      
      case 0x6:
         t.cmd   = IOCTL_Evr_RunDelay6;
         break;      
      case 0x7:
         t.cmd   = IOCTL_Evr_RunDelay7;
         break;          
      default:
         t.cmd   = IOCTL_NOP;
         break;
   }   
   t.data  = (__u32*) runDelay;
   return(write(fd, &t, sizeof(PgpCardTx)));
}

// Set EVR Accept Delay
inline int pgpcard_setEvrAcceptDelay(int fd, uint lane, uint acceptDelay) {
   PgpCardTx  t;

   t.model = sizeof(PgpCardTx*);
   // Determine command
   switch ( lane ) {   
      case 0x0:
         t.cmd   = IOCTL_Evr_AcceptDelay0;
         break;      
      case 0x1:
         t.cmd   = IOCTL_Evr_AcceptDelay1;
         break;      
      case 0x2:
         t.cmd   = IOCTL_Evr_AcceptDelay2;
         break;      
      case 0x3:
         t.cmd   = IOCTL_Evr_AcceptDelay3;
         break; 
      case 0x4:
         t.cmd   = IOCTL_Evr_AcceptDelay4;
         break;      
      case 0x5:
         t.cmd   = IOCTL_Evr_AcceptDelay5;
         break;      
      case 0x6:
         t.cmd   = IOCTL_Evr_AcceptDelay6;
         break;      
      case 0x7:
         t.cmd   = IOCTL_Evr_AcceptDelay7;
         break;          
      default:
         t.cmd   = IOCTL_NOP;
         break;
   }   
   t.data  = (__u32*) acceptDelay;
   return(write(fd, &t, sizeof(PgpCardTx)));
}

// Enable EVR 
inline int pgpcard_enableEvr(int fd) {
   PgpCardTx  t;

   t.model = sizeof(PgpCardTx*);
   t.cmd   = IOCTL_Evr_Enable;
   t.data  = (__u32*) 0x0;
   return(write(fd, &t, sizeof(PgpCardTx)));
}

// Disable EVR 
inline int pgpcard_disableEvr(int fd) {
   PgpCardTx  t;

   t.model = sizeof(PgpCardTx*);
   t.cmd   = IOCTL_Evr_Disable;
   t.data  = (__u32*) 0x0;
   return(write(fd, &t, sizeof(PgpCardTx)));
}

// Set EVR Reset
inline int pgpcard_setEvrRst(int fd) {
   PgpCardTx  t;

   t.model = sizeof(PgpCardTx*);
   t.cmd   = IOCTL_Evr_Set_Reset;
   t.data  = (__u32*) 0x0;
   return(write(fd, &t, sizeof(PgpCardTx)));
}

// Clear EVR Reset
inline int pgpcard_clrEvrRst(int fd) {
   PgpCardTx  t;

   t.model = sizeof(PgpCardTx*);
   t.cmd   = IOCTL_Evr_Clr_Reset;
   t.data  = (__u32*) 0x0;
   return(write(fd, &t, sizeof(PgpCardTx)));
}

// Set EVR PLL Reset
inline int pgpcard_setEvrPllRst(int fd) {
   PgpCardTx  t;

   t.model = sizeof(PgpCardTx*);
   t.cmd   = IOCTL_Evr_Set_PLL_RST;
   t.data  = (__u32*) 0x0;
   return(write(fd, &t, sizeof(PgpCardTx)));
}

// Clear EVR PLL Reset
inline int pgpcard_clrEvrPllRst(int fd) {
   PgpCardTx  t;

   t.model = sizeof(PgpCardTx*);
   t.cmd   = IOCTL_Evr_Clr_PLL_RST;
   t.data  = (__u32*) 0x0;
   return(write(fd, &t, sizeof(PgpCardTx)));
}

// Set EVR Run Code
// Note: 
//    mask[31:28] = Lane[7].VC[3:0]
//    mask[27:24] = Lane[6].VC[3:0]
//    mask[23:20] = Lane[5].VC[3:0]
//    mask[19:16] = Lane[4].VC[3:0]
//    mask[15:12] = Lane[3].VC[3:0]
//    mask[11:08] = Lane[2].VC[3:0]
//    mask[07:04] = Lane[1].VC[3:0]
//    mask[03:00] = Lane[0].VC[3:0]
inline int pgpcard_evrMask(int fd, uint mask) {
   PgpCardTx  t;

   t.model = sizeof(PgpCardTx*);
   t.cmd   = IOCTL_Evr_Mask;
   t.data  = (__u32*) mask;
   return(write(fd, &t, sizeof(PgpCardTx)));
}

// Set debug
inline int pgpcard_setDebug(int fd, uint level) {
   PgpCardTx  t;

   t.model = sizeof(PgpCardTx*);
   t.cmd   = IOCTL_Set_Debug;
   t.data  = (__u32*) level;
   return(write(fd, &t, sizeof(PgpCardTx)));
}

// Dump Debug
inline int pgpcard_dumpDebug(int fd) {
   PgpCardTx  t;

   t.model = sizeof(PgpCardTx*);
   t.cmd   = IOCTL_Dump_Debug;
   return(write(fd, &t, sizeof(PgpCardTx)));
}

#endif
