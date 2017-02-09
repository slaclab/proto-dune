//-----------------------------------------------------------------------------
// File          : MultDest.h
// Author        : Ryan Herbst  <rherbst@slac.stanford.edu>
// Created       : 06/18/2014
// Project       : General Purpose
//-----------------------------------------------------------------------------
// Description :
// Destination container for MultLink class.
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
// 06/18/2014: created
//-----------------------------------------------------------------------------
#ifndef __MULT_DEST_H__
#define __MULT_DEST_H__

#include <vector>
#include <stdint.h>
#include <unistd.h>
#include <sys/types.h>
using namespace std;

class Register;

//! Class to contain PGP communications link
class MultDest {

   public:

      // Supported transaction types
      enum MultType { MultTypeRegisterRead,
                      MultTypeRegisterWrite,
                      MultTypeCommand,
                      MultTypeData };

   protected:

      // Data source configuration
      vector<uint32_t> dataSources_;

      // File descriptor
      int32_t fd_;

      // index
      uint32_t idx_;

      // Register RX Holder
      Register *rxRegister_;
      uint32_t registerSize_;

      // Data RX Holder
      uint8_t * rxData_;
      uint8_t * txData_;
      uint32_t  dataSize_;

      // Debug
      bool debug_;

      // Register access is synchronous
      bool regIsSync_;

   protected:

      // Look for data source
      bool isDataSource ( uint32_t source );

   public:

      //! Constructor
      MultDest (uint32_t maxRegister);

      //! Deconstructor
      virtual ~MultDest ( );

      // Add data source
      void addDataSource ( uint32_t source);

      // The remaining functions are called only from the MultLink class.

      // Set debug
      void setDebug ( bool debug );

      // Set FD
      void fdSet ( fd_set *fds, int32_t *maxFd );

      // Is FD Set?
      bool fdIsSet ( fd_set *fds );

      //! Open link
      virtual void open ( uint32_t idx, uint32_t maxRxTx );

      //! Stop link
      virtual void close ();

      //! Set data sources

      //! Transmit data.
      virtual int32_t transmit ( MultType type, void *ptr, uint32_t size, uint32_t context, uint32_t config );

      // Receive data
      virtual int32_t receive ( MultType *type, void **ptr, uint32_t *context);

      //! Determine if register access is synchronous
      bool regIsSync ();
};
#endif
