//-----------------------------------------------------------------------------
// File          : MultLink.cpp
// Author        : Ryan Herbst  <rherbst@slac.stanford.edu>
// Created       : 06/18/2014
// Project       : General Purpose
//-----------------------------------------------------------------------------
// Description :
// RCE communications link
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
// 06/18/2014: created
//-----------------------------------------------------------------------------
#include <MultLink.h>
#include <PgpCardMod.h>
#include <PgpCardWrap.h>
#include <sstream>
#include "Register.h"
#include "Command.h"
#include "Data.h"
#include "AxiStreamDma.h"
#include <fcntl.h>
#include <iostream>
#include <iomanip>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/socket.h>
#include <netdb.h>
#include <stdarg.h>
#include <sys/select.h>
#include <MultDest.h>
#include <stdint.h>
using namespace std;

// Transmit thread
void MultLink::ioHandler() {
   uint32_t lastReqCnt;
   uint32_t lastCmdCnt;
   uint32_t lastRunCnt;
   uint32_t lastDataCnt;
   uint32_t idx;

   MultDest::MultType type;
   
   // Setup
   lastReqCnt  = regReqCnt_;
   lastCmdCnt  = cmdReqCnt_;
   lastRunCnt  = runReqCnt_;
   lastDataCnt = dataReqCnt_;

   // While enabled
   while ( runEnable_ ) {

      // Run Command TX is pending
      if ( lastRunCnt != runReqCnt_ ) {
         idx = runReqConf_ & 0xFF;

         if ( idx < destCount_ && dests_[idx] != NULL ) {
            dests_[idx]->transmit(MultDest::MultTypeCommand,runReqEntry_,sizeof(Command),runReqCnt_,runReqConf_);
         }

         // Match request count
         lastRunCnt = runReqCnt_;
      }

      // Register TX is pending
      else if ( lastReqCnt != regReqCnt_ ) {
         idx = regReqConf_ & 0xFF;

         if ( regReqWrite_ ) type = MultDest::MultTypeRegisterWrite;
         else type = MultDest::MultTypeRegisterRead;

         if ( idx < destCount_ && dests_[idx] != NULL ) {
            dests_[idx]->transmit(type,regReqEntry_,sizeof(Register),regReqCnt_,regReqConf_);
         }

         if ( dests_[idx]->regIsSync() ) {
            regRespCnt_++;
            mainThreadWakeup();
         }

         // Match request count
         lastReqCnt = regReqCnt_;
      }

      // Command TX is pending
      else if ( lastCmdCnt != cmdReqCnt_ ) {
         idx = cmdReqConf_ & 0xFF;

         if ( idx < destCount_ && dests_[idx] != NULL ) {
            dests_[idx]->transmit(MultDest::MultTypeCommand,cmdReqEntry_,sizeof(Command),cmdReqCnt_,cmdReqConf_);
         }

         // Match request count
         lastCmdCnt = cmdReqCnt_;
         cmdRespCnt_++;
         mainThreadWakeup();
      }

      // Data TX is pending
      else if ( lastDataCnt != dataReqCnt_ ) {
         idx = dataReqConf_ & 0xFF;

         if ( idx < destCount_ && dests_[idx] != NULL ) {
            dests_[idx]->transmit(MultDest::MultTypeData,dataReqEntry_,dataReqLength_*4,dataReqCnt_,dataReqConf_);
         }

         // Match request count
         lastDataCnt = dataReqCnt_;
         dataRespCnt_++;
         mainThreadWakeup();
      }

      // Nothing. Go to sleep
      else ioThreadWait(1000);
   }
}

