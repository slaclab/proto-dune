#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <string>
#include <string.h>
#include <unistd.h>
#include <sys/mman.h>
#include <iostream>
#include <iomanip>
#include "BsiSpace.h"
#include "Bsi_Cfg.h"
 
BsiSpace::BsiSpace () {
   _devFd         = -1;
   _memMappedBase = NULL;
}

BsiSpace::~BsiSpace () {
   this->close();
}

// Open the port
bool BsiSpace::open () {

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
void BsiSpace::close () {
   if ( _devFd > 0 ) {
      ::close(_devFd);
      _devFd = -1;
   }
}

// Serial Number
unsigned long BsiSpace::serial () {
   if ( _devFd < 0 ) return(0);

   unsigned long * ptr = (unsigned long *)(_memMappedBase + (BSI_CMB_SERIAL_NUMBER_OFFSET*4));

   return(*ptr);
}

// Cluster
uint BsiSpace::cluster () {
   if ( _devFd < 0 ) return(0);

   uint * ptr = (uint *)(_memMappedBase);
   return(BSI_CLUSTER_FROM_CLUSTER_ADDR(ptr[BSI_CLUSTER_ADDR_OFFSET]));
}

// Bay 
uint BsiSpace::bay () {
   if ( _devFd < 0 ) return(0);

   uint * ptr = (uint *)(_memMappedBase);
   return(BSI_BAY_FROM_CLUSTER_ADDR(ptr[BSI_CLUSTER_ADDR_OFFSET]));
}

// Element
uint BsiSpace::element() {
   if ( _devFd < 0 ) return(0);

   uint * ptr = (uint *)(_memMappedBase);
   return(BSI_ELEMENT_FROM_CLUSTER_ADDR(ptr[BSI_CLUSTER_ADDR_OFFSET]));
}

// Shelf
const char * BsiSpace::groupName() {
   if ( _devFd < 0 ) return(0);

   const char * ptr = (const char *)(_memMappedBase + (BSI_CLUSTER_GROUP_NAME_OFFSET*4));

   if ( strlen(ptr) < 4 ) strcpy(_groupName,"");
   else {
      strncpy(_groupName,ptr,BsiGroupNameSize);
      _groupName[BsiGroupNameSize-1] = 0;
   }
      
   return(_groupName);
}

