//-----------------------------------------------------------------------------
// File          : AxiStreamDmaRing.cpp
// Author        : Ben Reese <bareese@slac.stanford.edu>
// Created       : 11/14/2013
// Project       : HPS SVT
//-----------------------------------------------------------------------------
// Description :
// Device container for AxiStreamDmaRing.vhd
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
// Modification history :
// 11/14/2013: created
//-----------------------------------------------------------------------------
#include <AxiStreamDmaRing.h>
#include <Register.h>
#include <RegisterLink.h>
#include <Variable.h>
#include <Command.h>
#include <sstream>
#include <iostream>
#include <string>
#include <iomanip>
using namespace std;

// Constructor
AxiStreamDmaRing::AxiStreamDmaRing ( uint32_t linkConfig, uint32_t baseAddress, uint32_t index, Device *parent,
                                     uint32_t numBuffers ) : 
                        Device(linkConfig,baseAddress,"AxiStreamDmaRing",index,parent) {

   // Description
   desc_ = "";

   RegisterLink *rl;
   stringstream s;
   s.str("");

   addVariable(new Variable("NumBuffers", Variable::Status));
   getVariable("NumBuffers")->setInt(numBuffers);


   // Start addresses
   for (uint32_t i=0; i<numBuffers; i++) {
      s.str(""); s << "Buf" << i << "Addr0Start";      
      addRegisterLink(new RegisterLink(s.str(),  getBufferAddr(START, i), Variable::Configuration));
   }
   // End Addresses
   for (uint32_t i=0; i<numBuffers; i++) {
      s.str(""); s << "Buf" << i << "Addr1End";      
      addRegisterLink(new RegisterLink(s.str(),  getBufferAddr(END, i), Variable::Configuration));
   }
   for (uint32_t i=0; i<numBuffers; i++) {
      s.str(""); s << "Buf" << i << "Addr1Next";      
      addRegisterLink(new RegisterLink(s.str(),  getBufferAddr(NEXT, i), Variable::Status));
   }
   for (uint32_t i=0; i<numBuffers; i++) {
      s.str(""); s << "Buf" << i << "Addr2Trigger";      
      addRegisterLink(new RegisterLink(s.str(),  getBufferAddr(TRIG, i), Variable::Status));
   }
   for (uint32_t i=0; i<numBuffers; i++) {
      s.str(""); s << "Buf" << i << "Mode";      
      addRegisterLink(rl = new RegisterLink(s.str(),  getBufferAddr(MODE, i), 1, 5,
                                       string(s.str()+"2Enabled").c_str(), Variable::Configuration, 0, 0x1,
                                       string(s.str()+"3Mode").c_str(), Variable::Configuration, 1, 0x1,
                                       string(s.str()+"4Init").c_str(), Variable::Configuration, 2, 0x1,
                                       string(s.str()+"5Dest").c_str(), Variable::Configuration, 4, 0xF,
                                       string(s.str()+"5FAT").c_str(), Variable::Configuration, 16, 0xFFFF));

      rl->getVariable(1)->setTrueFalse();
      map<uint32_t,string>modes;
      modes[0] = "Wrap";
      modes[1] = "DoneWhenFull";
      rl->getVariable(1)->setMap(modes);
      
      rl->getVariable(0)->setHidden(true); // Enabled
      rl->getVariable(2)->setHidden(true); // Init used only by commands
      //rl->getVariable(3)->setHidden(true); // Dest
      rl->getVariable(4)->setHidden(true); // FAT           
   }
   for (uint32_t i=0; i<numBuffers; i++) {
      s.str(""); s << "Buf" << i << "Status";
      string regName = s.str();
      cout << "C++ " << i << endl;      
      addRegisterLink(rl = new RegisterLink(regName,  getBufferAddr(STATUS, i), 1, 7,
                                            string(regName+"3Empty").c_str(), Variable::Status, 0, 0x1,
                                            string(regName+"4Full").c_str(), Variable::Status, 1, 0x1,
                                            string(regName+"5Done").c_str(), Variable::Status, 2, 0x1,
                                            string(regName+"6Triggered").c_str(), Variable::Status, 3, 0x1,
                                            string(regName+"7Error").c_str(), Variable::Status, 4, 0x1,
                                            string(regName+"8FST").c_str(), Variable::Status, 16, 0xFFFF,
                                            string(regName+"9BurstSize").c_str(), Variable::Status, 8, 0xF));
      rl->getVariable(0)->setTrueFalse(); // Empty
      rl->getVariable(1)->setTrueFalse(); // Full
      rl->getVariable(2)->setTrueFalse(); // Done
      rl->getVariable(3)->setTrueFalse(); // Triggered
      rl->getVariable(4)->setTrueFalse(); // Error
            
      rl->getVariable(5)->setHidden(true); // FST
      if (i != 0) {
         // All Burst Sizes are the same so hide all but the first one
         rl->getVariable(6)->setHidden(true);
      }
   }
   
   //Commands
   Command *c;

   for (uint32_t i=0; i<numBuffers; i++) {
      s.str(""); s << "Buf" << i << "Init";      
      c = new Command(s.str(), i);
      addCommand(c);
   }

   for (uint32_t i=0; i<numBuffers; i++) {
      s.str(""); s << "Buf" << i << "SoftTrigger";      
      c = new Command(s.str(), i);
      addCommand(c);
   }

}

// Deconstructor
AxiStreamDmaRing::~AxiStreamDmaRing ( ) { }

// Process Commands
void AxiStreamDmaRing::command(string name, string arg) {
   Register *r;
   stringstream s;
   s.str("");


   if (commands_.count(name) == 1) {
      uint32_t i = commands_[name]->opCode();
      s << "Buf" << i << "Mode";
      r = getRegister(s.str());
      REGISTER_LOCK
      r->set(name.find("Init")!=string::npos,        2, 0x1); // init bit
      r->set(name.find("SoftTrigger")!=string::npos, 3, 0x1); // software trigger bit
      writeRegister(r, true);
      r->set(0, 2, 0x1); // Set init bit low
      r->set(0, 3, 0x1); // Set software trigger bit low
      writeRegister(r, true);
         
      REGISTER_UNLOCK
            return;
   }
   Device::command(name, arg);
}

uint32_t AxiStreamDmaRing::getBufferAddr(uint32_t var, uint32_t buf) {
   if (var != MODE && var != STATUS) {
      return baseAddress_ | ( var<<9 | buf << 3);
   } else {
      return baseAddress_ | ( var<<9 | buf << 2);
   }

}

// Method to read status registers and update variables
// void AxiStreamDmaRing::readStatus ( ) {
//    Device::readStatus();
//    REGISTER_LOCK
//    REGISTER_UNLOCK
//}

// void AxiStreamDmaRing::readConfig ( ) {
//    REGISTER_LOCK
//    regReadInt("ScratchPad");
//    REGISTER_UNLOCK
// }

// // Method to write configuration registers
// void AxiStreamDmaRing::writeConfig ( bool force ) {
//    REGISTER_LOCK
//    regWrite("ScratchPad",force);
//    REGISTER_UNLOCK
// }

