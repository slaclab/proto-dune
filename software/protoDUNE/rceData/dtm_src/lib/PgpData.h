#ifndef __PGP_DATA_H__
#define __PGP_DATA_H__
#include "BsiSpace.h"

#define PGP_STATUS_MARKER  0x5A5AA5A5
#define PGP_CONTROL_MARKER 0xA5A55A5A

// Per Lane Status
typedef struct {
   char groupName[BsiSpace::BsiGroupNameSize];
   uint bay;
   uint element;
   uint cluster;
   uint lane;
   uint enabled;
   uint linkReady;
   uint cellErrorCount;
   uint linkDownCount;
   uint linkErrorCount;
   uint transmitCount;
   uint receiveCount;
   uint eofeCount;
   uint fpgaVersion;
   uint clockRate;

   unsigned long serial;

   uint marker;

} PgpLaneStatus;

// Per RCE Control
typedef struct {
   char groupName[BsiSpace::BsiGroupNameSize];
   uint bay;
   uint element;
   uint cluster;
   uint pllReset;
   uint countReset;
   uint refSelect;
   uint loopBack;
   uint pgpRxEnable[12];
   uint pgpTxEnable[12];
   char message[128];
   uint marker;
} PgpRceControl;

#endif

