#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <string>
#include <unistd.h>
#include <sys/mman.h>
#include <iostream>
#include <iomanip>
#include "PgpRegisters.h"
 
PgpRegisters::PgpRegisters () {
   _devFd         = -1;
   _memMappedBase = NULL;
}

PgpRegisters::~PgpRegisters () {
   this->close();
}

// Open the port
bool PgpRegisters::open () {

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

void * PgpRegisters::laneBase ( uint idx ) {
   if ( idx > 11 ) idx = 0;

   void * base = (_memMappedBase + 0x200 + (idx*32));
   return(base);
}

// Close the port
void PgpRegisters::close () {
   if ( _devFd > 0 ) {
      ::close(_devFd);
      _devFd = -1;
   }
}


bool PgpRegisters::linkReady ( uint idx ) {
   if ( _devFd < 0 || idx > 11 ) return(0);

   uint * ptr = (uint *)(laneBase(idx) + 0x00);

   return((*ptr&0x1) != 0);
}

uint PgpRegisters::cellErrorCount ( uint idx ) {
   if ( _devFd < 0 || idx > 11 ) return(0);

   uint * ptr = (uint *)(laneBase(idx) + 0x04);

   return(*ptr);
}

uint PgpRegisters::linkDownCount ( uint idx ) {
   if ( _devFd < 0 || idx > 11 ) return(0);

   uint * ptr = (uint *)(laneBase(idx) + 0x08);

   return(*ptr);
}

uint PgpRegisters::linkErrorCount ( uint idx ) {
   if ( _devFd < 0 || idx > 11 ) return(0);

   uint * ptr = (uint *)(laneBase(idx) + 0x0C);

   return(*ptr);
}

uint PgpRegisters::txCount ( uint idx ) {
   if ( _devFd < 0 || idx > 11 ) return(0);

   uint * ptr = (uint *)(laneBase(idx) + 0x10);

   return(*ptr);
}

uint PgpRegisters::rxCount ( uint idx ) {
   if ( _devFd < 0 || idx > 11 ) return(0);

   uint * ptr = (uint *)(laneBase(idx) + 0x14);

   return(*ptr);
}

uint PgpRegisters::eofeCount ( uint idx ) {
   if ( _devFd < 0 || idx > 11 ) return(0);

   uint * ptr = (uint *)(laneBase(idx) + 0x18);

   return(*ptr);
}

void PgpRegisters::countReset (bool set ) {
   if ( _devFd < 0 ) return;

   uint * ptr = (uint *)(_memMappedBase + 0x00);

   *ptr = set;
}

bool PgpRegisters::countReset () {
   if ( _devFd < 0 ) return(false);

   uint * ptr = (uint *)(_memMappedBase + 0x00);

   return(*ptr);
}

void PgpRegisters::loopEnable (bool set ) {
   if ( _devFd < 0 ) return;

   uint * ptr = (uint *)(_memMappedBase + 0x04);

   if ( set ) *ptr = 0x2;
   else *ptr = 0;
}

bool PgpRegisters::loopEnable () {
   if ( _devFd < 0 ) return(false);

   uint * ptr = (uint *)(_memMappedBase + 0x04);

   return(*ptr == 0x2);
}

void PgpRegisters::flowDis (bool set ) {
   if ( _devFd < 0 ) return;

   uint * ptr = (uint *)(_memMappedBase + 0x18);

   if ( set ) *ptr = 0x1;
   else *ptr = 0;
}

bool PgpRegisters::flowDis () {
   if ( _devFd < 0 ) return(false);

   uint * ptr = (uint *)(_memMappedBase + 0x18);

   return(*ptr == 1);
}


uint PgpRegisters::clockCount () {
   if ( _devFd < 0 ) return(0);

   uint * ptr = (uint *)(_memMappedBase + 0x08);

   return(*ptr);
}

uint PgpRegisters::status () {
   if ( _devFd < 0 ) return(0);

   uint * ptr = (uint *)(_memMappedBase + 0x0C);

   return(*ptr);
}

void PgpRegisters::pgpTxReset (uint idx, bool set ) {
   uint bit;
   uint mask;
   uint val;

   if ( _devFd < 0 || idx > 11 ) return;

   uint * ptr = (uint *)(_memMappedBase + 0x10);

   bit  = 1 << idx;
   mask = bit ^ 0xFFFFFFFF;

   val = *ptr & mask;

   if ( set ) val |= bit;

   *ptr = val;
}

bool PgpRegisters::pgpTxReset (uint idx ) {
   uint bit;

   if ( _devFd < 0 || idx > 11 ) return(false);

   uint * ptr = (uint *)(_memMappedBase + 0x10);

   bit  = 1 << idx;

   return((*ptr & bit) != 0);
}

void PgpRegisters::pgpRxReset (uint idx, bool set ) {
   uint bit;
   uint mask;
   uint val;

   if ( _devFd < 0 || idx > 11 ) return;

   uint * ptr = (uint *)(_memMappedBase + 0x10);

   bit  = 0x10000 << idx;
   mask = bit ^ 0xFFFFFFFF;

   val = *ptr & mask;

   if ( set ) val |= bit;

   *ptr = val;
}

bool PgpRegisters::pgpRxReset (uint idx ) {
   uint bit;

   if ( _devFd < 0 || idx > 11 ) return(false);

   uint * ptr = (uint *)(_memMappedBase + 0x10);

   bit  = 0x10000 << idx;

   return((*ptr & bit) != 0);
}

void PgpRegisters::pgpResetAll () {
   if ( _devFd < 0 ) return;

   uint * ptr = (uint *)(_memMappedBase + 0x10);

   *ptr = 0xFFFFFFFF;
}

void PgpRegisters::pllReset ( bool set ) {
   if ( _devFd < 0 ) return;

   uint * ptr = (uint *)(_memMappedBase + 0x14);

   *ptr = set;
}

bool PgpRegisters::pllReset ( ) {
   if ( _devFd < 0 ) return(false);

   uint * ptr = (uint *)(_memMappedBase + 0x14);

   return(*ptr);
}

