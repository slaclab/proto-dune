//-----------------------------------------------------------------------------
// File          : MappedMemory.cpp
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
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include "MappedMemory.h"
#include <stdarg.h>
#include <stdint.h>

#ifndef RTEMS
#include <sys/mman.h>
#endif

void MappedMemory::init (uint32_t count, va_list a_list ) {
   uint32_t x;

   _debug   = false;
   _emulate = false;
   _open    = false;
   _devFd   = -1;

   _count = count;

   if ( _count != 0 ) {
      _memBase = (uint32_t *)malloc(sizeof(uint32_t)*count);
      _memSize = (size_t *)malloc(sizeof(size_t)*count);
      _map     = (volatile char **)malloc(sizeof(char *)*count);

      for ( x=0; x < _count; x++ ) {
         _map[x]     = NULL;
         _memBase[x] = va_arg(a_list,uint32_t);
         _memSize[x] = va_arg(a_list,size_t);
      }
   }
}

MappedMemory::MappedMemory (uint32_t count, ... ) {
   va_list a_list;

   va_start(a_list,count);
   init(count,a_list);
   va_end(a_list);
}

MappedMemory::MappedMemory (uint32_t count, va_list a_list ) {
   init(count,a_list);
}

MappedMemory::~MappedMemory () {
   this->close();

   if ( _count != 0 ) {
      free(_memBase);
      free(_memSize);
      free(_map);
   }
}

// Open the port
bool MappedMemory::open () {
   uint32_t x;

   this->close();
   _open = true;

   // Emulate only
   if ( _emulate ) {
      for (x=0; x < _count; x++) {
         _map[x] = (char *) malloc (_memSize[x]);
         if (_map[x] == NULL) {
            fprintf(stderr,"MappedMemory::open-> Could not allocate emulation memory.\n");
            this->close();
            return(false);
         }

         if ( _debug ) 
            printf("MappedMemory::open -> Emulating 0x%08x with size 0x%08x.\n",_memBase[x],(uint32_t)_memSize[x]);
      }
   }
   else {

#ifndef RTEMS

      // Open devmem
      _devFd = ::open("/dev/mem", O_RDWR | O_SYNC);
      if (_devFd == -1) {
         fprintf(stderr,"MappedMemory::open-> Can't open /dev/mem.\n");
         this->close();
         return(false);
      }

#endif

      // Map memory space 
      for (x=0; x < _count; x++) {
      
#ifdef RTEMS
      _map[x] = (char *)(_memBase[x]);
#else
         _map[x] = (char *)mmap(NULL, _memSize[x], PROT_READ | PROT_WRITE, MAP_SHARED, _devFd, _memBase[x]);
         if (_map[x] == (void *) -1) {
            fprintf(stderr,"MappedMemory::open -> Can't map the memory to user space.\n");
            _map[x] = NULL;
            this->close();
            return(false);
         }
#endif
         if ( _debug ) 
            printf("MappedMemory::open -> Mapped 0x%08x with size 0x%08x.\n",_memBase[x],(uint32_t)_memSize[x]);
      }
   }

   // Sucess
   return(true);
}

// Close the port
void MappedMemory::close () {
   uint32_t x;

   if ( _emulate ) {
      for (x=0; x < _count; x++) {
         if ( _map[x] != NULL ) {
            free((void *)_map[x]);
            _map[x] = NULL;

            if ( _debug ) 
               printf("MappedMemory::close -> Un-Emulating 0x%08x with size 0x%08x.\n",_memBase[x],(uint32_t)_memSize[x]);
         }
      }
   } else {
      for (x=0; x < _count; x++) {
         if ( _map[x] != NULL ) {

#ifndef RTEMS
            munmap((void *)_memBase[x],_memSize[x]);
#endif
            _map[x] = NULL;

            if ( _debug ) 
               printf("MappedMemory::close -> UnMapped 0x%08x with size 0x%08x.\n",_memBase[x],(uint32_t)_memSize[x]);
         }
         _map[x] = NULL;
      }
#ifndef RTEMS
      ::close(_devFd);
#endif
      _devFd = -1;
   }
   _open = false;
}

// Debug enable
void MappedMemory::debug (bool enable) {
   _debug = enable;
}

// Emulate enable
void MappedMemory::emulate (bool enable) {
   if ( _open == false ) {
      _emulate = enable;
   }
}

// Write
void MappedMemory::write (uint32_t address, uint32_t value, bool *err ) {
   uint32_t x;

   if ( !_open ) return;

   for (x=0; x < _count; x++) {
      if ( (address >= _memBase[x]) && (address < (_memBase[x] + _memSize[x])) ) {
         if (_debug) printf("MappedMemory::write -> Write 0x%08x to address 0x%08x.\n",value,address);
         volatile uint32_t *ptr = (uint32_t *)(_map[x] + (address-_memBase[x]));
         *ptr = value;
         return;
      }
   }
   *err = true;
   if (_debug) printf("MappedMemory::write -> No space matches address 0x%08x.\n",address);
}

// Read
uint32_t MappedMemory::read (uint32_t address, bool *err ) {
   uint32_t   x;
   uint32_t   ret;
   volatile uint32_t * ptr;

   if ( !_open ) return(0);

   for (x=0; x < _count; x++) {
      if ( (address >= _memBase[x]) && (address < (_memBase[x] + _memSize[x])) ) {
         ptr = (volatile uint32_t *)(_map[x] + (address-_memBase[x]));
         ret = *ptr;
         if (_debug) printf("MappedMemory::read -> Read 0x%08x from address 0x%08x.\n",ret,address);
         return(ret);
      }
   } 
   if (_debug) printf("MappedMemory::read -> No space matches address 0x%08x.\n",address);
   *err = true;
   return(0);
}

