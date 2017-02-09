//-----------------------------------------------------------------------------
// File          : SimLink.cpp
// Author        : Ryan Herbst  <rherbst@slac.stanford.edu>
// Created       : 09/07/2012
// Project       : General Purpose
//-----------------------------------------------------------------------------
// Description :
// Communications link for simulation
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
// 09/07/2012: created
//-----------------------------------------------------------------------------
#include <SimLink.h>
#include <sstream>
#include "Register.h"
#include "Command.h"
#include "Data.h"
#include <fcntl.h>
#include <iostream>
#include <iomanip>
#include <string.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdint.h>

#ifndef RTEMS
#include <sys/mman.h>
#endif

using namespace std;

// Receive Thread
void SimLink::rxHandler() {
   uint32_t          *rxBuff;
   int32_t            ret;
   Data           *data;
   uint32_t           lane;
   uint32_t           vc;
   uint32_t           eofe;
   uint32_t           vcMaskRx;
   uint32_t           laneMaskRx;
   uint32_t           vcMask;
   uint32_t           laneMask;

   // Init buffer
   rxBuff = (uint32_t *) malloc(sizeof(uint32_t)*maxRxTx_);

   // While enabled
   while ( runEnable_ && smem_ != NULL ) {

      // Data is available
      if ( smem_->usReqCount != smem_->usAckCount ) {

         // Too large
         if ( smem_->usSize > maxRxTx_ ) {
            ret  = 0;
            lane = 0;
            vc   = smem_->usVc;
            eofe = 1;
            smem_->usAckCount = smem_->usReqCount;
         }

         // Size is ok
         else {
            memcpy(rxBuff,smem_->usData,(smem_->usSize)*4);
            ret  = smem_->usSize;
            lane = 0;
            vc   = smem_->usVc;
            eofe = smem_->usEofe;
            smem_->usAckCount = smem_->usReqCount;
         }

         // Bad size or error
         if ( ret < 4 || eofe ) {
            if ( debug_ ) {
               cout << "SimLink::ioHandler -> "
                    << "Error in data receive. Rx=" << dec << ret
                    << ", Lane=" << dec << lane << ", Vc=" << dec << vc
                    << ", EOFE=" << dec << eofe << endl;
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
                  cout << "SimLink::rxHandler -> Unexpected frame received"
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
      else usleep(1000); // Emulate select timeout
   }

   free(rxBuff);
}

// Transmit thread
void SimLink::ioHandler() {
   uint32_t           cmdBuff[4];
   uint32_t           runBuff[4];
   uint32_t           lastReqCnt;
   uint32_t           lastCmdCnt;
   uint32_t           lastRunCnt;
   uint32_t           runVc;
   //uint32_t         runLane;
   uint32_t           regVc;
   //uint32_t         regLane;
   uint32_t           cmdVc;
   //uint32_t         cmdLane;
   
   // Setup
   lastReqCnt = regReqCnt_;
   lastCmdCnt = cmdReqCnt_;
   lastRunCnt = runReqCnt_;

   // Init register buffer
   regBuff_ = (uint32_t *) malloc(sizeof(uint32_t)*maxRxTx_);

   // While enabled
   while ( runEnable_ && smem_ != NULL ) {

      // Run Command TX is pending
      if ( lastRunCnt != runReqCnt_ ) {

         // Setup tx buffer
         runBuff[0]  = 0;
         runBuff[1]  = runReqEntry_->opCode() & 0xFF;
         runBuff[2]  = 0;
         runBuff[3]  = 0;
 
         // Setup transmit
         //runLane = (runReqEntry_->opCode()>>12) & 0xF;
         runVc   = (runReqEntry_->opCode()>>8)  & 0xF;
        
         // Send data
         smem_->dsSize = 4;
         smem_->dsVc   = runVc;
         memcpy(smem_->dsData,runBuff,(smem_->dsSize)*4);
         smem_->dsReqCount++;
         while (smem_->dsReqCount != smem_->dsAckCount) usleep(100); // handshake

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
         //regLane = (regReqEntry_->address()>>28) & 0xF;
         regVc   = (regReqEntry_->address()>>24) & 0xF;

         // Send data
         smem_->dsSize = ((regReqWrite_)?regReqEntry_->size()+3:4);
         smem_->dsVc   = regVc;
         memcpy(smem_->dsData,regBuff_,(smem_->dsSize)*4);
         smem_->dsReqCount++;
         while (smem_->dsReqCount != smem_->dsAckCount) usleep(100); // handshake

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
         //cmdLane = (cmdReqEntry_->opCode()>>12) & 0xF;
         cmdVc   = (cmdReqEntry_->opCode()>>8)  & 0xF;
        
         // Send data
         smem_->dsSize = 4;
         smem_->dsVc   = cmdVc;
         memcpy(smem_->dsData,cmdBuff,(smem_->dsSize)*4);
         smem_->dsReqCount++;
         while (smem_->dsReqCount != smem_->dsAckCount) usleep(100); // handshake

         // Match request count
         lastCmdCnt = cmdReqCnt_;
         cmdRespCnt_++;
      }
      else ioThreadWait(1000);
   }

   free(regBuff_);
}

// Constructor
SimLink::SimLink ( ) : CommLink() {
   smem_      = NULL;
   toDisable_ = true;
}

// Deconstructor
SimLink::~SimLink ( ) {
   close();
}

// Open link and start threads
void SimLink::open ( string system, uint32_t id ) {
   int32_t      smemFd;
   char         shmName[200];

   smem_ = NULL;


   // Generate shared memory
   sprintf(shmName,"%s.%i.%i",SHM_BASE,getuid(),id);

   // Debug
   cout << "SimLink::open -> Using shared memory file " << shmName << endl;

   // Open shared memory
#ifndef RTEMS
   smemFd = shm_open(shmName, (O_CREAT | O_RDWR), (S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH));
#else
   smemFd = -1;
#endif

   // Failed to open shred memory
   if ( smemFd < 0 ) throw string("SimLink::open -> Could Not Open Shared Memory");
  
   // Force permissions regardless of umask
   fchmod(smemFd, (S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH));
 
   // Set the size of the shared memory segment
   ftruncate(smemFd, sizeof(SimLinkMemory));

   // Map the shared memory
#ifndef RTEMS
   if((smem_ = (SimLinkMemory *)mmap(0, sizeof(SimLinkMemory),
              (PROT_READ | PROT_WRITE), MAP_SHARED, smemFd, 0)) == MAP_FAILED) {
      smem_   = NULL;
      if ( smemFd < 0 ) throw string("SimLink::open -> Could Not Map Shared Memory");
   }
#endif

   // Init records
   smem_->usReqCount = 0;
   smem_->usAckCount = 0;
   smem_->dsReqCount = 0;
   smem_->dsAckCount = 0;

   CommLink::open();
}

// Stop threads and close link
void SimLink::close () {
   CommLink::close();
#ifndef RTEMS
   shm_unlink(smem_->path);
#endif
   smem_ = NULL;
}

