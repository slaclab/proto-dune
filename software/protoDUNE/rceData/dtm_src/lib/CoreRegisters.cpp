#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <string>
#include <string.h>
#include <unistd.h>
#include <sys/mman.h>
#include <iostream>
#include <iomanip>
#include "CoreRegisters.h"
 
CoreRegisters::CoreRegisters () {
   _devFd         = -1;
   _memMappedBase = NULL;
}

CoreRegisters::~CoreRegisters () {
   this->close();
}

// Open the port
bool CoreRegisters::open () {

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
void CoreRegisters::close () {
   if ( _devFd > 0 ) {
      ::close(_devFd);
      _devFd = -1;
   }
}

// FPGA Version
uint CoreRegisters::fpgaVersion ( ) {
   if ( _devFd < 0 ) return(0);

   uint * ptr = (uint *)(_memMappedBase + 0x00);

   return(*ptr);
}

// Scratchpad
uint CoreRegisters::scratchPad ( ) {
   if ( _devFd < 0 ) return(0);

   uint * ptr = (uint *)(_memMappedBase + 0x04);

   return(*ptr);
}

void CoreRegisters::scratchPad (uint value) {
   if ( _devFd < 0 ) return;

   uint * ptr = (uint *)(_memMappedBase + 0x04);

   *ptr = value;
}

// RCE Version
uint CoreRegisters::rceVersion ( ) {
   if ( _devFd < 0 ) return(0);

   uint * ptr = (uint *)(_memMappedBase + 0x08);

   return(*ptr);
}

// Ref Clock Select
uint CoreRegisters::refClockSelect (uint idx) {
   if ( _devFd < 0 || idx > 1 ) return(0);

   uint * ptr = (uint *)(_memMappedBase + 0x10 + (idx*4) );

   return(*ptr);
}

void CoreRegisters::refClockSelect (uint idx, uint value) {
   if ( _devFd < 0 || idx > 1 ) return;

   uint * ptr = (uint *)(_memMappedBase + 0x10 + (idx*4) );

   *ptr = value;
}

// Build String
const char * CoreRegisters::buildString ( ) {
   if ( _devFd < 0 ) return(0);

   uint x;
   uint *ptri = (uint *)_buildString;

   for (x=0; x < 64; x++ ) {
      ptri[x] = ((uint *)(_memMappedBase+0x1000))[x];
   }

   return(_buildString);
}

