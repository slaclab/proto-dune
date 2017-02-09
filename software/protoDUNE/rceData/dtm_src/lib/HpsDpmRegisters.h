#ifndef __HPS_DPM_REGISTERS_H__
#define __HPS_DPM_REGISTERS_H__

#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <string>
#include <sys/mman.h>
#include <iostream>
 
class HpsDpmRegisters {

      static const uint MemoryBase             = 0xA0010000;
      static const unsigned long MemoryMapSize = 0x00002000;

      int    _devFd;

      void * _memMappedBase;

      void * laneBase (uint idx); 

   public:

      HpsDpmRegisters ();

      ~HpsDpmRegisters ();

      // Open and close
      bool open ( );
      void close ( );

      // Trigger Enable
      void trigEnable ( bool enable );
      bool trigEnable ( );

      // Data Size
      void dataSize ( uint size );
      uint dataSize ( );

      // Trigger Ack
      void trigAck  ( );
};

#endif