// Receive Thread
void MultLink::rxHandler() {
   uint32_t           x;
   fd_set             fds;
   int32_t            maxFd;
   struct timeval     timeout;
   int32_t            ret;
   MultDest::MultType type;
   uint32_t           context;
   void             * ptr;
   Register         * rxReg;
   Data             * data;
   time_t             dataTime;
   time_t             currTime;

   // While enabled
   while ( runEnable_ ) {

      // Init fds
      FD_ZERO(&fds);
      maxFd = -1;

      // Process each dest
      for (x=0; x < destCount_; x++) {
         if ( dests_[x] != NULL ) dests_[x]->fdSet(&fds,&maxFd);
      }

      // Setup timeout
      timeout.tv_sec  = 0;
      timeout.tv_usec = 1000;

      // Select
      if ( maxFd < 0 ) usleep(1000); // Nothing to listen to
      else if ( select(maxFd+1, &fds, NULL, NULL, &timeout) <= 0 ) continue;

      // Process each dest
      for (x=0; x < destCount_; x++) {
         if ( dests_[x] != NULL && dests_[x]->fdIsSet(&fds) ) {

            // Receive
            ret = dests_[x]->receive ( &type, &ptr, &context );

            // Return
            if ( ret > 0 ) {

               // Data is received
               if ( type == MultDest::MultTypeData ) {
                  data = new Data((uint32_t *)ptr,ret/4);
                  time(&dataTime);

                  // Don't overflow receive buffer
                  while ( ! dataQueue_.push(data) ) {
                     time(&currTime);
                     if ( currTime != dataTime ) {
                        cout << "MultLink::rxHandler -> Waiting on data fifo!!!!!\n";
                        dataTime = currTime;
                     }
                     usleep(100);
                  }
                  dataThreadWakeup();
               }

               // Register is received
               else if ( type == MultDest::MultTypeRegisterWrite || type == MultDest::MultTypeRegisterRead ) {
                  rxReg = (Register *)ptr;

                  // Matches outstanding register request
                  if ( (regReqCnt_ == context) && rxReg->address() == regReqEntry_->address() ) {

                     // Read 
                     if ( regReqWrite_ == 0 ) {

                        // Status is zero, success
                        if ( rxReg->status() == 0 ) memcpy(regReqEntry_->data(),rxReg->data(),(regReqEntry_->size()*4));

                        // Fail
                        else memset(regReqEntry_->data(),0xFF,(regReqEntry_->size()*4));
                     }
                     regReqEntry_->setStatus(rxReg->status());
                     regRespCnt_++;
                     mainThreadWakeup();
                  }

                  // Unexpected frame
                  else {
                     unexpCount_++;
                     if ( debug_ ) {
                        cout << "MultLink::rxHandler -> Unexpected frame received"
                             << " Exp Count=0x" << hex << setw(8) << setfill('0') << regReqCnt_
                             << " Got Count=0x" << hex << setw(8) << setfill('0') << context
                             << " Exp Addr=0x" << hex << setw(8) << setfill('0') << regReqEntry_->address()
                             << " Got Addr=0x" << hex << setw(8) << setfill('0') << rxReg->address() << endl;
                     }
                  }
               }
            }
         }
      }
   }
}

// Constructor
MultLink::MultLink (bool enDataThread) : CommLink() {
   dests_     = NULL;
   destCount_ = 0;
   enDataThread_ = enDataThread;
}

// Deconstructor
MultLink::~MultLink ( ) {
   close();
}

// Open link and start threads
void MultLink::open ( uint32_t count, ... ) {
   va_list    a_list;
   uint32_t       x;
   MultDest * idests[count];

   // Get list
   va_start(a_list,count);

   for (x=0; x < count; x++) idests[x] = va_arg(a_list,MultDest *);

   this->open(count,idests);

   va_end(a_list);
}

// Open link and start threads
void MultLink::open ( uint32_t count, MultDest **dests ) {
   stringstream tmp;
   uint32_t         x;

   if ( dests_ != NULL || count == 0 || count > 255 ) return;

   // Allocate memory
   destCount_ = count;
   dests_     = (MultDest **) malloc(count * sizeof(MultDest *));

   // Set and open each destination
   for (x =0; x < count; x++) { 
      dests_[x] = dests[x];
      if ( dests_[x] != NULL ) dests_[x]->open(x,maxRxTx_);
   }

   // Start threads
   CommLink::open(enDataThread_);
}

// Stop threads and close link
void MultLink::close () {
   if ( dests_ != NULL ) {
      CommLink::close();
      for(uint32_t x=0; x < destCount_; x++) { 
         if ( dests_[x] != NULL ) dests_[x]->close();
      }
      free(dests_);
   }
   dests_     = NULL;
   destCount_ = 0;
}

void MultLink::addDataSource(uint32_t source) {

   if ( destCount_ == 0 ) throw("MultLink::addDataSource -> Called when not open!");

   uint32_t idx;
   idx = source & 0xFF;
   if (idx < destCount_ && dests_[idx] != NULL) {
      dests_[idx]->addDataSource(source);
   }
   CommLink::addDataSource(source);
}

