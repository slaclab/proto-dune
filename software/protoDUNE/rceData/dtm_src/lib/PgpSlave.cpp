#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <iostream>
#include "PgpSlave.h"
 
PgpSlave::PgpSlave () {
   sockRx = -1;
   sockTx = -1;
}

PgpSlave::~PgpSlave () {
   this->close();
}

// Open and close
bool PgpSlave::open ( uint myPort, uint theirPort, const char *bAddr ) {
   int iOptVal;
   int iOptLen;

   // Open Server
  
   if ( (sockRx = socket(AF_INET, SOCK_DGRAM, 0)) == -1 ) {
      sockRx = -1;
      printf("PgpSlave::open -> Failed to create receive socket\n");
      return(false);
   }

   locAddr.sin_family      = AF_INET;
   locAddr.sin_addr.s_addr = INADDR_ANY;
   locAddr.sin_port        = htons(myPort);
   memset(&(locAddr.sin_zero), '\0', 8);

	if (bind(sockRx, (struct sockaddr *)&locAddr, sizeof(struct sockaddr)) == -1) {
      ::close(sockRx);
      sockRx = -1;
      printf("PgpSlave::open -> Failed to bind receive socket\n");
      return(false);
	}

   // Open Client

   if ( (sockTx = socket(AF_INET, SOCK_DGRAM, 0)) == -1 ) {
      printf("PgpSlave::open -> Failed to create transmit socket\n");
      ::close(sockRx);
      sockRx = -1;
      sockTx = -1;
      return(false);
   }

   remAddr.sin_family      = AF_INET;
   remAddr.sin_addr.s_addr = inet_addr(bAddr);
   remAddr.sin_port        = htons(theirPort);

   // Set socket to use MAC-level broadcast
   iOptVal = 1;
   iOptLen = sizeof(int);
   setsockopt(sockTx, SOL_SOCKET, SO_BROADCAST, (char*)&iOptVal, iOptLen);

   return(true);
}

void PgpSlave::close ( ) {
   if ( sockRx > 0 ) {
      ::close(sockRx);
      sockRx = -1;
   }
   if ( sockTx > 0 ) {
      ::close(sockTx);
      sockTx = -1;
   }
}

// Send a lane status message
void PgpSlave::sendStatus ( PgpLaneStatus *status, uint count ) {
   int  ret;
   uint x;

   for ( x=0; x < count; x++ ) {
      status[x].marker = PGP_STATUS_MARKER;
      ret = sendto(sockTx, &(status[x]), sizeof(PgpLaneStatus), 0, (struct sockaddr *)&remAddr, sizeof(remAddr));
      if ( ret < 0 ) printf("PgpSlave::sendStatus -> Failed to send status message\n");
      usleep(1000);
   }
}

// Receive an RCE control message if ready
bool PgpSlave::receiveControl ( PgpRceControl *control, struct timeval *timeout ) {
   struct sockaddr_storage addr;;
   socklen_t               addrLen;
   fd_set                  fdSet;
   int                     ret;
   bool                    status;
   PgpRceControl           newControl;

   if ( sockRx < 0 ) return (false);

   FD_ZERO(&fdSet);
   FD_SET(sockRx,&fdSet);

   ret = select(sockRx+1, &fdSet, NULL, NULL, timeout );
   status = false;

   if ( ret > 0 && FD_ISSET(sockRx,&fdSet) ) {
      addrLen = sizeof (addr);

      ret = recvfrom(sockRx, &newControl, sizeof(PgpRceControl), 0, (struct sockaddr *)&addr, &addrLen);

      if ( ret == sizeof(PgpRceControl) && newControl.marker == PGP_CONTROL_MARKER ) {
         if ( control->bay     == newControl.bay     && control->element == newControl.element &&
              control->cluster == newControl.cluster && strcmp(control->groupName,newControl.groupName) == 0 ) {
            memcpy(control,&newControl,sizeof(PgpRceControl));
            status = true;
         }
      } else printf("PgpSlave::receiveControl -> Bad message size %i or marker %x received\n",ret,newControl.marker);
   }

   return (status);
}

