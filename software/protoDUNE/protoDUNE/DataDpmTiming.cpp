//-----------------------------------------------------------------------------
// File          : DataDpmTiming.cpp
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

#include <DataDpmTiming.h>

#include <iostream>
#include <fstream>
#include <string>
#include <list>
#include <sstream>
#include <stdlib.h>
#include <unistd.h>
#include <iomanip>

using namespace std;

DataDpmTiming::DataDpmTiming(uint32_t linkConfig, uint32_t baseAddress, uint32_t index, Device *parent, uint32_t addrSize) : 
                        Device(linkConfig,baseAddress,"DataDpmTiming",index,parent){
   // Description
   desc_ = "DataDpmTiming object";
   RegisterLink * rl;
   uint32_t i;

   // Register links
   addRegisterLink(rl = new RegisterLink("RunEnableCnt", baseAddress_ + 0x000, Variable::Status));
   rl->getVariable()->setDescription("RunEnable Status Counter");
	rl->getVariable()->setComp(0,1,0,"");
   rl->setPollEnable(true);
 
   addRegisterLink(rl = new RegisterLink("StatusRegister", baseAddress_ + 0x400, 1, 1,
                                         "RunEnable",      Variable::Status, 0, 0x1));// 0                              

   rl->setPollEnable(true);  
   for(i=0;i<1;i++) {
      rl->getVariable(i)->setTrueFalse();  
   }
   
   addRegisterLink(rl = new RegisterLink("SwFlush",  baseAddress_ + 0x800, Variable::Configuration,0,0x1));
   rl->getVariable()->setDescription("true = forces the WIB receiver to generate flush frames, false = run mode");     
   rl->getVariable()->setTrueFalse(); 
   
   addRegister(new Register("CounterReset", baseAddress_ + 0xFF4));
   addRegister(new Register("HardReset",    baseAddress_ + 0xFFC));
   
   // Enable polling
   pollEnable_ = true;   
   
}

// Deconstructor
DataDpmTiming::~DataDpmTiming ( ) { }

// command
void DataDpmTiming::command(string name, string arg) {
   Device::command(name, arg);
}

// Hard Reset
void DataDpmTiming::hardReset () {
   Register *r;
   REGISTER_LOCK
   r = getRegister("HardReset"); r->set(0x1); writeRegister(r, true);
   REGISTER_UNLOCK
   Device::hardReset();
}

// Soft Reset
void DataDpmTiming::softReset () {
   Device::softReset();
}

// Count Reset
void DataDpmTiming::countReset () {
   Register *r;
   REGISTER_LOCK
   r = getRegister("CounterReset"); r->set(0x1); writeRegister(r, true);
   REGISTER_UNLOCK
   Device::countReset();
}

// Method to read status registers and update variables
void DataDpmTiming::readStatus ( ) {
   Device::readStatus();
}

// Method to read configuration registers and update variables
void DataDpmTiming::readConfig ( ) {
   Device::readConfig();
}

// Method to write configuration registers
void DataDpmTiming::writeConfig ( bool force ) {  
   Device::writeConfig(force);  
}

// Verify hardware state of configuration
void DataDpmTiming::verifyConfig ( ) {
   Device::verifyConfig();
}
