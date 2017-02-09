#ifndef __PGP_REGISTERS_H__
#define __PGP_REGISTERS_H__

#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <string>
#include <sys/mman.h>
#include <iostream>
 
class PgpRegisters {

      static const uint MemoryBase             = 0xA0010000;
      static const unsigned long MemoryMapSize = 0x00002000;

      int    _devFd;

      void * _memMappedBase;

      void * laneBase (uint idx); 

   public:

      PgpRegisters ();

      ~PgpRegisters ();

      // Open and close
      bool open ( );
      void close ( );

      // Link Ready
      bool linkReady (uint idx);

      // Cell Error Count
      uint cellErrorCount (uint idx);

      // Link Down Count
      uint linkDownCount (uint idx);

      // Link Error Count
      uint linkErrorCount (uint idx);

      // TX Count
      uint txCount (uint idx);

      // RX Count
      uint rxCount (uint idx);

      // EOFE Count
      uint eofeCount (uint idx);

      // Count 
      void countReset (bool set);
      bool countReset ();

      // Loop Enable
      void loopEnable (bool set);
      bool loopEnable ();

      // Flow control disable
      void flowDis (bool set);
      bool flowDis ();

      // Clock Count
      uint clockCount ();

      // Status
      uint status ();

      // Pll Reset
      void pllReset ( bool set);
      bool pllReset ();

      // Rx Reset
      void pgpRxReset (uint idx, bool set);
      bool pgpRxReset (uint idx);

      // Tx Reset
      void pgpTxReset (uint idx, bool set);
      bool pgpTxReset (uint idx);

      // Reset all
      void pgpResetAll ();

};

#endif
