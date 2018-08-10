//-----------------------------------------------------------------------------
// File          : DataDpmWib.cpp
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

#include <DataDpmWib.h>

#include <iostream>
#include <fstream>
#include <string>
#include <list>
#include <sstream>
#include <stdlib.h>
#include <unistd.h>
#include <iomanip>

using namespace std;

DataDpmWib::DataDpmWib(uint32_t linkConfig, uint32_t baseAddress, uint32_t index, Device *parent, uint32_t addrSize) : 
                        Device(linkConfig,baseAddress,"DataDpmWib",index,parent){
   // Description
   desc_ = "DataDpmWib object";
   RegisterLink * rl;
   Command      * c;   
   uint32_t i;

   // Register links
   addRegisterLink(rl = new RegisterLink("PktSentCnt", baseAddress_ + 0x000, Variable::Status));
   rl->getVariable()->setDescription("Packet Sent to HLS Status Counter");
	rl->getVariable()->setComp(0,1,0,"");
   rl->setPollEnable(true);  
   
   addRegisterLink(rl = new RegisterLink("RxDecErr0Cnt", baseAddress_ + 0x004, Variable::Status));
   rl->getVariable()->setDescription("RX Decode Error for bit0 Status Counter");
	rl->getVariable()->setComp(0,1,0,"");
   rl->setPollEnable(true);  

   addRegisterLink(rl = new RegisterLink("RxDecErr1Cnt", baseAddress_ + 0x008, Variable::Status));
   rl->getVariable()->setDescription("RX Decode Error for bit1 Status Counter");
	rl->getVariable()->setComp(0,1,0,"");
   rl->setPollEnable(true);     
   
   addRegisterLink(rl = new RegisterLink("RxDispErr0Cnt", baseAddress_ + 0x00C, Variable::Status));
   rl->getVariable()->setDescription("RX Disparity Error for bit0 Status Counter");
	rl->getVariable()->setComp(0,1,0,"");
   rl->setPollEnable(true);  

   addRegisterLink(rl = new RegisterLink("RxDispErr1Cnt", baseAddress_ + 0x010, Variable::Status));
   rl->getVariable()->setDescription("RX Disparity Error for bit1 Status Counter");
	rl->getVariable()->setComp(0,1,0,"");
   rl->setPollEnable(true);     

   addRegisterLink(rl = new RegisterLink("RxLinkUpCnt", baseAddress_ + 0x014, Variable::Status));
   rl->getVariable()->setDescription("RX Link up Status Counter");
	rl->getVariable()->setComp(0,1,0,"");
   rl->setPollEnable(true);

   addRegisterLink(rl = new RegisterLink("BackPressureCnt", baseAddress_ + 0x018, Variable::Status));
   rl->getVariable()->setDescription("HLS Back Pressure Status Counter");
	rl->getVariable()->setComp(0,1,0,"");
   rl->setPollEnable(true);   

   addRegisterLink(rl = new RegisterLink("ErrPktDropCnt", baseAddress_ + 0x01C, Variable::Status));
   rl->getVariable()->setDescription("Error: Back Pressure Packet Dropped Status Counter");
	rl->getVariable()->setComp(0,1,0,"");
   rl->setPollEnable(true);  

   addRegisterLink(rl = new RegisterLink("ErrPktLenCnt", baseAddress_ + 0x020, Variable::Status));
   rl->getVariable()->setDescription("Error: Packet Length Invalid Status Counter");
	rl->getVariable()->setComp(0,1,0,"");
   rl->setPollEnable(true);    

   addRegisterLink(rl = new RegisterLink("ErrCrcCnt", baseAddress_ + 0x024, Variable::Status));
   rl->getVariable()->setDescription("Error: CRC Status Counter");
	rl->getVariable()->setComp(0,1,0,"");
   rl->setPollEnable(true);

   addRegisterLink(rl = new RegisterLink("CPllLockCnt", baseAddress_ + 0x028, Variable::Status));
   rl->getVariable()->setDescription("GT's CPll Lock Status Counter");
	rl->getVariable()->setComp(0,1,0,"");
   rl->setPollEnable(true); 

   addRegisterLink(rl = new RegisterLink("ErrOverflowCnt", baseAddress_ + 0x02C, Variable::Status));
   rl->getVariable()->setDescription("Error: FIFO Overflow Status Counter");
	rl->getVariable()->setComp(0,1,0,"");
   rl->setPollEnable(true);

   addRegisterLink(rl = new RegisterLink("ErrFrameTermCnt", baseAddress_ + 0x030, Variable::Status));
   rl->getVariable()->setDescription("Error Frame Terminated Status Counter");
	rl->getVariable()->setComp(0,1,0,"");
   rl->setPollEnable(true);   

   addRegisterLink(rl = new RegisterLink("ErrGtRxBuffCnt", baseAddress_ + 0x034, Variable::Status));
   rl->getVariable()->setDescription("Error Frame Terminated Status Counter");
	rl->getVariable()->setComp(0,1,0,"");
   rl->setPollEnable(true);   
   
   addRegisterLink(rl = new RegisterLink("StatusRegister", baseAddress_ + 0x400, 1, 14,
                                         "PktSent",     Variable::Status,  0, 0x1,  // 0                              
                                         "RxDecErr0",   Variable::Status,  1, 0x1,  // 1
                                         "RxDecErr1",   Variable::Status,  2, 0x1,  // 2
                                         "RxDispErr0",  Variable::Status,  3, 0x1,  // 3 
                                         "RxDispErr1",  Variable::Status,  4, 0x1,  // 4 
                                         "RxLinkUp",    Variable::Status,  5, 0x1,  // 5 
                                         "BackPressure",Variable::Status,  6, 0x1,  // 6
                                         "ErrPktDrop",  Variable::Status,  7, 0x1,  // 7
                                         "ErrPktLen",   Variable::Status,  8, 0x1,  // 8
                                         "ErrCrc",      Variable::Status,  9, 0x1,  // 9
                                         "CPllLock",    Variable::Status, 10, 0x1,  // 10
                                         "ErrOverflow", Variable::Status, 11, 0x1,  // 11
                                         "ErrFrameTerm",Variable::Status, 12, 0x1,  // 12
                                         "ErrGtRxBuff", Variable::Status, 13, 0x1));// 13    
   rl->setPollEnable(true);  
   for(i=0;i<14;i++) {
      rl->getVariable(i)->setTrueFalse();
      rl->getVariable(i)->setHidden(true);  
   }

   // UnHide slow  transisiting status registers
   rl->getVariable(5)->setHidden(false);  rl->getVariable()->setDescription("RX Link up Status");
   rl->getVariable(6)->setHidden(false);  rl->getVariable()->setDescription("HLS Back Pressure Status");
   rl->getVariable(10)->setHidden(false); rl->getVariable()->setDescription("GT's CPll Lock Status");
   
   addRegisterLink(rl = new RegisterLink("RxBufStatus",  baseAddress_ + 0x404, Variable::Status,0,0x7));
   rl->getVariable()->setComp(0,1,0,"");
   rl->getVariable()->setDescription("RxBufStatus (Refer to ug476_7Series_Transceivers.pdf)"); 
   rl->setPollEnable(true); 
   
   addRegisterLink(rl = new RegisterLink("MinPktLen",  baseAddress_ + 0x408, Variable::Status,0,0xFF));
   rl->getVariable()->setComp(0,1,0,"");
   rl->getVariable()->setDescription("Min. WIB packet length received."); 
   rl->setPollEnable(true); 

   addRegisterLink(rl = new RegisterLink("PktLen",  baseAddress_ + 0x40C, Variable::Status,0,0xFF));
   rl->getVariable()->setComp(0,1,0,"");
   rl->getVariable()->setDescription("Current value of WIB packet length received"); 
   rl->setPollEnable(true);    

   addRegisterLink(rl = new RegisterLink("PktRate",  baseAddress_ + 0x410, Variable::Status));
   rl->getVariable()->setComp(0,1,0,"");
   rl->getVariable()->setDescription("WIB Packet rate (units of Hz)"); 
   rl->setPollEnable(true);    

   addRegisterLink(rl = new RegisterLink("WibSofRate",  baseAddress_ + 0x414, Variable::Status));
   rl->getVariable()->setComp(0,1,0,"");
   rl->getVariable()->setDescription("WIB Start Of Frame (SOF) rate (units of Hz).  This rate measurement is independent of WIB receiver's state machine's state"); 
   rl->setPollEnable(true); 
   
   addRegisterLink(rl = new RegisterLink("DbgCrcRcv",  baseAddress_ + 0x418, Variable::Status));
   rl->getVariable()->setDescription("Debug event's receiver 32-bit CRC value"); 
   rl->setPollEnable(true); 
   
   addRegisterLink(rl = new RegisterLink("DbgCrcCalc",  baseAddress_ + 0x41C, Variable::Status));
   rl->getVariable()->setDescription("Debug event's calculated 32-bit CRC value");    
   rl->setPollEnable(true); 
      
   addRegisterLink(rl = new RegisterLink("RxPolarity",  baseAddress_ + 0x700, Variable::Configuration,0,0x1));
   rl->getVariable()->setComp(0,1,0,"");
   rl->getVariable()->setTrueFalse();
   rl->getVariable()->setDescription("RxPolarity (Refer to ug476_7Series_Transceivers.pdf)"); 

   addRegisterLink(rl = new RegisterLink("TxPolarity",  baseAddress_ + 0x704, Variable::Configuration,0,0x1));
   rl->getVariable()->setComp(0,1,0,"");
   rl->getVariable()->setTrueFalse();
   rl->getVariable()->setDescription("TxPolarity (Refer to ug476_7Series_Transceivers.pdf)"); 
   
   addRegister(new Register("GtRst", baseAddress_ + 0x708));
   addCommand(c = new Command("GtReset"));
   c->setDescription("GT Reset");
   
   addRegister(new Register("debugArmErrorCapture", baseAddress_ + 0x710));
   addCommand(c = new Command("DebugArmErrorCapture"));
   c->setDescription("Start the debug waveform capturing of bad WIB frame");   
   
   addRegister(new Register("debugOneShotCapture", baseAddress_ + 0x714));
   addCommand(c = new Command("DebugOneShotCapture"));
   c->setDescription("Start the one-shot waveform capture of WIB frame");   

   addRegisterLink(rl = new RegisterLink("WibFrameBlowoff",  baseAddress_ + 0x718, Variable::Configuration,0,0x1));
   rl->getVariable()->setComp(0,1,0,"");
   rl->getVariable()->setTrueFalse();
   rl->getVariable()->setDescription("WIB Frame blowoff (default=True)");    

   addRegisterLink(rl = new RegisterLink("RollOverEn",  baseAddress_ + 0x7F0, Variable::Configuration,0,0x7FF));
   rl->getVariable()->setDescription("Rollover enable bit mask for status counters");

   addRegister(new Register("CounterReset", baseAddress_ + 0x7F4));
   addRegister(new Register("HardReset",    baseAddress_ + 0x7FC));
   
   // Enable polling
   pollEnable_ = true;   
   
}

