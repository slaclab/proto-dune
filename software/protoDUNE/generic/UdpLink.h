//-----------------------------------------------------------------------------
// File          : UdpLink.h
// Author        : Ryan Herbst  <rherbst@slac.stanford.edu>
// Created       : 04/12/2011
// Project       : General Purpose
//-----------------------------------------------------------------------------
// Description :
// UDP communications link
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
#ifndef __UDP_LINK_H__
#define __UDP_LINK_H__

#include <string>
#include <sstream>
#include <map>
#include <pthread.h>
#include <unistd.h>
#include <CommLink.h>
#include <stdint.h>

using namespace std;

//! Class to contain PGP communications link
class UdpLink : public CommLink {

   protected:

      // Values used for udp version
      uint32_t   udpCount_;
      int32_t    *udpFd_;
      struct sockaddr_in *udpAddr_;

      // Data order fix
      bool dataOrderFix_;

      //! IO handling thread
      void ioHandler();

      //! RX handling thread
      void rxHandler();

   public:

      //! Constructor
      UdpLink ( );

      //! Deconstructor
      ~UdpLink ( );

      //! Set max receive size
      /*! 
       * \param size max receive size
      */
      void setMaxRx(uint32_t size);

      //! Open link and start threads
      /*! 
       * Throw string on error.
       * \param port  udp port
       * \param count host count
       * \param host  udp hosts
      */
      void open ( int32_t port, uint32_t count, ... );

      //! Stop threads and close link
      void close ();

      //! Set data order fix flag
      /*! 
       * \param enable Enable flag
      */
      void setDataOrderFix (bool enable);

};
#endif
