//-----------------------------------------------------------------------------
// File          : MultDestUdp.cpp
// Author        : Ryan Herbst  <rherbst@slac.stanford.edu>
// Created       : 06/18/2014
// Project       : General Purpose
//-----------------------------------------------------------------------------
// Description :
// UDP Destination container for MultLink class.
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
#include <MultDestUdp.h>
#include <stdio.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <iostream>
#include <iomanip>
#include <stdlib.h>
#include <string.h>
#include <PgpCardMod.h>
#include <PgpCardWrap.h>
#include <sstream>
#include <Register.h>
#include <Command.h>
#include <sys/socket.h>
#include <netdb.h>
#include <stdint.h>
#include <unistd.h>
#include <sys/select.h>
#include <sys/types.h>

#ifdef RTEMS
#include <rtems/libio.h>
#endif

#define CONTINUE_BIT_MASK 0x00800000
#define LANE_VC_MASK      0xFF000000

using namespace std;

//! Constructor
MultDestUdp::MultDestUdp (string address, int32_t port) : MultDest(512) { 
   address_ = address;
   port_    = port;
   curPos_  = 0;
}

//! Deconstructor
MultDestUdp::~MultDestUdp() { 
   this->close();
}

//! Open link
void MultDestUdp::open ( uint32_t idx, uint32_t maxRxTx ) {
#ifndef RTEMS // FIX
   stringstream        tmp;
   struct addrinfo     aiHints;
   struct addrinfo*    aiList=0;
   const  sockaddr_in* addr;
   int32_t             error;

   this->close();

   // Create socket
   fd_ = socket(AF_INET,SOCK_DGRAM,0);
   if ( fd_ == -1 ) {
      tmp.str("");
      tmp << "MultDestUdp::open -> Could Not Create Socket For index " << dec << idx;
      throw tmp.str();
   }

   // Lookup host address
   aiHints.ai_flags    = AI_CANONNAME;
   aiHints.ai_family   = AF_INET;
   aiHints.ai_socktype = SOCK_DGRAM;
   aiHints.ai_protocol = IPPROTO_UDP;
   error = ::getaddrinfo(address_.c_str(), 0, &aiHints, &aiList);

   if (error || !aiList) {
      tmp.str("");
      tmp << "MultDestPgp::open -> Failed to open UDP host " 
           << address_ << ":" << port_ << endl;
      throw tmp.str();
   }

   addr = (const sockaddr_in*)(aiList->ai_addr);

   // Setup Remote Address
   memset(&addr_,0,sizeof(struct sockaddr_in));
   ((struct sockaddr_in *)(&addr_))->sin_family=AF_INET;
   ((struct sockaddr_in *)(&addr_))->sin_addr.s_addr=addr->sin_addr.s_addr;
   ((struct sockaddr_in *)(&addr_))->sin_port=htons(port_);

   // Debug
   if ( debug_ ) 
      cout << "MultDest::open -> Opened index " << dec << idx << ", UDP device " << address_ << ":" 
           << dec << port_ << ", Fd=" << dec << fd_
           << ", Addr=0x" << hex << ((struct sockaddr_in *)(&addr_))->sin_addr.s_addr << endl;

#endif
   MultDest::open(idx,maxRxTx);
}

