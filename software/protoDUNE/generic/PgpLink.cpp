//-----------------------------------------------------------------------------
// File          : PgpLink.cpp
// Author        : Ryan Herbst  <rherbst@slac.stanford.edu>
// Created       : 04/12/2011
// Project       : General Purpose
//-----------------------------------------------------------------------------
// Description :
// PGP communications link
//-----------------------------------------------------------------------------
// This file is part of 'SLAC Generic DAQ Software'.
// It is subject to the license terms in the LICENSE.txt file found in the 
// top-level directory of this distribution and at: 
//    https://confluence.slac.stanford.edu/display/ppareg/LICENSE.html. 
// No part of 'SLAC Generic DAQ Software', including this file, 
// may be copied, modified, propagated, or distributed except according to 
// the terms contained in the LICENSE.txt file.
// Proprietary and confidential to SLAC.
//-----------------------------------------------------------------------------
// Modification history :
// 04/12/2011: created
//-----------------------------------------------------------------------------
#include <PgpLink.h>
#include <PgpCardMod.h>
#include <PgpCardWrap.h>
#include <sstream>
#include "Register.h"
#include "Command.h"
#include "Data.h"
#include <fcntl.h>
#include <iostream>
#include <iomanip>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <sys/select.h>
using namespace std;


// Receive Thread
void PgpLink::rxHandler() {
   uint32_t      *rxBuff;
   int32_t        maxFd;
   struct timeval timeout;
   int32_t        ret;
   fd_set         fds;
   Data           *data;
   uint32_t       lane;
   uint32_t       vc;
   uint32_t       eofe;
   uint32_t       fifoErr;
   uint32_t       lengthErr;
   uint32_t       vcMaskRx;
   uint32_t       laneMaskRx;
   uint32_t       vcMask;
   uint32_t       laneMask;

   // Init buffer
   rxBuff = (uint32_t *) malloc(sizeof(uint32_t)*maxRxTx_);

   // While enabled
   while ( runEnable_ ) {

      // Init fds
      FD_ZERO(&fds);
      FD_SET(fd_,&fds);
      maxFd = fd_;

      // Setup timeout
      timeout.tv_sec  = 0;
      timeout.tv_usec = 1000;

      // Select
      if ( select(maxFd+1, &fds, NULL, NULL, &timeout) <= 0 ) continue;

      // Data is available
      if ( FD_ISSET(fd_,&fds) ) {

         // Setup and attempt receive
         ret = pgpcard_recv(fd_, rxBuff, maxRxTx_, &lane, &vc, &eofe, &fifoErr, &lengthErr);

         // No data
         if ( ret <= 0 ) continue;

         // Bad size or error
         if ( ret < 4 || eofe || fifoErr || lengthErr ) {
            if ( debug_ ) {
               cout << "PgpLink::ioHandler -> "
                    << "Error in data receive. Rx=" << dec << ret
                    << ", Lane=" << dec << lane << ", Vc=" << dec << vc
                    << ", EOFE=" << dec << eofe << ", FifoErr=" << dec << fifoErr
                    << ", LengthErr=" << dec << lengthErr << endl;
            }
            errorCount_++;
            continue;
         }

         // Check for data packet
         vcMaskRx   = (0x1 << vc);
         laneMaskRx = (0x1 << lane);
         vcMask     = (dataSource_ & 0xF);
         laneMask   = ((dataSource_ >> 4) & 0xF);

         if ( (vcMaskRx & vcMask) != 0 && (laneMaskRx & laneMask) != 0 ) {
            data = new Data(rxBuff,ret);
            dataQueue_.push(data);
            dataThreadWakeup();
         }

         // Reformat header for register rx
         else {

            // Data matches outstanding register request
            if ( memcmp(rxBuff,regBuff_,8) == 0 && (uint32_t)(ret-3) == regReqEntry_->size()) {
               if ( ! regReqWrite_ ) {
                  if ( rxBuff[ret-1] == 0 ) 
                     memcpy(regReqEntry_->data(),&(rxBuff[2]),(regReqEntry_->size()*4));
                  else memset(regReqEntry_->data(),0xFF,(regReqEntry_->size()*4));
               }
               regReqEntry_->setStatus(rxBuff[ret-1]);
               regRespCnt_++;
               mainThreadWakeup();
            }

            // Unexpected frame
            else {
               unexpCount_++;
               if ( debug_ ) {
                  cout << "PgpLink::rxHandler -> Unexpected frame received"
                       << " Comp=" << dec << (memcmp(rxBuff,regBuff_,8))
                       << " Word0_Exp=0x" << hex << regBuff_[0]
                       << " Word0_Got=0x" << hex << rxBuff[0]
                       << " Word1_Exp=0x" << hex << regBuff_[1]
                       << " Word1_Got=0x" << hex << rxBuff[1]
                       << " ExpSize=" << dec << regReqEntry_->size()
                       << " GotSize=" << dec << (ret-3) 
                       << " VcMaskRx=0x" << hex << vcMaskRx
                       << " VcMask=0x" << hex << vcMask
                       << " LaneMaskRx=0x" << hex << laneMaskRx
                       << " LaneMask=0x" << hex << laneMask << endl;
               }
            }
         }
      }
   }

   free(rxBuff);
}

