#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <string>
#include <unistd.h>
#include <sys/mman.h>
#include <iostream>
#include <iomanip>
#include "HpsDtmRegisters.h"
 
HpsDtmRegisters::HpsDtmRegisters () {
   _devFd         = -1;
   _memMappedBase = NULL;
}

HpsDtmRegisters::~HpsDtmRegisters () {
   this->close();
}

// Open the port
bool HpsDtmRegisters::open () {

   if ( _devFd > 0 ) return(true);

   // Open devmem
   _devFd = ::open("/dev/mem", O_RDWR | O_SYNC);
   if (_devFd == -1) {
      std::cout << "Can't open /dev/mem." << std::endl;
      return(false);
   }

   // Map memory space 
   _memMappedBase = mmap(0, MemoryMapSize, PROT_EXEC | PROT_READ | PROT_WRITE, MAP_SHARED, _devFd, MemoryBase);
   if (_memMappedBase == (void *) -1) {
      std::cout << "Can't map the dma memory to user space." << std::endl;
      ::close(_devFd);
      _devFd = -1;
      return(false);
   }

   // Sucess
   return(true);
}

// Close the port
void HpsDtmRegisters::close () {
   if ( _devFd > 0 ) {
      ::close(_devFd);
      _devFd = -1;
   }
}


// Trigger Enable
void HpsDtmRegisters::trigEnable ( bool enable ) {
   uint * ptr = (uint *)(_memMappedBase);
   ptr[0] = enable;
}

bool HpsDtmRegisters::trigEnable ( ) {
   uint * ptr = (uint *)(_memMappedBase);
   return(ptr[0]);
}

// Trigger Period
void HpsDtmRegisters::trigPeriod ( uint period ) {
   uint * ptr = (uint *)(_memMappedBase);
   ptr[1] = period;
}

uint HpsDtmRegisters::trigPeriod ( ) {
   uint * ptr = (uint *)(_memMappedBase);
   return(ptr[1]);
}

// Software Trigger
void HpsDtmRegisters::swTrigger  ( ) {
   uint * ptr = (uint *)(_memMappedBase);
   ptr[2] = 1;
}

// Ack Enable
void HpsDtmRegisters::ackEnable ( uint channel, bool enable ) {
   if ( channel > 7 ) return;

   uint * ptr = (uint *)(_memMappedBase);

   uint mask = 1 << channel;
   uint temp = ptr[3] & (0xFFFFFFFF ^ mask);

   if ( enable ) temp |= mask;

   ptr[3] = temp;
}

bool HpsDtmRegisters::ackEnable ( uint channel ) {
   if ( channel > 7 ) return(false);

   uint * ptr = (uint *)(_memMappedBase);

   uint mask = 1 << channel;
   return(ptr[3] & mask);
}

// Trigger count
uint HpsDtmRegisters::trigCount ( ) {
   uint * ptr = (uint *)(_memMappedBase);
   return(ptr[4]);
}

// Trigger Buffer Size
void HpsDtmRegisters::trigBuffSize ( uint buffSize ) {
   uint * ptr = (uint *)(_memMappedBase);
   ptr[5] = buffSize;
}

uint HpsDtmRegisters::trigBuffSize ( ) {
   uint * ptr = (uint *)(_memMappedBase);
   return(ptr[5]);
}

// Reset counters
void HpsDtmRegisters::countReset() {
   uint * ptr = (uint *)(_memMappedBase);
   ptr[6] = 1;
}

// Ack Count
uint HpsDtmRegisters::ackCount (uint channel) {
   if ( channel > 7 ) return(0);

   uint * ptr = (uint *)(_memMappedBase);
   return(ptr[32+channel]);
}

