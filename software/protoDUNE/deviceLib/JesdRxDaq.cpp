//-----------------------------------------------------------------------------
// File          : JesdRxDaq.cpp
// Author        : Uros legat <ulegat@slac.stanford.edu>
//                            <uros.legat@cosylab.com>
// Created       : 27/04/2015
// Project       : 
//-----------------------------------------------------------------------------
// Description :
//    Device container for Rx DAQ
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
// 27/04/2015: created
//-----------------------------------------------------------------------------
#include <JesdRxDaq.h>
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
JesdRxDaq::JesdRxDaq ( uint32_t linkConfig, uint32_t baseAddress, uint32_t index, Device *parent, uint32_t addrSize ) : 
                        Device(linkConfig,baseAddress,"JesdRxDaq",index,parent) {
   //Command      *c;
   
   // Description
   desc_ = "ADC data acquisition control";

   // Create Registers: name, address
   RegisterLink *rl;
   
   addRegisterLink(rl = new RegisterLink("ControlReg", baseAddress_ + (0x00*addrSize), 1, 4,
                             "SoftwareTrigger",   Variable::Configuration, 0, 0x1,                             
                             "Mode",              Variable::Configuration, 1, 0x1,
                             "HwTriggerMask",     Variable::Configuration, 2, 0x1,
                             "DecimationMode",    Variable::Configuration, 3, 0x1));   
   
   
   rl->getVariable(0) ->setDescription("Trigger data acquisition (bit0 - Auto clear register)");
   rl->getVariable(0) ->setPerInstance(true);
   rl->getVariable(1) ->setDescription("Mode of DAQ - '0'  - until packet size and needs trigger - '1'  - sends the 4k frames continuously no trigger");
   rl->getVariable(1) ->setPerInstance(true);
   rl->getVariable(2) ->setDescription("Hardware Trigger Mask '1' - Enabled, '0' - Disabled");
   rl->getVariable(2) ->setPerInstance(true);
   rl->getVariable(3) ->setDescription("Decimation Mode '1' - 16bit, '0' - 32bit");
   rl->getVariable(3) ->setPerInstance(true);
   
   addRegisterLink(rl = new RegisterLink("SampleDecimation", baseAddress_ + (0x02*addrSize), Variable::Configuration));
   rl->getVariable()->setDescription("Reduces the sample rate. 16bit DecimationMode: 0 - SR, 1 - SR/2, 2 - SR/4, 3 - SR/6 etc. 32bit DecimationMode: 0 - SR, 1 - SR, 2 - SR/2, 3 - SR/3 etc.");
   rl->getVariable()->setPerInstance(true);
   addRegisterLink(rl = new RegisterLink("DaqBufferSize", baseAddress_ + (0x03*addrSize), Variable::Configuration));
   rl->getVariable()->setDescription("Size of a single continuous data buffer (number of 32-bit words)"); 
   rl->getVariable()->setPerInstance(true);
   addRegisterLink(rl = new RegisterLink("S1ChannelSelect", baseAddress_ + (0x10*addrSize), Variable::Configuration));
   rl->getVariable()->setDescription("Select channel for AXI stream 1 (0 - Disabled, 1 - Ch1, etc)");                                                    
   rl->getVariable()->setPerInstance(true);   
   addRegisterLink(rl = new RegisterLink("S2ChannelSelect", baseAddress_ + (0x11*addrSize), Variable::Configuration));
   rl->getVariable()->setDescription("Select channel for AXI stream 2 (0 - Disabled, 1 - Ch1, etc)");     
   rl->getVariable()->setPerInstance(true);
   
   addRegisterLink(rl = new RegisterLink("StatusStream0", baseAddress_ + (0x20*addrSize), 1, 7,
                             "Pause0",        Variable::Status, 0, 0x1,
                             "Ready0",        Variable::Status, 1, 0x1,
                             "Overflow0",     Variable::Status, 2, 0x1,
                             "Error0",        Variable::Status, 3, 0x1,
                             "JESDValid0",    Variable::Status, 4, 0x1,
                             "Enabled0",      Variable::Status, 5, 0x1,                             
                             "PacketCnt0",    Variable::Status, 6,0x3ffffff));
                             
   rl->getVariable(0) ->setDescription("DAQ interface busy");
   rl->getVariable(1) ->setDescription("Ready to take new data packet");
   rl->getVariable(2) ->setDescription("Buffer overflow");
   rl->getVariable(3) ->setDescription("Error during last Acquisition - received buffer might be shorter");
   rl->getVariable(4) ->setDescription("The incoming JESD data is Valid");
   rl->getVariable(5) ->setDescription("Stream 1-enabled, 0-Disabled");
   rl->getVariable(6) ->setDescription("Number of frames sent. In the last transfer!");
   
   addRegisterLink(rl = new RegisterLink("StatusStream1", baseAddress_ + (0x21*addrSize), 1, 7,
                             "Pause1",        Variable::Status, 0, 0x1,
                             "Ready1",        Variable::Status, 1, 0x1,
                             "Overflow1",     Variable::Status, 2, 0x1,
                             "Error1",        Variable::Status, 3, 0x1,
                             "JESDValid1",    Variable::Status, 4, 0x1,
                             "Enabled1",      Variable::Status, 5, 0x1,                             
                             "PacketCnt1",    Variable::Status, 6,0x3ffffff));
   
   rl->getVariable(0) ->setDescription("DAQ interface busy");
   rl->getVariable(1) ->setDescription("Ready to take new data packet");
   rl->getVariable(2) ->setDescription("Buffer overflow");
   rl->getVariable(3) ->setDescription("Error during last Acquisition - received buffer might be shorter");
   rl->getVariable(4) ->setDescription("The incoming JESD data is Valid");
   rl->getVariable(5) ->setDescription("Stream 1-enabled, 0-Disabled");
   rl->getVariable(6) ->setDescription("Number of frames sent. In the last transfer!");

   // Variables
   getVariable("Enabled")->setHidden(true);
   
   //Commands
   //addCommand(c = new Command("TriggerDAQ"));
   //c->setDescription("Trigger data aquisition.");

}

// Deconstructor
JesdRxDaq::~JesdRxDaq ( ) { }

// Process Commands
void JesdRxDaq::command(string name, string arg) {
   if (name == "TriggerDAQ") trigDaq();
   else Device::command(name,arg);
}

//! Clear errors
void JesdRxDaq::trigDaq () {

   Register *r;
   REGISTER_LOCK
   r = getRegister("ControlReg");
   r->set(0x1,0,0x1);
   writeRegister(r, true);
   r->set(0x0,0,0x1);
   writeRegister(r, true);
   REGISTER_UNLOCK

}

