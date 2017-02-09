#ifndef __HPS_DTM_REGISTERS_H__
#define __HPS_DTM_REGISTERS_H__

#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <string>
#include <sys/mman.h>
#include <iostream>
 
class HpsDtmRegisters {

      static const uint MemoryBase             = 0xA0010000;
      static const unsigned long MemoryMapSize = 0x00002000;

      int    _devFd;

      void * _memMappedBase;

      void * laneBase (uint idx); 

   public:

      HpsDtmRegisters ();

      ~HpsDtmRegisters ();

      // Open and close
      bool open ( );
      void close ( );

      // Trigger Enable
      void trigEnable ( bool enable );
      bool trigEnable ( );

      // Trigger Period
      void trigPeriod ( uint period );
      uint trigPeriod ( );

      // Software Trigger
      void swTrigger  ( );

      // Ack Enable
      void ackEnable ( uint channel, bool enable );
      bool ackEnable ( uint channel );

      // Trigger count
      uint trigCount ( );

      // Trigger Buffer Size
      void trigBuffSize ( uint buffSize );
      uint trigBuffSize ( );

      // Reset counters
      void countReset();

      // Ack Count
      uint ackCount (uint channel);
};

#endif
