#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <string>
#include <unistd.h>
#include <sys/mman.h>
#include <iostream>
#include <iomanip>
#include "HpsDpmRegisters.h"
 
HpsDpmRegisters::HpsDpmRegisters () {
   _devFd         = -1;
   _memMappedBase = NULL;
}

HpsDpmRegisters::~HpsDpmRegisters () {
   this->close();
}

// Open the port
bool HpsDpmRegisters::open () {

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
void HpsDpmRegisters::close () {
   if ( _devFd > 0 ) {
      ::close(_devFd);
      _devFd = -1;
   }
}


// Trigger Enable
void HpsDpmRegisters::trigEnable ( bool enable ) {
   uint * ptr = (uint *)(_memMappedBase);
   ptr[0] = enable;
}

bool HpsDpmRegisters::trigEnable ( ) {
   uint * ptr = (uint *)(_memMappedBase);
   return(ptr[0]);
}

// Data Size
void HpsDpmRegisters::dataSize ( uint size ) {
   uint * ptr = (uint *)(_memMappedBase);
   ptr[1] = size;
}

uint HpsDpmRegisters::dataSize ( ) {
   uint * ptr = (uint *)(_memMappedBase);
   return(ptr[1]);
}

// Trigger Ack
void HpsDpmRegisters::trigAck  ( ) {
   uint * ptr = (uint *)(_memMappedBase);
   ptr[2] = 1;
}

