//-----------------------------------------------------------------------------
// File          : MultDestPgpG3.cpp
// Author        : Ryan Herbst  <rherbst@slac.stanford.edu>
// Created       : 06/18/2014
// Project       : General Purpose
//-----------------------------------------------------------------------------
// Description :
// PGP Destination container for MultLink class.
//
// LinkConfig Field usage:
// bits 7:0 = Index (ignored)
// bits 11:8 =  PGP VC for register transactions
// bits 15:12 = PGP Lane for register transactions
// bits 19:16 = PGP VC for commands
// bits 23:20 = PGP Lane for commands
// bits 27:24 = PGP VC for data
// bits 31:28 = PGP Lane for data
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
#include <MultDestPgpG3.h>
#include <stdio.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <iostream>
#include <unistd.h>
#include <string.h>
#include <iomanip>
#include <PgpCardG3Mod.h>
#include <PgpCardG3Wrap.h>
#include <sstream>
#include <Register.h>
#include <Command.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdint.h>
#include <sys/select.h>
#include <sys/mman.h>

using namespace std;

//! Constructor
MultDestPgpG3::MultDestPgpG3 (string path) : MultDest(512) { 
   path_ = path;
}

//! Deconstructor
MultDestPgpG3::~MultDestPgpG3() { 
   this->close();
}

//! Open link
void MultDestPgpG3::open ( uint32_t idx, uint32_t maxRxTx ) {
   stringstream tmp;

   this->close();

   fd_ = ::open(path_.c_str(),O_RDWR | O_NONBLOCK);

   if ( fd_ < 0 ) {
      tmp.str("");
      tmp << "MultDestPgpG3::open -> Could Not Open PGP path " << path_;
      throw tmp.str();
   }

   // Memory Map
   reg_ = (struct PgpCardReg *) mmap(0,sizeof(struct PgpCardReg),PROT_READ | PROT_WRITE, MAP_SHARED,fd_,0);

   if ( reg_ == MAP_FAILED ) {
      tmp.str("");
      tmp << "MultDestPgpG3::open -> ";
      tmp << "Failed to memory map ";
      tmp << path_ << endl;
      cout << tmp.str();
      throw(tmp.str());
   }

   if ( debug_ ) 
      cout << "MultDestPgpG3::open -> Opened pgp device " << path_
           << ", Fd=" << dec << fd_ << endl;

   MultDest::open(idx,maxRxTx);
}

//! Transmit data
int32_t MultDestPgpG3::transmit ( MultType type, void *ptr, uint32_t size, uint32_t context, uint32_t config ) {
   uint32_t   lane;
   uint32_t   vc;
   Register * reg;
   Command  * cmd;
   bool       isWrite;
   uint32_t   txSize;
   uint32_t * txData;
   int32_t    ret;

   isWrite = false;

   // Types
   switch ( type ) {
      case MultTypeRegisterWrite :
         isWrite = true;
      case MultTypeRegisterRead  :
         txData  = (uint32_t *)txData_;
         reg     = (Register*)ptr;
         lane    = (config >> 12) & 0xf;
         vc      = (config >>  8) & 0xf;

         // Setup buffer
         txData[0]  = context;
         txData[1]  = (isWrite)?0x40000000:0x00000000;
         txData[1] |= (reg->address() >> 2) & 0x3FFFFFFF; // Drop lower 2 address bits

         // Write has data
         if ( isWrite ) {
            memcpy(&(txData[2]),reg->data(),(reg->size()*4));
            txData[reg->size()+2] = 0;
            txSize = reg->size()+3;
         }

         // Read is always small
         else {
            txData[2]  = (reg->size()-1);
            txData[3]  = 0;
            txSize = 4;
         }
         break;

      case MultTypeCommand :
         txData = (uint32_t *)txData_;
         cmd    = (Command*)ptr;
         lane   = (config >> 20) & 0xf;
         vc     = (config >> 16) & 0xf;

         // Setup buffer
         txData[0]  = 0;
         txData[1]  = cmd->opCode() & 0xFF;
         txData[2]  = 0;
         txData[3]  = 0;
         txSize     = 4;
         break;

      case MultTypeData :
         lane   = (config >> 28) & 0xf;
         vc     = (config >> 24) & 0xf;
         txData = (uint32_t *)ptr;
         txSize = size/4;
         break;

      default:
         // -----------------------------------------------------------
         //
         // 2016.09.16 -- jjr
         // -----------------
         // While technically all 4 cases of 'type' are covered, the
         // compiler has no way of verifying that some piece of code
         // did not cast random value into type that was not a member
         // of the enumeration.  Adding a 'default' case covers this
         // eventuality.
         //
         // There is a trade-off to this.  Suppose one now adds another
         // enumeration to the list.  Magically it is covered by this
         // default case.  If the compiler ignored the possibility that
         // a value other than one of the enumerations got stuffed into
         // 'type', then adding a new type would result in a compiler
         // error indicating that this new case was not covered.  
         //
         // In the end, no happy with this kludge.  Personally I would
         // prefer 4 functions rather than this switch statement. The
         // odds that 'type' is not programmatically set, but statically
         // coded are high.  In that case, 4 functions are cleaner and
         // avoid this issue.
         //
         // NOTE:
         // A similar construct appears in MultDestPgp.cpp and 
         // MULTDestPgAxis.cpp with the same problem.
         // -----------------------------------------------------------
         return -1;


   }

   ret = pgpcard_send(fd_, txData, txSize, lane, vc);
   while(ret < 0 ){
      ret = pgpcard_send(fd_, txData, txSize, lane, vc);
   }

   if ( ret > 0 ) ret = ret * 4;
   return(ret);
}

