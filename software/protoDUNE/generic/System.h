//-----------------------------------------------------------------------------
// File          : System.h
// Author        : Ryan Herbst  <rherbst@slac.stanford.edu>
// Created       : 04/12/2011
// Project       : General Purpose
//-----------------------------------------------------------------------------
// Description :
// Generic system level container.
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
#ifndef __SYSTEM_H__
#define __SYSTEM_H__

#include <Device.h>
#include <string>
#include <sstream>
#include <map>
#include <vector>
#include <ControlCmdMem.h>
#include <libxml/tree.h>
#include <stdint.h>
using namespace std;

//! Class to contain generic device data.
class System : public Device {

   protected:

      // Comm link
      CommLink *commLink_;

      // Thread Routines
      static void *swRunStatic ( void *t );

      // Thread pointers
      pthread_t swRunThread_;

      // Defaults File
      string defaults_;

      // Software run thread
      virtual void swRunThread();

      // Run Variables
      bool     swRunEnable_;
      bool     swRunning_;
      uint32_t swRunPeriod_;
      uint32_t swRunCount_;
      uint32_t swRunProgress_;
      string   swRunRetState_;
      string   swRunError_;
      bool     hwRunning_;

      // Current state for polling
      bool allStatusReq_;
      bool topStatusReq_;
      bool allConfigReq_;

      // Configure status
      string configureMsg_;
      bool   configureFlag_;

      // Return data buffer
      string errorBuffer_;
      bool   errorFlag_;

      // Tracking counters
      uint32_t lastFileCount_;
      uint32_t lastDataCount_;
      time_t lastTime_;
      time_t pollTime_;

      // Run time tracker
      time_t lastRunTime_;

      // Parse XML
      bool parseXml ( string input, bool force );

      // Add start xml to file
      void addRunStart();

      // Add stop xml to file
      void addRunStop();

      // Add timestamp to file
      void addRunTime();

      // Unique xml id and mutex
      uint xmlId_;
      pthread_mutex_t xmlMutex_;

   public:

      //! Constructor
      /*! 
       * \param name     System name
       * \param commLink CommLink object
      */
      System ( string name, CommLink *commLink=NULL );

      //! Deconstructor
      virtual ~System ( );

      //! Get comm link
      CommLink * commLink();

      //! Method to process a command
      /*!
       * Throws string on error
       * \param name     Command name
       * \param arg      Optional arg
      */
      virtual void command ( string name, string arg );

      //! Method to set run state
      /*!
       * Set run state for the system. Default states are
       * Stopped & Running. Stopped must always be supported.
       * \param state    New run state
      */
      virtual void setRunState ( string state );

      //! Parse XML string
      /*!
       * \param xml XML string
      */
      void parseXmlString ( string input );

      //! Parse XML File
      /*!
       * \param xml XML string
      */
      void parseXmlFile ( string file );

      //! Method to perform soft reset
      virtual void softReset ( );

      //! Method to perform hard reset
      virtual void hardReset ( );

      //! Method to perform count reset
      virtual void countReset ( );

      //! Return local state machine, specific to each implementation
      virtual string localState();

      //! Poll system level status and process return messages
      string poll(ControlCmdMemory *cmem = NULL);

      //! Return structure string
      /*! 
       * \param hidden Set true to include hidden variables & commands
      */
      string structureString(bool hidden, bool indent);

      //! Return status string
      string statusString(bool hidden, bool indent, bool compact, bool recursive);

      //! Return config string
      string configString(bool hidden, bool indent);

      //! Method to write configuration registers
      /*! 
       * Throws string on error.
       * \param force Write all registers if true, only stale if false
      */
      virtual void writeConfig ( bool force );

      //! Set debug flag
      /*! 
       * \param enable    Debug state
      */
      void setDebug( bool enable );

      //! Generate timestamp from passed time value
      /*! 
       * \param time    Unix timestamp
      */
      static string genTime( time_t tme );

};
#endif
