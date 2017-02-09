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

// Commands
#define CMD_GET_TOTAL  0x01
#define CMD_POST_BLOCK 0x02
#define CMD_READ_BLOCK 0x03

struct AxiStreamDmaEntry {
   unsigned char cmd;
   unsigned int  idx;
   unsigned char firstUser;
   unsigned char lastUser;
   unsigned char dest;
   unsigned char overflow;
   unsigned char writeError;
   unsigned int  blockCount;
   unsigned int  blockSize;
   unsigned int  rxSize;
   int           rxIdx;
};

// Get buffer
inline ssize_t axiStreamGetTotal(int fd, struct AxiStreamDmaEntry *data) {
   int ret = 0;

   data->cmd = CMD_GET_TOTAL;

#ifndef AXIS_IN_KERNEL
   ret = read(fd,data,sizeof(struct AxiStreamDmaEntry));
#endif

   return(ret);
}

// Post buffer to hardware free list
inline ssize_t axiStreamPostBlock(int fd, struct AxiStreamDmaEntry *data) {
   int ret = 0;

   data->cmd = CMD_POST_BLOCK;

#ifndef AXIS_IN_KERNEL
   ret = read(fd,data,sizeof(struct AxiStreamDmaEntry));
#endif

   return(ret);
}

// read data
inline ssize_t axiStreamReadBlock(int fd, struct AxiStreamDmaEntry *data) {
   int ret = 0;

   data->cmd = CMD_READ_BLOCK;

#ifndef AXIS_IN_KERNEL
   ret = read(fd,data,sizeof(struct AxiStreamDmaEntry));
#endif

   return(ret);
}

#endif

