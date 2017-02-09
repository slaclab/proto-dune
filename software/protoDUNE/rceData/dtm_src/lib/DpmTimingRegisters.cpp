#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <string>
#include <unistd.h>
#include <sys/mman.h>
#include <iostream>
#include <iomanip>
#include "DpmTimingRegisters.h"
 
DpmTimingRegisters::DpmTimingRegisters () {
   _devFd         = -1;
   _memMappedBase = NULL;
}

DpmTimingRegisters::~DpmTimingRegisters () {
   this->close();
}

// Open the port
bool DpmTimingRegisters::open () {

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
void DpmTimingRegisters::close () {
   if ( _devFd > 0 ) {
      ::close(_devFd);
      _devFd = -1;
   }
}

// Reset control
void DpmTimingRegisters::reset ( bool state ) {
   if ( _devFd < 0 ) return;

   uint * ptr = (uint *)(_memMappedBase);

   *ptr = state;
}

bool DpmTimingRegisters::reset ( ) {
   if ( _devFd < 0 ) return(false);

   uint * ptr = (uint *)(_memMappedBase);

   return(*ptr != 0);
}


// Opcode store enable
void DpmTimingRegisters::ocEnable ( bool enable ) {
   if ( _devFd < 0 ) return;

   uint * ptr = (uint *)(_memMappedBase + 0x04);

   if ( enable ) *ptr = 1;
   else *ptr = 0;
}

bool DpmTimingRegisters::ocEnable ( ) {
   if ( _devFd < 0 ) return(false);

   uint * ptr = (uint *)(_memMappedBase + 0x04);

   return(*ptr != 0);
}


// Opcode register delay
void DpmTimingRegisters::ocDelay ( uint delay ) {
   if ( _devFd < 0 ) return;

   uint * ptr = (uint *)(_memMappedBase + 0x08);

   *ptr = delay;
}

uint DpmTimingRegisters::ocDelay ( ) {
   if ( _devFd < 0 ) return(0);

   uint * ptr = (uint *)(_memMappedBase + 0x08);

   return(*ptr);
}

// Opcode sync Count
uint DpmTimingRegisters::ocIdleCount ( ) {
   if ( _devFd < 0 ) return(0);

   uint * ptr = (uint *)(_memMappedBase + 0x0C);

   return(*ptr & 0xFFFF);
}


// Opcode error Count
uint DpmTimingRegisters::ocErrorCount ( ) {
   uint value;

   if ( _devFd < 0 ) return(0);

   uint * ptr = (uint *)(_memMappedBase + 0x0C);

   value = *ptr;

   return((value >> 16) & 0xFFFF);
}


// Read an opcode entry
bool DpmTimingRegisters::ocEntry ( uint *code ) {
   uint value;

   if ( _devFd < 0 ) return(false);

   uint * ptr = (uint *)(_memMappedBase + 0x10);

   value = *ptr;

   *code = value & 0xFF;

   return((value & 0x100) != 0);
}


// Clock counter
uint DpmTimingRegisters::clkCount ( ) {
   if ( _devFd < 0 ) return(0);

   uint * ptr = (uint *)(_memMappedBase + 0x14);

   return(*ptr);
}

