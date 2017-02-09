//---------------------------------------------------------------------------------
// Title         : Kernel Module For PGP To PCI Bridge Card
// Project       : PGP To PCI-E Bridge Card
//---------------------------------------------------------------------------------
// File          : PgpCardG3Mod.h
// Author        : Ryan Herbst, rherbst@slac.stanford.edu
// Created       : 09/20/2013
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
// 09/20/2013: created.
//---------------------------------------------------------------------------------

#ifndef __PGP_CARD_G3_MOD_H__
#define __PGP_CARD_G3_MOD_H__

#include <linux/types.h>

// Return values
#define SUCCESS 0
#define ERROR   -1

// Scratchpad write value
#define SPAD_WRITE 0x55441122

// TX Structure
typedef struct {

   __u32  model; // large=8, small=4
   __u32  cmd; // ioctl commands
   __u32* data;
   // Lane & VC
   __u32  pgpLane;
   __u32  pgpVc;

   // Data
   __u32   size;  // dwords

} PgpCardTx;

// RX Structure
typedef struct {
    __u32   model; // large=8, small=4
    __u32   maxSize; // dwords
    __u32*  data;

   // Lane & VC
   __u32    pgpLane;
   __u32    pgpVc;

   // Data
   __u32   rxSize;  // dwords

   // Error flags
   __u32   eofe;
   __u32   fifoErr;
   __u32   lengthErr;

} PgpCardRx;

// Status Structure
typedef struct {

   // General Status
   __u32 Version;
   __u32 SerialNumber[2];
   __u32 ScratchPad;
   __u32 BuildStamp[64];
   __u32 CountReset;
   __u32 CardReset;

   // PCI Status & Control Registers
   __u32 PciCommand;
   __u32 PciStatus;
   __u32 PciDCommand;
   __u32 PciDStatus;
   __u32 PciLCommand;
   __u32 PciLStatus;
   __u32 PciLinkState;
   __u32 PciFunction;
   __u32 PciDevice;
   __u32 PciBus;
   __u32 PciBaseHdwr;
   __u32 PciBaseLen;   

   // PGP Status
   __u32 PpgRate;
   __u32 PgpLoopBack[8];
   __u32 PgpTxReset[8];
   __u32 PgpRxReset[8];
   __u32 PgpTxPllRst[2];
   __u32 PgpRxPllRst[2];
   __u32 PgpTxPllRdy[2];
   __u32 PgpRxPllRdy[2];   
   __u32 PgpLocLinkReady[8];
   __u32 PgpRemLinkReady[8];
   __u32 PgpRxCount[8][4];
   __u32 PgpCellErrCnt[8];
   __u32 PgpLinkDownCnt[8];
   __u32 PgpLinkErrCnt[8];
   __u32 PgpFifoErrCnt[8];
   
   // EVR Status & Control Registers   
   __u32 EvrRunCode[8];   
   __u32 EvrAcceptCode[8];   
   __u32 EvrEnHdrCheck[8][4];   
   __u32 EvrEnable;   
   __u32 EvrReady;   
   __u32 EvrReset;   
   __u32 EvrPllRst;   
   __u32 EvrErrCnt;   
   __u32 EvrRunDelay[8];   
   __u32 EvrAcceptDelay[8];   
   
   // RX Descriptor Status
   __u32 RxFreeFull[8];
   __u32 RxFreeValid[8];
   __u32 RxFreeFifoCount[8];
   __u32 RxReadReady;
   __u32 RxRetFifoCount;   
   __u32 RxCount;
   __u32 RxWrite;
   __u32 RxRead;
 
   // TX Descriptor Status
   __u32 TxDmaAFull[8];
   __u32 TxFifoCnt[8];
   __u32 TxReadReady;
   __u32 TxRetFifoCount;
   __u32 TxCount;
   __u32 TxWrite;
   __u32 TxRead;

} PgpCardStatus;

