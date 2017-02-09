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

#include <stdint.h>
#include "PgpCardMod.h"

// Send Frame, size in dwords
// int32_t pgpcard_send(int32_t fd, void *buf, size_t count, uint32_t lane, uint32_t vc);

// Send Frame, size in dwords, return in dwords
// int32_t pgpcard_recv(int32_t fd, void *buf, size_t maxSize, uint32_t *lane, uint32_t *vc, uint32_t *eofe, uint32_t *fifoErr, uint32_t *lengthErr);

// Read Status
// int32_t pgpcard_status(int32_t fd, PgpCardStatus *status);

// Set debug
// int32_t pgpcard_setDebug(int32_t fd, uint32_t level);

// Set/Clear RX Reset For Lane
// int32_t pgpcard_setRxReset(int32_t fd, uint32_t lane);
// int32_t pgpcard_clrRxReset(int32_t fd, uint32_t lane);

// Set/Clear TX Reset For Lane
// int32_t pgpcard_setTxReset(int32_t fd, uint32_t lane);
// int32_t pgpcard_clrTxReset(int32_t fd, uint32_t lane);

// Set/Clear Loopback For Lane
// int32_t pgpcard_setLoop(int32_t fd, uint32_t lane);
// int32_t pgpcard_clrLoop(int32_t fd, uint32_t lane);

// Reset Counters
// int32_t pgpcard_rstCount(int32_t fd);

// Dump Debug
// int32_t pgpcard_dumpDebug(int32_t fd);


// Send Frame, size in dwords
inline int32_t pgpcard_send(int32_t fd, void *buf, size_t size, uint32_t lane, uint32_t vc) {
   PgpCardTx pgpCardTx;

   pgpCardTx.model   = (sizeof(buf));
   pgpCardTx.cmd     = IOCTL_Normal_Write;
   pgpCardTx.pgpVc   = vc;
   pgpCardTx.pgpLane = lane;
   pgpCardTx.size    = size;
   pgpCardTx.data    = (uint32_t*)buf;

   return(write(fd,&pgpCardTx,sizeof(PgpCardTx)));
}


// Send Frame, size in dwords, return in dwords
inline int32_t pgpcard_recv(int32_t fd, void *buf, size_t maxSize, uint32_t *lane, uint32_t *vc, uint32_t *eofe, uint32_t *fifoErr, uint32_t *lengthErr) {
   PgpCardRx pgpCardRx;
   int32_t       ret;

   pgpCardRx.maxSize = maxSize;
   pgpCardRx.data    = (uint32_t*)buf;
   pgpCardRx.model   = sizeof(buf);

   ret = read(fd,&pgpCardRx,sizeof(PgpCardRx));

   *lane      = pgpCardRx.pgpLane;
   *vc        = pgpCardRx.pgpVc;
   *eofe      = pgpCardRx.eofe;
   *fifoErr   = pgpCardRx.fifoErr;
   *lengthErr = pgpCardRx.lengthErr;

   return(ret);
}


// Read Status
inline int32_t pgpcard_status(int32_t fd, PgpCardStatus *status) {

   // the buffer is a PgpCardTx on the way in and a PgpCardStatus on the way out
   uint8_t*   c = (uint8_t*) status;  // this adheres to strict aliasing rules
   PgpCardTx* p = (PgpCardTx*) c;

   p->model = sizeof(p);
   p->cmd   = IOCTL_Read_Status;
   p->data  = (uint32_t*)status;
   return(write(fd, p, sizeof(PgpCardStatus)));
}


// Set debug
inline int32_t pgpcard_setDebug(int32_t fd, uint32_t level) {
   PgpCardTx  t;

   t.model = sizeof(PgpCardTx*);
   t.cmd   = IOCTL_Set_Debug;
   t.data  = (uint32_t*) level;
   return(write(fd, &t, sizeof(PgpCardTx)));
}

// Set/Clear RX Reset For Lane
inline int32_t pgpcard_setRxReset(int32_t fd, uint32_t lane) {
   PgpCardTx  t;

   t.model = sizeof(PgpCardTx*);
   t.cmd   = IOCTL_Set_Rx_Reset;;
   t.data  = (uint32_t*) lane;
   return(write(fd, &t, sizeof(PgpCardTx)));
}

inline int32_t pgpcard_clrRxReset(int32_t fd, uint32_t lane){
   PgpCardTx  t;

   t.model = sizeof(PgpCardTx*);
   t.cmd   = IOCTL_Clr_Rx_Reset;
   t.data  = (uint32_t*) lane;
   return(write(fd, &t, sizeof(PgpCardTx)));
}

// Set/Clear TX Reset For Lane
inline int32_t pgpcard_setTxReset(int32_t fd, uint32_t lane) {
   PgpCardTx  t;

   t.model = sizeof(PgpCardTx*);
   t.cmd   = IOCTL_Set_Tx_Reset;;
   t.data  = (uint32_t*) lane;
   return(write(fd, &t, sizeof(PgpCardTx)));

}

inline int32_t pgpcard_clrTxReset(int32_t fd, uint32_t lane) {
   PgpCardTx  t;

   t.model = sizeof(PgpCardTx*);
   t.cmd   = IOCTL_Clr_Tx_Reset;
   t.data  = (uint32_t*) lane;
   return(write(fd, &t, sizeof(PgpCardTx)));
}

// Set/Clear Loopback For Lane
inline int32_t pgpcard_setLoop(int32_t fd, uint32_t lane) {
   PgpCardTx  t;

   t.model = sizeof(PgpCardTx*);
   t.cmd   = IOCTL_Set_Loop;
   t.data  = (uint32_t*) lane;
   return(write(fd, &t, sizeof(PgpCardTx)));
}

inline int32_t pgpcard_clrLoop(int32_t fd, uint32_t lane) {
   PgpCardTx  t;

   t.model = sizeof(PgpCardTx*);
   t.cmd   = IOCTL_Clr_Loop;
   t.data  = (uint32_t*) lane;
   return(write(fd, &t, sizeof(PgpCardTx)));
}

// Reset Counters
inline int32_t pgpcard_rstCount(int32_t fd) {
   PgpCardTx  t;

   t.model = sizeof(PgpCardTx*);
   t.cmd   = IOCTL_Count_Reset;
   t.data  = (uint32_t*)0;
   return(write(fd, &t, sizeof(PgpCardTx)));
}

// Dump Debug
inline int32_t pgpcard_dumpDebug(int32_t fd) {
   PgpCardTx  t;

   t.model = sizeof(PgpCardTx*);
   t.cmd   = IOCTL_Dump_Debug;
   return(write(fd, &t, sizeof(PgpCardTx)));
}

#endif
