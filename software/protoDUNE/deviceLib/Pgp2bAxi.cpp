//-----------------------------------------------------------------------------
// File          : Pgp2bAxi.cpp
// Author        : Ben Reese <bareese@slac.stanford.edu>
// Created       : 11/14/2013
// Project       : HPS SVT
//-----------------------------------------------------------------------------
// Description :
// Device container for Pgp2bAxi.vhd
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
#include <Pgp2bAxi.h>
#include <Register.h>
#include <Variable.h>
#include <RegisterLink.h>
#include <Command.h>
#include <sstream>
#include <iostream>
#include <string>
#include <iomanip>
using namespace std;

// Constructor
Pgp2bAxi::Pgp2bAxi ( uint32_t linkConfig, uint32_t baseAddress, uint32_t index, Device *parent, uint32_t addrSize ) : 
                        Device(linkConfig,baseAddress,"Pgp2bAxi",index,parent) {

   Command      *c;
   Variable     *v;
   RegisterLink *rl;

   // Description
   desc_ = "Firmware Version object.";

   // Create Registers: name, address
   addRegister(new Register("CountReset", baseAddress_ + (0x00*addrSize)));
   addRegister(new Register("ResetRx",    baseAddress_ + (0x01*addrSize)));
   addRegister(new Register("Flush",      baseAddress_ + (0x02*addrSize)));

   addRegisterLink(rl = new RegisterLink("Loopback", baseAddress_ + (0x03*addrSize), Variable::Configuration));
   rl->getVariable()->setDescription("Controls the GT loopback mode");
   map<uint32_t,string> loopModes;
   loopModes[0] = "Off";
   loopModes[1] = "NearPcs";
   loopModes[2] = "NearPma";
   loopModes[4] = "FarPma";
   loopModes[5] = "FarPcs";
   rl->getVariable()->setMap(loopModes);
   rl->getVariable()->setPerInstance(true);
   rl->getVariable()->setInt(0);

   addRegisterLink(rl = new RegisterLink("LocData", baseAddress_ + (0x04*addrSize), 1, 2,
                                         "LocData",   Variable::Configuration, 0, 0xFF,
                                         "LocDataEn", Variable::Configuration, 8, 0x1 ));
   rl->getVariable(0)->setDescription("Sideband data to transmit");
   rl->getVariable(0)->setPerInstance(true);
   rl->getVariable(1)->setDescription("Enable Sideband data to transmit");
   rl->getVariable(1)->setTrueFalse();
   rl->getVariable(1)->setPerInstance(true);

   addRegisterLink(rl = new RegisterLink("AutoStatus", baseAddress_ + (0x05*addrSize), Variable::Configuration));
   rl->getVariable()->setDescription("Auto Status Send Enable (PPI)");
   rl->getVariable()->setTrueFalse();
   rl->getVariable()->setPerInstance(true);
   
   addRegisterLink(rl = new RegisterLink("Status", baseAddress_ + (0x08*addrSize), 1, 10,
                                         "RxPhyReady",      Variable::Status,  0, 0x1,   // 0
                                         "TxPhyReady",      Variable::Status,  1, 0x1,   // 1
                                         "RxLocalLinkReady",Variable::Status,  2, 0x1,   // 2
                                         "RxRemLinkReady",  Variable::Status,  3, 0x1,   // 3
                                         "TxLinkReady",     Variable::Status,  4, 0x1,   // 4
                                         "RxLinkPolarity",  Variable::Status,  8, 0x3,   // 5
                                         "RxRemPause",      Variable::Status, 12, 0xF,   // 6
                                         "TxLocPause",      Variable::Status, 16, 0xF,   // 7
                                         "RxRemOverflow",   Variable::Status, 20, 0xF,   // 8
                                         "TxLocOverflow",   Variable::Status, 24, 0xF)); // 9

   rl->getVariable(0)->setTrueFalse();
   rl->getVariable(0)->setDescription("RX Phy Is ready");
   rl->getVariable(1)->setTrueFalse();
   rl->getVariable(1)->setDescription("TX Phy Is ready");
   rl->getVariable(2)->setTrueFalse();
   rl->getVariable(2)->setDescription("RX Local Link Ready");
   rl->getVariable(3)->setTrueFalse();
   rl->getVariable(3)->setDescription("RX Remote Link Ready");
   rl->getVariable(4)->setTrueFalse();
   rl->getVariable(4)->setDescription("TX Link Ready");
   rl->getVariable(5)->setDescription("RX Link Polarity");
   rl->getVariable(6)->setDescription("RX Remote Pause Is Asserted");
   rl->getVariable(7)->setDescription("TX Local Pause Is Asserted");
   rl->getVariable(8)->setDescription("Received remote overlflow flag");
   rl->getVariable(9)->setDescription("Transmit local overlflow flag");
   rl->setPollEnable(true);

   addRegisterLink(rl = new RegisterLink("RxRemLinkData",       baseAddress_ + (0x09*addrSize), Variable::Status));
   rl->getVariable()->setDescription("Link data received from remote link");

   addRegisterLink(rl = new RegisterLink("RxCellErrorCount",    baseAddress_ + (0x0A*addrSize), Variable::Status));
   rl->getVariable()->setDescription("Receive Cell Error Count");
   rl->getVariable()->setComp(0,1,0,"");
   rl->setPollEnable(true);

   addRegisterLink(rl = new RegisterLink("RxLinkDownCount",     baseAddress_ + (0x0B*addrSize), Variable::Status));
   rl->getVariable()->setDescription("Receive Link Down Count");
   rl->getVariable()->setComp(0,1,0,"");
   rl->setPollEnable(true);

   addRegisterLink(rl = new RegisterLink("RxLinkErrorCount",    baseAddress_ + (0x0C*addrSize), Variable::Status));
   rl->getVariable()->setDescription("Receive Link Error Count");
   rl->getVariable()->setComp(0,1,0,"");
   rl->setPollEnable(true);

   addRegisterLink(rl = new RegisterLink("RxRemOverflow0Count", baseAddress_ + (0x0D*addrSize), Variable::Status));
   rl->getVariable()->setDescription("Receive Remote Overflow VC 0 Counter");
   rl->getVariable()->setComp(0,1,0,"");

   addRegisterLink(rl = new RegisterLink("RxRemOverflow1Count", baseAddress_ + (0x0E*addrSize), Variable::Status));
   rl->getVariable()->setDescription("Receive Remote Overflow VC 1 Counter");
   rl->getVariable()->setComp(0,1,0,"");

   addRegisterLink(rl = new RegisterLink("RxRemOverflow2Count", baseAddress_ + (0x0F*addrSize), Variable::Status));
   rl->getVariable()->setDescription("Receive Remote Overflow VC 2 Counter");
   rl->getVariable()->setComp(0,1,0,"");

   addRegisterLink(rl = new RegisterLink("RxRemOverflow3Count", baseAddress_ + (0x10*addrSize), Variable::Status));
   rl->getVariable()->setDescription("Receive Remote Overflow VC 3 Counter");
   rl->getVariable()->setComp(0,1,0,"");

   addRegisterLink(rl = new RegisterLink("RxFrameErrorCount",   baseAddress_ + (0x11*addrSize), Variable::Status));
   rl->getVariable()->setDescription("Receive Frame Error Counter");
   rl->getVariable()->setComp(0,1,0,"");
   rl->setPollEnable(true);

   addRegisterLink(rl = new RegisterLink("RxFrameCount",        baseAddress_ + (0x12*addrSize), Variable::Status));
   rl->getVariable()->setDescription("Receive Frame Counter");
   rl->getVariable()->setComp(0,1,0,"");
   rl->setPollEnable(true);
   
   addRegisterLink(rl = new RegisterLink("TxLocOverflow0Count", baseAddress_ + (0x13*addrSize), Variable::Status));
   rl->getVariable()->setDescription("Local Overflow VC 0 Counter");
   rl->getVariable()->setComp(0,1,0,"");

   addRegisterLink(rl = new RegisterLink("TxLocOverflow1Count", baseAddress_ + (0x14*addrSize), Variable::Status));
   rl->getVariable()->setDescription("Local Overflow VC 1 Counter");
   rl->getVariable()->setComp(0,1,0,"");

   addRegisterLink(rl = new RegisterLink("TxLocOverflow2Count", baseAddress_ + (0x15*addrSize), Variable::Status));
   rl->getVariable()->setDescription("Local Overflow VC 2 Counter");
   rl->getVariable()->setComp(0,1,0,"");

   addRegisterLink(rl = new RegisterLink("TxLocOverflow3Count", baseAddress_ + (0x16*addrSize), Variable::Status));
   rl->getVariable()->setDescription("Local Overflow VC 3 Counter");
   rl->getVariable()->setComp(0,1,0,"");

   addRegisterLink(rl = new RegisterLink("TxFrameErrorCount",   baseAddress_ + (0x17*addrSize), Variable::Status));
   rl->getVariable()->setDescription("Transmit frame error counter");
   rl->getVariable()->setComp(0,1,0,"");

   addRegisterLink(rl = new RegisterLink("TxFrameCount",        baseAddress_ + (0x18*addrSize), Variable::Status));
   rl->getVariable()->setDescription("Transmit frame counter");
   rl->getVariable()->setComp(0,1,0,"");

   addRegisterLink(rl = new RegisterLink("RxClkFreq",           baseAddress_ + (0x19*addrSize), Variable::Status));
   rl->getVariable()->setDescription("Receive clock frequency");
   rl->getVariable()->setComp(0,1E-6,0,"MHz");

   addRegisterLink(rl = new RegisterLink("TxClkFreq",           baseAddress_ + (0x1A*addrSize), Variable::Status));
   rl->getVariable()->setDescription("Transmit clock frequency");
   rl->getVariable()->setComp(0,1E-6,0,"MHz");

   addRegisterLink(rl = new RegisterLink("TxOpCodeStatus",  baseAddress_ + (0x1C*addrSize), 1, 1,
                                         "LastTxOpCode",    Variable::Status,  0, 0xFF));

   addRegisterLink(rl = new RegisterLink("RxOpCodeStatus",  baseAddress_ + (0x1D*addrSize), 1, 1,
                                         "LastRxOpCode",    Variable::Status,  0, 0xFF));        
                                         
   addRegisterLink(rl = new RegisterLink("TxOpCodeCount",   baseAddress_ + (0x1E*addrSize), Variable::Status));
   rl->getVariable()->setComp(0,1,0,"");

   addRegisterLink(rl = new RegisterLink("RxOpCodeCount",   baseAddress_ + (0x1F*addrSize), Variable::Status));
   rl->getVariable()->setComp(0,1,0,"");                                         
                                         
   v = getVariable("Enabled");
   v->set("True");

   addCommand(c = new Command("ResetRx"));
   c->setDescription("");

   addCommand(c = new Command("Flush"));
   c->setDescription("");

   addCommand(c = new Command("CountReset"));
   c->setDescription("");
}

