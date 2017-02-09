//////////////////////////////////////////////////////////////////////////////
// This file is part of 'AXI Stream DMA Core'.
// It is subject to the license terms in the LICENSE.txt file found in the 
// top-level directory of this distribution and at: 
//    https://confluence.slac.stanford.edu/display/ppareg/LICENSE.html. 
// No part of 'AXI Stream DMA Core', including this file, 
// may be copied, modified, propagated, or distributed except according to 
// the terms contained in the LICENSE.txt file.
//////////////////////////////////////////////////////////////////////////////
#ifndef __AXI_STREAM_DMA_H__
#define __AXI_STREAM_DMA_H__

#define AXIS_DMA_POLL 1
#define AXIS_DMA_ACK  1

struct AxiStreamDmaTx {
   const void    * buffer;
   size_t          size;
   unsigned char   firstUser;
   unsigned char   lastUser;
   unsigned char   dest;
};

struct AxiStreamDmaRx {
   void          * buffer;
   size_t          maxSize;
   unsigned char   firstUser;
   unsigned char   lastUser;
   unsigned char   dest;
   unsigned char   overflow;
   unsigned char   writeError;
};

// Write Data
inline ssize_t axiStreamWrite(int fd, const void *buf, size_t size, 
                              unsigned int firstUser, unsigned int lastUser, unsigned int dest ) {

   struct AxiStreamDmaTx dmaTx;
   ssize_t ret = 0;

   dmaTx.buffer    = buf;
   dmaTx.size      = size;
   dmaTx.firstUser = (unsigned char)firstUser;
   dmaTx.lastUser  = (unsigned char)lastUser;
   dmaTx.dest      = (unsigned char)dest;

#ifndef AXIS_IN_KERNEL
   ret = write(fd,&dmaTx,sizeof(struct AxiStreamDmaTx));
#endif

   return(ret);
}

// Read Data
inline ssize_t axiStreamRead(int fd, void *buf, size_t size, 
                             unsigned int *firstUser, unsigned int *lastUser, unsigned int *dest,
                             unsigned int *overflow, unsigned int *writeError ) {

   struct AxiStreamDmaRx dmaRx;
   ssize_t ret = 0;

   dmaRx.buffer     = buf;
   dmaRx.maxSize    = size;
   dmaRx.firstUser  = 0;
   dmaRx.lastUser   = 0;
   dmaRx.dest       = 0;
   dmaRx.overflow   = 0;
   dmaRx.writeError = 0;

#ifndef AXIS_IN_KERNEL
   ret = read(fd,&dmaRx,sizeof(struct AxiStreamDmaRx));
#endif

   if ( firstUser  != NULL ) *firstUser  = (unsigned int)dmaRx.firstUser;
   if ( lastUser   != NULL ) *lastUser   = (unsigned int)dmaRx.lastUser;
   if ( dest       != NULL ) *dest       = (unsigned int)dmaRx.dest;
   if ( overflow   != NULL ) *overflow   = (unsigned int)dmaRx.overflow;
   if ( writeError != NULL ) *writeError = (unsigned int)dmaRx.writeError;

   return(ret);
}

// Return > 0 if data is ready
inline ssize_t axiStreamPoll (int fd) {
   ssize_t ret = 0;

#ifndef AXIS_IN_KERNEL
   unsigned char c;
   ret = read(fd,&c,AXIS_DMA_POLL);
#endif
   return(ret);
}

// Ack DMA
inline void axiStreamAck (int fd) {
#ifndef AXIS_IN_KERNEL
   unsigned char c;
   write(fd,&c,AXIS_DMA_ACK);
#endif
}

#endif

