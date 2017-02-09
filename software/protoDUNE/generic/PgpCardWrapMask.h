//---------------------------------------------------------------------------------
// Title         : Kernel Module For PGP To PCI Bridge Card
// Project       : PGP To PCI-E Bridge Card
//---------------------------------------------------------------------------------
// File          : PgpCardWrap.h
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
#ifndef __PGP_CARD_WRAP_H__
#define __PGP_CARD_WRAP_H__

#include <linux/types.h>
#include "PgpCardMod.h"

// Send Frame, size in dwords
// int pgpcard_send(int fd, void *buf, size_t count, uint lane, uint vc, uint cont);

// Send Frame, size in dwords, return in dwords
// int pgpcard_recv(int fd, void *buf, size_t maxSize, uint *lane, uint *vc, uint *eofe, uint *fifoErr, uint *lengthErr, uint *cont);

// Read Status
// int pgpcard_status(int fd, PgpCardStatus *status);

// Set debug
// int pgpcard_setDebug(int fd, uint level);

// Set/Clear RX Reset For Lane
// int pgpcard_setRxReset(int fd, uint lane);
// int pgpcard_clrRxReset(int fd, uint lane);

// Set/Clear TX Reset For Lane
// int pgpcard_setTxReset(int fd, uint lane);
// int pgpcard_clrTxReset(int fd, uint lane);

// Set/Clear Loopback For Lane
// int pgpcard_setLoop(int fd, uint lane);
// int pgpcard_clrLoop(int fd, uint lane);

// Reset Counters
// int pgpcard_rstCount(int fd);

// Dump Debug
// int pgpcard_dumpDebug(int fd);

// int pgpcard_setData(int fd, uint lane, uint data);
// int pgpcard_setMask(int fd, uint mask);

// Send Frame, size in dwords
inline int pgpcard_send(int fd, void *buf, size_t size, uint lane, uint vc, uint cont=0) {
   PgpCardTx pgpCardTx;

   pgpCardTx.model   = (sizeof(buf));
   pgpCardTx.cmd     = IOCTL_Normal_Write;
   pgpCardTx.pgpVc   = vc;
   pgpCardTx.pgpLane = lane;
   pgpCardTx.size    = size;
   pgpCardTx.cont    = cont;
   pgpCardTx.data    = (__u32*)buf;

   return(write(fd,&pgpCardTx,sizeof(PgpCardTx)));
}


// Send Frame, size in dwords, return in dwords
inline int pgpcard_recv(int fd, void *buf, size_t maxSize, uint *lane, uint *vc, uint *eofe, uint *fifoErr, uint *lengthErr, uint *cont=NULL) {
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

   if ( cont != NULL ) *cont      = pgpCardRx.cont;

   return(ret);
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


// Set debug
inline int pgpcard_setDebug(int fd, uint level) {
   PgpCardTx  t;

   t.model = sizeof(PgpCardTx*);
   t.cmd   = IOCTL_Set_Debug;
   t.data  = (__u32*) level;
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

// set/get sideband data For Lane
inline int pgpcard_setData(int fd, uint lane, uint data) {
   PgpCardTx  t;
   uint passData;

   passData  = (data << 8) & 0xFF00;
   passData += lane & 0xFF;

   t.model = sizeof(PgpCardTx*);
   t.cmd   = IOCTL_Set_Data;
   t.data  = (__u32*) passData;
   return(write(fd, &t, sizeof(PgpCardTx)));
}

// set lane/vc rx mask, one bit per vc
inline int pgpcard_setMask(int fd, uint mask) {
   PgpCardTx  t;

   t.model = sizeof(PgpCardTx*);
   t.cmd   = IOCTL_Set_Mask;
   t.data  = (__u32*) mask;
   return(write(fd, &t, sizeof(PgpCardTx)));
}

// Reset Counters
inline int pgpcard_rstCount(int fd) {
   PgpCardTx  t;

   t.model = sizeof(PgpCardTx*);
   t.cmd   = IOCTL_Count_Reset;
   t.data  = (__u32*)0;
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
