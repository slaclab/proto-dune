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
#include <signal.h>
#include <sys/mman.h>
#include <iostream>
#include <iomanip>
#include "../driverV3/AxiStreamDma.h"
using namespace std;

uint count;
uint lastSize;
uint errors;

uint buffSize;
unsigned char * rxData;
int  fd;
int  rxSize;

void int_handler ( int parm ) {
   do {
      rxSize = axisRead(fd,rxData,buffSize,0,0,0);
      if (rxSize > 0 ) {
         lastSize = rxSize;
         count++;
      } else if ( rxSize < 0 ) errors++;
   } while(rxSize > 0);
}

int main(int argc, char **argv) {
   time_t            lastTme;
   time_t            currTme;
   uint              lastCount;

   fd = open("/dev/axi_stream_dma_1",O_RDWR|O_NONBLOCK);

   if ( fd < 0 ) {
      printf("Error opening device\n");
      return(1);
   }

   buffSize  = axisGetReadBufferSize(fd);
   rxData = (unsigned char *)malloc(buffSize);

   lastCount = 0;
   count     = 0;
   lastSize  = 0;
   errors    = 0;

   axisAssignHandler(fd,int_handler);

   time(&lastTme);
   currTme = lastTme;

   int_handler ( 0 );

   while (1) {
      usleep(10);
      time(&currTme);
      if ( currTme != lastTme ) {
         printf("Total %i, Rate %i hz, Size %i, %i MBps, %i Mbps, %i Errors\n",
            count, (count-lastCount), lastSize, ((count-lastCount)*lastSize)/(1024*1024), 
            (((count-lastSize)*lastSize)/(1024*1024))*8, errors);
      
         lastTme = currTme;
         lastCount = count;
      }
   }
}

