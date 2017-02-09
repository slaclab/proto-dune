//////////////////////////////////////////////////////////////////////////////
// This file is part of 'DUNE Development Software'.
// It is subject to the license terms in the LICENSE.txt file found in the 
// top-level directory of this distribution and at: 
//    https://confluence.slac.stanford.edu/display/ppareg/LICENSE.html. 
// No part of 'DUNE Development Software', including this file, 
// may be copied, modified, propagated, or distributed except according to 
// the terms contained in the LICENSE.txt file.
// Proprietary and confidential to SLAC.
//////////////////////////////////////////////////////////////////////////////

#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <iostream>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h> 
#include <arpa/inet.h>

using namespace std;
extern int h_errno;  
int main(int argc, char **argv) {
   uint              buffSize  = 1024*1024;
   unsigned char     rxData[buffSize];
   uint              lrxSize;
   time_t            lastTme;
   time_t            currTme;
   uint              count;
   uint              total;
   uint              errors;
   uint              overflows;
   int servFd_, newFd,n;  
   int portno;
   socklen_t cliLen;
   struct sockaddr_in servAddr_, cliAddr_;
   portno = 8099;
   
   // Setup the connection
   servFd_ = socket(AF_INET, SOCK_STREAM, 0);
   bzero((char *)&servAddr_,sizeof(servAddr_));
   servAddr_.sin_family = AF_INET;
   servAddr_.sin_addr.s_addr = INADDR_ANY;
   servAddr_.sin_port = htons(portno);
   if ( bind(servFd_, (struct sockaddr *) &servAddr_, sizeof(servAddr_)) < 0 ) {
     servFd_ = -1;
     cout << "ControlServer::startListen -> Failed to bind socket " << dec << portno << endl;
     exit(0);
   }
   
   // Start listen
   cout<<"Listening for a connection on port "<<portno<<endl;
   listen(servFd_,5);
   
   cliLen = sizeof(cliAddr_);
   newFd = accept(servFd_,(struct sockaddr *)&cliAddr_,&cliLen);
   if(newFd<0){
     cout<<"Error on Accept"<<endl;    
   }     else {
     cout<<"Accepted connection from client"<<endl;
   }
   
   bzero(rxData,buffSize);
   while(1){
     n = read(newFd,rxData,buffSize-1);
     if (n < 0) cout<<"ERROR reading from socket"<<endl;
     if(n==0)usleep(1);
     else {
       lrxSize = n;
       
       //if ( ((*((uint *)rxData) >> 24) & 0x1) != 0 ) errors++;
       count++;
       total++;
     }
     time(&currTme);
     if ( currTme != lastTme ) {
       printf("Rate %i Hz, %i Mbps, %i errors, %i size, %i total\n",count,(((count*lrxSize)<<3)/1000000),errors,lrxSize,total);
       count = 0;
       errors = 0;
       overflows = 0;
       lastTme = currTme;
     }
   }
   close(servFd_);
   return 0;
}
