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
#include "PgpMaster.h"
 
PgpMaster::PgpMaster () {
   uint x,y,z;

   sockRx = -1;
   sockTx = -1;

   // Init status records
   for (x=0; x < BayCount; x++) {
      for (y=0; y < RceCount; y++) {
         for (z=0; z < LaneCount; z++) {
            locStatus[(x*RceCount*LaneCount)+(y*LaneCount+z)] = 
               (PgpLaneStatus *)(malloc(sizeof(PgpLaneStatus)));
            memset(locStatus[(x*RceCount*LaneCount)+(y*LaneCount)+z],0,sizeof(PgpLaneStatus));
            locStatus[(x*RceCount*LaneCount)+(y*LaneCount)+z]->bay     = x;
            locStatus[(x*RceCount*LaneCount)+(y*LaneCount)+z]->element = y*2; // Rce0, Rce2
            locStatus[(x*RceCount*LaneCount)+(y*LaneCount)+z]->lane    = z;
         }
      }
   }

   // Extra DTM Channels
   for (x=0; x < DtmCount; x++) {
      locStatus[(BayCount*RceCount*LaneCount)+x] = (PgpLaneStatus *)(malloc(sizeof(PgpLaneStatus)));
      memset(locStatus[(BayCount*RceCount*LaneCount)+x],0,sizeof(PgpLaneStatus));
      locStatus[(BayCount*RceCount*LaneCount)+x]->bay     = 4;
      locStatus[(BayCount*RceCount*LaneCount)+x]->element = 0;
      locStatus[(BayCount*RceCount*LaneCount)+x]->lane    = x;
   }
}

PgpMaster::~PgpMaster () {
   uint x;

   this->close();
   for (x=0; x < getCount(); x++) free(locStatus[x]);
}

// Open and close
bool PgpMaster::open ( uint myPort, uint theirPort, const char *bAddr ) {
   int iOptVal;
   int iOptLen;

   // Open Server
  
   if ( (sockRx = socket(AF_INET, SOCK_DGRAM, 0)) == -1 ) {
      sockRx = -1;
      printf("PgpMaster::open -> Failed to create receive socket\n");
      return(false);
   }

   locAddr.sin_family      = AF_INET;
   locAddr.sin_addr.s_addr = INADDR_ANY;
   locAddr.sin_port        = htons(myPort);
   memset(&(locAddr.sin_zero), '\0', 8);

	if (bind(sockRx, (struct sockaddr *)&locAddr, sizeof(struct sockaddr)) == -1) {
      printf("PgpMaster::open -> Failed to bind receive socket\n");
      ::close(sockRx);
      sockRx = -1;
      return(false);
	}

   // Open Client

   if ( (sockTx = socket(AF_INET, SOCK_DGRAM, 0)) == -1 ) {
      printf("PgpMaster::open -> Failed to create transmit socket\n");
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

void PgpMaster::close ( ) {
   if ( sockRx > 0 ) {
      ::close(sockRx);
      sockRx = -1;
   }
   if ( sockTx > 0 ) {
      ::close(sockTx);
      sockTx = -1;
   }
}

uint PgpMaster::getCount() { 
   return((BayCount*RceCount*LaneCount)+DtmCount); 
}

// Send a lane status message
void PgpMaster::sendControl ( PgpRceControl *control, bool *txEnable, bool *rxEnable ) {
   uint x, y, z;
   int  ret;

   for ( x=0; x < BayCount; x++ ) {
      for ( y=0; y < RceCount; y++ ) {
         control->bay     = x;
         control->element = y*2; // RCE0, RCE2
         control->marker  = PGP_CONTROL_MARKER;

         for ( z=0; z < LaneCount; z++ ) {
            control->pgpTxEnable[z] = txEnable[(x*RceCount*LaneCount)+(y*LaneCount)+z];
            control->pgpRxEnable[z] = rxEnable[(x*RceCount*LaneCount)+(y*LaneCount)+z];
         }

         ret = sendto(sockTx, control, sizeof(PgpRceControl), 0, (struct sockaddr *)&remAddr, sizeof(remAddr));
         if ( ret < 0 ) printf("PgpMaster::sendControl -> Failed to send control message\n");
         usleep(100);
      }
   }

   // DTM Channels
   control->bay     = 4;
   control->element = 0;
   control->marker  = PGP_CONTROL_MARKER;

   for (x=0; x < DtmCount; x++) {
      control->pgpTxEnable[x] = txEnable[(BayCount*RceCount*LaneCount)+x];
      control->pgpRxEnable[x] = rxEnable[(BayCount*RceCount*LaneCount)+x];
   }
   for (x=DtmCount; x < LaneCount; x++) {
      control->pgpTxEnable[x] = 0;
      control->pgpRxEnable[x] = 0;
   }

   ret = sendto(sockTx, control, sizeof(PgpRceControl), 0, (struct sockaddr *)&remAddr, sizeof(remAddr));
   if ( ret < 0 ) printf("PgpMaster::sendControl -> Failed to send control message\n");
   usleep(100);
}

// Receive an RCE control message if ready
bool PgpMaster::receiveStatus ( PgpRceControl *control, struct timeval *timeout ) {
   struct sockaddr_storage addr;;
   socklen_t               addrLen;
   fd_set                  fdSet;
   int                     ret;
   uint                    x;
   bool                    stat;
   PgpLaneStatus           newStatus;

   if ( sockRx < 0 ) return (false);

   FD_ZERO(&fdSet);
   FD_SET(sockRx,&fdSet);

   ret = select(sockRx+1, &fdSet, NULL, NULL, timeout );
   stat = false;

   if ( ret > 0 && FD_ISSET(sockRx,&fdSet) ) {
      addrLen = sizeof (addr);

      memset(&newStatus,0,sizeof(PgpLaneStatus));
      ret = recvfrom(sockRx, &newStatus, sizeof(PgpLaneStatus), 0, (struct sockaddr *)&addr, &addrLen);

      if ( ret == sizeof(PgpLaneStatus) && newStatus.marker == PGP_STATUS_MARKER ) {
         if ( newStatus.cluster == control->cluster && strcmp(newStatus.groupName,control->groupName) == 0 ) {
            for (x=0; x < getCount(); x++) {
               if ( newStatus.bay     == locStatus[x]->bay &&
                    newStatus.element == locStatus[x]->element &&
                    newStatus.lane    == locStatus[x]->lane ) {
                  memcpy(locStatus[x],&newStatus,sizeof(PgpLaneStatus));
                  stat = true;
                  break;
               }
            }
         }
      } 
   }

   return (stat);
}

// Get status record
PgpLaneStatus * PgpMaster::getStatus ( uint idx ) {

   if ( idx > getCount() ) return(NULL);

   return(locStatus[idx]);
}

void PgpMaster::setEnables ( char *str, bool *enables, uint count ) {
   int  start;
   int  stop;
   int  x;
   char *sub;

   start = -1;
   stop  = -1;

   sub = strtok(str,"-");
   start = atoi(sub);
   stop = start;
   sub = strtok(NULL,"-");
   if ( sub != NULL ) stop = atoi(sub);

   for (x=start; x<= stop; x++) { 
      if ( x < (int)count ) enables[x] = true;
   }
}

