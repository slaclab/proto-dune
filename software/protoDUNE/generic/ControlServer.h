//-----------------------------------------------------------------------------
// File          : ControlServer.h
// Author        : Ryan Herbst  <rherbst@slac.stanford.edu>
// Created       : 08/29/2011
// Project       : General Purpose
//-----------------------------------------------------------------------------
// Description :
// Control server class
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
// 08/29/2011: created
//-----------------------------------------------------------------------------
#ifndef __CONTROL_SERVER_H__
#define __CONTROL_SERVER_H__
#include <iostream>
#include <sstream>
#include <string>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <ControlCmdMem.h>
#include <XmlVariables.h>
#include <stdint.h>
using namespace std;

class System;

//! Class to manage control interface
class ControlServer {

      // Number of clients to support
      static const uint32_t MaxClients_ = 8;

      // Debug flag
      bool debug_;

      // Server fdes
      int32_t servFd_;

      // Connection fdes
      int32_t connFd_[MaxClients_];
      bool quietMode_[MaxClients_];

      // Port number
      int32_t port_;

      // Socket addresses
      struct sockaddr_in servAddr_;
      struct sockaddr_in connAddr_;

      // Current received data
      stringstream rxData_[MaxClients_];

      // Top level device
      System *system_;

      // Shared memory
      uint32_t          smemFd_;
      ControlCmdMemory *smem_;

      // Poll Variables
      char   buffer_[9001];
      char   xmlCmd_[9001];
      char   regStr_[9001];

      // Variable engine used in receive, must be created here so we are
      // not constantly calling the xml parser init function
      XmlVariables vars_;

      void sendData ( uint32_t idx, const char *buffer, uint32_t size );

   public:

      //! Constructor
      ControlServer ();

      //! DeConstructor
      ~ControlServer ();

      //! Set system instance
      /*! 
       * \param system System object
      */
      void setSystem ( System *system );

      //! Set debug flag
      /*! 
       * \param debug  Debug flag
      */
      void setDebug ( bool debug );

      //! Start tcpip listen socket
      /*! 
       * \param port Listen port number, pass zero to auto assign
       * resulting port number is returned
      */
      int32_t startListen ( int32_t port );

      //! Stop tcpip listen socket
      void stopListen ( );

      //! Enable shared memory for control
      /*! 
       * \param system System name
       * \param id ID to identify your process
      */
      void enableSharedMemory ( string system, uint32_t id );

      //! Receive and process data if ready
      /*! 
       * \param timeout    timeout value in microseconds
       * \param pollPeriod poll period in microseconds
       * \param stop       stop pointer
      */
      void receive ( uint32_t selectPeriod, uint32_t pollPeriod=0, bool *stop=NULL );
};

#endif
