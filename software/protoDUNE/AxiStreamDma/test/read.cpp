//////////////////////////////////////////////////////////////////////////////
// This file is part of 'AXI Stream DMA Core'.
// It is subject to the license terms in the LICENSE.txt file found in the 
// top-level directory of this distribution and at: 
//    https://confluence.slac.stanford.edu/display/ppareg/LICENSE.html. 
// No part of 'AXI Stream DMA Core', including this file, 
// may be copied, modified, propagated, or distributed except according to 
// the terms contained in the LICENSE.txt file.
//////////////////////////////////////////////////////////////////////////////
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <sys/mman.h>
#include <iostream>
#include <iomanip>
#include <signal.h>
#include "../driverV3/AxiStreamDma.h"
using namespace std;
 
int main(int argc, char **argv) {
   uint              buffSize;
   unsigned char   * rxData;
   int               rxSize;
   uint              count;

   int fd = open("/dev/axi_stream_dma_1",O_RDWR|O_NONBLOCK);

   if ( fd < 0 ) {
      printf("Error opening device\n");
      return(1);
   }

   buffSize = axisGetReadBufferSize(fd);
   rxData = (unsigned char *)malloc(buffSize);

   count = 0;

   do {
      rxSize = axisRead(fd,rxData,buffSize,0,0,0);
      if ( rxSize > 0 ) count++;
   } while (rxSize > 0);

   printf("Read %i blocks\n",count);

}