// Transmit thread
void PgpLink::ioHandler() {
   uint32_t           cmdBuff[4];
   uint32_t           runBuff[4];
   uint32_t           lastReqCnt;
   uint32_t           lastCmdCnt;
   uint32_t           lastRunCnt;
   uint32_t           lastDataCnt;
   uint32_t           runVc;
   uint32_t           runLane;
   uint32_t           regVc;
   uint32_t           regLane;
   uint32_t           cmdVc;
   uint32_t           cmdLane;
   uint32_t           dataVc;
   uint32_t           dataLane;
   
   // Setup
   lastReqCnt  = regReqCnt_;
   lastCmdCnt  = cmdReqCnt_;
   lastRunCnt  = runReqCnt_;
   lastDataCnt = dataReqCnt_;

   // Init register buffer
   regBuff_ = (uint32_t *) malloc(sizeof(uint32_t)*maxRxTx_);

   // While enabled
   while ( runEnable_ ) {

      // Run Command TX is pending
      if ( lastRunCnt != runReqCnt_ ) {

         // Setup tx buffer
         runBuff[0]  = 0;
         runBuff[1]  = runReqEntry_->opCode() & 0xFF;
         runBuff[2]  = 0;
         runBuff[3]  = 0;
 
         // Setup transmit
         runLane = (runReqEntry_->opCode()>>12) & 0xF;
         runVc   = (runReqEntry_->opCode()>>8)  & 0xF;
        
         // Send data
         pgpcard_send(fd_, runBuff, 4, runLane, runVc);
  
         // Match request count
         lastRunCnt = runReqCnt_;
      }

      // Register TX is pending
      else if ( lastReqCnt != regReqCnt_ ) {

         // Setup tx buffer
         regBuff_[0]  = 0;
         regBuff_[1]  = (regReqWrite_)?0x40000000:0x00000000;
         regBuff_[1] |= regReqEntry_->address() & 0x00FFFFFF;

         // Write has data
         if ( regReqWrite_ ) {
            memcpy(&(regBuff_[2]),regReqEntry_->data(),(regReqEntry_->size()*4));
            regBuff_[regReqEntry_->size()+2]  = 0;
         }

         // Read is always small
         else {
            regBuff_[2]  = (regReqEntry_->size()-1);
            regBuff_[3]  = 0;
         }

         // Set lane and vc from upper address bits
         regLane = (regReqEntry_->address()>>28) & 0xF;
         regVc   = (regReqEntry_->address()>>24) & 0xF;

         // Send data
         //printf("Address: %x\n",regRegEntry_->address());
         //printf("Data 0: %x\n",regBuff_[0]);
         //printf("Data 1: %x\n",regBuff_[1]);
         //printf("Data 2: %x\n",regBuff_[2]);
         //printf("Data 3: %x\n",regBuff_[3]);
         //printf("Lane: %i\n",regLane);
         //printf("Vc: %i\n",regLane);
         //printf("Size: %i\n",((regReqWrite_)?regReqEntry_->size()+3:4));
        
         pgpcard_send(fd_, regBuff_, ((regReqWrite_)?regReqEntry_->size()+3:4), regLane, regVc);
 
         // Match request count
         lastReqCnt = regReqCnt_;
      }

      // Command TX is pending
      else if ( lastCmdCnt != cmdReqCnt_ ) {

         // Setup tx buffer
         cmdBuff[0]  = 0;
         cmdBuff[1]  = cmdReqEntry_->opCode() & 0xFF;
         cmdBuff[2]  = 0;
         cmdBuff[3]  = 0;

         // Setup transmit
         cmdLane = (cmdReqEntry_->opCode()>>12) & 0xF;
         cmdVc   = (cmdReqEntry_->opCode()>>8)  & 0xF;
        
         // Send data
         pgpcard_send(fd_, cmdBuff, 4, cmdLane, cmdVc);

         // Match request count
         lastCmdCnt = cmdReqCnt_;
         cmdRespCnt_++;
      }

      // Data TX is pending
      else if ( lastDataCnt != dataReqCnt_ ) {

         // Setup transmit
         uint32_t dataLaneBits = (dataReqAddr_ >> 4) & 0xF;
         uint32_t dataVcBits   = (dataReqAddr_     ) & 0xF; 
         dataLane = 0;
         dataVc   = 0;
         while(dataLaneBits >>= 1) dataLane++;
         while(dataVcBits   >>= 1) dataVc++;

         // Send data
         pgpcard_send(fd_, dataReqEntry_, dataReqLength_, dataLane, dataVc);

         // Match request count
         lastDataCnt = dataReqCnt_;
         dataRespCnt_++;
      }

      else ioThreadWait(1000);
   }

   free(regBuff_);
}

// Constructor
PgpLink::PgpLink ( ) : CommLink() {
   device_   = "";
   fd_       = -1;
}

// Deconstructor
PgpLink::~PgpLink ( ) {
   close();
}

// Open link and start threads
void PgpLink::open ( string device ) {
   stringstream dbg;

   device_ = device; 

   // Open device without blocking
   fd_ = ::open(device.c_str(),O_RDWR | O_NONBLOCK);

   // Debug result
   if ( fd_ < 0 ) {
      dbg.str("");
      dbg << "PgpLink::open -> ";
      if ( fd_ < 0 ) dbg << "Error opening file ";
      else dbg << "Opened device file ";
      dbg << device_ << endl;
      cout << dbg.str();
      throw(dbg.str());
   }

   // Status
   CommLink::open();
}

// Stop threads and close link
void PgpLink::close () {

   // Close link
   if ( fd_ >= 0 ) {
      CommLink::close();
      ::close(fd_);
   }
   fd_ = -1;
}

