#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <string>
#include <unistd.h>
#include <sys/mman.h>
#include <iostream>
#include <iomanip>
#include "DtmTimingRegisters.h"
 
DtmTimingRegisters::DtmTimingRegisters () {
   _devFd         = -1;
   _memMappedBase = NULL;
}

DtmTimingRegisters::~DtmTimingRegisters () {
   this->close();
}

// Open the port
bool DtmTimingRegisters::open () {

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
void DtmTimingRegisters::close () {
   if ( _devFd > 0 ) {
      ::close(_devFd);
      _devFd = -1;
   }
}
 
// Opcode store enable
void DtmTimingRegisters::fbEnable ( uint channel, bool enable ) {
   uint val;
   uint bit;
   uint mask;

   if ( _devFd < 0 || channel > 7 ) return;

   uint * ptr = (uint *)(_memMappedBase);

   bit  = 1 << channel;
   mask = bit ^ 0xFF;

   val = *ptr & mask;

   if ( enable ) {
      val |= bit;
      *ptr = val;
   }
}

bool DtmTimingRegisters::fbEnable ( uint channel ) {
   uint val;
   uint bit;

   if ( _devFd < 0 || channel > 7 ) return(false);

   uint * ptr = (uint *)(_memMappedBase);

   bit  = 1 << channel;

   val = *ptr;

   if ( (val & bit) != 0 ) return(true);
   else return(false);
}


void DtmTimingRegisters::fbDelay ( uint channel, uint delay ) {
   if ( _devFd < 0 || channel > 7 ) return;

   uint * ptr = (uint *)(_memMappedBase + 0x100 + (channel * 4));

   *ptr = delay;
}

uint DtmTimingRegisters::fbDelay ( uint channel ) {
   if ( _devFd < 0 || channel > 7 ) return(false);

   uint * ptr = (uint *)(_memMappedBase + 0x100 + (channel * 4));

   return(*ptr);
}

// Opcode idle Count
uint DtmTimingRegisters::fbIdleCount (uint channel ) {
   uint value;

   if ( _devFd < 0 || channel > 7 ) return(0);

   uint * ptr = (uint *)(_memMappedBase + 0x200 + (channel * 4));

   value = *ptr;

   return(value & 0xFFFF);
}

// Opcode error Count
uint DtmTimingRegisters::fbErrorCount (uint channel ) {
   uint value;

   if ( _devFd < 0 || channel > 7 ) return(0);

   uint * ptr = (uint *)(_memMappedBase + 0x200 + (channel * 4));

   value = *ptr;

   return((value >> 16) & 0xFFFF);
}

// Read a feedback entry
bool DtmTimingRegisters::fbEntry (uint channel, uint *code ) {
   uint value;

   if ( _devFd < 0 || channel > 7 ) return(false);

   uint * ptr = (uint *)(_memMappedBase + 0x300 + (channel * 4));

   value = *ptr;

   *code = value & 0xFF;

   return((value & 0x100) != 0);
}

// Opcode write
void DtmTimingRegisters::ocWrite ( uint code ) {
   if ( _devFd < 0 ) return;

   uint * ptr = (uint *)(_memMappedBase + 0x400);

   *ptr = code;
}

// Read an opcode entry
bool DtmTimingRegisters::ocEntry ( uint *code ) {
   uint value;

   if ( _devFd < 0 ) return(false);

   uint * ptr = (uint *)(_memMappedBase + 0x404);

   value = *ptr;

   *code = value & 0xFF;

   return((value & 0x100) != 0);
}

// Opcode store enable
void DtmTimingRegisters::ocEnable ( bool enable ) {
   if ( _devFd < 0 ) return;

   uint * ptr = (uint *)(_memMappedBase + 0x408);

   if ( enable ) *ptr = 1;
   else *ptr = 0;
}

bool DtmTimingRegisters::ocEnable ( ) {
   if ( _devFd < 0 ) return(false);

   uint * ptr = (uint *)(_memMappedBase + 0x408);

   return(*ptr != 0);
}

// Clock counter
uint DtmTimingRegisters::clkCount ( ) {
   if ( _devFd < 0 ) return(0);

   uint * ptr = (uint *)(_memMappedBase + 0x40C);

   return(*ptr);
}

