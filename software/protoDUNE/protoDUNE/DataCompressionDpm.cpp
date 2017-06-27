//-----------------------------------------------------------------------------
// File          : DataCompressionDpm.h
// Author        : Ryan Herbst <rherbst@slac.stanford.edu>
// Created       : 06/19/2014
// Project       : LBNE DAQ
//-----------------------------------------------------------------------------
// Description :
//    LBNE Data DPM Registers
//-----------------------------------------------------------------------------
// This file is part of 'DUNE Development Software'.
// It is subject to the license terms in the LICENSE.txt file found in the 
// top-level directory of this distribution and at: 
//    https://confluence.slac.stanford.edu/display/ppareg/LICENSE.html. 
// No part of 'DUNE Development Software', including this file, 
// may be copied, modified, propagated, or distributed except according to 
// the terms contained in the LICENSE.txt file.
// Proprietary and confidential to SLAC.
//-----------------------------------------------------------------------------



// ----------------------------------------------------------------------
// 
// HISTORY
// 
//       DATE WHO WHAT
// ---------- --- -------------------------------------------------------
// 2017.06.20 jjr Removed extraneous reference to RceCommon.h
// 2014.06.19     Created
// ----------------------------------------------------------------------

#include <sstream>
#include <iostream>
#include <string>
#include <stdlib.h>
#include <unistd.h>
#include <iomanip>

#include <Register.h>
#include <Variable.h>
#include <RegisterLink.h>
#include <Command.h>

#include <DataCompressionDpm.h>
#include <DataCompression.h>

using namespace std;

// Constructor
DataCompressionDpm::DataCompressionDpm ( uint32_t linkConfig, uint32_t index, Device *parent ) : 
                        Device(linkConfig,0,"DataCompressionDpm",index,parent) {
   Variable *v;
   Device   *d;

   // Description
   desc_ = "DataCompressionDpm Object.";

   v = getVariable("Enabled");
   v->set("True");
   v->setHidden(true);

   // RCE Firmware Devices
   addDevice(d = new DataCompression(linkConfig, 0xA0000000, 0, this, 4));d->pollEnable(true);
}

// Deconstructor
DataCompressionDpm::~DataCompressionDpm ( ) { }