//! Transmit data.
int32_t MultDestUdp::transmit ( MultType type, void *ptr, uint32_t size, uint32_t context, uint32_t config ) {
   uint32_t   header;
   Register * reg;
   Command  * cmd;
   bool       isWrite;
   uint32_t   txSize;
   uint32_t * txData;
   uint32_t   x;

   struct msghdr msg;
   struct iovec  msg_iov[2];

   isWrite = false;
   header  = (config << 16) & 0xFF000000;

   // Types
   switch ( type ) {
      case MultTypeRegisterWrite :
         isWrite = true;
      case MultTypeRegisterRead  :
         txData  = (uint32_t *)txData_;
         reg     = (Register*)ptr;

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
         txData = (uint32_t *)txData_;
         cmd    = (Command*)ptr;

         // Setup buffer
         txData[0]  = 0;
         txData[1]  = cmd->opCode() & 0xFF;
         txData[2]  = 0;
         txData[3]  = 0;
         txSize     = 16;
         break;

      case MultTypeData :
         txData = (uint32_t *)ptr;
         txSize = size;
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

   // Convert data
   header = htonl(header);
   for (x=0; x < txSize/4; x++) txData[x] = htonl(txData[x]);

   // Setup message header
   msg.msg_name       = &addr_;
   msg.msg_namelen    = sizeof(struct sockaddr_in);
   msg.msg_iov        = msg_iov;
   msg.msg_iovlen     = 2;
   msg.msg_control    = NULL;
   msg.msg_controllen = 0;
   msg.msg_flags      = 0;

   // Setup IOVs
   msg_iov[0].iov_base = &header;
   msg_iov[0].iov_len  = 4;
   msg_iov[1].iov_base = txData;
   msg_iov[1].iov_len  = txSize;

   return(sendmsg(fd_,&msg,0));
}

// Receive data
int32_t MultDestUdp::receive ( MultType *type, void **ptr, uint32_t *context) {
   uint32_t   x;
   int32_t    ret;
   uint32_t   header;
   uint32_t   dataSource = 0;
   bool       eof = true;

   // Attempt receive
   ret = ::read(fd_, &rxData_[curPos_], dataSize_);

   // No data
   if ( ret == 0 ) return(0);

   // Bad size or error
   if ( (ret < 0) || (ret % 4) != 0 || (ret-4) < 5 ) {
      if ( debug_ ) {
         cout << "MultDestUdp::receive -> "
              << "Error in data receive. Rx=" << dec << ret << endl;
      }
      return(-1);
   }

   // Convert data
   uint *rxData = (uint32_t *) rxData_;
   for ( x=curPos_/4; x < (uint32_t)(ret/4+curPos_/4); x++ ) rxData[x] = ntohl(rxData[x]);
   header = rxData[curPos_/4];

   // Extract header
   dataSource += header & LANE_VC_MASK;
   eof = (~header & CONTINUE_BIT_MASK);

   // Is this a data receive?
   if ( isDataSource(dataSource) ) {
      //Move the data back by one to remove the header
      for (x = curPos_/4+1; x < (uint32_t)(ret/4+curPos_/4); x++) {
         rxData[x-1] = rxData[x];
      }
      ret -= 4; //We removed the header, so chop one 32-bit word off the return value
      if (eof) {
         *ptr     = &(rxData[0]);
         *context = 0;
         *type    = MultTypeData;
         ret += curPos_;
         curPos_  = 0;
      } else {
         curPos_ += ret;
         return 0;
      }
   }

   // Otherwise this is a register receive
   else {
      curPos_ = 0;

      *ptr = rxRegister_;

      // Setup buffer
      *context = rxData[1];
      rxRegister_->setAddress(rxData[2] << 2);

      // Set type
      if ( rxData[2] & 0x40000000 ) *type = MultTypeRegisterWrite;
      else *type = MultTypeRegisterRead;

      // Double check size
      if ( (ret/4)-4 > (int32_t)rxRegister_->size() ) {
         if ( debug_ ) {
            cout << "MultDestUdp::receive -> "
                 << "Bad size in register receive. Address = 0x" 
                 << hex << setw(8) << setfill('0') << rxRegister_->address()
                 << ", RxSize=" << dec << ((ret/4)-4)
                 << ", Max Size=" << dec << rxRegister_->size() << endl;
         }
         return(-1);
      }

      // Copy data and status
      memcpy(rxRegister_->data(),&(rxData[3]),(ret)-4);
      rxRegister_->setStatus(rxData[(ret/4)-1]);
   }

   return(ret);
}

