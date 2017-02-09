//////////////////////////////////////////////////////////////////////////////
// This file is part of 'SLAC Generic DAQ Software'.
// It is subject to the license terms in the LICENSE.txt file found in the 
// top-level directory of this distribution and at: 
//    https://confluence.slac.stanford.edu/display/ppareg/LICENSE.html. 
// No part of 'SLAC Generic DAQ Software', including this file, 
// may be copied, modified, propagated, or distributed except according to 
// the terms contained in the LICENSE.txt file.
//////////////////////////////////////////////////////////////////////////////
#ifndef __AXI_STREAM_DMA_H__
#define __AXI_STREAM_DMA_H__

#include <stdint.h>

// Return Errors
#define ERR_DRIVER     -1
#define ERR_BUFF_OFLOW -2
#define ERR_DMA_OFLOW  -3
#define ERR_AXI_WRITE  -4

// Get Write Buffer Count
// inline void axisGetWriteBufferCount(int32_t fd)

// Get Write Buffer Size 
// inline void axisGetWriteBufferSize(int32_t fd)

// Write Data
// inline ssize_t axisWrite(int32_t fd, const void *buf, size_t size, uint32_t fUser, uint32_t lUser, uint32_t dest )

// Read Data
// inline ssize_t axisRead(int32_t fd, void *buf, size_t size, uint32_t *fUser, uint32_t *lUser, uint32_t *dest )

// Read Index
// inline ssize_t axisReadUser(int32_t fd, uint32_t *index, uint32_t *fUser, uint32_t *lUser, uint32_t *dest )

// Post Index
// inline ssize_t axisPostUser(int32_t fd, uint32_t index )

// Get Read Buffer Count
// inline void axisGetReadBufferCount(int32_t fd)

// Get Read Buffer Size 
// inline void axisGetReadBufferSize(int32_t fd)

// Read ACK
// inline void axisReadAck (int32_t fd)

// Read Ready
// inline void axisReadReady (int32_t fd)

// Return user space mapping to dma buffers
// inline uint8_t ** axisMapUser(int32_t fd, uint32_t *count, uint32_t *size)

// Free space mapping to dma buffers
// inline void axisUnMapUser(int32_t fd, uint8_t ** buffer)

struct AxiStreamDmaWrite {
   uint32_t     command;
   const void * buffer;
   size_t       size;
   uint32_t     fUser;
   uint32_t     lUser;
   uint32_t     dest;
};

struct AxiStreamDmaRead {
   uint32_t  command;
   void * buffer;
   int32_t   index;
   int32_t   size;
   uint32_t  fUser;
   uint32_t  lUser;
   uint32_t  dest;
};

// Commands
#define CMD_GET_BSIZE  0x01
#define CMD_GET_BCOUNT 0x02
#define CMD_WRITE_DATA 0x03
#define CMD_READ_COPY  0x04
#define CMD_READ_USER  0x05
#define CMD_POST_USER  0x06
#define CMD_READ_ACK   0x07
#define CMD_READ_READY 0x08

// Everything below is hidden during kernel module compile
#ifndef AXIS_IN_KERNEL
#include <stdlib.h>

#ifndef RTEMS
#include <sys/mman.h>
#endif

// Get Write Buffer Count
inline ssize_t axisGetWriteBufferCount(int32_t fd) {
   struct AxiStreamDmaWrite dmaWr;

   dmaWr.command = CMD_GET_BCOUNT;

   return(write(fd,&dmaWr,sizeof(struct AxiStreamDmaWrite)));
}

// Get Write Buffer Size 
inline ssize_t axisGetWriteBufferSize(int32_t fd) {
   struct AxiStreamDmaWrite dmaWr;

   dmaWr.command = CMD_GET_BSIZE;

   return(write(fd,&dmaWr,sizeof(struct AxiStreamDmaWrite)));
}

// Write Data
inline ssize_t axisWrite(int32_t fd, const void *buf, size_t size, uint32_t fUser, uint32_t lUser, uint32_t dest ) {
   struct AxiStreamDmaWrite dmaWr;

   dmaWr.command = CMD_WRITE_DATA;
   dmaWr.buffer  = buf;
   dmaWr.size    = size;
   dmaWr.fUser   = fUser;
   dmaWr.lUser   = lUser;
   dmaWr.dest    = dest;

   return(write(fd,&dmaWr,sizeof(struct AxiStreamDmaWrite)));
}

