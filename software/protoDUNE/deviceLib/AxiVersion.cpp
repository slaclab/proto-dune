//-----------------------------------------------------------------------------
// File          : AxiVersion.cpp
// Author        : Ben Reese <bareese@slac.stanford.edu>
// Created       : 11/14/2013
// Project       : HPS SVT
//-----------------------------------------------------------------------------
// Description :
// Device container for AxiVersion.vhd
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
#include <AxiVersion.h>
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
AxiVersion::AxiVersion ( uint32_t linkConfig, uint32_t baseAddress, uint32_t index, Device *parent, uint32_t addrSize ) : 
                        Device(linkConfig,baseAddress,"AxiVersion",index,parent) {

   // Description
   desc_ = "Firmware Version object.";

   RegisterLink *rl;


   // Create Registers: name, address
   addRegisterLink(new RegisterLink("FpgaVersion",   baseAddress_ + 0x00*addrSize, Variable::Status));
   addRegisterLink(new RegisterLink("ScratchPad",    baseAddress_ + 0x01*addrSize, Variable::Configuration));
   addRegister(new Register("DeviceDnaLow", baseAddress_ + 0x02*addrSize));
   addRegister(new Register("DeviceDnaHigh",  baseAddress_ + 0x03*addrSize));
   addRegister(new Register("FdSerialLow",  baseAddress_ + 0x04*addrSize));
   addRegister(new Register("FdSerialHigh",   baseAddress_ + 0x05*addrSize));
   addRegister(new Register("MasterReset",   baseAddress_ + 0x06*addrSize));
   addRegister(new Register("FpgaReload",    baseAddress_ + 0x07*addrSize));

   addRegisterLink(new RegisterLink("FpgaReloadAddress", baseAddress_ + 8*addrSize, Variable::Configuration));
   addRegisterLink(rl = new RegisterLink("Counter", baseAddress_ + 9 *addrSize, Variable::Status));
   rl->setPollEnable(true);
   addRegisterLink(new RegisterLink("FpgaReloadHalt", baseAddress_ + 10*addrSize, Variable::Configuration, 0, 0x1));
   getVariable("FpgaReloadHalt")->setTrueFalse();
   
   addRegister(new Register("UserConstants", baseAddress_ + 0x100*addrSize, 64));
   addRegister(new Register("BuildStamp",    baseAddress_ + 0x200*addrSize, 64));
   
   // Variables
   Variable* v;

   v = getVariable("Enabled");
   v->set("True");
   v->setHidden(true);

   getVariable("FpgaVersion")->setDescription("FPGA Firmware Version Number");

   getVariable("ScratchPad")->setDescription("Register to test reads and writes");
   getVariable("ScratchPad")->setPerInstance(true);

   v = new Variable("DeviceDna", Variable::Status);
   v->setDescription("Xilinx Device DNA value burned into FPGA");
   addVariable(v);

   v = new Variable("FdSerial", Variable::Status);
   v->setDescription("Board ID value read from DS2411 chip");
   addVariable(v);

   v = new Variable("BuildStamp", Variable::Status);
   v->setDescription("Firmware Build String");
   addVariable(v);

   //Commands
   Command *c;

   c = new Command("MasterReset");
   c->setDescription("Perform a logical reset of every FPGA register");
   addCommand(c);

   c = new Command("FpgaReload");
   c->setDescription("Reload the FPGA from the attached PROM");
   addCommand(c);
                                    
   c = new Command("CounterReset");
   c->setDescription("Reset the counter");
   addCommand(c);

}

// Deconstructor
AxiVersion::~AxiVersion ( ) { }

// Process Commands
void AxiVersion::command(string name, string arg) {
   Register *r;
   if (name == "MasterReset") {
      REGISTER_LOCK
      r = getRegister("MasterReset");
      r->set(0x1);
      writeRegister(r, true, false);
      REGISTER_UNLOCK
   }
   else if (name == "FpgaReload") {
      REGISTER_LOCK
      r = getRegister("FpgaReload");
      r->set(0x1);
      writeRegister(r, true, false);
      parent_->setAllStale();
      REGISTER_UNLOCK
   }
   else if (name == "CounterReset") {
      REGISTER_LOCK
      r = getRegister("Counter");
      writeRegister(r, true, true);
      REGISTER_UNLOCK
   }
}

// Method to read status registers and update variables
void AxiVersion::readStatus ( ) {
   Device::readStatus();
   REGISTER_LOCK
         
   stringstream ss;

   
//    // Read registers
//    regReadInt("FpgaVersion");

   readRegister(getRegister("DeviceDnaHigh"));
   readRegister(getRegister("DeviceDnaLow"));
   ss.str("");
   ss << "0x" << hex << setw(8) << setfill('0');
   ss << getRegister("DeviceDnaHigh")->get();
   ss << getRegister("DeviceDnaLow")->get();
   getVariable("DeviceDna")->set(ss.str());

   readRegister(getRegister("FdSerialHigh"));
   readRegister(getRegister("FdSerialLow"));
   ss.str("");
   ss << "0x" << hex << setw(8) << setfill('0');
   ss << getRegister("FdSerialHigh")->get();
   ss << getRegister("FdSerialLow")->get();
   getVariable("FdSerial")->set(ss.str());

   readRegister(getRegister("BuildStamp"));
   string tmp = string((char *)(getRegister("BuildStamp")->data()));
   getVariable("BuildStamp")->set(tmp);

   REGISTER_UNLOCK
}

// void AxiVersion::readConfig ( ) {
//    REGISTER_LOCK
//    regReadInt("ScratchPad");
//    REGISTER_UNLOCK
// }

// // Method to write configuration registers
// void AxiVersion::writeConfig ( bool force ) {
//    REGISTER_LOCK
//    regWrite("ScratchPad",force);
//    REGISTER_UNLOCK
// }