// Address Map, offset from base
struct PgpCardReg {
   //PciApp.vhd  
   __u32 version;       // Software_Addr = 0x000,        Firmware_Addr(13 downto 2) = 0x000
   __u32 serNumLower;   // Software_Addr = 0x004,        Firmware_Addr(13 downto 2) = 0x001
   __u32 serNumUpper;   // Software_Addr = 0x008,        Firmware_Addr(13 downto 2) = 0x002
   __u32 scratch;       // Software_Addr = 0x00C,        Firmware_Addr(13 downto 2) = 0x003
   __u32 cardRstStat;   // Software_Addr = 0x010,        Firmware_Addr(13 downto 2) = 0x004
   __u32 irq;           // Software_Addr = 0x014,        Firmware_Addr(13 downto 2) = 0x005 
   __u32 pgpRate;       // Software_Addr = 0x018,        Firmware_Addr(13 downto 2) = 0x006
   __u32 reboot;        // Software_Addr = 0x01C,        Firmware_Addr(13 downto 2) = 0x007
   __u32 pgpOpCode;     // Software_Addr = 0x020,        Firmware_Addr(13 downto 2) = 0x008
   __u32 sysSpare0[2];  // Software_Addr = 0x028:0x024,  Firmware_Addr(13 downto 2) = 0x00A:0x009
   __u32 pciStat[4];    // Software_Addr = 0x038:0x02C,  Firmware_Addr(13 downto 2) = 0x00E:0x00B
   __u32 sysSpare1;     // Software_Addr = 0x03C,        Firmware_Addr(13 downto 2) = 0x00F 
   
   __u32 evrCardStat[4];// Software_Addr = 0x048:0x040,  Firmware_Addr(13 downto 2) = 0x012:0x010  
   __u32 evrSpare0[12]; // Software_Addr = 0x07C:0x04C,  Firmware_Addr(13 downto 2) = 0x01F:0x013
   
   __u32 pgpCardStat[2];// Software_Addr = 0x084:0x080,  Firmware_Addr(13 downto 2) = 0x021:0x020       
   __u32 pgpSpare0[62]; // Software_Addr = 0x17C:0x088,  Firmware_Addr(13 downto 2) = 0x05F:0x022
   
   __u32 runCode[8];   // Software_Addr = 0x19C:0x180,  Firmware_Addr(13 downto 2) = 0x067:0x060       
   __u32 acceptCode[8];// Software_Addr = 0x1BC:0x1A0,  Firmware_Addr(13 downto 2) = 0x06F:0x068         
      
   __u32 runDelay[8];   // Software_Addr = 0x1DC:0x1C0,  Firmware_Addr(13 downto 2) = 0x077:0x070       
   __u32 acceptDelay[8];// Software_Addr = 0x1FC:0x1E0,  Firmware_Addr(13 downto 2) = 0x07F:0x078       

   __u32 pgpLaneStat[8];// Software_Addr = 0x21C:0x200,  Firmware_Addr(13 downto 2) = 0x087:0x080       
   __u32 pgpSpare1[56]; // Software_Addr = 0x2FC:0x220,  Firmware_Addr(13 downto 2) = 0x0BF:0x088
   __u32 BuildStamp[64];// Software_Addr = 0x3FC:0x300,  Firmware_Addr(13 downto 2) = 0x0FF:0x0C0
   
   //PciRxDesc.vhd   
   __u32 rxFree[8];     // Software_Addr = 0x41C:0x400,  Firmware_Addr(13 downto 2) = 0x107:0x100   
   __u32 rxSpare0[24];  // Software_Addr = 0x47C:0x420,  Firmware_Addr(13 downto 2) = 0x11F:0x108
   __u32 rxFreeStat[8]; // Software_Addr = 0x49C:0x480,  Firmware_Addr(13 downto 2) = 0x127:0x120      
   __u32 rxSpare1[24];  // Software_Addr = 0x4FC:0x4A0,  Firmware_Addr(13 downto 2) = 0x13F:0x128
   __u32 rxMaxFrame;    // Software_Addr = 0x500,        Firmware_Addr(13 downto 2) = 0x140 
   __u32 rxCount;       // Software_Addr = 0x504,        Firmware_Addr(13 downto 2) = 0x141 
   __u32 rxStatus;      // Software_Addr = 0x508,        Firmware_Addr(13 downto 2) = 0x142
   __u32 rxRead[2];     // Software_Addr = 0x510:0x50C,  Firmware_Addr(13 downto 2) = 0x144:0x143      
   __u32 rxSpare2[187]; // Software_Addr = 0x77C:0x514,  Firmware_Addr(13 downto 2) = 0x1FF:0x145
   
