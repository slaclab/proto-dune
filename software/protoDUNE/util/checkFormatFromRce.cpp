//////////////////////////////////////////////////////////////////////////////
// This file is part of 'LBNE Software'.
// It is subject to the license terms in the LICENSE.txt file found in the 
// top-level directory of this distribution and at: 
//    https://confluence.slac.stanford.edu/display/ppareg/LICENSE.html. 
// No part of 'LBNE Software', including this file, 
// may be copied, modified, propagated, or distributed except according to 
// the terms contained in the LICENSE.txt file.
//////////////////////////////////////////////////////////////////////////////
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sstream>
#include <iomanip>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <iostream>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h> 
#include <arpa/inet.h>
#include <algorithm>


using namespace std;
extern int h_errno;  

const uint		    wordSize=4;//bytes
const uint            headerSize=4*wordSize;//bytes
//uint		  maxWords=66002+7;//in uint 
const uint		  maxWords=66002+7-1;//in uint....header says the size is 264028 instead of 264036 
unsigned char   rxData[maxWords*wordSize];//Byte array
unsigned char   rxDataOld[maxWords*wordSize];//byte array
const uint   safeWord=0x708b309e;

bool checkHeader(unsigned char *, unsigned char*);
int main(int argc, char **argv) {
  
 // uint              buffSize  = 2048*2048;
 // unsigned char     rxData[buffSize];
  uint              lrxSize;
  time_t            lastTme;
  time_t            currTme;
  uint              count;
  uint              total;
  uint              errors;
  uint              overflows;
  
  // server fdes
  int servFd_, newFd,n;  
  // Port number
  int portno;
  socklen_t cliLen;
  struct sockaddr_in servAddr_, cliAddr_;
  //  struct in_addr rceaddr;
  portno = atoi(argv[1]);
   // Init structures
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
   total=0;  
   bzero(rxData,maxWords*wordSize);
   bzero(rxDataOld,maxWords*wordSize);
   size_t received = 0;
   uint bigDataError=0;
   uint oldTail=0;
   uint prevSeq=0;
   uint seqError=0;
   uint blowoff=0;
   unsigned char   boArray[wordSize];
   while(1){
         usleep(1);//add backpressure
     n = recv(newFd,rxData,headerSize,0);
     if (n < 0) cout<<"ERROR reading from socket"<<endl;
     if (n < 8) cout << "Read " << n << " bytes instead of header" << endl;

     if(n==0)usleep(1);
     else {
       //       lrxSize = n;
       //	     cout<<"getting this amount of data: "<<dataSize<<endl;
       bool isOk=checkHeader(rxData,rxDataOld);
       
       if(isOk){
	 received=n;
	 lrxSize += n;
	 uint dataSize=(uint)((rxData[3] <<24) | (rxData[2] << 16) | (rxData[1] << 8) | (rxData[0] << 0));
	 uint seqid=(uint)((rxData[7] <<24) | (rxData[6] << 16) | (rxData[5] << 8) | (rxData[4] << 0));
	 while (received < dataSize)
	   {
	     //			     ssize_t r = recv(newFd, rxData + received, maxWords*wordSize - received, 0);
	     ssize_t r = recv(newFd, rxData + received, dataSize - received, 0);
	     //if (r <= 0) break;
	     if (r <= 0) r=0;
	     received += r;
	     //cout<<"in receiver loop...got this much so far: "<<received<<"...need "<<dataSize<<endl;
	   }	
	 lrxSize+=received;
	 
	 if(dataSize!=received){
	   cout<<"microSlice size from header = "<<dataSize<<";  total received size = "<<received<<"; maxSize ="<<maxWords*wordSize<<endl;
	 }
	 //	 rxDataOld=rxData;
	 std::copy(rxData,rxData+maxWords*wordSize,rxDataOld);
	 oldTail=(uint)((rxData[dataSize-1] <<24) | (rxData[dataSize-2] << 16) | (rxData[dataSize-3] << 8) | (rxData[dataSize-4] << 0));
	 if(seqid!=prevSeq+1){
	   cout<<"sequence error! seqid = "<<seqid<<" while previous one was "<<prevSeq<<endl;
	   seqError++; 
	 }
	 prevSeq=seqid;
	 //if ( ((*((uint *)rxData[1]) >> 28) & 0x1) != 0 ) overflows++;
	 //if ( ((*((uint *)rxData[1]) >> 31) & 0x1) != 0 ) errors++;
	 count++;
	 total++;
       }else{
	 bigDataError++;	
	 //	 cout<<"Well...try reading another header..."<<endl;
	 cout<<"Well...blow off data until we get the safe word"<<endl;
	 blowoff=0;
	 std::cout <<"   safeWord  : 0x"
		   << std::hex << std::setw(8) << std::setfill('0') << safeWord <<std::dec << std::endl;
	 while(blowoff != safeWord){
	   n = recv(newFd,boArray,wordSize,0);
	   blowoff=(uint)((boArray[3] <<24) | (boArray[2] << 16) | (boArray[1] << 8) | (boArray[0] << 0));
	   std::cout << "n = "<<n<<"   blowoff  : 0x"
		     << std::hex << std::setw(8) << std::setfill('0') << blowoff <<std::dec << std::endl;
	 }
	 //	 exit(0);
       }
     }
     time(&currTme);
     if ( currTme != lastTme ) {
       printf("Rate %i hz, %i Bps, %i packet errors, %i sequence errors, %i last size, %i total\n",count,lrxSize,bigDataError,seqError,n,total);
       lrxSize=0;
       count = 0;
       errors = 0;
       overflows = 0;
       lastTme = currTme;
     }
   } 
   
close(servFd_);
return 0;

}
static uint64_t getMask(int param){
        uint64_t mask=0;
        mask = (1 << param) - 1;//sets the mask to 0000...11111...11
        return mask;
 };

