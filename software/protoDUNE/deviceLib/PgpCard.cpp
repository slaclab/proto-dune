//-----------------------------------------------------------------------------
// File          : PgpCard.cpp
// Author        : Ryan Herbst  <rherbst@slac.stanford.edu>
// Created       : 08/19/2016
// Project       : General Purpose
//-----------------------------------------------------------------------------
// Description :
// PGP Card Controller
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
//
// DATE        WHO  WHAT
// ----------  ---  -----------------------------------------------------------
// 2016.09.10  jjr  Add inttypes.h to properly format uint64_t types
//-----------------------------------------------------------------------------


#include <Variable.h>
#include <sstream>
#include <iostream>
#include <string>
#include <iomanip>
#include <PgpCard.h>
#include <PgpCardLane.h>
#include <System.h>
#include <Command.h>
#include <PgpDriver.h>


// ---------------------------------------------
// 2016.09.10 -- jjr
// -----------------
// Added to properly format the uint<nn>_t types
// ---------------------------------------------
#define __STDC_FORMAT_MACROS
#include <inttypes.h>


using namespace std;

// Constructor
PgpCard::PgpCard ( Device *parent, string path, uint32_t mask ) : Device(0,0,"pgpCard",0,parent) {
   stringstream tmp;
   Variable *v;
   Command  *c;
   uint32_t x;

   // Description
   desc_ = "PGP Card Control";
   path_ = path;
   mask_ = mask;

   if ( (fd_ = ::open(path_.c_str(),O_RDWR | O_NONBLOCK)) < 0 ) {
      tmp.str("");
      tmp << "PgpCard::PgpCard -> Could Not Open PGP path " << path_;
      throw tmp.str();
   }

   pgpGetInfo(fd_,&info_);

   addVariable(v = new Variable("Serial",     Variable::Status));
   v->setDescription("Card serial number");

   addVariable(v = new Variable("Type",       Variable::Status));
   v->setDescription("PGP Card Type");

   addVariable(v = new Variable("Version",    Variable::Status));
   v->setDescription("Card Version Field");

   addVariable(v = new Variable("LaneMask",   Variable::Status));
   v->setDescription("Mask of available lanes");

   addVariable(v = new Variable("VcPerMask",  Variable::Status));
   v->setDescription("Mask of available VCs in each lane");

   addVariable(v = new Variable("PgpRate",    Variable::Status));
   v->setDescription("PGP Lane rate in Mbps");

   addVariable(v = new Variable("PromPrgEn",  Variable::Status));
   v->setDescription("PROM Programming Supported");
   v->setTrueFalse();

   addVariable(v = new Variable("EvrSupport", Variable::Status));
   v->setDescription("EVR functions supported");
   v->setTrueFalse();

   addVariable(v = new Variable("BuildStamp", Variable::Status));
   v->setDescription("Build stamp");

   addCommand(c = new Command("SendOpCode"));
   c->setDescription("Send zero opcode");

   for (x=0; x < 8; x++) {
      if ( ( mask_ & (0xF << (x*4))) != 0 &&
           ( info_.laneMask & (0x1 << (x))) != 0 ) addDevice(new PgpCardLane(this,fd_,x,info_.evrSupport));
   }

   pollEnable(true);
   getVariable("Enabled")->setHidden(true);
}

// Deconstructor
PgpCard::~PgpCard ( ) { }

// Method to read status registers and update variables
void PgpCard::readStatus ( ) {
   char buffer[100];

   sprintf(buffer,"0x%.16" PRIx64 ,info_.serial);
   getVariable("Serial")->set(buffer);

   getVariable("Type")->setInt(info_.type);
   getVariable("Version")->setInt(info_.version);
   getVariable("LaneMask")->setInt(info_.laneMask);
   getVariable("VcPerMask")->setInt(info_.vcPerMask);

   sprintf(buffer,"%i",info_.pgpRate);
   getVariable("PgpRate")->set(buffer);

   getVariable("PromPrgEn")->setInt(info_.promPrgEn);
   getVariable("EvrSupport")->setInt(info_.evrSupport);

   getVariable("BuildStamp")->set(info_.buildStamp);

   Device::readStatus();
}

void PgpCard::pollStatus ( ) {
   readStatus();
}

// Method to process a command
void PgpCard::command ( string name, string arg) {

   if ( name == "SendOpCode" ) {
      pgpSendOpCode(fd_,0);
   }
   else Device::command(name, arg);
}

void PgpCard::softReset() {
   pgpCountReset(fd_);
   Device::softReset();
}

