#ifndef __BSI_SPACE_H__
#define __BSI_SPACE_H__

#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <string>
#include <sys/mman.h>
#include <iostream>
#include <iostream>
#include "Bsi_Cfg.h"
 
class BsiSpace {

      static const uint MemoryBase             = 0x84000000;
      static const unsigned long MemoryMapSize = 0x00002000;

      int    _devFd;

      void * _memMappedBase;

      char  _groupName[BSI_GROUP_NAME_SIZE+1];

   public:

      static const uint BsiGroupNameSize = BSI_GROUP_NAME_SIZE;

      BsiSpace ();

      ~BsiSpace ();

      // Open and close
      bool open ( );
      void close ( );

      // Serial Number
      unsigned long serial ();

      // Cluster
      uint cluster ();

      // Bay 
      uint bay ();

      // Rce
      uint element();

      // Shelf
      const char* groupName();
};

#endif
