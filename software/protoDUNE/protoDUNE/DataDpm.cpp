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

#define USE_MMAP 1

class RceCommon;

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
   addDevice(m_dataBuffer  = new DataBuffer(linkConfig, 0, this));
   m_dataBuffer->pollEnable(true);

   addDevice(m_rceCommon = new RceCommon (linkConfig, 0, this));
   m_rceCommon->pollEnable(false);

#if USE_MMAP // USE_MMAP = 1
   uint32_t linkConfigApp = linkConfig;
#else// USE_MMAP = 0
   uint32_t linkConfigApp = 00000002;
#endif

   // RCE Firmware Devices
   addDevice(d = new DataCompression(linkConfigApp, 0xA0000000, 0, this, 4));d->pollEnable(true);
   addDevice(d = new DataCompression(linkConfigApp, 0xA0010000, 1, this, 4));d->pollEnable(true);
   addDevice(d = new DataDpmHlsMon(  linkConfigApp, 0xA0020000, 0, this, 4));d->pollEnable(true);
   addDevice(d = new DataDpmWib(     linkConfigApp, 0xA1000000, 0, this, 4));d->pollEnable(true);
   addDevice(d = new DataDpmWib(     linkConfigApp, 0xA1010000, 1, this, 4));d->pollEnable(true);
   //addDevice(d = new DataDpmWibDbg(linkConfigApp, 0xA1020000, 0, this, 4));d->pollEnable(true);
   //addDevice(d = new DataDpmWibDbg(linkConfigApp, 0xA1030000, 1, this, 4));d->pollEnable(true);   
   addDevice(d = new DataDpmEmu(     linkConfigApp, 0xA2000000, 0, this, 4));d->pollEnable(true);
   addDevice(d = new DataDpmTiming(  linkConfigApp, 0xA3000000, 0, this, 4));d->pollEnable(true);
   addDevice(d = new UdpReg(         linkConfigApp, 0xA4000000, 0, this, 4));d->pollEnable(false); d->hideAllVariables(); d->hideAllCommands();
   addDevice(d = new RssiReg(        linkConfigApp, 0xA4010000, 0, this, 4));d->pollEnable(false); d->hideAllVariables(); d->hideAllCommands();
   //addDevice(d = new PrbsTx(       linkConfigApp, 0xA4020000, 0, this, 4));d->pollEnable(true); d->hideAllVariables(); d->hideAllCommands();
   //addDevice(d = new PrbsRx(       linkConfigApp, 0xA4030000, 0, this, 4));d->pollEnable(true); d->hideAllVariables(); d->hideAllCommands();
}

// Deconstructor
DataDpm::~DataDpm ( ) { }

