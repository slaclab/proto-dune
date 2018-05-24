//-----------------------------------------------------------------------------
// File          : RssiReg.cpp
// Author        : Uros legat <ulegat@slac.stanford.edu>
//                            <uros.legat@cosylab.com>
// Created       : 20/01/2016
// Project       : 
//-----------------------------------------------------------------------------
// Description :
//    Device container for RSSI registers 
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
// 20/01/2016: created
//-----------------------------------------------------------------------------
#include <RssiReg.h>
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
RssiReg::RssiReg ( uint32_t linkConfig, uint32_t baseAddress, uint32_t index, Device *parent, uint32_t addrSize ) : 
                        Device(linkConfig,baseAddress,"RssiReg",index,parent) {

   // Description
   desc_ = "RSSI interface";

   // Create Registers: name, address
   RegisterLink *rl;
   Command      *c; 
   
   // Configuration registers
   addRegisterLink(rl = new RegisterLink("ConfigurationReg",   baseAddress_ + 0x00, 1, 5,
                                "OpenConn",       Variable::Configuration, 0, 0x1,
                                "CloseConn",      Variable::Configuration, 1, 0x1,
                                "Mode",           Variable::Configuration, 2, 0x1,
                                "HeaderChksumEn", Variable::Configuration, 3, 0x1,
                                "InjectFault",    Variable::Configuration, 4, 0x1));
                                
   rl->getVariable(0)->setPerInstance(true);                               
   rl->getVariable(0)->setDescription("Open Connection Request (Server goes to listen state, Client actively requests the connection by sending SYN segment)");
   rl->getVariable(1)->setPerInstance(true);   
   rl->getVariable(1)->setDescription("Close Connection Request (Send a RST Segment to peer and close the connection)");
   rl->getVariable(2)->setPerInstance(true);
   rl->getVariable(2)->setDescription("Mode:'0': Use internal parameters from generics,'1': Use parameters from registers ");
   rl->getVariable(3)->setPerInstance(true);
   rl->getVariable(3)->setDescription("Header checksum: '1': Enable calculation and check, '0': Disable check and insert 0 in place of header checksum");
   rl->getVariable(4)->setPerInstance(true);
   rl->getVariable(4)->setDescription("Inject fault to the next packet header checksum (Default '0'). Acts on rising edge - injects exactly one fault in next segment");
  
   addRegisterLink(rl = new RegisterLink("InitSeqN",           baseAddress_ + 0x04, Variable::Configuration));
   rl->getVariable()->setDescription("Initial sequence number[3:0](Default 0x80)");
   rl->getVariable()->setPerInstance(true);
   
   addRegisterLink(rl = new RegisterLink("Version",            baseAddress_ + 0x08, Variable::Configuration));
   rl->getVariable()->setDescription("Version register [3:0](Default 0x1)");
   
   addRegisterLink(rl = new RegisterLink("MaxOutstandingSeg",  baseAddress_ + 0x0C, Variable::Configuration));
   rl->getVariable()->setDescription("Maximum out standing segments [7:0](Default 0x8). Defines the max number of segments in the RSSI receiver buffer.");
   
   addRegisterLink(rl = new RegisterLink("MaxSegSize",         baseAddress_ + 0x10, Variable::Configuration));
   rl->getVariable()->setDescription("Maximum segment size [15:0](Default 0x4). Defines the size of segment buffer! Number of bytes! ");
   
   addRegisterLink(rl = new RegisterLink("ToutRetransmission", baseAddress_ + 0x14, Variable::Configuration));
   rl->getVariable()->setDescription("Retransmission timeout [15:0](Default 50ms). Unit depends on TIMEOUT_UNIT_G! ");
   
   addRegisterLink(rl = new RegisterLink("ToutCumulativeAck",  baseAddress_ + 0x18, Variable::Configuration));
   rl->getVariable()->setDescription("Cumulative acknowledgment timeout [15:0](Default 50ms). Unit depends on TIMEOUT_UNIT_G!");
   
   addRegisterLink(rl = new RegisterLink("ToutNullSegment",    baseAddress_ + 0x1C, Variable::Configuration));
   rl->getVariable()->setDescription("Null segment timeout [15:0](Default 50ms). Unit depends on TIMEOUT_UNIT_G!");
   
   addRegisterLink(rl = new RegisterLink("MaxRetransmissions", baseAddress_ + 0x20, Variable::Configuration));
   rl->getVariable()->setDescription("Maximum number of retransmissions [7:0](Default 0x02). How many times segments are retransmitted before the connection gets broken!");
   
   addRegisterLink(rl = new RegisterLink("MaxCumulativeAcks",  baseAddress_ + 0x24, Variable::Configuration));
   rl->getVariable()->setDescription("Maximum cumulative acknowledgments [7:0](Default 0x03). When more than maxCumAck are received and not acknowledged the ACK packet will be sent to acknowledge the received packets. Even though the cumulative acknowledgment timeout has not been reached yet!");
   
   addRegisterLink(rl = new RegisterLink("MaxOutOfSeqSeg",     baseAddress_ + 0x28, Variable::Configuration));
   rl->getVariable()->setDescription("Max out of sequence segments (EACK) [7:0](Default 0x03). Currently not used!");
   
   // Status registers   
   addRegisterLink(rl = new RegisterLink("StatusReg", baseAddress_ + 0x40, 1, 7,
                                "ConnectionActive",Variable::Status, 0, 0x1,
                                "ErrMaxRetrans",   Variable::Status, 1, 0x1,
                                "ErrNullTout",     Variable::Status, 2, 0x1,
                                "ErrAck",          Variable::Status, 3, 0x1,
                                "ErrSsiFrameLen",  Variable::Status, 4, 0x1,
                                "ErrConnTout",     Variable::Status, 5, 0x1,
                                "ParamRejected",   Variable::Status, 6, 0x1));
                                
   rl->getVariable(0)->setDescription("Connection Active");
   rl->getVariable(1)->setDescription("Maximum retransmissions exceeded retransMax.");
   rl->getVariable(2)->setDescription("Null timeout reached (server) nullTout.");
   rl->getVariable(3)->setDescription("Error in acknowledgment mechanism.");
   rl->getVariable(4)->setDescription("SSI Frame length too long");
   rl->getVariable(5)->setDescription("Connection to peer timed out. Timeout defined in generic PEER_CONN_TIMEOUT_G (Default: 1000 ms)");
   rl->getVariable(6)->setDescription("Client rejected the connection (parameters out of range), Server proposed new parameters (parameters out of range)");
   rl->setPollEnable(true);   

   addRegisterLink(rl = new RegisterLink("ValidSegCnt", baseAddress_ + 0x44, Variable::Status));
   rl->getVariable()->setDescription("Number of valid segments [31:0]. The value rests to 0 when new connection open is requested.");   
   rl->setPollEnable(true);   
   
   addRegisterLink(rl = new RegisterLink("DropSegCnt",  baseAddress_ + 0x48, Variable::Status));
   rl->getVariable()->setDescription("Number of dropped segments [31:0]. The value rests to 0 when new connection open is requested.");
   rl->setPollEnable(true);   
   
   addRegisterLink(rl = new RegisterLink("RetransmitCnt",  baseAddress_ + 0x4C, Variable::Status));
   rl->getVariable()->setDescription("Counts all retransmission requests within the active connection");   
   rl->setPollEnable(true);   
   
   addRegisterLink(rl = new RegisterLink("ReconnectCnt",  baseAddress_ + 0x50, Variable::Status));
   rl->getVariable()->setDescription("Counts all reconnections from reset");   
   rl->setPollEnable(true);   
   
   addRegisterLink(rl = new RegisterLink("IbFrameRate",  baseAddress_ + 0x54, Variable::Status));
   rl->getVariable()->setDescription("Inbound Frame Rate (in units of Hz)");    
	rl->getVariable()->setComp(0,1,0,"");
   rl->setPollEnable(true);   
   
   addRegisterLink(rl = new RegisterLink("ObFrameRate",  baseAddress_ + 0x58, Variable::Status));
   rl->getVariable()->setDescription("Outbound Frame Rate (in units of Hz)");   
	rl->getVariable()->setComp(0,1,0,"");
   rl->setPollEnable(true);   
   
   addRegisterLink(rl = new RegisterLink("IbBandwidth",  baseAddress_ + 0x5C, 2, Variable::Status));
   rl->getVariable()->setDescription("Inbound Bandwidth (in units of bytes per second)");    
	rl->getVariable()->setComp(0,1,0,"");
   rl->setPollEnable(true);   
   
   addRegisterLink(rl = new RegisterLink("ObBandwidth",  baseAddress_ + 0x64, 2, Variable::Status));
   rl->getVariable()->setDescription("Outbound Bandwidth (in units of bytes per second)");
	rl->getVariable()->setComp(0,1,0,"");
   rl->setPollEnable(true);      

   // Variables
   getVariable("Enabled")->setHidden(true);
   
   //Commands
   addCommand(c = new Command("OpenConn"));
   c->setDescription("Open connection request.");
   
   addCommand(c = new Command("CloseConn"));
   c->setDescription("Close connection request.");
   
   addCommand(c = new Command("InjectFault"));
   c->setDescription("Inject a single fault(for debug and test purposes only). Corrupts checksum during transmission");
   
}

