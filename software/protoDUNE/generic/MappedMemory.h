//-----------------------------------------------------------------------------
// File          : MappedMemory.h
// Author        : Ryan Herbst  <rherbst@slac.stanford.edu>
// Created       : 08/15/2014
// Project       : General Purpose
//-----------------------------------------------------------------------------
// Description :
// Class to handle mapping of hardware memory space.
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
// 08/15/2014: created
//-----------------------------------------------------------------------------
#ifndef __MAPPED_MEMORY_H__
#define __MAPPED_MEMORY_H__

#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <string>
#include <stdarg.h>
#include <stdint.h>

#ifndef RTEMS
#include <sys/mman.h>
#endif
 
class MappedMemory {

      uint32_t * _memBase;
      size_t * _memSize;
      volatile char ** _map;

      int32_t  _devFd;
      bool _debug;
      bool _emulate;
      uint32_t _count;
      bool _open;

      // Init
      void init (uint32_t count, va_list a_list);

   public:

      // Pass count and uint32_t base, size_t size combinations
      MappedMemory (uint32_t count, ... );
      MappedMemory (uint32_t count, va_list a_list );
      ~MappedMemory ();

      // Open and close
      bool open ( );
      void close ( );

      // Debug enable
      void debug (bool enable);

      // Set emulate mode
      void emulate (bool enable);

      // Write
      void write (uint32_t address, uint32_t value, bool *err = NULL );

      // Read
      uint32_t read (uint32_t address, bool *err = NULL );
};

#endif
