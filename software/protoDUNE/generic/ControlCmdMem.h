//-----------------------------------------------------------------------------
// File          : ControlCmdMem.h
// Author        : Ryan Herbst  <rherbst@slac.stanford.edu>
// Created       : 01/11/2012
// Project       : General Purpose
//-----------------------------------------------------------------------------
// Description :
// Interface Server
//-----------------------------------------------------------------------------
// This file is part of 'SLAC Generic DAQ Software'.
// It is subject to the license terms in the LICENSE.txt file found in the 
// top-level directory of this distribution and at: 
//    https://confluence.slac.stanford.edu/display/ppareg/LICENSE.html. 
// No part of 'SLAC Generic DAQ Software', including this file, 
// may be copied, modified, propagated, or distributed except according to 
// the terms contained in the LICENSE.txt file.
// Proprietary and confidential to SLAC.
//-----------------------------------------------------------------------------
// Modification history :
// 01/11/2012: created
//-----------------------------------------------------------------------------
#ifndef __CONTROL_CMD_MEM_H__
#define __CONTROL_CMD_MEM_H__

#ifndef RTEMS
#include <sys/mman.h>
#endif

#include <sys/stat.h>
#include <fcntl.h>   
#include <string.h>   
#include <unistd.h>
#include <stdio.h>
#include <time.h>
#include <sys/time.h>
#include <stdint.h>

// Sizes
#define CONTROL_CMD_STR_SIZE  1024
#define CONTROL_CMD_XML_SIZE  1048576
#define CONTROL_CMD_NAME_SIZE 200

// Specific Sizes
#define CONTROL_CMD_ARGA_SIZE   CONTROL_CMD_XML_SIZE
#define CONTROL_CMD_ARGB_SIZE   CONTROL_CMD_STR_SIZE
#define CONTROL_CMD_RESULT_SIZE CONTROL_CMD_STR_SIZE
#define CONTROL_CMD_ERROR_SIZE  CONTROL_CMD_STR_SIZE
#define CONTROL_CMD_STATUS_SIZE CONTROL_CMD_XML_SIZE
#define CONTROL_CMD_CONFIG_SIZE CONTROL_CMD_XML_SIZE

// Command Constants
#define CONTROL_CMD_TYPE_SEND_XML      1 // One arg,  XML string, No Result
#define CONTROL_CMD_TYPE_SET_CONFIG    2 // Two args, Config Variable and String Value, No Result
#define CONTROL_CMD_TYPE_GET_CONFIG    3 // One Arg, Config Variable, Result is Value
#define CONTROL_CMD_TYPE_GET_STATUS    4 // One Arg, Status Variable, Result is Value
#define CONTROL_CMD_TYPE_EXEC_COMMAND  5 // Two Args, Command and arg, No Result
#define CONTROL_CMD_TYPE_SET_REGISTER  6 // Three Args, Device Path, register name and value, No Result
#define CONTROL_CMD_TYPE_GET_REGISTER  7 // Two Args, Device Path, register name, Result is value

typedef struct {

   // Commands
   uint8_t      cmdRdyCount;
   uint8_t      cmdAckCount;
   uint8_t      cmdType;
   char         cmdArgA[CONTROL_CMD_ARGA_SIZE];
   char         cmdArgB[CONTROL_CMD_ARGB_SIZE];
   char         cmdResult[CONTROL_CMD_RESULT_SIZE];

   // Error, Config and Status 
   char         errorBuffer[CONTROL_CMD_ERROR_SIZE];
   char         xmlStatusBuffer[CONTROL_CMD_STATUS_SIZE];
   char         xmlConfigBuffer[CONTROL_CMD_CONFIG_SIZE];
   char         xmlPerStatusBuffer[CONTROL_CMD_STATUS_SIZE];

   // Shared name
   char         sharedName[CONTROL_CMD_NAME_SIZE];

} ControlCmdMemory;


#ifndef RTEMS