// Deconstructor
DataDpmWib::~DataDpmWib ( ) { }

// command
void DataDpmWib::command(string name, string arg) {
   Register *r;

   if ( name == "GtReset" ) {
      REGISTER_LOCK
      r = getRegister("GtRst");
      r->set(0x1);
      writeRegister(r, true);
      REGISTER_UNLOCK

   } else if ( name == "DebugArmErrorCapture" ) {
      REGISTER_LOCK
      r = getRegister("debugArmErrorCapture");
      r->set(0x1);
      writeRegister(r, true);
      REGISTER_UNLOCK    

   } else if ( name == "DebugOneShotCapture" ) {
      REGISTER_LOCK
      r = getRegister("debugOneShotCapture");
      r->set(0x1);
      writeRegister(r, true);
      REGISTER_UNLOCK          
      
   } else Device::command(name, arg);
}

// Hard Reset
void DataDpmWib::hardReset () {
   Register *r;   
   REGISTER_LOCK
   r = getRegister("HardReset");
   r->set(0x1);
   writeRegister(r, true);
   REGISTER_UNLOCK   
   Device::hardReset();
}

// Soft Reset
void DataDpmWib::softReset () {
   Register *r;   
   REGISTER_LOCK
   r = getRegister("GtRst");
   r->set(0x1);
   writeRegister(r, true);
   REGISTER_UNLOCK 
   Device::softReset();
}

// Count Reset
void DataDpmWib::countReset () {
   Register *r;
   REGISTER_LOCK
   r = getRegister("CounterReset"); r->set(0x1); writeRegister(r, true);
   REGISTER_UNLOCK
   Device::countReset();
}

// Method to read status registers and update variables
void DataDpmWib::readStatus ( ) {
   Device::readStatus();
}

// Method to read configuration registers and update variables
void DataDpmWib::readConfig ( ) {
   Device::readConfig();
}

// Method to write configuration registers
void DataDpmWib::writeConfig ( bool force ) {  
   Device::writeConfig(force);  
}

// Verify hardware state of configuration
void DataDpmWib::verifyConfig ( ) {
   Device::verifyConfig();
}
