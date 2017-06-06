//-----------------------------------------------------------------------------
// File          : DataDpmEmu.cpp
// Author        : Matt Graham  <mgraham@slac.stanford.edu>
// Created       : 11/05/2015
// Project       : 
//-----------------------------------------------------------------------------
// Description :  Container for Waveform Extraction constants
//
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
// 11/05/2015: created
//-----------------------------------------------------------------------------

#include <Register.h>
#include <Variable.h>
#include <RegisterLink.h>
#include <Command.h>

#include <DataDpmEmu.h>
#include <DataDpmEmuCh.h>

#include <iostream>
#include <fstream>
#include <string>
#include <list>
#include <sstream>
#include <stdlib.h>
#include <unistd.h>
#include <iomanip>

using namespace std;

DataDpmEmu::DataDpmEmu(uint32_t linkConfig, uint32_t baseAddress, uint32_t index, Device *parent, uint32_t addrSize) : 
                        Device(linkConfig,baseAddress,"DataDpmEmu",index,parent){
   // Description
   desc_ = "DataDpmEmu object";
   RegisterLink * rl;

   addRegisterLink(rl = new RegisterLink("EnableTx",  baseAddress_ + 0xC00, Variable::Configuration,0,0x1));
   rl->getVariable()->setComp(0,1,0,"");
   rl->getVariable()->setTrueFalse();
   rl->getVariable()->setDescription("true = emulation TX enabled, false = emulation TX disabled");   
   
   addRegisterLink(rl = new RegisterLink("EnableTrig",  baseAddress_ + 0xC04, Variable::Configuration,0,0x1));
   rl->getVariable()->setComp(0,1,0,"");
   rl->getVariable()->setTrueFalse();
   rl->getVariable()->setDescription("true = triggers in emulation waveform, false = no triggers in emulation waveform");   
   
   addRegisterLink(rl = new RegisterLink("Loopback",  baseAddress_ + 0xC08, Variable::Configuration,0,0x1));
   rl->getVariable()->setComp(0,1,0,"");
   rl->getVariable()->setTrueFalse();
   rl->getVariable()->setDescription("true = MGT loopback, false = normal mode");   

   addRegisterLink(rl = new RegisterLink("ComModeNoise",  baseAddress_ + 0xC0C, Variable::Configuration,0,0x7));
   rl->getVariable()->setComp(0,1,0,"");
   rl->getVariable()->setDescription("Sets the number of common mode noise bits in the emulation adc data");   
   
   // addRegisterLink(rl = new RegisterLink("TrigRate",  baseAddress_ + 0xC10, Variable::Configuration,0,0xFFFFFFFF));
   // rl->getVariable()->setComp(0,1,0,"");
   // rl->getVariable()->setDescription("trigger rate = 125MHz/(TrigRate+1)");   

   addRegisterLink(rl = new RegisterLink("TxPreCursor",  baseAddress_ + 0xC14, Variable::Configuration,0,0x1F));
   rl->getVariable()->setComp(0,1,0,"");
   rl->getVariable()->setDescription("TxPreCursor (Refer to ug476_7Series_Transceivers.pdf)"); 

   addRegisterLink(rl = new RegisterLink("TxPostCursor",  baseAddress_ + 0xC18, Variable::Configuration,0,0x1F));
   rl->getVariable()->setComp(0,1,0,"");
   rl->getVariable()->setDescription("TxPostCursor (Refer to ug476_7Series_Transceivers.pdf)"); 

   addRegisterLink(rl = new RegisterLink("TxDiffCtrl",  baseAddress_ + 0xC1C, Variable::Configuration,0,0xF));
   rl->getVariable()->setComp(0,1,0,"");
   rl->getVariable()->setDescription("TxDiffCtrl (Refer to ug476_7Series_Transceivers.pdf)"); 
   
   addRegisterLink(rl = new RegisterLink("SendCntData",  baseAddress_ + 0xC20, Variable::Configuration,0,0x1));
   rl->getVariable()->setComp(0,1,0,"");
   rl->getVariable()->setTrueFalse();
   rl->getVariable()->setDescription("true = Sends counter data pattern, false = normal mode"); 

   addRegister(new Register("HardReset",    baseAddress_ + 0xFFC));

   // Enable polling
   pollEnable_ = true;   
   
   addDevice(new DataDpmEmuCh(linkConfig, baseAddress_, 0, this, 4));
   
}

// Deconstructor
DataDpmEmu::~DataDpmEmu ( ) { }

// command
void DataDpmEmu::command(string name, string arg) {
   Device::command(name, arg);
}

// Hard Reset
void DataDpmEmu::hardReset () {
   Register *r;
   REGISTER_LOCK
   r = getRegister("HardReset"); r->set(0x1); writeRegister(r, true);
   REGISTER_UNLOCK
   Device::hardReset();
}

// Soft Reset
void DataDpmEmu::softReset () {
   Device::softReset();
}

// Count Reset
void DataDpmEmu::countReset () {
   Device::countReset();
}

// Method to read status registers and update variables
void DataDpmEmu::readStatus ( ) {
   Device::readStatus();
}

// Method to read configuration registers and update variables
void DataDpmEmu::readConfig ( ) {
   Device::readConfig();
}

// Method to write configuration registers
void DataDpmEmu::writeConfig ( bool force ) {  
   Device::writeConfig(force);  
}

// Verify hardware state of configuration
void DataDpmEmu::verifyConfig ( ) {
   Device::verifyConfig();
}