// Read Data
inline ssize_t axisRead(int32_t fd, void *buf, size_t size, uint32_t *fUser, uint32_t *lUser, uint32_t *dest ) {
   struct AxiStreamDmaRead dmaRd;
   ssize_t ret;

   dmaRd.command = CMD_READ_COPY;
   dmaRd.buffer  = buf;
   dmaRd.size    = size;

   ret = read(fd,&dmaRd,sizeof(struct AxiStreamDmaRead));

   if ( fUser  != NULL ) *fUser  = dmaRd.fUser;
   if ( lUser  != NULL ) *lUser  = dmaRd.lUser;
   if ( dest   != NULL ) *dest   = dmaRd.dest;

   return(ret);
}

// Read Index
inline ssize_t axisReadUser(int32_t fd, uint32_t *index,  uint32_t *fUser, uint32_t *lUser, uint32_t *dest ) {
   struct AxiStreamDmaRead dmaRd;
   ssize_t ret;

   dmaRd.command = CMD_READ_USER;

   ret = read(fd,&dmaRd,sizeof(struct AxiStreamDmaRead));
   
   *index = dmaRd.index;

   if ( fUser  != NULL ) *fUser  = dmaRd.fUser;
   if ( lUser  != NULL ) *lUser  = dmaRd.lUser;
   if ( dest   != NULL ) *dest   = dmaRd.dest;

   return(ret);
}

// Post Index
inline ssize_t axisPostUser(int32_t fd, uint32_t index ) {
   struct AxiStreamDmaRead dmaRd;

   dmaRd.command = CMD_POST_USER;
   dmaRd.index   = index;

   return(read(fd,&dmaRd,sizeof(struct AxiStreamDmaRead)));
}

// Get Read Buffer Count
inline ssize_t axisGetReadBufferCount(int32_t fd) {
   struct AxiStreamDmaRead dmaRd;

   dmaRd.command = CMD_GET_BCOUNT;

   return(read(fd,&dmaRd,sizeof(struct AxiStreamDmaRead)));
}

// Get Read Buffer Size 
inline ssize_t axisGetReadBufferSize(int32_t fd) {
   struct AxiStreamDmaRead dmaRd;

   dmaRd.command = CMD_GET_BSIZE;

   return(read(fd,&dmaRd,sizeof(struct AxiStreamDmaRead)));
}

// Read ACK
inline void axisReadAck (int32_t fd) {
   struct AxiStreamDmaRead dmaRd;

   dmaRd.command = CMD_READ_ACK;

   read(fd,&dmaRd,sizeof(struct AxiStreamDmaRead));
}

// Read Ready
inline ssize_t axisReadReady (int32_t fd) {
   struct AxiStreamDmaRead dmaRd;

   dmaRd.command = CMD_READ_READY;

   return(read(fd,&dmaRd,sizeof(struct AxiStreamDmaRead)));
}

// Return user space mapping to dma buffers
inline uint8_t ** axisMapUser(int32_t fd, uint32_t *count, uint32_t *size) {
#ifndef RTEMS
   void *  temp;
   uint8_t ** ret;
   uint32_t   bCount;
   uint32_t   bSize;
   uint32_t   x;;

   bCount = axisGetReadBufferCount(fd);
   bSize  = axisGetReadBufferSize(fd);

   if ( count != NULL ) *count = bCount;
   if ( size  != NULL ) *size  = bSize;

   if ( (ret = (uint8_t **)malloc(sizeof(uint8_t *) * bCount)) == 0 ) return(NULL);

   for (x=0; x < bCount; x++) {

      if ( (temp = mmap (0, bSize, PROT_READ, MAP_PRIVATE, fd, (bSize*x))) == MAP_FAILED) {
         free(ret);
         return(NULL);
      }

      ret[x] = (uint8_t *)temp;
   }

   return(ret);
#else
   return(NULL);
#endif
}

// Free space mapping to dma buffers
inline void axisUnMapUser(int32_t fd, uint8_t ** buffer) {
#ifndef RTEMS
   uint32_t   bCount;
   uint32_t   bSize;
   uint32_t   x;;

   bCount = axisGetReadBufferCount(fd);
   bSize  = axisGetReadBufferSize(fd);

   for (x=0; x < bCount; x++) munmap (buffer, bSize);

   free(buffer);
#endif
}
#endif
#endif

