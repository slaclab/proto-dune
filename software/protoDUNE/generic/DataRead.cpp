//-----------------------------------------------------------------------------
// File          : DataRead.cpp
// Author        : Ryan Herbst  <rherbst@slac.stanford.edu>
// Created       : 04/12/2011
// Project       : General Purpose
//-----------------------------------------------------------------------------
// Description :
// Read data & configuration from disk
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
// 04/12/2011: created
//-----------------------------------------------------------------------------

#include <DataRead.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <iostream>
#include <iomanip>
#include <stdint.h>
using namespace std;

#ifdef RTEMS
#define O_LARGEFILE 0
#endif

// Constructor
DataRead::DataRead ( ) {
   fd_          = -1;
   size_        = 0;
   sawRunStart_ = false;
   sawRunStop_  = false;
   sawRunTime_  = false;
   rdAddr_      = 0;
   rdCount_     = 0;
   smem_        = NULL;
}

// Deconstructor
DataRead::~DataRead ( ) { }

// Process xml
void DataRead::xmlParse ( uint32_t size, char *data ) {
   char         *buff;
   uint32_t      mySize;
   uint32_t      myType;

   // Decode size
   myType = (size >> 28) & 0xF;
   mySize = (size & 0x0FFFFFFF);

   //cout << "Found Marker: Type=" << dec << myType << ", Size=" << dec << mySize << endl;

   // Read file
   buff = (char *) malloc(mySize+1);
   if ( data != NULL ) memcpy(buff,data,mySize);
   else if ( ::read(fd_, buff, mySize) != (int32_t)mySize) {
      cout << "DataRead::xmlParse -> Read error!" << endl;
      return;
   }
   buff[mySize-1] = 0;

   if ( myType == Data::XmlConfig   ) config_.parse("config",buff);
   if ( myType == Data::XmlStatus   ) status_.parse("status",buff);
   if ( myType == Data::XmlRunStart ) {
      //cout << "-----------XML Start---------------" << endl;
      //cout << buff << endl;
      //cout << "-----------------------------------" << endl;
      start_.parse("runStart",buff);
   }
   if ( myType == Data::XmlRunStop  ) {
      //cout << "-----------XML Stop----------------" << endl;
      //cout << buff << endl;
      //cout << "-----------------------------------" << endl;
      stop_.parse("runStop",buff);
   }
   if ( myType == Data::XmlRunTime  ) {
      //cout << "-----------XML Time----------------" << endl;
      //cout << buff << endl;
      //cout << "-----------------------------------" << endl;
      time_.parse("runTime",buff);
   }

   free(buff);
}

// Open file
bool DataRead::open ( string file, bool compressed ) {
   size_ = 0;
   status_.clear();
   config_.clear();

   // Attempt to open file
   if ( (fd_ = ::open (file.c_str(),O_RDONLY | O_LARGEFILE)) < 0 ) {
      cout << "DataRead::open -> Failed to open file: " << file << endl;
      return(false);
   }
   return(true);
}

// Open file
void DataRead::close () {
   ::close(fd_);
   fd_ = -1;
}

//! Return file size in bytes
off_t DataRead::size ( ) {
   off_t curr;

   if ( fd_ < 0 ) return(0);
   if ( size_ == 0 ) {
      curr  = lseek(fd_, 0, SEEK_CUR);
      size_ = lseek(fd_, 0, SEEK_END);
      lseek(fd_, curr, SEEK_SET);
   }
   return(size_);
}

//! Return file position in bytes
off_t DataRead::pos ( ) {
   if ( fd_ < 0 ) return(0);
   return(lseek(fd_, 0, SEEK_CUR));
}

