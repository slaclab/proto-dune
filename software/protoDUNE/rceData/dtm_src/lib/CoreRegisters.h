#ifndef __CORE_REGISTERS_H__
#define __CORE_REGISTERS_H__

#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <string>
#include <sys/mman.h>
#include <iostream>

class CoreRegisters {

      static const uint MemoryBase             = 0x80000000;
      static const unsigned long MemoryMapSize = 0x00002000;

      int    _devFd;

      void * _memMappedBase;

      char _buildString[256];

   public:

      CoreRegisters ();

      ~CoreRegisters ();

      // Open and close
      bool open ( );
      void close ( );

      // FPGA Version
      uint fpgaVersion ( );

      // Scratchpad
      uint scratchPad ( );
      void scratchPad (uint value);

      // RCE Version
      uint rceVersion ( );

      // Ref Clock Select
      uint refClockSelect (uint idx);
      void refClockSelect (uint idx, uint value);

      // Build String
      const char * buildString ( );

};

#endif