// Open and map shared memory
inline int32_t controlCmdOpenAndMap ( ControlCmdMemory **ptr, const char *system, uint32_t id, int32_t uid=-1 ) {
   int32_t  smemFd;
   char     shmName[CONTROL_CMD_NAME_SIZE];
   int32_t  lid;

   // ID to use?
   if ( uid == -1 ) lid = getuid();
   else lid = uid;

   // Generate shared memory
   sprintf(shmName,"control_cmd.%i.%s.%i",lid,system,id);

   // Attempt to open existing shared memory
   if ( (smemFd = shm_open(shmName, O_RDWR, (S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH)) ) < 0 ) {

      // Otherwise open and create shared memory
      if ( (smemFd = shm_open(shmName, (O_CREAT | O_RDWR), (S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH)) ) < 0 ) return(-1);

      // Force permissions regardless of umask
      fchmod(smemFd, (S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH));
    
      // Set the size of the shared memory segment
      ftruncate(smemFd, sizeof(ControlCmdMemory));
   }

   // Map the shared memory
   if((*ptr = (ControlCmdMemory *)mmap(0, sizeof(ControlCmdMemory),
              (PROT_READ | PROT_WRITE), MAP_SHARED, smemFd, 0)) == MAP_FAILED) return(-2);

   // Store name
   strncpy((*ptr)->sharedName,shmName,CONTROL_CMD_NAME_SIZE);
   (*ptr)->sharedName[CONTROL_CMD_NAME_SIZE-1] = '\0';

   return(smemFd);
}

// Close shared memory
inline void controlCmdClose ( ControlCmdMemory *ptr ) {
   char shmName[200];

   // Get shared name
   strcpy(shmName,ptr->sharedName);

   // Unlink
   shm_unlink(shmName);
}

// Init data structure, called by ControlServer
inline void controlCmdInit ( ControlCmdMemory *ptr ) {
   memset(ptr->cmdArgA, 0, CONTROL_CMD_ARGA_SIZE);
   memset(ptr->cmdArgB, 0, CONTROL_CMD_ARGB_SIZE);
   memset(ptr->cmdResult, 0, CONTROL_CMD_RESULT_SIZE);
   memset(ptr->errorBuffer, 0, CONTROL_CMD_ERROR_SIZE);
   memset(ptr->xmlStatusBuffer, 0, CONTROL_CMD_STATUS_SIZE);
   memset(ptr->xmlConfigBuffer, 0, CONTROL_CMD_CONFIG_SIZE);
   memset(ptr->xmlPerStatusBuffer, 0, CONTROL_CMD_STATUS_SIZE);

   ptr->cmdType     = 0;
   ptr->cmdRdyCount = 0;
   ptr->cmdAckCount = 0;
}

// Send command
inline void controlCmdSetCommand ( ControlCmdMemory *ptr, uint8_t cmdType, const char *argA, const char *argB ) {
   if ( argA != NULL ) {
      strncpy(ptr->cmdArgA,argA,CONTROL_CMD_ARGA_SIZE);
      ptr->cmdArgA[CONTROL_CMD_ARGA_SIZE-1] = '\0';
   }
   if ( argB != NULL ) {
      strncpy(ptr->cmdArgB,argB,CONTROL_CMD_ARGB_SIZE);
      ptr->cmdArgB[CONTROL_CMD_ARGB_SIZE-1] = '\0';
   }
   strcpy(ptr->errorBuffer,"");

   ptr->cmdType = cmdType;
 
   ptr->cmdRdyCount++;
}

// Check for pending command
inline int32_t controlCmdGetCommand ( ControlCmdMemory *ptr, uint8_t *cmdType, char **argA, char **argB ) {
   if ( ptr->cmdRdyCount == ptr->cmdAckCount ) return(0);
   else {
      ptr->cmdArgA[CONTROL_CMD_ARGA_SIZE-1] = '\0';
      ptr->cmdArgB[CONTROL_CMD_ARGB_SIZE-1] = '\0';

      *cmdType = ptr->cmdType;
      *argA    = ptr->cmdArgA;
      *argB    = ptr->cmdArgB;
      return(1);
   }
}

// Command Set Result
inline void controlCmdSetResult ( ControlCmdMemory *ptr, const char *result ) {
   if ( result != NULL ) {
      strncpy(ptr->cmdResult,result,CONTROL_CMD_RESULT_SIZE);
      ptr->cmdResult[CONTROL_CMD_RESULT_SIZE-1] = '\0';
   }
}

// Command ack
inline void controlCmdAckCommand ( ControlCmdMemory *ptr ) {
   ptr->cmdAckCount = ptr->cmdRdyCount;
}

// Wait for command completion
inline int32_t controlCmdGetResult ( ControlCmdMemory *ptr, char *result ) {
   if ( ptr->cmdRdyCount != ptr->cmdAckCount ) return(0);
   else { 
      if ( result != NULL ) {
         ptr->cmdResult[CONTROL_CMD_RESULT_SIZE-1] = '\0';
         strncpy(result,ptr->cmdResult,CONTROL_CMD_RESULT_SIZE);
      }
      return(1);
   }
}

