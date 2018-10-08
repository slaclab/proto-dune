//-----------------------------------------------------------------------------
// File          : DataDpmHlsMon.cpp
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
//
//       DATE WHO WHAT
// ---------- --- -------------------------------------------------------
// 2018.09.04 jjr Provide access to blowOff mask.  In addition to the
//                blowOff mask, a counter indicating when this value has
//                possibly been modified (m_wrote) is added. High leve
//                software should use this to prevent needless accessing
//                of the actual hardware register. If this value has not
//                changed since the last query, then the contract is that
//                its value has not either.
// 11/05/2015: created
//-----------------------------------------------------------------------------

#include <Register.h>
#include <Variable.h>
#include <RegisterLink.h>
#include <Command.h>

#include <DataDpmHlsMon.h>

#include <iostream>
#include <fstream>
#include <string>
#include <list>
#include <sstream>
#include <stdlib.h>
#include <unistd.h>
#include <iomanip>

using namespace std;

DataDpmHlsMon::DataDpmHlsMon(uint32_t linkConfig, uint32_t baseAddress, uint32_t index, Device *parent, uint32_t addrSize) : 
                        Device(linkConfig,baseAddress,"DataDpmHlsMon",index,parent){
   // Description
   desc_ = "DataDpmHlsMon object";
   RegisterLink * rl;

   // Register links
   addRegisterLink(rl = new RegisterLink("IbFrameRate0", baseAddress_ + 0x000, Variable::Status));
   rl->getVariable()->setDescription("Inbound frame rate for HLS0 (units of Hz)");
	rl->getVariable()->setComp(0,1,0,"");
   rl->setPollEnable(true);
   
   addRegisterLink(rl = new RegisterLink("IbBandwidth0", baseAddress_ + 0x004, 2, Variable::Status));
   rl->getVariable()->setDescription("Inbound bandwidth for HLS0 (units of Byte/s)");
	rl->getVariable()->setComp(0,1,0,"");
   rl->setPollEnable(true);   
   
   addRegisterLink(rl = new RegisterLink("ObFrameRate0", baseAddress_ + 0x00C, Variable::Status));
   rl->getVariable()->setDescription("Outbound frame rate for HLS0 (units of Hz)");
	rl->getVariable()->setComp(0,1,0,"");
   rl->setPollEnable(true);
   
   addRegisterLink(rl = new RegisterLink("ObBandwidth0", baseAddress_ + 0x010, 2, Variable::Status));
   rl->getVariable()->setDescription("Outbound bandwidth for HLS1 (units of Byte/s)");
	rl->getVariable()->setComp(0,1,0,"");
   rl->setPollEnable(true);   

   addRegisterLink(rl = new RegisterLink("IbFrameRate1", baseAddress_ + 0x018, Variable::Status));
   rl->getVariable()->setDescription("Inbound frame rate for HLS1 (units of Hz)");
	rl->getVariable()->setComp(0,1,0,"");
   rl->setPollEnable(true);
   
   addRegisterLink(rl = new RegisterLink("IbBandwidth1", baseAddress_ + 0x01C, 2, Variable::Status));
   rl->getVariable()->setDescription("Inbound bandwidth for HLS1 (units of Byte/s)");
	rl->getVariable()->setComp(0,1,0,"");
   rl->setPollEnable(true);   
   
   addRegisterLink(rl = new RegisterLink("ObFrameRate1", baseAddress_ + 0x024, Variable::Status));
   rl->getVariable()->setDescription("Outbound frame rate for HLS1 (units of Hz)");
	rl->getVariable()->setComp(0,1,0,"");
   rl->setPollEnable(true);
   
   addRegisterLink(rl = new RegisterLink("ObBandwidth1", baseAddress_ + 0x028, 2, Variable::Status));
   rl->getVariable()->setDescription("Outbound bandwidth for HLS1 (units of Byte/s)");
	rl->getVariable()->setComp(0,1,0,"");
   rl->setPollEnable(true);      

   addRegisterLink(rl = new RegisterLink("StatusRegister", baseAddress_ + 0x400, 1, 4,
                                         "IbValid", Variable::Status, 0, 0x3,  
                                         "IbReady", Variable::Status, 2, 0x3,  
                                         "ObValid", Variable::Status, 4, 0x3,  
                                         "ObReady", Variable::Status, 6, 0x3));    
   rl->setPollEnable(true);  
     
   addRegisterLink(rl = new RegisterLink("Blowoff",  baseAddress_ + 0x800, Variable::Configuration,0,0x3));
   rl->getVariable()->setDescription("true = blow off HLS output data, false = normal mode");  
   m_blowOff = rl;
   m_wrote   =  0;
   
   addRegister(new Register("HlsRst",    baseAddress_ + 0xFF8));
   addRegister(new Register("HardReset", baseAddress_ + 0xFFC));   

   // Enable polling
   pollEnable_ = true;   
   
}

// Deconstructor
DataDpmHlsMon::~DataDpmHlsMon ( ) { }

// command
void DataDpmHlsMon::command(string name, string arg) {
   Register *r;
   if (name == "HlsReset") {
      REGISTER_LOCK
      r = getRegister("HlsRst"); r->set(0x1); writeRegister(r, true);
      REGISTER_UNLOCK
   }
   ///////////////////////////////////////////////////////////////////////////////////
   // After executing the HlsReset, you "might" have to execute enableHLSmodule and 
   //                               reload the HLS configurations
   ///////////////////////////////////////////////////////////////////////////////////
   //void DataCompression::enableHLSmodule (uint32_t addrSize)
   //{
   //   uint32_t address = baseAddress_ 
   //                    + (XDUNEDATACOMPRESSIONCORE_BUS_A_ADDR_AP_CTRL>>2)*addrSize;   
   // 
   //   Register *r = new Register ("AxiLiteCtrl", address);
   //   addRegister (r);
   //
   //   REGISTER_LOCK
   //      r->set        (0x81); 
   //      writeRegister (r, true); 
   //      readRegister  (r);
   //   REGISTER_UNLOCK     
   //   
   //   return;
   //}   
   ///////////////////////////////////////////////////////////////////////////////////
}

// Hard Reset
void DataDpmHlsMon::hardReset () {
   Register *r;
   REGISTER_LOCK
   r = getRegister("HardReset"); r->set(0x1); writeRegister(r, true);
   REGISTER_UNLOCK
   Device::hardReset();
}

// Soft Reset
void DataDpmHlsMon::softReset () {
   Device::softReset();
}

// Count Reset
void DataDpmHlsMon::countReset () {
   Device::countReset();
}

// Method to read status registers and update variables
void DataDpmHlsMon::readStatus ( ) {
   Device::readStatus();
}

// Method to read configuration registers and update variables
void DataDpmHlsMon::readConfig ( ) {
   Device::readConfig();
}

// Method to write configuration registers
void DataDpmHlsMon::writeConfig ( bool force ) {  
   m_wrote += 1;
   Device::writeConfig(force);  
}

// Verify hardware state of configuration
void DataDpmHlsMon::verifyConfig ( ) {
   Device::verifyConfig();
}
