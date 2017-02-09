//---------------------------------------------------------------------------------
// Title         : Kernel Module For PGP To PCI Bridge Card
// Project       : PGP To PCI-E Bridge Card
//---------------------------------------------------------------------------------
// File          : PgpCardMod.h
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
#ifndef __PGP_CARD_MOD_H__
#define __PGP_CARD_MOD_H__

#include <stdint.h>

// Return values
#define SUCCESS 0
#define ERROR   -1

// Scratchpad write value
#define SPAD_WRITE 0x55441122

// TX Structure
typedef struct {

   uint32_t  model; // large=8, small=4
   uint32_t  cmd; // ioctl commands
   uint32_t* data;
   // Lane & VC
   uint32_t  pgpLane;
   uint32_t  pgpVc;

   // Data
   uint32_t   size;  // dwords

} PgpCardTx;

// RX Structure
typedef struct {
    uint32_t   model; // large=8, small=4
    uint32_t   maxSize; // dwords
    uint32_t*  data;

   // Lane & VC
   uint32_t    pgpLane;
   uint32_t    pgpVc;

   // Data
   uint32_t   rxSize;  // dwords

   // Error flags
   uint32_t   eofe;
   uint32_t   fifoErr;
   uint32_t   lengthErr;

} PgpCardRx;

// Status Structure
typedef struct {

   // General Status
   uint32_t Version;

   // Scratchpad
   uint32_t ScratchPad;

   // PCI Status & Control Registers
   uint32_t PciCommand;
   uint32_t PciStatus;
   uint32_t PciDCommand;
   uint32_t PciDStatus;
   uint32_t PciLCommand;
   uint32_t PciLStatus;
   uint32_t PciLinkState;
   uint32_t PciFunction;
   uint32_t PciDevice;
   uint32_t PciBus;

   // PGP 0 Status
   uint32_t Pgp0LoopBack;
   uint32_t Pgp0RxReset;
   uint32_t Pgp0TxReset;
   uint32_t Pgp0LocLinkReady;
   uint32_t Pgp0RemLinkReady;
   uint32_t Pgp0RxReady;
   uint32_t Pgp0TxReady;
   uint32_t Pgp0RxCount;
   uint32_t Pgp0CellErrCnt;
   uint32_t Pgp0LinkDownCnt;
   uint32_t Pgp0LinkErrCnt;
   uint32_t Pgp0FifoErr;

   // PGP 1 Status
   uint32_t Pgp1LoopBack;
   uint32_t Pgp1RxReset;
   uint32_t Pgp1TxReset;
   uint32_t Pgp1LocLinkReady;
   uint32_t Pgp1RemLinkReady;
   uint32_t Pgp1RxReady;
   uint32_t Pgp1TxReady;
   uint32_t Pgp1RxCount;
   uint32_t Pgp1CellErrCnt;
   uint32_t Pgp1LinkDownCnt;
   uint32_t Pgp1LinkErrCnt;
   uint32_t Pgp1FifoErr;

   // PGP 2 Status
   uint32_t Pgp2LoopBack;
   uint32_t Pgp2RxReset;
   uint32_t Pgp2TxReset;
   uint32_t Pgp2LocLinkReady;
   uint32_t Pgp2RemLinkReady;
   uint32_t Pgp2RxReady;
   uint32_t Pgp2TxReady;
   uint32_t Pgp2RxCount;
   uint32_t Pgp2CellErrCnt;
   uint32_t Pgp2LinkDownCnt;
   uint32_t Pgp2LinkErrCnt;
   uint32_t Pgp2FifoErr;

   // PGP 3 Status
   uint32_t Pgp3LoopBack;
   uint32_t Pgp3RxReset;
   uint32_t Pgp3TxReset;
   uint32_t Pgp3LocLinkReady;
   uint32_t Pgp3RemLinkReady;
   uint32_t Pgp3RxReady;
   uint32_t Pgp3TxReady;
   uint32_t Pgp3RxCount;
   uint32_t Pgp3CellErrCnt;
   uint32_t Pgp3LinkDownCnt;
   uint32_t Pgp3LinkErrCnt;
   uint32_t Pgp3FifoErr;

   // TX Descriptor Status
   uint32_t TxDma3AFull;
   uint32_t TxDma2AFull;
   uint32_t TxDma1AFull;
   uint32_t TxDma0AFull;
   uint32_t TxReadReady;
   uint32_t TxRetFifoCount;
   uint32_t TxCount;
   uint32_t TxWrite;
   uint32_t TxRead;

   // RX Descriptor Status
   uint32_t RxFreeEmpty;
   uint32_t RxFreeFull;
   uint32_t RxFreeValid;
   uint32_t RxFreeFifoCount;
   uint32_t RxReadReady;
   uint32_t RxRetFifoCount;
   uint32_t RxCount;
   uint32_t RxWrite;
   uint32_t RxRead;

} PgpCardStatus;

// IO Control Commands

// Normal Write commmand
#define IOCTL_Normal_Write 0

// Read Status, Pass PgpCardStatus as arg
#define IOCTL_Read_Status 0x01

// Set Debug, Pass Debug Value As Arg
#define IOCTL_Set_Debug 0x02

// Set RX Reset, Pass PGP Channel As Arg
#define IOCTL_Set_Rx_Reset 0x03
#define IOCTL_Clr_Rx_Reset 0x04

// Set TX Reset, Pass PGP Channel As Arg
#define IOCTL_Set_Tx_Reset 0x05
#define IOCTL_Clr_Tx_Reset 0x06

// Set Loopback, Pass PGP Channel As Arg
#define IOCTL_Set_Loop 0x07
#define IOCTL_Clr_Loop 0x08

// Reset counters
#define IOCTL_Count_Reset 0x09

// Dump debug
#define IOCTL_Dump_Debug 0x0A

#endif
