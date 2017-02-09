//-----------------------------------------------------------------------------
// File          : DataRead.h
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
#ifndef __DATA_READ_H__
#define __DATA_READ_H__

#include <string>
#include <map>
#include <Data.h>
#include <XmlVariables.h>
#include <DataSharedMem.h>
#include <stdint.h>

using namespace std;

#ifdef __CINT__
#define uint32_t unsigned int
#endif

// Define variable holder
typedef map<string,string> VariableHolder;

//! Class to contain generic register data.
class DataRead {

      // Shared memory
      uint32_t smemFd_;
      void *smem_;
      uint32_t rdAddr_;
      uint32_t rdCount_;

      // File descriptor
      int32_t fd_;

      // File size
      off_t size_;

      // Process xml
      void xmlParse ( uint32_t size, char *data );

      // Variables
      XmlVariables status_;
      XmlVariables config_;
      XmlVariables start_;
      XmlVariables stop_;
      XmlVariables time_;

      // Start/Stop flags
      bool sawRunStart_;
      bool sawRunStop_;
      bool sawRunTime_;

   public:

      //! Constructor
      DataRead ( );

      //! Deconstructor
      ~DataRead ( );

      //! Open File
      /*! 
       * \param file Filename
      */
      bool open ( string file, bool compressed = false );

      //! Open Shared Memory
      /*! 
       * \param system System name
       * \param id ID to identify your process
      */
      void openShared ( string system, uint32_t id, int32_t uid=-1 );

      //! Close File
      void close ( );

      //! Return file size in bytes
      off_t size ( );

      //! Return file position in bytes
      off_t pos ( );

      //! Get next data record
      /*! 
       * Returns true on success
       * \param data Data object to store data
      */
      bool next ( Data *data );

      //! Get next data record & create new data object
      /*! 
       * Returns NULL on failure
      */
      Data *next ( );

      //! Get a config value
      /*! 
       * \param var Config variable name
      */
      string getConfig ( string var );

      //! Get a config value as integer
      /*! 
       * \param var Config variable name
      */
      uint32_t getConfigInt ( string var );

      //! Get a status value
      /*! 
       * \param var Status variable name
      */
      string getStatus ( string var );

      //! Get a status value as integer
      /*! 
       * \param var Status variable name
      */
      uint32_t getStatusInt ( string var );

      //! Dump config
      void dumpConfig ( );

      //! Dump status
      void dumpStatus ( );

      //! Get config as XML
      string getConfigXml ( );

      //! Dump status
      string getStatusXml ( );

      //! Get a run start value
      /*! 
       * \param var Variable name
      */
      string getRunStart ( string var );

      //! Get a run stop value
      /*! 
       * \param var Variable name
      */
      string getRunStop ( string var );

      //! Get a run time value
      /*! 
       * \param var Variable name
      */
      string getRunTime ( string var );

      //! Dump start
      void dumpRunStart ( );

      //! Dump stop
      void dumpRunStop ( );

      //! Dump time
      void dumpRunTime ( );

      //! Return true if we saw start marker, self clearing
      bool  sawRunStart ( );

      //! Return true if we saw stop marker, self clearing
      bool  sawRunStop ( );

      //! Return true if we saw time marker, self clearing
      bool  sawRunTime ( );
};
#endif
