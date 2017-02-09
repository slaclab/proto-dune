//-----------------------------------------------------------------------------
// File          : DataSharedMem.h
// Author        : Ryan Herbst  <rherbst@slac.stanford.edu>
// Created       : 03/29/2013
// Project       : General Purpose
//-----------------------------------------------------------------------------
// Description :
// Shared memory for live display
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
// 01/11/2013: created
//-----------------------------------------------------------------------------
#ifndef __DATA_SHARED_MEM_H__
#define __DATA_SHARED_MEM_H__

#ifndef RTEMS
#include <sys/mman.h>
#endif

#include <sys/stat.h>
#include <fcntl.h>   
#include <string.h>   
#include <unistd.h>
#include <stdio.h>
#include <stdint.h>

#define DATA_BUFF_SIZE 2097152
#define DATA_BUFF_CNT  20
#define DATA_NAME_SIZE 200

typedef struct {

   uint32_t wrAddr;
   uint32_t wrCount;
   uint32_t wrAddrLast;
   uint32_t wrCountLast;
   char         sharedName[DATA_NAME_SIZE];
   uint8_t  buffer[DATA_BUFF_CNT][DATA_BUFF_SIZE];

} DataSharedMemory;


#ifndef RTEMS

// Open and map shared memory
inline int32_t dataSharedOpenAndMap ( DataSharedMemory **ptr, const char *system, int32_t id, int32_t uid=-1 ) {
   int32_t       smemFd;
   char          shmName[200];
   int32_t       lid;

   // ID to use?
   if ( uid == -1 ) lid = getuid();
   else lid = uid;

   // Generate shared memory
   sprintf(shmName,"data_shared.%i.%s.%i",lid,system,id);

   // Attempt to open existing shared memory
   if ( (smemFd = shm_open(shmName, O_RDWR, (S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH)) ) < 0 ) {

      // Otherwise open and create shared memory
      if ( (smemFd = shm_open(shmName, (O_CREAT | O_RDWR), (S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH)) ) < 0 ) return(-1);

      // Force permissions regardless of umask
      fchmod(smemFd, (S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH));
    
      // Set the size of the shared memory segment
      ftruncate(smemFd, sizeof(DataSharedMemory));
   }

   // Map the shared memory
   if((*ptr = (DataSharedMemory *)mmap(0, sizeof(DataSharedMemory),
              (PROT_READ | PROT_WRITE), MAP_SHARED, smemFd, 0)) == MAP_FAILED) return(-2);

   // Store name
   strcpy((*ptr)->sharedName,shmName);

   return(smemFd);
}

// Close shared memory
inline void dataSharedClose ( DataSharedMemory *ptr ) {
   char shmName[200];

   // Get shared name
   strcpy(shmName,ptr->sharedName);

   // Unlink
   shm_unlink(shmName);
}

// Init data structure, called by ControlServer
inline void dataSharedInit ( DataSharedMemory *ptr ) {
   ptr->wrAddr      = 0;
   ptr->wrAddrLast  = 0;
   ptr->wrCount     = 0;
   ptr->wrCountLast = 0;
}

// Write to shared buffer
inline void dataSharedWrite ( DataSharedMemory *ptr, uint32_t flag, const uint8_t *data, uint32_t count ) {
   if ( (count+1) < DATA_BUFF_SIZE ) {
      ptr->wrAddrLast  = ptr->wrAddr;
      ptr->wrCountLast = ptr->wrCount;

      uint32_t *u32 = (uint32_t *)ptr->buffer[ptr->wrAddr];

      *u32 = flag;
      //*((int32_t *)ptr->buffer[ptr->wrAddr]) = flag;

      memcpy(&(ptr->buffer[ptr->wrAddr][4]),data,count);

      ptr->wrCount++;
      ptr->wrAddr++;
      if ( ptr->wrAddr == DATA_BUFF_CNT ) ptr->wrAddr = 0;
   }
}

// Read from shared buffer
inline int32_t dataSharedRead ( DataSharedMemory *ptr, uint32_t *rdAddr, uint32_t *rdCount, uint32_t *flag, uint8_t **data ) {

   // Detect if reader is ahead of writer or too far behind writer
   if ( (*rdCount == 0 && ptr->wrCount != 0) || ( *rdCount > ptr->wrCount ) || ( (ptr->wrCount - *rdCount) >= (DATA_BUFF_CNT/2) ) ) {
      printf("Adjusting pointers. wrAddr=%i, wrCount=%i, wrAddrLast=%i, wrCountLast=%i, rdAddr=%i, rdCount=%i\n",ptr->wrAddr,ptr->wrCount,
         ptr->wrAddrLast,ptr->wrCountLast,*rdAddr,*rdCount);
      *rdAddr  = ptr->wrAddrLast;
      *rdCount = ptr->wrCountLast;
   }

   if ( *rdCount == ptr->wrCount ) return(0);

   uint32_t *u32 = (uint32_t *)ptr->buffer[*rdAddr];
   *flag = *u32;

   //*flag = *((uint32_t *)ptr->buffer[*rdAddr]);
   *data = &(ptr->buffer[*rdAddr][4]);

   (*rdCount)++;
   (*rdAddr)++;
   if ( *rdAddr == DATA_BUFF_CNT ) *rdAddr = 0;
   return(1);
}

#else

// For RTEMS
inline int32_t  dataSharedOpenAndMap ( DataSharedMemory **ptr, const char *system, int32_t id, int32_t uid=-1 ) { return -1;}
inline void dataSharedClose ( DataSharedMemory *ptr ) {}
inline void dataSharedInit ( DataSharedMemory *ptr ) {}
inline void dataSharedWrite ( DataSharedMemory *ptr, int32_t flag, const char *data, int32_t count ) {}
inline int32_t  dataSharedRead ( DataSharedMemory *ptr, uint32_t *rdAddr, uint32_t *rdCount, uint32_t *flag, char **data ) { return(1); }

#endif
#endif

