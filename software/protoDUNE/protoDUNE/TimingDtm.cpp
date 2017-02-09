//-----------------------------------------------------------------------------
// File          : TimingDtm.cpp
// Author        : Ryan Herbst <rherbst@slac.stanford.edu>
// Created       : 06/19/2014
// Project       : LBNE
//-----------------------------------------------------------------------------
// Description :
//    Timing DTM Registers
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

#include <TimingDtm.h>

#include <Register.h>
#include <Variable.h>
#include <Command.h>
#include <RegisterLink.h>

#include <sstream>
#include <iostream>
#include <string>
#include <iomanip>

#include <RceCommon.h>

using namespace std;

// Constructor
TimingDtm::TimingDtm ( uint32_t linkConfig, uint32_t index, Device *parent ) : 
                        Device(linkConfig,0,"TimingDtm",index,parent) {
   // Description
   desc_ = "Timing DTM";
   RegisterLink * rl;
   // uint32_t i;

   // Register links
   addRegisterLink(rl = new RegisterLink("DpmBusyIn0Cnt", 0xA0000000, Variable::Status));
   rl->getVariable()->setDescription("DpmBusyIn0 Status Counter");
	rl->getVariable()->setComp(0,1,0,"");
   rl->setPollEnable(true);   
   
   addRegisterLink(rl = new RegisterLink("DpmBusyIn1Cnt", 0xA0000004, Variable::Status));
   rl->getVariable()->setDescription("DpmBusyIn1 Status Counter");
	rl->getVariable()->setComp(0,1,0,"");
   rl->setPollEnable(true);  

   addRegisterLink(rl = new RegisterLink("DpmBusyIn2Cnt", 0xA0000008, Variable::Status));
   rl->getVariable()->setDescription("DpmBusyIn2 Status Counter");
	rl->getVariable()->setComp(0,1,0,"");
   rl->setPollEnable(true);   
   
   addRegisterLink(rl = new RegisterLink("DpmBusyIn3Cnt", 0xA000000C, Variable::Status));
   rl->getVariable()->setDescription("DpmBusyIn3 Status Counter");
	rl->getVariable()->setComp(0,1,0,"");
   rl->setPollEnable(true);     
   
   addRegisterLink(rl = new RegisterLink("DpmBusyIn4Cnt", 0xA0000010, Variable::Status));
   rl->getVariable()->setDescription("DpmBusyIn4 Status Counter");
	rl->getVariable()->setComp(0,1,0,"");
   rl->setPollEnable(true);   
   
   addRegisterLink(rl = new RegisterLink("DpmBusyIn5Cnt", 0xA0000014, Variable::Status));
   rl->getVariable()->setDescription("DpmBusyIn5 Status Counter");
	rl->getVariable()->setComp(0,1,0,"");
   rl->setPollEnable(true);  

   addRegisterLink(rl = new RegisterLink("DpmBusyIn6Cnt", 0xA0000018, Variable::Status));
   rl->getVariable()->setDescription("DpmBusyIn6 Status Counter");
	rl->getVariable()->setComp(0,1,0,"");
   rl->setPollEnable(true);   
   
   addRegisterLink(rl = new RegisterLink("DpmBusyIn7Cnt", 0xA000001C, Variable::Status));
   rl->getVariable()->setDescription("DpmBusyIn7 Status Counter");
	rl->getVariable()->setComp(0,1,0,"");
   rl->setPollEnable(true);  

   addRegisterLink(rl = new RegisterLink("RtmBusyOutCnt", 0xA0000020, Variable::Status));
   rl->getVariable()->setDescription("RtmBusyOut Status Counter");
	rl->getVariable()->setComp(0,1,0,"");
   rl->setPollEnable(true);     
   
   addRegisterLink(rl = new RegisterLink("StatusRegister", 0xA0000400, 1, 2,
                                         "DpmBusyIn", Variable::Status, 0, 0xFF,  // 0                              
                                         "RtmBusyOut", Variable::Status, 8, 0x1));// 1
   
   addRegisterLink(rl = new RegisterLink("DpmBusyMask",  0xA0000800, Variable::Configuration,0,0xFF));
   rl->getVariable()->setDescription("DPM Busy Mask, which is used to mask off DPM busy outputs");
   
   addRegisterLink(rl = new RegisterLink("ForceRtmBusy",  0xA0000804, Variable::Configuration,0,0x1));
   rl->getVariable()->setTrueFalse();
   rl->getVariable()->setDescription("true = forces RTM Busy out to 0x0, false = normal mode");     
   
   addRegister(new Register("SoftReset",    0xA0000FF0));
   addRegister(new Register("HardReset",    0xA0000FF4));
   addRegister(new Register("CounterReset", 0xA0000FFC));

   // Sub-devices
   addDevice(new RceCommon (linkConfig, 0, this));

   // Enable polling
   pollEnable_ = true;

}

// Deconstructor
TimingDtm::~TimingDtm ( ) { }

// Process Commands
void TimingDtm::command(string name, string arg) {
   Device::command(name, arg);
}

// Hard Reset
void TimingDtm::hardReset () {
   Register *r;
   REGISTER_LOCK
   r = getRegister("HardReset");
   r->set(0x1);
   writeRegister(r, true);
   r->set(0x0);
   writeRegister(r, true);
   REGISTER_UNLOCK
   Device::hardReset();
}

// Soft Reset
void TimingDtm::softReset () {
   Register *r;
   REGISTER_LOCK
   r = getRegister("SoftReset");
   r->set(0x1);
   writeRegister(r, true);
   r->set(0x0);
   writeRegister(r, true);
   REGISTER_UNLOCK
   Device::softReset();
}

// Count Reset
void TimingDtm::countReset () {
   Register *r;
   REGISTER_LOCK
   r = getRegister("CounterReset");
   r->set(0x1);
   writeRegister(r, true);
   r->set(0x0);
   writeRegister(r, true);
   REGISTER_UNLOCK
   Device::countReset();
}

// Method to read status registers and update variables
void TimingDtm::readStatus ( ) {
   Device::readStatus();
}

void TimingDtm::readConfig ( ) {
   Device::readConfig();
}

// Method to write configuration registers
void TimingDtm::writeConfig ( bool force ) {
   Device::writeConfig(force);
}

// Verify hardware state of configuration
void TimingDtm::verifyConfig ( ) {
   Device::verifyConfig();
}
