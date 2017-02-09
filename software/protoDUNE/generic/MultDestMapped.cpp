//-----------------------------------------------------------------------------
// File          : MultDestMapped.cpp
// Author        : Ryan Herbst  <rherbst@slac.stanford.edu>
// Created       : 06/18/2014
// Project       : General Purpose
//-----------------------------------------------------------------------------
// Description :
// Mapped Memory Destination container for MultLink class.
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
#include <MultDestMapped.h>
#include <stdio.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <iostream>
#include <iomanip>
#include <unistd.h>
#include <string.h>
#include <PgpCardMod.h>
#include <PgpCardWrap.h>
#include <sstream>
#include <Register.h>
#include <Command.h>
#include <stdarg.h>
#include <MappedMemory.h>
#include <stdint.h>
using namespace std;

//! Constructor
MultDestMapped::MultDestMapped (uint32_t count, ... ) : MultDest(1) { 
   va_list a_list;

   regIsSync_ = true;

   va_start(a_list,count);
   map_ = new MappedMemory(count,a_list);
   va_end(a_list);
}

//! Deconstructor
MultDestMapped::~MultDestMapped() { 
   this->close();
}

//! Open link
void MultDestMapped::open ( uint32_t dest, uint32_t maxRxTx ) {
   this->close();
   
   map_->debug(debug_);
   map_->open();

   MultDest::open(dest,maxRxTx);
}

//! Close link
void MultDestMapped::close ( ) {
   map_->debug(debug_);
   map_->close();
   MultDest::close();
}

//! Transmit data.
int32_t MultDestMapped::transmit ( MultType type, void *ptr, uint32_t size, uint32_t context, uint32_t config ) {
   Register * reg;
   Command  * cmd;
   uint32_t       x;
   bool       err;

   map_->debug(debug_);
   err = false;

   // Types
   switch ( type ) {
      case MultTypeRegisterWrite :
         reg = (Register*)ptr;

         for (x=0; x < reg->size(); x++) {
            map_->write(reg->address()+(x*4),reg->data()[x],&err);
         }
         reg->setStatus(err?1:0);
         return(reg->size());
         break;

      case MultTypeRegisterRead  :
         reg = (Register*)ptr;

         for (x=0; x < reg->size(); x++) {
            reg->data()[x] = map_->read(reg->address()+(x*4),&err);
         }
         reg->setStatus(err?1:0);
         return(reg->size());
         break;

      case MultTypeCommand :
         cmd    = (Command*)ptr;
         map_->write(cmd->opCode(),1);
         return(1);
         break;

      case MultTypeData :
         return(0);
         break;
   }
   return(0);
}

