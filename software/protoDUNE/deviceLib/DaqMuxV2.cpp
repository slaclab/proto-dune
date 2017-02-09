//-----------------------------------------------------------------------------
// File          : DaqMuxV2.cpp
// Author        : Uros legat <ulegat@slac.stanford.edu>
//                            <uros.legat@cosylab.com>
// Created       : 7/19/2016
// Project       : 
//-----------------------------------------------------------------------------
// Description :
//    Device container for DAQ Multiplexer Version 2
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
//-----------------------------------------------------------------------------
#include <DaqMuxV2.h>
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
DaqMuxV2::DaqMuxV2 ( uint32_t linkConfig, uint32_t baseAddress, uint32_t index, Device *parent, uint32_t addrSize ) : 
                        Device(linkConfig,baseAddress,"DaqMuxV2",index,parent) {
   Command      *c;
   
   // Description
   desc_ = "ADC data acquisition control";

   // Create Registers: name, address
   RegisterLink *rl;
   
   addRegisterLink(rl = new RegisterLink("ControlReg", baseAddress_ + (0x00*addrSize), 1, 9,
                             "TriggerSw",             Variable::Configuration, 0, 0x1,                             
                             "TriggerCascMask",       Variable::Configuration, 1, 0x1,
                             "TriggerHwAutoRearm",    Variable::Configuration, 2, 0x1,
                             "TriggerHwArm",          Variable::Configuration, 3, 0x1,
                             "TriggerClearStatus",    Variable::Configuration, 4, 0x1,                             
                             "DaqMode",               Variable::Configuration, 5, 0x1,
                             "PacketHeaderEn",        Variable::Configuration, 6, 0x1,
                             "FreezeSw",              Variable::Configuration, 7, 0x1,
                             "FreezeHwMask",          Variable::Configuration, 8, 0x1));
                             
   rl->getVariable(0) ->setDescription("Software Trigger (triggers DAQ on all enabled channels).");
   rl->getVariable(0) ->setPerInstance(true);
   rl->getVariable(1) ->setDescription("Mask for enabling/disabling cascaded trigger.");
   rl->getVariable(1) ->setPerInstance(true);
   rl->getVariable(2) ->setDescription("Mask for enabling/disabling hardware trigger. If disabled it has to be rearmed by ArmHwTrigger.");
   rl->getVariable(2) ->setPerInstance(true);
   rl->getVariable(3) ->setDescription("Arm the Hardware trigger (On the rising edge). After trigger occurs the trigger has to be rearmed.");
   rl->getVariable(3) ->setPerInstance(true);
   rl->getVariable(4) ->setDescription("Trigger status will be cleared (On the rising edge).");
   rl->getVariable(4) ->setPerInstance(true);
   rl->getVariable(5) ->setDescription("Select the data acquisition mode.");
   rl->getVariable(5) ->setPerInstance(true);
   rl->getVariable(6) ->setDescription("Applies only to Triggered mode.");
   rl->getVariable(6) ->setPerInstance(true);
   rl->getVariable(7) ->setDescription("Software freeze buffer (Freezes all enabled circular buffers).");
   rl->getVariable(7) ->setPerInstance(true);
   rl->getVariable(8) ->setDescription("Mask for enabling/disabling hardware freaze buffer request.");
   rl->getVariable(8) ->setPerInstance(true); 
   
   addRegisterLink(rl = new RegisterLink("StatusReg", baseAddress_ + (0x01*addrSize), 1, 6,
                          "TriggerSwStatus",       Variable::Status, 0, 0x1,
                          "TriggerCascStatus",     Variable::Status, 1, 0x1,
                          "TriggerHwStatus",       Variable::Status, 2, 0x1,
                          "TriggerHwArmed",        Variable::Status, 3, 0x1,
                          "TriggerStatus",         Variable::Status, 4, 0x1,
                          "FreezeStatus",          Variable::Status, 5, 0x1));

   rl->getVariable(0) ->setDescription("Software Trigger Status (Registered on first trigger until cleared by TriggerClearStatus).");
   rl->getVariable(0) ->setPerInstance(true);
   rl->getVariable(1) ->setDescription("Cascade Trigger Status (Registered on first trigger until cleared by TriggerClearStatus).");
   rl->getVariable(1) ->setPerInstance(true);
   rl->getVariable(2) ->setDescription("Hardware Trigger Status (Registered on first trigger until cleared by TriggerClearStatus).");
   rl->getVariable(2) ->setPerInstance(true);
   rl->getVariable(3) ->setDescription("Hardware Trigger Armed Status (Registered on rising edge Control(3) and cleared when Hw trigger occurs).");
   rl->getVariable(3) ->setPerInstance(true);
   rl->getVariable(4) ->setDescription("Combined Trigger Status (Registered on first trigger until cleared by TriggerClearStatus).");
   rl->getVariable(4) ->setPerInstance(true);
   rl->getVariable(5) ->setDescription("Freeze Buffers Status (Registered on first freeze until cleared by TriggerClearStatus)");
   rl->getVariable(5) ->setPerInstance(true);
   
   addRegisterLink(rl = new RegisterLink("DecimationRateDiv", baseAddress_ + (0x02*addrSize), Variable::Configuration));
   rl->getVariable()->setDescription("Averaging Enabled: (powers of two) 1,2,4,8,16,etc (max 2^12). Averaging Disabled (32-bit): 1,2,3,4,etc (max 2^16-1). Averaging Disabled (16-bit): 1,2,4,6,8,etc (max 2^16-1).");
   rl->getVariable()->setPerInstance(true);
   
   addRegisterLink(rl = new RegisterLink("DaqBufferSize", baseAddress_ + (0x03*addrSize), Variable::Configuration));
   rl->getVariable()->setDescription("Number of 32-bit words. Minimum size is 4."); 
   rl->getVariable()->setPerInstance(true);
   
   addRegisterLink(rl = new RegisterLink("S0ChannelSelect", baseAddress_ + (0x10*addrSize), Variable::Configuration));
   rl->getVariable()->setDescription("Input Mux select. Maximum number of channels is 29. (0 - Disabled, 1 - Test, 2-Ch0,3-Ch1 etc)");                                                    
   rl->getVariable()->setPerInstance(true);   
   addRegisterLink(rl = new RegisterLink("S1ChannelSelect", baseAddress_ + (0x11*addrSize), Variable::Configuration));
   rl->getVariable()->setDescription("Input Mux select. Maximum number of channels is 29. (0 - Disabled, 1 - Test, 2-Ch0,3-Ch1 etc)");     
   rl->getVariable()->setPerInstance(true);
   // addRegisterLink(rl = new RegisterLink("S2ChannelSelect", baseAddress_ + (0x12*addrSize), Variable::Configuration));
   // rl->getVariable()->setDescription("Input Mux select. Maximum number of channels is 29. (0 - Disabled, 1 - Test, 2-Ch0,3-Ch1 etc)");     
   // rl->getVariable()->setPerInstance(true);
   // addRegisterLink(rl = new RegisterLink("S3ChannelSelect", baseAddress_ + (0x13*addrSize), Variable::Configuration));
   // rl->getVariable()->setDescription("Input Mux select. Maximum number of channels is 29. (0 - Disabled, 1 - Test, 2-Ch0,3-Ch1 etc)");     
   // rl->getVariable()->setPerInstance(true);  

   addRegisterLink(rl = new RegisterLink("StatusStream0", baseAddress_ + (0x20*addrSize), 1, 7,
                             "Pause0",        Variable::Status, 0, 0x1,
                             "Ready0",        Variable::Status, 1, 0x1,
                             "Overflow0",     Variable::Status, 2, 0x1,
                             "Error0",        Variable::Status, 3, 0x1,
                             "JESDValid0",    Variable::Status, 4, 0x1,
                             "Enabled0",      Variable::Status, 5, 0x1,                             
                             "FrameCnt0",    Variable::Status, 6,0x3ffffff));
                             
   rl->getVariable(0) ->setDescription("DAQ interface busy");
   rl->getVariable(1) ->setDescription("Ready to take new data packet");
   rl->getVariable(2) ->setDescription("Buffer overflow");
   rl->getVariable(3) ->setDescription("Error during last Acquisition. Raw diagnostic stream control Ready or incoming data valid dropped. Received buffer might be shorter.");
   rl->getVariable(4) ->setDescription("The incoming JESD data is Valid");
   rl->getVariable(5) ->setDescription("Stream 1-enabled, 0-Disabled");
   rl->getVariable(6) ->setDescription("Number of 4096 byte frames sent.");
   
   addRegisterLink(rl = new RegisterLink("StatusStream1", baseAddress_ + (0x21*addrSize), 1, 7,
                             "Pause1",        Variable::Status, 0, 0x1,
                             "Ready1",        Variable::Status, 1, 0x1,
                             "Overflow1",     Variable::Status, 2, 0x1,
                             "Error1",        Variable::Status, 3, 0x1,
                             "JESDValid1",    Variable::Status, 4, 0x1,
                             "Enabled1",      Variable::Status, 5, 0x1,                             
                             "FrameCnt1",     Variable::Status, 6,0x3ffffff));
   
   rl->getVariable(0) ->setDescription("DAQ interface busy");
   rl->getVariable(1) ->setDescription("Ready to take new data packet");
   rl->getVariable(2) ->setDescription("Buffer overflow");
   rl->getVariable(3) ->setDescription("Error during last Acquisition. Raw diagnostic stream control Ready or incoming data valid dropped. Received buffer might be shorter.");
   rl->getVariable(4) ->setDescription("The incoming JESD data is Valid");
   rl->getVariable(5) ->setDescription("Stream 1-enabled, 0-Disabled");
   rl->getVariable(6) ->setDescription("Number of frames sent. In the last transfer!");
   
   // addRegisterLink(rl = new RegisterLink("StatusStream2", baseAddress_ + (0x22*addrSize), 1, 7,
                             // "Pause2",        Variable::Status, 0, 0x1,
                             // "Ready2",        Variable::Status, 1, 0x1,
                             // "Overflow2",     Variable::Status, 2, 0x1,
                             // "Error2",        Variable::Status, 3, 0x1,
                             // "JESDValid2",    Variable::Status, 4, 0x1,
                             // "Enabled2",      Variable::Status, 5, 0x1,                             
                             // "FrameCnt2",    Variable::Status, 6,0x3ffffff));
                             
   // rl->getVariable(0) ->setDescription("DAQ interface busy");
   // rl->getVariable(1) ->setDescription("Ready to take new data packet");
   // rl->getVariable(2) ->setDescription("Buffer overflow");
   // rl->getVariable(3) ->setDescription("Error during last Acquisition. Raw diagnostic stream control Ready or incoming data valid dropped. Received buffer might be shorter.");
   // rl->getVariable(4) ->setDescription("The incoming JESD data is Valid");
   // rl->getVariable(5) ->setDescription("Stream 1-enabled, 0-Disabled");
   // rl->getVariable(6) ->setDescription("Number of 4096 byte frames sent.");
   
   // addRegisterLink(rl = new RegisterLink("StatusStream3", baseAddress_ + (0x23*addrSize), 1, 7,
                             // "Pause3",        Variable::Status, 0, 0x1,
                             // "Ready3",        Variable::Status, 1, 0x1,
                             // "Overflow3",     Variable::Status, 2, 0x1,
                             // "Error3",        Variable::Status, 3, 0x1,
                             // "JESDValid3",    Variable::Status, 4, 0x1,
                             // "Enabled3",      Variable::Status, 5, 0x1,                             
                             // "FrameCnt3",     Variable::Status, 6,0x3ffffff));
   
   // rl->getVariable(0) ->setDescription("DAQ interface busy");
   // rl->getVariable(1) ->setDescription("Ready to take new data packet");
   // rl->getVariable(2) ->setDescription("Buffer overflow");
   // rl->getVariable(3) ->setDescription("Error during last Acquisition. Raw diagnostic stream control Ready or incoming data valid dropped. Received buffer might be shorter.");
   // rl->getVariable(4) ->setDescription("The incoming JESD data is Valid");
   // rl->getVariable(5) ->setDescription("Stream 1-enabled, 0-Disabled");
   // rl->getVariable(6) ->setDescription("Number of frames sent. In the last transfer!");
   
   
   addRegisterLink(rl = new RegisterLink("S0DataFormat", baseAddress_ + (0x30*addrSize), 1, 4,
                          "S0FormatSignWidth",       Variable::Configuration, 0, 0x5,                             
                          "S0FormatDataWidth",       Variable::Configuration, 5, 0x1,
                          "S0FormatSign",            Variable::Configuration, 6, 0x1,
                          "S0DecimationAveraging",   Variable::Configuration, 7, 0x1));
                             
   rl->getVariable(0) ->setDescription("Indicating sign extension point.");
   rl->getVariable(0) ->setPerInstance(true);
   rl->getVariable(1) ->setDescription("Data width 32-bit or 16-bit.");
   rl->getVariable(1) ->setPerInstance(true);
   rl->getVariable(2) ->setDescription("Sign format. 0-Unsigned, 1-Signed");
   rl->getVariable(2) ->setPerInstance(true);
   rl->getVariable(3) ->setDescription("Decimation Averaging.");
   rl->getVariable(3) ->setPerInstance(true);

   addRegisterLink(rl = new RegisterLink("S1DataFormat", baseAddress_ + (0x31*addrSize), 1, 4,
                          "S1FormatSignWidth",       Variable::Configuration, 0, 0x5,                             
                          "S1FormatDataWidth",       Variable::Configuration, 5, 0x1,
                          "S1FormatSign",            Variable::Configuration, 6, 0x1,
                          "S1DecimationAveraging",   Variable::Configuration, 7, 0x1));
                             
   rl->getVariable(0) ->setDescription("Indicating sign extension point.");
   rl->getVariable(0) ->setPerInstance(true);
   rl->getVariable(1) ->setDescription("Data width 32-bit or 16-bit.");
   rl->getVariable(1) ->setPerInstance(true);
   rl->getVariable(2) ->setDescription("Sign format. 0-Unsigned, 1-Signed");
   rl->getVariable(2) ->setPerInstance(true);
   rl->getVariable(3) ->setDescription("Decimation Averaging.");
   rl->getVariable(3) ->setPerInstance(true);
   
   
   // Variables
   getVariable("Enabled")->setHidden(true);
   
   //Commands
   addCommand(c = new Command("TriggerDAQ"));
   c->setDescription("Trigger data acquisition.");
   addCommand(c = new Command("ArmHwTrigger"));
   c->setDescription("Arm Hw Trigger.");
   addCommand(c = new Command("FreezeBuffers"));
   c->setDescription("Freeze circular buffers.");
}

