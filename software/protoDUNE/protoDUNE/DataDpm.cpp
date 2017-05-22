//-----------------------------------------------------------------------------
// File          : DataDpm.h
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
// Modification history :
// 06/19/2014: created
//-----------------------------------------------------------------------------

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

#include <DataDpm.h>

#include <DataBuffer.h>
#include <RceCommon.h>
#include <DataCompression.h>
#include <DataDpmHlsMon.h>
#include <DataDpmWib.h>
#include <DataDpmWibDbg.h>
#include <DataDpmEmu.h>
#include <DataDpmTiming.h>
#include <UdpReg.h>
#include <RssiReg.h>
#include <PrbsTx.h>
#include <PrbsRx.h>

using namespace std;


// Constructor
DataDpm::DataDpm ( uint32_t linkConfig, uint32_t index, Device *parent ) : 
                        Device(linkConfig,0,"DataDpm",index,parent) {
   Variable *v;
   Device   *d;

   // Description
   desc_ = "Data DPM.";

   v = getVariable("Enabled");
   v->set("True");
   v->setHidden(true);

   // Software Devices 
   addDevice(d = new DataBuffer(linkConfig, 0, this));d->pollEnable(true);
   addDevice(d = new RceCommon (linkConfig, 0, this));d->pollEnable(false);

   // RCE Firmware Devices
   addDevice(d = new DataCompression(0x00000002, 0xA0000000, 0, this, 4));d->pollEnable(true);
   addDevice(d = new DataCompression(0x00000002, 0xA0010000, 1, this, 4));d->pollEnable(true);
   addDevice(d = new DataDpmHlsMon(  0x00000002, 0xA0020000, 0, this, 4));d->pollEnable(true);
   addDevice(d = new DataDpmWib(     0x00000002, 0xA1000000, 0, this, 4));d->pollEnable(true);
   addDevice(d = new DataDpmWib(     0x00000002, 0xA1010000, 1, this, 4));d->pollEnable(true);
   //addDevice(d = new DataDpmWibDbg(  0x00000002, 0xA1020000, 0, this, 4));d->pollEnable(true);
   //addDevice(d = new DataDpmWibDbg(  0x00000002, 0xA1030000, 1, this, 4));d->pollEnable(true);   
   addDevice(d = new DataDpmEmu(     0x00000002, 0xA2000000, 0, this, 4));d->pollEnable(true);
   addDevice(d = new DataDpmTiming(  0x00000002, 0xA3000000, 0, this, 4));d->pollEnable(true);
   addDevice(d = new UdpReg(         0x00000002, 0xA4000000, 0, this, 4));d->pollEnable(false); d->hideAllVariables(); d->hideAllCommands();
   addDevice(d = new RssiReg(        0x00000002, 0xA4010000, 0, this, 4));d->pollEnable(false); d->hideAllVariables(); d->hideAllCommands();
   //addDevice(d = new PrbsTx(         0x00000002, 0xA4020000, 0, this, 4));d->pollEnable(true); d->hideAllVariables(); d->hideAllCommands();
   //addDevice(d = new PrbsRx(         0x00000002, 0xA4030000, 0, this, 4));d->pollEnable(true); d->hideAllVariables(); d->hideAllCommands();
}

// Deconstructor
DataDpm::~DataDpm ( ) { }