// Deconstructor
RssiReg::~RssiReg ( ) { }

// Process Commands
void RssiReg::command(string name, string arg) {
   if (name == "OpenConn") openConn();
   else if (name == "CloseConn") closeConn();
   else if (name == "InjectFault") injectFault();
   else Device::command(name,arg);
}

//! Open connection
void RssiReg::openConn () {

   Register *r;
   REGISTER_LOCK
   r = getRegister("ConfigurationReg");
   r->set(0x1,0,0x1);
   writeRegister(r, true);
   r->set(0x0,0,0x1);
   writeRegister(r, true);
   REGISTER_UNLOCK
}

//! Close connection
void RssiReg::closeConn () {

   Register *r;
   REGISTER_LOCK
   r = getRegister("ConfigurationReg");
   r->set(0x1,1,0x1);
   writeRegister(r, true);
   r->set(0x0,1,0x1);
   writeRegister(r, true);
   REGISTER_UNLOCK
}

//! Inject fault
void RssiReg::injectFault () {

   Register *r;
   REGISTER_LOCK
   r = getRegister("ConfigurationReg");
   r->set(0x1,4,0x1);
   writeRegister(r, true);
   r->set(0x0,4,0x1);
   writeRegister(r, true);
   REGISTER_UNLOCK

}