// Get next data record
bool DataRead::next (Data *data) {
   uint32_t size;
   char *shBuff;
   bool found = false;

   if ( fd_ < 0 && smem_ == NULL ) return(false);

   // Read until we get data
   do { 

      // First read frame size from data file
      if ( smem_ != NULL ) {
         if ( dataSharedRead((DataSharedMemory *)smem_,&rdAddr_,&rdCount_, &size, (uint8_t **)(&shBuff) ) == 0 ) {
            return(false);
         }
      } 
      else {
         if ( read(fd_,&size,4) != 4 ) return(false);
         shBuff = NULL;
      }

      if ( size == 0 ) continue;

      //cout << "Size field = 0x" << hex << size << endl;

      // Frame type
      switch ( (size >> 28) & 0xF ) {
         
         // Data
         case Data::RawData : found = true; break;

         // Configuration
         case Data::XmlConfig : xmlParse(size,shBuff); break;

         // Status
         case Data::XmlStatus : xmlParse(size,shBuff); break;

         // Start
         case Data::XmlRunStart : sawRunStart_ = true; xmlParse(size,shBuff); break;

         // Stop
         case Data::XmlRunStop : sawRunStop_ = true; xmlParse(size,shBuff); break;

         // Time
         case Data::XmlRunTime : sawRunTime_ = true; xmlParse(size,shBuff); break;

         // Unknown
         default: 
            cout << "DataRead::next -> Unknown data type 0x" 
                 << hex << setw(8) << setfill('0') << ((size >> 28) & 0xF) << " skipping." << endl;
            if ( smem_ != NULL ) return(false);   
            else return(lseek(fd_, (size & 0x0FFFFFFF), SEEK_CUR));
            break;
      }
   } while ( ! found );

   // Read data
   if ( smem_ != NULL ) {
      data->copy ( (uint32_t *)shBuff,size );
      return(true);
   }
   else {
      return(data->read(fd_,size));
   }
}

// Get next data record
Data *DataRead::next ( ) {
   Data *tmp = new Data;
   if ( next(tmp) ) return(tmp);
   else {
      delete tmp;
      return(NULL);
   }
}

// Get a config value
string DataRead::getConfig ( string var ) {
   return(config_.get(var));
}

// Get a status value
string DataRead::getStatus ( string var ) {
   return(status_.get(var));
}

// Get a config value
uint32_t DataRead::getConfigInt ( string var ) {
   return(config_.getInt(var));
}

// Get a status value
uint32_t DataRead::getStatusInt ( string var ) {
   return(status_.getInt(var));
}

// Dump config
void DataRead::dumpConfig ( ) {
   cout << "Dumping current config variables:" << endl;
   cout << config_.getList("   Config: ");
}

// Dump status
void DataRead::dumpStatus ( ) {
   cout << "Dumping current status variables:" << endl;
   cout << status_.getList("   Status: ");
}

//! Get config as XML
string DataRead::getConfigXml ( ) {
   string ret;
   ret = "";
   ret.append("<system>\n");
   ret.append("   <config>\n");
   ret.append(config_.getXml());
   ret.append("   </config>\n");
   ret.append("</system>\n");
   return(ret);
}

//! Dump status
string DataRead::getStatusXml ( ) {
   string ret;
   ret = "";
   ret.append("<system>\n");
   ret.append("   <status>\n");
   ret.append(status_.getXml());
   ret.append("   </status>\n");
   ret.append("</system>\n");
   return(ret);
}

// Get a start value
string DataRead::getRunStart ( string var ) {
   return(start_.get(var));
}

// Get a stop value
string DataRead::getRunStop ( string var ) {
   return(stop_.get(var));
}

// Get a time value
string DataRead::getRunTime ( string var ) {
   return(time_.get(var));
}

// Dump start
void DataRead::dumpRunStart ( ) {
   cout << "Dumping run start variables:" << endl;
   cout << start_.getList("   RunStart: ");
}

// Dump stop
void DataRead::dumpRunStop ( ) {
   cout << "Dumping run stop variables:" << endl;
   cout << stop_.getList("    RunStop: ");
}

// Dump time
void DataRead::dumpRunTime ( ) {
   cout << "Dumping run time variables:" << endl;
   cout << time_.getList("    RunTime: ");
}

// Return true if we saw start marker, self clearing
bool  DataRead::sawRunStart ( ) {
   bool ret = sawRunStart_;
   sawRunStart_ = false;
   return(ret);
}

// Return true if we saw stop marker, self clearing
bool  DataRead::sawRunStop ( ) {
   bool ret = sawRunStop_;
   sawRunStop_ = false;
   return(ret);
}

// Return true if we saw time marker, self clearing
bool  DataRead::sawRunTime ( ) {
   bool ret = sawRunTime_;
   sawRunTime_ = false;
   return(ret);
}

// Enable shared 
void DataRead::openShared ( string system, uint32_t id, int32_t uid ) {

   // Attempt to open and init shared memory
   if ( (smemFd_ = dataSharedOpenAndMap ( (DataSharedMemory **)(&smem_) , system.c_str(), id, uid )) < 0 ) {
      smem_ = NULL;
      throw string("CommLink::enabledSharedMemory -> Failed to open shared memory");
   }
   rdAddr_  = 0;
   rdCount_ = 0;
}

