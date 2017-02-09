//-----------------------------------------------------------------------------
// File          : MultLink.h
// Author        : Ryan Herbst  <rherbst@slac.stanford.edu>
// Created       : 06/18/2014
// Project       : General Purpose
//-----------------------------------------------------------------------------
// Description :
// Mult link
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
#ifndef __MULT_LINK_H__
#define __MULT_LINK_H__

#include <string>
#include <sstream>
#include <map>
#include <pthread.h>
#include <unistd.h>
#include <CommLink.h>
#include <stdint.h>
using namespace std;

class MultDest;

//! Class to contain PGP communications link
class MultLink : public CommLink {
   protected:

      // Destinations
      uint32_t    destCount_;
      MultDest ** dests_;
      bool enDataThread_;

      //! IO handling thread
      void ioHandler();

      //! RX handling thread
      void rxHandler();

   public:

      //! Constructor
      MultLink (bool enDataThread = true);

      //! Deconstructor
      ~MultLink ( );

      //! Open link and start threads
      /*! 
       * Throw string on error.
       * \param count destination count
       * \param dest  destinations
      */
      void open ( uint32_t count, ... );
      void open ( uint32_t count, MultDest **dests );

      //! Stop threads and close link
      void close ();

      // ! Add data source
      void addDataSource ( uint32_t source );

};
#endif
