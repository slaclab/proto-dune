//-----------------------------------------------------------------------------
// File          : TimingTrigPulse.cpp
// Author        : Uros legat <ulegat@slac.stanford.edu>
//                            <uros.legat@cosylab.com>
// Created       : 7/19/2016
// Project       : 
//-----------------------------------------------------------------------------
// Description :
//    Device container for pulse generator
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
#include <TimingTrigPulse.h>
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
TimingTrigPulse::TimingTrigPulse ( uint32_t linkConfig, uint32_t baseAddress, uint32_t index, Device *parent, uint32_t addrSize ) : 
                        Device(linkConfig,baseAddress,"TimingTrigPulse",index,parent) {
   //Command      *c;
   // Create Registers: name, address
   RegisterLink *rl;
   
   // Description
   desc_ = "Timing Trigger pulse generator";

   addRegisterLink(rl = new RegisterLink("OpCodeMask31_0", baseAddress_ + (0x00*addrSize), Variable::Configuration));
   rl->getVariable()->setDescription("Opcode mask 31 downto 0");
   rl->getVariable()->setPerInstance(true);

   addRegisterLink(rl = new RegisterLink("OpCodeMask63_32", baseAddress_ + (0x01*addrSize), Variable::Configuration));
   rl->getVariable()->setDescription("Opcode mask 63 downto 32");
   rl->getVariable()->setPerInstance(true);

   addRegisterLink(rl = new RegisterLink("OpCodeMask95_64", baseAddress_ + (0x02*addrSize), Variable::Configuration));
   rl->getVariable()->setDescription("Opcode mask 95 downto 64");
   rl->getVariable()->setPerInstance(true);

   addRegisterLink(rl = new RegisterLink("OpCodeMask127_96", baseAddress_ + (0x03*addrSize), Variable::Configuration));
   rl->getVariable()->setDescription("Opcode mask 127 downto 96");
   rl->getVariable()->setPerInstance(true);

   addRegisterLink(rl = new RegisterLink("OpCodeMask159_128", baseAddress_ + (0x04*addrSize), Variable::Configuration));
   rl->getVariable()->setDescription("Opcode mask 159 downto 128");
   rl->getVariable()->setPerInstance(true);

   addRegisterLink(rl = new RegisterLink("OpCodeMask191_160", baseAddress_ + (0x05*addrSize), Variable::Configuration));
   rl->getVariable()->setDescription("Opcode mask 191 downto 160");
   rl->getVariable()->setPerInstance(true);

   addRegisterLink(rl = new RegisterLink("OpCodeMask223_192", baseAddress_ + (0x06*addrSize), Variable::Configuration));
   rl->getVariable()->setDescription("Opcode mask 223 downto 192");
   rl->getVariable()->setPerInstance(true);

   addRegisterLink(rl = new RegisterLink("OpCodeMask255_224", baseAddress_ + (0x07*addrSize), Variable::Configuration));
   rl->getVariable()->setDescription("Opcode mask 255 downto 224");
   rl->getVariable()->setPerInstance(true);

   
   addRegisterLink(rl = new RegisterLink("PulseDelay", baseAddress_ + (0x08*addrSize), Variable::Configuration));
   rl->getVariable()->setDescription("Pulse delay (Number of recovered clock cycles)");
   rl->getVariable()->setPerInstance(true);

   addRegisterLink(rl = new RegisterLink("PulseWidth", baseAddress_ + (0x09*addrSize), Variable::Configuration));
   rl->getVariable()->setDescription("Pulse Width (Number of recovered clock cycles)");
   rl->getVariable()->setPerInstance(true);
   
   
   // Variables
   getVariable("Enabled")->setHidden(true);
   

}

// Deconstructor
TimingTrigPulse::~TimingTrigPulse ( ) { }