// Receive data
int32_t MultDestPgpG3::receive ( MultType *type, void **ptr, uint32_t *context ) {
   int32_t  ret;
   uint32_t lane;
   uint32_t vc;
   uint32_t eofe;
   uint32_t fifoErr;
   uint32_t lengthErr;
   uint32_t dataSource;
   uint32_t * rxData;
   
   rxData = (uint32_t *)rxData_;

   // attempt receive
   ret = pgpcard_recv(fd_, rxData, (dataSize_/4), &lane, &vc, &eofe, &fifoErr, &lengthErr);

   // No data
   if ( ret == 0 ) return(0);

   // Bad size or error
   if ( ret < 4 || eofe || fifoErr || lengthErr ) {
      if ( debug_ ) {
         cout << "MultDestPgpG3::receive -> "
              << "Error in data receive. Rx=" << dec << ret
              << ", Lane=" << dec << lane << ", Vc=" << dec << vc
              << ", EOFE=" << dec << eofe << ", FifoErr=" << dec << fifoErr
              << ", LengthErr=" << dec << lengthErr << endl;
      }
      return(-1);
   }

   dataSource  = (lane << 28) & 0xF0000000;
   dataSource += (vc   << 24) & 0x0F000000;

   // Is this a data receive?
   if ( isDataSource(dataSource) ) {
      *ptr = rxData;
      *context = 0;
      *type  = MultTypeData;
   }

   // Otherwise this is a register receive
   else {
      *ptr = rxRegister_;
      
      // Setup buffer
      *context = rxData[0];
      rxRegister_->setAddress(rxData[1] << 2);

      // Set type
      if ( rxData[1] & 0x40000000 ) *type = MultTypeRegisterWrite;
      else *type = MultTypeRegisterRead;

      // Double check size
      if ( ret-3 > (int32_t)rxRegister_->size() ) {
         if ( debug_ ) {
            cout << "MultDestPgpG3::receive -> "
                 << "Bad size in register receive. Address = 0x" 
                 << hex << setw(8) << setfill('0') << rxRegister_->address()
                 << ", RxSize=" << dec << (ret-3)
                 << ", Max Size=" << dec << rxRegister_->size() << endl;
         }
         return(-1);
      }

      // Copy data and status
      memcpy(rxRegister_->data(),&(rxData[2]),(ret-3)*4);
      rxRegister_->setStatus(rxData[ret-1]);
   }

   return(ret*4);
}

//! Transmit Op-Code
int32_t MultDestPgpG3::sendOpCode ( uint32_t opCode ) {
   int32_t    ret;
   ret = pgpcard_sendOpCode(fd_,opCode);
   return(ret);
}


// Evr Link Status
bool MultDestPgpG3::getEvrStatus() {
   uint32_t val = ((reg_->evrCardStat[0] >> 4) & 0x1);
   return(val == 1);
}

// Evr Link Errors
uint32_t MultDestPgpG3::getEvrErrors() {
   uint32_t val = reg_->evrCardStat[3];
   return(val);
}

// Evr Link Count
uint32_t MultDestPgpG3::getEvrCount(uint32_t idx) {
   uint32_t val = reg_->pgpSpare1[idx];
   return(val);
}

// set/Get EVR Enble
bool MultDestPgpG3::getEvrEnable() {
   uint32_t val = (reg_->evrCardStat[1] & 0x1);
   return(val == 1);
}

uint32_t MultDestPgpG3::getEvrStatRaw() {
   uint32_t val = reg_->evrCardStat[1];
   return(val);
}

void MultDestPgpG3::setEvrEnable(bool enable) {
   if ( enable ) reg_->evrCardStat[1] |= 0x1;
   else reg_->evrCardStat[1] &= 0xFFFFFFFE;
}

// set/Get EVR Enble mask
uint32_t MultDestPgpG3::getEvrEnableLane() {
   uint32_t val = ((reg_->evrCardStat[1] >> 16) & 0xFF);
   return(val);
}

void MultDestPgpG3::setEvrEnableLane(uint32_t mask) {
   uint32_t val = (mask << 16);
   reg_->evrCardStat[1] &= 0xFF00FFFF;
   reg_->evrCardStat[1] |= val;
}

// set/Get EVR Lane Code
uint32_t MultDestPgpG3::getEvrLaneRunOpCode(uint32_t lane) {
   return(reg_->runCode[lane]);
}

void MultDestPgpG3::setEvrLaneRunOpCode(uint32_t lane, uint32_t code) {
   reg_->runCode[lane] = code;
}

// set/Get EVR Lane Code
uint32_t MultDestPgpG3::getEvrLaneAcceptOpCode(uint32_t lane) {
   return(reg_->acceptCode[lane]);
}

void MultDestPgpG3::setEvrLaneAcceptOpCode(uint32_t lane, uint32_t code) {
   reg_->acceptCode[lane] = code;
}

// set/Get EVR Lane Code
uint32_t MultDestPgpG3::getEvrLaneRunDelay(uint32_t lane) {
   return(reg_->runDelay[lane]);
}

void MultDestPgpG3::setEvrLaneRunDelay(uint32_t lane, uint32_t delay) {
   reg_->runDelay[lane] = delay;
}

// set/Get EVR Lane Code
uint32_t MultDestPgpG3::getEvrLaneAcceptDelay(uint32_t lane) {
   return(reg_->acceptDelay[lane]);
}

void MultDestPgpG3::setEvrLaneAcceptDelay(uint32_t lane, uint32_t delay) {
   reg_->acceptDelay[lane] = delay;
}