// Wait for command completion with timeout in milliseconds
inline int32_t controlCmdGetResultTimeout ( ControlCmdMemory *ptr, char *result, int32_t timeout ) {
   struct timeval now;
   struct timeval sum;
   struct timeval add;

   gettimeofday(&now,NULL);
   add.tv_sec =  (timeout / 1000);
   add.tv_usec = (timeout % 1000);
   timeradd(&now,&add,&sum);

   while ( ! controlCmdGetResult(ptr,result) ) {
      gettimeofday(&now,NULL);
      if ( timercmp(&now,&sum,>) ) return(0);
      usleep(1000); // Wait 1 millisecond
   }
   return(1);
}

// Set Config
inline void controlCmdSetConfig ( ControlCmdMemory *ptr, const char *config ) {
   strncpy(ptr->xmlConfigBuffer,config,CONTROL_CMD_CONFIG_SIZE);
   ptr->xmlConfigBuffer[CONTROL_CMD_CONFIG_SIZE-1] = '\0';
}

// Get Config
inline const char * controlCmdGetConfig ( ControlCmdMemory *ptr ) {
   ptr->xmlConfigBuffer[CONTROL_CMD_CONFIG_SIZE-1] = '\0';
   return(ptr->xmlConfigBuffer);
}

// Set Status
inline void controlCmdSetStatus ( ControlCmdMemory *ptr, const char *status ) {
   strncpy(ptr->xmlStatusBuffer,status,CONTROL_CMD_STATUS_SIZE);
   ptr->xmlStatusBuffer[CONTROL_CMD_STATUS_SIZE-1] = '\0';
}

// Get Status
inline const char * controlCmdGetStatus ( ControlCmdMemory *ptr ) {
   ptr->xmlStatusBuffer[CONTROL_CMD_STATUS_SIZE-1] = '\0';
   return(ptr->xmlStatusBuffer);
}

inline void controlCmdSetPerStatus ( ControlCmdMemory *ptr, const char *status ) {
   strncpy(ptr->xmlPerStatusBuffer,status,CONTROL_CMD_STATUS_SIZE);
   ptr->xmlPerStatusBuffer[CONTROL_CMD_STATUS_SIZE-1] = '\0';
}

// Get Status
inline const char * controlCmdGetPerStatus ( ControlCmdMemory *ptr ) {
   ptr->xmlPerStatusBuffer[CONTROL_CMD_STATUS_SIZE-1] = '\0';
   return(ptr->xmlPerStatusBuffer);
}

// Set Error 
inline void controlCmdSetError ( ControlCmdMemory *ptr, const char *error ) {
   strncpy(ptr->errorBuffer,error,CONTROL_CMD_ERROR_SIZE);
   ptr->errorBuffer[CONTROL_CMD_ERROR_SIZE-1] = '\0';
}

// Get Error
inline const char * controlCmdGetError ( ControlCmdMemory *ptr ) {
   ptr->errorBuffer[CONTROL_CMD_ERROR_SIZE-1] = '\0';
   return(ptr->errorBuffer);
}

#else 

inline int32_t controlCmdOpenAndMap ( ControlCmdMemory **ptr, const char *system, uint32_t id, int32_t uid=-1 ) { return -1; }
inline void controlCmdClose ( ControlCmdMemory *ptr ) { }
inline void controlCmdInit ( ControlCmdMemory *ptr ) { }
inline void controlCmdSetCommand ( ControlCmdMemory *ptr, uint8_t cmdType, const char *argA, const char *argB ) { }
inline int32_t controlCmdGetCommand ( ControlCmdMemory *ptr, uint8_t *cmdType, char **argA, char **argB ) { return 0; }
inline void controlCmdSetResult ( ControlCmdMemory *ptr, const char *result ) { }
inline void controlCmdAckCommand ( ControlCmdMemory *ptr ) { }
inline int32_t controlCmdGetResult ( ControlCmdMemory *ptr, char *result ) { return 0; }
inline int32_t controlCmdGetResultTimeout ( ControlCmdMemory *ptr, char *result, int32_t timeout ) { return 0; }
inline void controlCmdSetConfig ( ControlCmdMemory *ptr, const char *config ) { }
inline const char * controlCmdGetConfig ( ControlCmdMemory *ptr ) { return NULL; }
inline void controlCmdSetStatus ( ControlCmdMemory *ptr, const char *status ) { }
inline const char * controlCmdGetStatus ( ControlCmdMemory *ptr ) { return NULL; }
inline void controlCmdSetPerStatus ( ControlCmdMemory *ptr, const char *status ) { }
inline const char * controlCmdGetPerStatus ( ControlCmdMemory *ptr ) { return NULL; }
inline void controlCmdSetError ( ControlCmdMemory *ptr, const char *error ) { }
inline const char * controlCmdGetError ( ControlCmdMemory *ptr ) { return NULL; }

#endif
#endif

