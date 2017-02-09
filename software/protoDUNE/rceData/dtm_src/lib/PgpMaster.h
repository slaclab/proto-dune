#ifndef __PGP_MASTER_H__
#define __PGP_MASTER_H__

#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <string>
#include <sys/mman.h>
#include <iostream>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "PgpData.h"
 
class PgpMaster {

      static const uint BayCount  = 4;
      static const uint RceCount  = 2;
      static const uint LaneCount = 12;
      static const uint DtmCount  = 1;

      struct sockaddr_in locAddr;
      struct sockaddr_in remAddr;
      int                sockRx;
      int                sockTx;
      PgpLaneStatus *    locStatus[BayCount*RceCount*LaneCount+DtmCount];

   public:

      PgpMaster ();

      ~PgpMaster ();

      // Open and close
      bool open ( uint myPort, uint theirPort, const char *bAddr );
      void close ( );

      // Get number of slave elements
      uint getCount();

      // Send a lane status message
      void sendControl ( PgpRceControl *control, bool *txEnable, bool *rxEnable );

      // Receive an RCE control message if ready
      bool receiveStatus ( PgpRceControl *control, struct timeval *timeout );

      // Get Status record
      PgpLaneStatus * getStatus ( uint idx );

      static void setEnables ( char *str, bool *enables, uint count );
};

#endif

