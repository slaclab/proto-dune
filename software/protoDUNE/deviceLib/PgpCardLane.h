//-----------------------------------------------------------------------------
// File          : PgpCardLane.h
// Author        : Ryan Herbst  <rherbst@slac.stanford.edu>
// Created       : 08/19/2016
// Project       : General Purpose
//-----------------------------------------------------------------------------
// Description :
// PGP Card Lane Controller
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
#ifndef __PGP_CARD_LANE_H__
#define __PGP_CARD_LANE_H__

#include <Device.h>
using namespace std;

//! Class to contain APV25 
class PgpCardLane : public Device {

      int32_t  fd_;
      int32_t  evrEn_;
      uint32_t locData_;
      uint32_t loopBack_;

   public:

      PgpCardLane ( Device *parent, int32_t fd_, uint32_t index, bool evrEn );

      ~PgpCardLane ( );

      void readStatus ( );
      void pollStatus ( );

      void readConfig ( );

      void writeConfig ( bool force );

      void softReset ();

};
#endif
