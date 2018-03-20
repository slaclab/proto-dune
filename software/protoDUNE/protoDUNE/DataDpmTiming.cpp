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

   addRegisterLink(rl = new RegisterLink("TimingRdyCnt", baseAddress_ + 0x004, Variable::Status));
   rl->getVariable()->setDescription("TimingRdy Status Counter");
	rl->getVariable()->setComp(0,1,0,"");
   rl->setPollEnable(true);

   addRegisterLink(rl = new RegisterLink("CdrLockedCnt", baseAddress_ + 0x008, Variable::Status));
   rl->getVariable()->setDescription("CdrLocked Status Counter");
	rl->getVariable()->setComp(0,1,0,"");
   rl->setPollEnable(true);

   addRegisterLink(rl = new RegisterLink("TimingMsgDropCnt", baseAddress_ + 0x00C, Variable::Status));
   rl->getVariable()->setDescription("TimingMsgDrop Status Counter");
	rl->getVariable()->setComp(0,1,0,"");
   rl->setPollEnable(true);

   addRegisterLink(rl = new RegisterLink("TriggerDetCnt", baseAddress_ + 0x010, Variable::Status));
   rl->getVariable()->setDescription("TriggerDet Status Counter");
	rl->getVariable()->setComp(0,1,0,"");
   rl->setPollEnable(true);
   

   addRegisterLink(rl = new RegisterLink("TimingRstCnt", baseAddress_ + 0x014, Variable::Status));
   rl->getVariable()->setDescription("TimingRst Status Counter");
	rl->getVariable()->setComp(0,1,0,"");
   rl->setPollEnable(true);   


   addRegisterLink(rl = new RegisterLink("StatusRegister", baseAddress_ + 0x400, 1, 6,
                                         "RunEnable",     Variable::Status, 0, 0x1,  // 0                              
                                         "TimingRdy",     Variable::Status, 1, 0x1,  // 1                              
                                         "CdrLocked",     Variable::Status, 2, 0x1,  // 2 
                                         "TimingMsgDrop", Variable::Status, 3, 0x1,  // 3                              
                                         "TriggerDet",    Variable::Status, 4, 0x1,  // 4                             
                                         "TimingRst",     Variable::Status, 5, 0x1));// 5                             
   rl->setPollEnable(true);  
   for(i=0;i<6;i++) {
      rl->getVariable(i)->setTrueFalse();  
   }
   
   addRegisterLink(rl = new RegisterLink("CdrFreqHz", baseAddress_ + 0x404, Variable::Status));
   rl->getVariable()->setDescription("CDR's clock frequency (in units of Hz)");
   rl->getVariable()->setComp(0,1,0,"");
   rl->setPollEnable(true);

   addRegisterLink(rl = new RegisterLink("TimingRxStatus", baseAddress_ + 0x408, Variable::Status,0,0xF));
   rl->getVariable()->setDescription("Timing Receiver's status vector");
   rl->getVariable()->setComp(0,1,0,"");
   rl->setPollEnable(true);

   addRegisterLink(rl = new RegisterLink("TimingRxEventCnt", baseAddress_ + 0x40C, Variable::Status));
   rl->getVariable()->setDescription("Timing Receiver's Event Counter");
   rl->getVariable()->setComp(0,1,0,"");
   rl->setPollEnable(true);

   addRegisterLink(rl = new RegisterLink("TimingTrigRate", baseAddress_ + 0x410, Variable::Status));
   rl->getVariable()->setDescription("Timing Trigger Rate (in units of Hz)");
   rl->getVariable()->setComp(0,1,0,"");
   rl->setPollEnable(true);

   addRegisterLink(rl = new RegisterLink("SwFlush",  baseAddress_ + 0x800, Variable::Configuration,0,0x1));
   rl->getVariable()->setDescription("true = forces the WIB receiver to generate flush frames, false = run mode");     
   rl->getVariable()->setTrueFalse(); 
   
//   addRegisterLink(rl = new RegisterLink("CdrEdgeSel",  0x804, Variable::Configuration,0,0x1));
//   rl->getVariable()->setTrueFalse();
//   rl->getVariable()->setDescription("false = using IDDR's rising edge sample, true = using IDDR's falling edge sample");

//   addRegisterLink(rl = new RegisterLink("CdrDataInv",  0x808, Variable::Configuration,0,0x1));
//   rl->getVariable()->setTrueFalse();
//   rl->getVariable()->setDescription("true = invert CDR data polarity, false = non-inverting CDC data polarity");

//   addRegisterLink(rl = new RegisterLink("PdtsEndpointAddr",  0x80C, Variable::Configuration,0,0xFF));
//   rl->getVariable()->setDescription("Drive's pdts_endpoint's addr port");

//   addRegisterLink(rl = new RegisterLink("PdtsEndpointTgrp",  0x810, Variable::Configuration,0,0x3));
//   rl->getVariable()->setDescription("Drive's pdts_endpoint's tgrp port");


   addRegister(new Register("CounterReset", baseAddress_ + 0xFF4));
   addRegister(new Register("SoftReset",    baseAddress_ + 0xFF8));
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
   Register *r;
   REGISTER_LOCK
   r = getRegister("SoftReset"); r->set(0x1); writeRegister(r, true);
   REGISTER_UNLOCK
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
