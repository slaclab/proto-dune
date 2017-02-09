//-----------------------------------------------------------------------------
// File          : PgpCardG3Link.h
// Author        : Ryan Herbst  <rherbst@slac.stanford.edu>
// Created       : 04/12/2011
// Project       : General Purpose
//-----------------------------------------------------------------------------
// Description :
// PGP communications link for PgpCardG3
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
// 07/28/2014: created
//-----------------------------------------------------------------------------
#ifndef __PGP_CARD_G3_LINK_H__
#define __PGP_CARD_G3_LINK_H__

#include <string>
#include <sstream>
#include <map>
#include <pthread.h>
#include <unistd.h>
#include <CommLink.h>
#include <stdint.h>
using namespace std;

//! Class to contain PGP communications link
class PgpCardG3Link : public CommLink {

   protected:

      // Device info
      string device_;
      int32_t    fd_;

      // Register Map
      struct PgpCardReg *reg_;

      //! IO handling thread
      void ioHandler();

      //! RX handling thread
      void rxHandler();

   public:

      //! Constructor
      PgpCardG3Link ( );

      //! Deconstructor
      ~PgpCardG3Link ( );

      //! Open link and start threads
      /*! 
       * Throw string on error.
       * \param device pgpcard device
      */
      void open ( string device );

      //! Stop threads and close link
      void close ();

      // Evr Link Status
      bool getEvrStatus();

      // Evr Link Errors
      uint32_t getEvrErrors();

      // Evr Link Count
      uint32_t getEvrCount(uint32_t idx);
      uint32_t getEvrStatRaw();

      // set/Get EVR Enble
      bool getEvrEnable();
      void setEvrEnable(bool enable);

      // set/Get EVR Enble mask
      uint32_t getEvrEnableLane();
      void setEvrEnableLane(uint32_t mask);

      // set/Get EVR Lane Code
      uint32_t getEvrLaneRunOpCode(uint32_t lane);
      void setEvrLaneRunOpCode(uint32_t lane, uint32_t code);

      // set/Get EVR Lane Code
      uint32_t getEvrLaneAcceptOpCode(uint32_t lane);
      void setEvrLaneAcceptOpCode(uint32_t lane, uint32_t code);

      // set/Get EVR Lane Code
      uint32_t getEvrLaneRunDelay(uint32_t lane);
      void setEvrLaneRunDelay(uint32_t lane, uint32_t delay);

      // set/Get EVR Lane Code
      uint32_t getEvrLaneAcceptDelay(uint32_t lane);
      void setEvrLaneAcceptDelay(uint32_t lane, uint32_t delay);

      //! Transmit Op-Code
      int32_t sendOpCode ( uint32_t opCode );      

};
#endif
