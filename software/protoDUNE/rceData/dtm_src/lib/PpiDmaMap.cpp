#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <fcntl.h>
#include <string>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <iostream>
#include <iomanip>
#include <queue>
#include <sys/stat.h>

#include "PpiDmaMap.h"
using namespace std;

PpiDmaMap::PpiDmaMap (uint idx) {
   _channel    = idx;
   
   int fd;

   fd = ::open("/dev/mem", O_RDWR | O_SYNC);

   if (fd == -1) {
      _regMap = NULL;
      _ocmMap = NULL;
   }
   else {
      _regMap = (volatile char *)mmap(0, RegMemoryMapSize, PROT_EXEC | PROT_READ | PROT_WRITE, MAP_SHARED, fd, RegMemoryBase);
      if (_regMap == (void *) -1) _regMap = NULL;
      _ocmMap = (volatile char *)mmap(0, OcmMemoryMapSize, PROT_EXEC | PROT_READ | PROT_WRITE, MAP_SHARED, fd, OcmMemoryBase);
      if (_ocmMap == (void *) -1) _ocmMap = NULL;
   }

   _baseAddr[0] = 0x50000000;
   _baseAddr[1] = 0x50020000;
   _baseAddr[2] = 0x50040000;
   _baseAddr[3] = 0x50060000;

   printf("Write to ob free list : 0x%08x\n",_obHdrAddr);
   regWrite(_baseAddr[idx]+_obWork,_obHdrAddr);
   printf("Write to ib free list : 0x%08x\n",_ibHdrAddr);
   regWrite(_baseAddr[idx]+_ibWork,_ibHdrAddr);
}

PpiDmaMap::~PpiDmaMap () { }

// Write a block of data
int PpiDmaMap::write(unsigned char *data, uint hdrSize, uint paySize, uint type) {
   uint addr;
   uint desc;
   uint uintPtr;
   uint ucharPtr;

   addr = regRead(_baseAddr[_channel]+_obFree);

   if ((addr & 0x1) != 0 ) return(0);
   printf("Got free outbound address : 0x%08x\n",addr);

   uintPtr  = addr;
   ucharPtr = addr + 24;

   ocmWrite(uintPtr,addr + 24 + hdrSize);
   ocmWrite(uintPtr+0x4,paySize);
   ocmWrite(uintPtr+0x8,_channel*2);
   ocmWrite(uintPtr+0xc,0x5a5a5a5a);

   ocmWrite(uintPtr+0x10,hdrSize);
   ocmWrite(uintPtr+0x14,paySize);

   ocmBlockWrite(ucharPtr,data,hdrSize+paySize);

   desc = addr | (((hdrSize/8)+3) << 18) | (type << 26);

   if ( paySize > 0 ) desc |= 0xC0000000;
   else desc |= 0x40000000;

   printf("Writing to outbound work\n");
   regWrite(_baseAddr[_channel]+_obWork,desc);

   if ( paySize > 0 ) {
      printf("Waiting for outbound completion\n");
      do {
         desc = regRead(_compFifos+(_channel*8));
         usleep(100);
      } while ( (desc & 0x1) != 0);
      printf("Got write completion : 0x%08x\n",desc);

      desc = regRead(_compFifos+(_channel*8));
      printf("Second write completion read : 0x%08x\n",desc);
   }

   return(hdrSize+paySize);
}

// Read a block of data, return -1 on error, 0 if no data, size if data
int PpiDmaMap::read(unsigned char *data, uint maxSize, uint *type, uint *err, uint *hdrSize, uint *paySize) {
   uint uintPtr;
   uint ucharPtr;
   uint desc;
   uint pay;
   uint addr;

   desc = regRead(_baseAddr[_channel]+_ibPend);
   if ((desc & 0x1) != 0 ) return(0);

   addr  = desc & 0x3FFFF;
   *type = (desc >> 26) & 0xF;
   *err  = (desc >> 30) & 0x1;
   pay   = (desc >> 31) & 0x1;

   uintPtr  = addr;
   ucharPtr = addr + 24;

   *hdrSize = ocmRead(uintPtr+0x10);
   *paySize = ocmRead(uintPtr+0x14);

   printf("Got inbound header Addr: 0x%08x, Type: %i, Err: %i, Pay: %i, HSize: %i, PSize: %i\n",
      addr,*type,*err,pay,*hdrSize,*paySize);
   ocmBlockRead(ucharPtr,data,*hdrSize);

   if ( pay == 0 ) {
      printf("Write to ib free list : 0x%08x\n",addr);
      regWrite(_baseAddr[_channel]+_ibWork,addr);
      return(*hdrSize);
   }

   ocmWrite(uintPtr,addr + 24 + *hdrSize);
   ocmWrite(uintPtr+0x4,*paySize);
   ocmWrite(uintPtr+0x8,(_channel*2)+1);
   ocmWrite(uintPtr+0xc,0x5a5a5a5a);

   printf("Writing to inbound work\n");
   desc = 0x60000000 | addr;
   regWrite(_baseAddr[_channel]+_ibWork,desc);

   printf("Waiting for inbound completion\n");
   do {
      desc = regRead(_compFifos+((_channel*8)+4));
      usleep(100);
   } while ( (desc & 0x1) != 0);
   printf("Got read completion : 0x%08x\n",desc);

   desc = regRead(_compFifos+((_channel*8)+4));
   printf("Second read completion read : 0x%08x\n",desc);

   ocmBlockRead(ucharPtr,data,*hdrSize+*paySize);

   return(*hdrSize+*paySize);
}

// Write to OCM
void PpiDmaMap::ocmBlockWrite ( uint addr, unsigned char *data, uint size) {
   uint diff = addr - OcmMemoryBase;
   uint x;

   for (x=0; x < size; x++) _ocmMap[diff+x] = data[x];
}

// Read from OCM
void PpiDmaMap::ocmBlockRead ( uint addr, unsigned char *data, uint size) {
   uint diff = addr - OcmMemoryBase;
   uint x;

   for (x=0; x < size; x++) data[x] = _ocmMap[diff+x];
}

// Write to OCM
void PpiDmaMap::ocmWrite ( uint addr, uint value) {
   uint diff = addr - OcmMemoryBase;

   volatile uint * ptr = (uint *)(_ocmMap + diff);

   *ptr = value;
}

// Read from OCM
uint PpiDmaMap::ocmRead ( uint addr) {
   uint diff = addr - OcmMemoryBase;

   volatile uint * ptr = (uint *)(_ocmMap + diff);

   return(*ptr);
}

// Write to register
void PpiDmaMap::regWrite (uint addr, uint value) {
   uint diff = addr - RegMemoryBase;

   volatile uint * ptr = (uint *)(_regMap + diff);

   *ptr = value;
}

// Read from register
uint PpiDmaMap::regRead (uint addr) {
   uint diff = addr - RegMemoryBase;

   volatile uint * ptr = (uint *)(_regMap + diff);

   return(*ptr);
}