bool checkHeader(unsigned char *rxData, unsigned char* rxDataOld ){
	uint dataSize=(uint)((rxData[3] <<24) | (rxData[2] << 16) | (rxData[1] << 8) | (rxData[0] << 0));
	uint rxseqid=(uint)((rxData[7] <<24) | (rxData[6] << 16) | (rxData[5] << 8) | (rxData[4] << 0));
	uint txseqid=(uint)((rxData[11] <<24) | (rxData[10] << 16) | (rxData[9] << 8) | (rxData[8] << 0));
	uint typid=(uint)((rxData[15] <<24) | (rxData[14] << 16) | (rxData[13] << 8) | (rxData[12] << 0));
	if(dataSize>265000){
	  
	//     cout<<"Big data size! "<< dataSize<<"  header size = "<<n<<"; previous received = "<<received<<endl;
	  cout<<"Big data size! "<< dataSize<<endl;
	  std::cout << " frame size header   : 0x"
		    << std::hex << std::setw(8) << std::setfill('0') << dataSize <<std::dec << std::endl;
	  std::cout << " Rx sequence id header   : 0x"
		    << std::hex << std::setw(8) << std::setfill('0') << rxseqid <<std::dec << std::endl;
	  std::cout << " Tx sequence id header   : 0x"
		    << std::hex << std::setw(8) << std::setfill('0') << txseqid <<std::dec << std::endl;
	  std::cout << " type id header   : 0x"
		    << std::hex << std::setw(8) << std::setfill('0') << typid <<std::dec << std::endl;
	  //std::cout << "  oldData:  "<<endl;
	  //	  for (int i=0;i<maxWords*wordSize;i=i+4){
	  //   uint tmp=(uint)((rxDataOld[i+3] <<24) | (rxDataOld[i+2] << 16) | (rxDataOld[i+1] << 8) | (rxData[i] << 0));
	  //  std::cout << " 0x"<<std::hex << std::setw(8) << std::setfill('0') << tmp <<std::dec << std::endl;
	  //}

	  
	  //drop out of this and try again
	  return false; 
	}
	if(dataSize>30){
	  //cout<<"Got a trigger! "<< dataSize<<"header size = "<<n<<"; previous received = "<<received<<endl;
	  //	  cout<<"Got a trigger! "<< dataSize<<endl;
	  //     std::cout << " frame size header   : 0x"
	  //	     << std::hex << std::setw(8) << std::setfill('0') << dataSize <<std::dec << std::endl;
	  //		     std::cout << "  oldTail  : 0x"
	  //                         << std::hex << std::setw(8) << std::setfill('0') << oldTail <<std::dec << std::endl;
	}
	if(dataSize<30){
	  //     cout<<".";
	}
	
	return true;
}

