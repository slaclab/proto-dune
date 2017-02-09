//-----------------------------------------------------------------------------
// File          : MultDest.cpp
// Author        : Ryan Herbst  <rherbst@slac.stanford.edu>
// Created       : 06/18/2014
// Project       : General Purpose
//-----------------------------------------------------------------------------
// Description :
// Destination container for MultLink class.
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
#include <MultDest.h>
#include <Register.h>
#include <sys/select.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdint.h>

using namespace std;

//! Constructor
MultDest::MultDest (uint32_t maxRegister) {
   rxRegister_  = new Register("none",0,maxRegister);
   rxData_      = NULL;
   txData_      = NULL;
   dataSize_    = 0;
   idx_         = 0;
   debug_       = false;
   regIsSync_   = false;
   fd_          = -1;
}

//! DeConstructor
MultDest::~MultDest () {
   this->close();
}

// Add data configuration. The lower 8 bits are ignored.
void MultDest::addDataSource ( uint32_t source ) {
   dataSources_.push_back(source);
}

// Set debug
void MultDest::setDebug ( bool debug ) {
   debug_ = debug;
}

// Look for matching data source
bool MultDest::isDataSource ( uint32_t source ) {
   vector<uint32_t>::iterator srcIter;

   for (srcIter = dataSources_.begin(); srcIter != dataSources_.end(); srcIter++) {
      if ( ((*srcIter)&0xFFFFFF00) == (source&0xFFFFFF00) ) return(true);
   }
   return(false);
}

// Set FD
void MultDest::fdSet ( fd_set *fds, int32_t *maxFd ) {
   if ( fd_ >= 0 ) {
      FD_SET(fd_,fds);
      if ( fd_ > *maxFd ) *maxFd = fd_;
   }
}

// Is FD Set?
bool MultDest::fdIsSet ( fd_set *fds ) {
   if ( fd_ >= 0 ) return(FD_ISSET(fd_,fds));
   else return(false);
}

//! Open link
void MultDest::open (uint32_t idx, uint32_t maxRxTx) {
   if ( rxData_ != NULL ) free(rxData_);
   if ( txData_ != NULL ) free(txData_);

   rxData_   = (uint8_t *)malloc(maxRxTx);
   txData_   = (uint8_t *)malloc(maxRxTx);
   dataSize_ = maxRxTx;
   idx_      = idx;
}

//! Stop link
void MultDest::close () {
   ::close(fd_);
   if ( rxData_ != NULL ) free(rxData_);
   if ( txData_ != NULL ) free(txData_);
   rxData_   = 0;
   txData_   = 0;
   dataSize_ = 0;
   idx_      = 0;
   fd_       = -1;
}

//! Transmit data.
int32_t MultDest::transmit ( MultType type, void *ptr, uint32_t size, uint32_t context, uint32_t config ) {
   return(0);
}

// Receive data
int32_t MultDest::receive ( MultType *type, void **ptr, uint32_t *context) {
   return(0);
}

//! Determine if register access is synchronous
bool MultDest::regIsSync () {
   return(regIsSync_);
}