   //PciTxDesc.vhd
   __u32 txWrA[8];      // Software_Addr = 0x81C:0x800,  Firmware_Addr(13 downto 2) = 0x207:0x200   
   __u32 txFifoCnt[8];  // Software_Addr = 0x83C:0x820,  Firmware_Addr(13 downto 2) = 0x20F:0x208
   __u32 txSpare0[16];  // Software_Addr = 0x87C:0x840,  Firmware_Addr(13 downto 2) = 0x21F:0x210
   __u32 txWrB[8];      // Software_Addr = 0x89C:0x880,  Firmware_Addr(13 downto 2) = 0x227:0x220      
   __u32 txSpare1[24];  // Software_Addr = 0x8FC:0x8A0,  Firmware_Addr(13 downto 2) = 0x23F:0x228   
   __u32 txStat[2];     // Software_Addr = 0x904:0x900,  Firmware_Addr(13 downto 2) = 0x241:0x240      
   __u32 txCount;       // Software_Addr = 0x908,        Firmware_Addr(13 downto 2) = 0x242  
   __u32 txRead;        // Software_Addr = 0x90C,        Firmware_Addr(13 downto 2) = 0x243  
};

//////////////////////
// IO Control Commands
//////////////////////

// Normal Write command
#define IOCTL_Normal_Write 0x00

// Read Status, Pass PgpCardStatus as arg
#define IOCTL_Read_Status 0x01

// Reset counters
#define IOCTL_Count_Reset 0x02

// No Operation
#define IOCTL_NOP 0x03

// No Operation
#define IOCTL_Pgp_OpCode 0x04

// Set Loopback, Pass PGP Channel As Arg
#define IOCTL_Set_Loop 0x10
#define IOCTL_Clr_Loop 0x11

// Set RX Reset, Pass PGP Channel As Arg
#define IOCTL_Set_Rx_Reset 0x12
#define IOCTL_Clr_Rx_Reset 0x13

// Set TX Reset, Pass PGP Channel As Arg
#define IOCTL_Set_Tx_Reset 0x14
#define IOCTL_Clr_Tx_Reset 0x15

// Set EVR configuration
#define IOCTL_Evr_RunCode     0x20
#define IOCTL_Evr_AcceptCode  0x21
#define IOCTL_Evr_Enable      0x22
#define IOCTL_Evr_Disable     0x23
#define IOCTL_Evr_Set_Reset   0x24
#define IOCTL_Evr_Clr_Reset   0x25
#define IOCTL_Evr_Set_PLL_RST 0x26
#define IOCTL_Evr_Clr_PLL_RST 0x27
#define IOCTL_Evr_Mask        0x28

#define IOCTL_Evr_RunCode0    0x30
#define IOCTL_Evr_RunCode1    0x31
#define IOCTL_Evr_RunCode2    0x32
#define IOCTL_Evr_RunCode3    0x33
#define IOCTL_Evr_RunCode4    0x34
#define IOCTL_Evr_RunCode5    0x35
#define IOCTL_Evr_RunCode6    0x36
#define IOCTL_Evr_RunCode7    0x37

#define IOCTL_Evr_AcceptCode0 0x38
#define IOCTL_Evr_AcceptCode1 0x39
#define IOCTL_Evr_AcceptCode2 0x3A
#define IOCTL_Evr_AcceptCode3 0x3B
#define IOCTL_Evr_AcceptCode4 0x3C
#define IOCTL_Evr_AcceptCode5 0x3D
#define IOCTL_Evr_AcceptCode6 0x3E
#define IOCTL_Evr_AcceptCode7 0x3F

#define IOCTL_Evr_RunDelay0    0x40
#define IOCTL_Evr_RunDelay1    0x41
#define IOCTL_Evr_RunDelay2    0x42
#define IOCTL_Evr_RunDelay3    0x43
#define IOCTL_Evr_RunDelay4    0x44
#define IOCTL_Evr_RunDelay5    0x45
#define IOCTL_Evr_RunDelay6    0x46
#define IOCTL_Evr_RunDelay7    0x47

#define IOCTL_Evr_AcceptDelay0 0x48
#define IOCTL_Evr_AcceptDelay1 0x49
#define IOCTL_Evr_AcceptDelay2 0x4A
#define IOCTL_Evr_AcceptDelay3 0x4B
#define IOCTL_Evr_AcceptDelay4 0x4C
#define IOCTL_Evr_AcceptDelay5 0x4D
#define IOCTL_Evr_AcceptDelay6 0x4E
#define IOCTL_Evr_AcceptDelay7 0x4F

// Set Debug, Pass Debug Value As Arg
#define IOCTL_Set_Debug 0xFE

// Dump debug
#define IOCTL_Dump_Debug 0xFF

#endif
