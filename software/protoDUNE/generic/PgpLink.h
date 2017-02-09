//-----------------------------------------------------------------------------
// File          : PgpLink.h
// Author        : Ryan Herbst  <rherbst@slac.stanford.edu>
// Created       : 04/12/2011
// Project       : General Purpose
//-----------------------------------------------------------------------------
// Description :
// PGP communications link
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
#ifndef __PGP_LINK_H__
#define __PGP_LINK_H__

#include <string>
#include <sstream>
#include <map>
#include <pthread.h>
#include <unistd.h>
#include <CommLink.h>
#include <stdint.h>
using namespace std;

//! Class to contain PGP communications link
class PgpLink : public CommLink {

   protected:

      // Device info
      string device_;
      int32_t    fd_;

      //! IO handling thread
      void ioHandler();

      //! RX handling thread
      void rxHandler();

   public:

      //! Constructor
      PgpLink ( );

      //! Deconstructor
      ~PgpLink ( );

      //! Open link and start threads
      /*! 
       * Throw string on error.
       * \param device pgpcard device
      */
      void open ( string device );

      //! Stop threads and close link
      void close ();

};
#endif