// Deconstructor
Pgp2bAxi::~Pgp2bAxi ( ) { }

// Process Commands
void Pgp2bAxi::command(string name, string arg) {
   if (name == "ResetRx") hardReset();
   else if (name == "Flush") softReset();
   else if (name == "CountReset") countReset();
   else Device::command(name,arg);
}

//! Hard Reset
void Pgp2bAxi::hardReset () {

   Register *r;
   REGISTER_LOCK
   r = getRegister("ResetRx");
   r->set(0x1);
   writeRegister(r, true, false);
   r->set(0x0);
   writeRegister(r, true, false);
   REGISTER_UNLOCK

   Device::hardReset();
}

//! Soft Reset
void Pgp2bAxi::softReset () {
   Device::softReset();
}

//! Count Reset
void Pgp2bAxi::countReset () {
   Register *r;

   REGISTER_LOCK
   r = getRegister("CountReset");
   r->set(0x1);
   writeRegister(r, true, false);
   r->set(0x0);
   writeRegister(r, true, false);
   REGISTER_UNLOCK
   Device::countReset();
}

// Dump Debug
void Pgp2bAxi::dumpDebug () {
   this->readStatus();

   printf("-------------- Pgp2bAxi -------------\n");
   printf("          RxPhyReady : %s\n",get("RxPhyReady").c_str());
   printf("          TxPhyReady : %s\n",get("TxPhyReady").c_str());
   printf("    RxLocalLinkReady : %s\n",get("RxLocalLinkReady").c_str());
   printf("      RxRemLinkReady : %s\n",get("RxRemLinkReady").c_str());
   printf("         TxLinkReady : %s\n",get("TxLinkReady").c_str());
   printf("      RxLinkPolarity : %s\n",get("RxLinkPolarity").c_str());
   printf("          RxRemPause : %s\n",get("RxRemPause").c_str());
   printf("          TxLocPause : %s\n",get("TxLocPause").c_str());
   printf("       RxRemOverflow : %s\n",get("RxRemOverflow").c_str());
   printf("       TxLocOverflow : %s\n",get("TxLocOverflow").c_str());
   printf("       RxRemLinkData : %s\n",get("RxRemLinkData").c_str());
   printf("    RxCellErrorCount : %s\n",get("RxCellErrorCount").c_str());
   printf("     RxLinkDownCount : %s\n",get("RxLinkDownCount").c_str());
   printf("    RxLinkErrorCount : %s\n",get("RxLinkErrorCount").c_str());
   printf(" RxRemOverflow0Count : %s\n",get("RxRemOverflow0Count").c_str());
   printf(" RxRemOverflow1Count : %s\n",get("RxRemOverflow1Count").c_str());
   printf(" RxRemOverflow2Count : %s\n",get("RxRemOverflow2Count").c_str());
   printf(" RxRemOverflow3Count : %s\n",get("RxRemOverflow3Count").c_str());
   printf("   RxFrameErrorCount : %s\n",get("RxFrameErrorCount").c_str());
   printf("        RxFrameCount : %s\n",get("RxFrameCount").c_str());
   printf(" TxLocOverflow0Count : %s\n",get("TxLocOverflow0Count").c_str());
   printf(" TxLocOverflow1Count : %s\n",get("TxLocOverflow1Count").c_str());
   printf(" TxLocOverflow2Count : %s\n",get("TxLocOverflow2Count").c_str());
   printf(" TxLocOverflow3Count : %s\n",get("TxLocOverflow3Count").c_str());
   printf("   TxFrameErrorCount : %s\n",get("TxFrameErrorCount").c_str());
   printf("        TxFrameCount : %s\n",get("TxFrameCount").c_str());
   printf("           RxClkFreq : %s\n",get("RxClkFreq").c_str());
   printf("           TxClkFreq : %s\n",get("TxClkFreq").c_str());
   printf("        LastTxOpCode : %s\n",get("LastTxOpCode").c_str());
   printf("        LastRxOpCode : %s\n",get("LastRxOpCode").c_str());
   printf("       TxOpCodeCount : %s\n",get("TxOpCodeCount").c_str());
   printf("       RxOpCodeCount : %s\n",get("RxOpCodeCount").c_str());
                                         
}

