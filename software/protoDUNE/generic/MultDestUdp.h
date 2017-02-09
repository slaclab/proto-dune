//-----------------------------------------------------------------------------
// File          : MultDestUdp.h
// Author        : Ryan Herbst  <rherbst@slac.stanford.edu>
// Created       : 06/18/2014
// Project       : General Purpose
//-----------------------------------------------------------------------------
// Description :
// UDP Destination container for MultLink class.
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
#ifndef __MULT_DEST_UDP_H__
#define __MULT_DEST_UDP_H__
#include <MultDest.h>
#include <string>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <stdint.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/select.h>
using namespace std;

//! Class to contain PGP communications link
class MultDestUdp : public MultDest {

      string address_;
      int32_t port_;
      struct sockaddr_in addr_;
      uint32_t curPos_;

   public:

      //! Constructor
      MultDestUdp (string address, int32_t port);

      //! Deconstructor
      ~MultDestUdp ( );

      // The remaining functions are only called from the MultLink class

      //! Open link
      void open ( uint32_t idx, uint32_t maxRxTx );

      //! Transmit data.
      int32_t transmit ( MultType type, void *ptr, uint32_t size, uint32_t context, uint32_t config );

      // Receive data
      int32_t receive ( MultType *type, void **ptr, uint32_t *context);

};
#endif
