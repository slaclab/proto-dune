//-----------------------------------------------------------------------------
// File          : PgpCard.h
// Author        : Ryan Herbst  <rherbst@slac.stanford.edu>
// Created       : 08/19/2016
// Project       : General Purpose
//-----------------------------------------------------------------------------
// Description :
// PGP Card Controller
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
#ifndef __PPG_CARD_H__
#define __PPG_CARD_H__

#include <Device.h>
#include <PgpDriver.h>
using namespace std;

class PgpCard : public Device {

      string   path_;
      int32_t  fd_;
      uint32_t mask_;
      PgpInfo  info_;

   public:

      PgpCard ( Device *parent, string path, uint32_t mask );

      ~PgpCard ( );

      void readStatus ( );
      void pollStatus ( );

      void command ( string name, string arg );

      void softReset ();

};
#endif