// Deconstructor
DaqMuxV2::~DaqMuxV2 ( ) { }

// Process Commands
void DaqMuxV2::command(string name, string arg) {
   if      (name == "TriggerDAQ")   trigDaq();
   else if (name == "ArmHwTrigger") trigArm();
   else if (name == "FreezeBuffers")freeze();
   else Device::command(name,arg);
}

//! Sw Trigger 
void DaqMuxV2::trigDaq () {

   Register *r;
   REGISTER_LOCK
   r = getRegister("ControlReg");
   r->set(0x1,0,0x1);
   writeRegister(r, true);
   r->set(0x0,0,0x1);
   writeRegister(r, true);
   REGISTER_UNLOCK

}

//! Arm Hw Trigger 
void DaqMuxV2::trigArm () {

   Register *r;
   REGISTER_LOCK
   r = getRegister("ControlReg");
   r->set(0x1,3,0x1);
   writeRegister(r, true);
   r->set(0x0,3,0x1);
   writeRegister(r, true);
   REGISTER_UNLOCK

}

//! Arm Hw Trigger 
void DaqMuxV2::freeze () {

   Register *r;
   REGISTER_LOCK
   r = getRegister("ControlReg");
   r->set(0x1,7,0x1);
   writeRegister(r, true);
   r->set(0x0,7,0x1);
   writeRegister(r, true);
   REGISTER_UNLOCK

}
