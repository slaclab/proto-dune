#ifndef __PGP_SLAVE_H__
#define __PGP_SLAVE_H__

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
 
class PgpSlave {

      struct sockaddr_in locAddr;
      struct sockaddr_in remAddr;
      int                sockRx;
      int                sockTx;

   public:

      PgpSlave ();

      ~PgpSlave ();

      // Open and close
      bool open ( uint myPort, uint theirPort, const char *bAddr );
      void close ( );

      // Send a lane status message
      void sendStatus ( PgpLaneStatus *status, uint count );

      // Receive an RCE control message if ready
      bool receiveControl ( PgpRceControl *control, struct timeval *timeout );

};

#endif

