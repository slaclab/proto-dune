#ifndef __DPM_TIMING_REGISTERS_H__
#define __DPM_TIMING_REGISTERS_H__

#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <string>
#include <sys/mman.h>
#include <iostream>
 
class DpmTimingRegisters {

      static const uint MemoryBase             = 0xA0000000;
      static const unsigned long MemoryMapSize = 0x00002000;

      int    _devFd;

      void * _memMappedBase;

      void * laneBase (uint idx); 

   public:

      DpmTimingRegisters ();

      ~DpmTimingRegisters ();

      // Open and close
      bool open ( );
      void close ( );

      // Reset control
      void reset ( bool reset );
      bool reset ( );

      // Opcode store enable
      void ocEnable ( bool enable );
      bool ocEnable ( );

      // Opcode register delay
      void ocDelay ( uint delay );
      uint ocDelay ( );

      // Opcode sync Count
      uint ocIdleCount ( );

      // Opcode error Count
      uint ocErrorCount ( );

      // Read an opcode entry
      bool ocEntry ( uint *code );

      // Clock counter
      uint clkCount ( );

};

#endif
