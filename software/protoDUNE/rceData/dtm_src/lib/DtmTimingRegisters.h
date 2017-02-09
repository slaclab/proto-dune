#ifndef __DTM_TIMING_REGISTERS_H__
#define __DTM_TIMING_REGISTERS_H__

#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <string>
#include <sys/mman.h>
#include <iostream>
 
class DtmTimingRegisters {

      static const uint MemoryBase             = 0xA0000000;
      static const unsigned long MemoryMapSize = 0x00002000;

      int    _devFd;

      void * _memMappedBase;

      void * laneBase (uint idx); 

   public:

      DtmTimingRegisters ();

      ~DtmTimingRegisters ();

      // Open and close
      bool open ( );
      void close ( );

      // Feedback store enable
      void fbEnable ( uint channel, bool enable );
      bool fbEnable ( uint channel );

      // Feedback register delay
      void fbDelay ( uint channel, uint delay );
      uint fbDelay ( uint channel );

      // Feedback sync Count
      uint fbIdleCount ( uint channel );

      // Feedback error Count
      uint fbErrorCount ( uint channel );

      // Read a feedback entry
      bool fbEntry ( uint channel, uint *code );

      // Opcode write
      void ocWrite ( uint code );

      // Read an opcode entry
      bool ocEntry ( uint *code );

      // Opcode store enable
      void ocEnable ( bool enable );
      bool ocEnable ( );

      // Clock counter
      uint clkCount ( );

};

#endif
