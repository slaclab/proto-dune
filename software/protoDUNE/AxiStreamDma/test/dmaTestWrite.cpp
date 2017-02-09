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
#include <signal.h>
#include <iomanip>
#include "../driverV3/AxiStreamDma.h"
using namespace std;

bool stop;

void sigTerm (int) { 
   cout << "Got Signal!" << endl;
   stop = true; 
}

int main(int argc, char **argv) {
   uint              buffSize;
   unsigned char   * txData;
   uint              x;
   uint              txSize;
   int               ret;
   time_t            lastTme;
   time_t            currTme;
   uint              count;
   uint              total;
   int               maxFd;
   struct timeval    timeout;
   fd_set            fds;
   int               fd;

   fd = open("/dev/axi_stream_dma_2",O_RDWR|O_NONBLOCK);

   if ( fd < 0 ) {
      printf("Error opening device\n");
      return(1);
   }

   if ( argc < 2 ) {
      printf("Usage: %s size\n",argv[0]);
      exit(1);
   }
   txSize = atoi(argv[1]);

   buffSize  = axisGetReadBufferSize(fd);

   if ( txSize > buffSize ) {
      printf("Bad transmit size. Max=%i\n",buffSize);
      exit(1);
   }

   txData = (unsigned char *)malloc(buffSize);

   time(&lastTme);
   currTme = lastTme;
   count = 0;
   total = 0;
   stop  = false;

   maxFd = fd;

   for (x=0; x < txSize; x++) txData[x] = x;

   signal (SIGINT,&sigTerm);

   while (!stop) {
      timeout.tv_sec  = 0;
      timeout.tv_usec = 10000;
      FD_ZERO(&fds);
      FD_SET(fd,&fds);

      for (x=0; x < txSize; x++) txData[x] = x+total;

      if ( select(maxFd+1,NULL,&fds, NULL, &timeout) > 0 ) {
         if ( FD_ISSET(fd,&fds) ) {
            ret = axisWrite(fd,txData,txSize,0,0,0);
            if ( ret > 0 ) {
               count++;
               total++;
            }
         }
      }

      time(&currTme);
      if ( currTme != lastTme ) {
         printf("Total %i, Rate %i hz, Size %i, %i MBps. %i Mbps\n",
            total, count, txSize, (count*txSize)/(1024*1024), ((count*txSize)/(1024*1024)*8));

         count = 0;
         lastTme = currTme;
      }
   }
   printf("\nTotal %i\n",total);
}

