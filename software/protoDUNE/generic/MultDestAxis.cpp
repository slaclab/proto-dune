//-----------------------------------------------------------------------------
// File          : MultDestAxis.cpp
// Author        : Ryan Herbst  <rherbst@slac.stanford.edu>
// Created       : 06/18/2014
// Project       : General Purpose
//-----------------------------------------------------------------------------
// Description :
// AXIS Destination container for MultLink class.
// 
// LinkConfig Field usage:
// bits 7:0 = Index (ignored)
// bits 15:8 = AXIS dest field for register transactions
// bits 23:16 = AXIS dest field for commands
// bits 31:24 = AXIS dest field for data
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
#include <MultDestAxis.h>
#include <stdio.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <iostream>
#include <unistd.h>
#include <string.h>
#include <iomanip>
#include <sstream>
#include <Register.h>
#include <Command.h>
#include <AxiStreamDma.h>
#include <PgpCardMod.h>
#include <PgpCardWrap.h>
#include <stdint.h>
using namespace std;

//! Constructor
MultDestAxis::MultDestAxis (string path) : MultDest(512) { 
   path_ = path;
}

//! Deconstructor
MultDestAxis::~MultDestAxis() { 
   this->close();
}

//! Open link
void MultDestAxis::open ( uint32_t idx, uint32_t maxRxTx ) {
   stringstream tmp;

   this->close();

   fd_ = ::open(path_.c_str(),O_RDWR | O_NONBLOCK);

   if ( fd_ < 0 ) {
      tmp.str("");
      tmp << "MultDestPgp::open -> Could Not Open AXIS path " << path_;
      throw tmp.str();
   }

   if ( debug_ ) 
      cout << "MultDestPgp::open -> Opened AXIS device " << path_
           << ", Fd=" << dec << fd_ << endl;

   MultDest::open(idx,maxRxTx);
}

//! Transmit data.
int32_t MultDestAxis::transmit ( MultType type, void *ptr, uint32_t size, uint32_t context, uint32_t config ) {
   uint32_t   firstUser;
   uint32_t   lastUser;
   uint32_t   axisDest;
   Register * reg;
   Command  * cmd;
   bool       isWrite;
   uint32_t   txSize;
   uint32_t * txData;

   isWrite   = false;
   firstUser = 0x2; // SOF
   lastUser  = 0;

   // Types
   switch ( type ) {
      case MultTypeRegisterWrite :
         isWrite = true;
      case MultTypeRegisterRead  :
         txData   = (uint32_t *)txData_;
         reg      = (Register*)ptr;
         axisDest = (config >> 8) & 0xFF;

         // Setup buffer
         txData[0]  = context;
         txData[1]  = (isWrite)?0x40000000:0x00000000;
         txData[1] |= (reg->address() >> 2) & 0x3FFFFFFF; // Drop lower 2 address bits

         // Write has data
         if ( isWrite ) {
            memcpy(&(txData[2]),reg->data(),(reg->size()*4));
            txData[reg->size()+2] = 0;
            txSize = (reg->size()+3)*4;
         }

         // Read is always small
         else {
            txData[2] = (reg->size()-1);
            txData[3] = 0;
            txSize    = 16;
         }
         break;

      case MultTypeCommand :
         txData   = (uint32_t *)txData_;
         cmd      = (Command*)ptr;
         axisDest = (config >> 16) & 0xFF;

         // Setup buffer
         txData[0]  = context;
         txData[1]  = cmd->opCode() & 0xFF;
         txData[2]  = 0;
         txData[3]  = 0;
         txSize     = 16;
         break;

      case MultTypeData :
         txData   = (uint32_t *)ptr;
         txSize   = size;
         axisDest = (config >> 24) & 0xFF;
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
         // MULTDestPgpG3.cpp with the same problem.
         // -----------------------------------------------------------
         return -1;

   }

   return(axisWrite(fd_, txData, txSize,firstUser,lastUser,axisDest));
}

// Receive data
int32_t MultDestAxis::receive ( MultType *type, void **ptr, uint32_t *context) {
   int32_t  ret;
   uint32_t firstUser;
   uint32_t lastUser;
   uint32_t axisDest;
   uint32_t dataSource;

   uint32_t *rxData = (uint32_t*)rxData_;

   // Attempt receive
   ret = axisRead(fd_, rxData_, dataSize_,&firstUser,&lastUser,&axisDest);

   // No data
   if ( ret == 0 ) return(0);

   // Bad size or error
   if ( (ret < 0) || (ret % 4) != 0 || (ret-4) < 5 || lastUser ) {
      cout << "MultDestAxis::receive -> "
           << "Error in data receive. Rx=" << dec << ret
           << ", Dest=" << dec << axisDest 
           << ", Last=" << dec << lastUser << endl;
      return(-1);
   }

   dataSource = (axisDest << 24) & 0xFF000000;

   // Is this a data receive?
   if ( isDataSource(dataSource) ) {
      *ptr     = rxData_;
      *context = 0;
      *type    = MultTypeData;
   }

   // Otherwise this is a register receive
   else {
      *ptr     = rxRegister_;

      // Setup buffer
      *context = rxData[0];
      rxRegister_->setAddress(rxData[1] << 2);

      // Set type
      if ( rxData[1] & 0x40000000 ) *type = MultTypeRegisterWrite;
      else *type = MultTypeRegisterRead;

      // Double check size
      if ( (ret/4)-3 > (int32_t)rxRegister_->size() ) {
         if ( debug_ ) {
            cout << "MultDestAxis::receive -> "
                 << "Bad size in register receive. Address = 0x" 
                 << hex << setw(8) << setfill('0') << rxRegister_->address()
                 << ", RxSize=" << dec << ((ret/4)-3)
                 << ", Max Size=" << dec << rxRegister_->size() << endl;
         }
         return(-1);
      }

      // Copy data and status
      memcpy(rxRegister_->data(),&(rxData[2]),((ret/4)-3)*4);
      rxRegister_->setStatus(rxData[(ret/4)-1]);
   }

   return(ret);
}

