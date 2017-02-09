//-----------------------------------------------------------------------------
// File          : PgpCardLane.cpp
// Author        : Ryan Herbst  <rherbst@slac.stanford.edu>
// Created       : 08/19/2016
// Project       : General Purpose
//-----------------------------------------------------------------------------
// Description :
// PGP Card Lane Controller
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
#include <Variable.h>
#include <sstream>
#include <iostream>
#include <string>
#include <iomanip>
#include <PgpCardLane.h>
#include <System.h>
#include <Command.h>
#include <PgpDriver.h>
using namespace std;

// Constructor
PgpCardLane::PgpCardLane ( Device *parent, int32_t fd, uint32_t index, bool evrEn ) : Device(0,0,"pgpLane",index,parent) {
   stringstream tmp;
   PgpInfo info_;
   Variable *v;

   // Description
   desc_  = "PGP Lane Control";
   fd_    = fd;
   evrEn_ = evrEn;

   pgpGetInfo(fd_,&info_);

   addVariable(v = new Variable("loopBack",   Variable::Configuration));
   v->setDescription("Loopback enable");
   v->setTrueFalse();

   addVariable(v = new Variable("locData",   Variable::Configuration));
   v->setDescription("Local data send value");

   addVariable(v = new Variable("locLinkready",   Variable::Status));
   v->setDescription("Local link is ready");
   v->setTrueFalse();

   addVariable(v = new Variable("remLinkready",   Variable::Status));
   v->setDescription("Remote link is ready");
   v->setTrueFalse();

   addVariable(v = new Variable("rxReady",   Variable::Status));
   v->setDescription("RX path is ready");
   v->setTrueFalse();

   addVariable(v = new Variable("txReady",   Variable::Status));
   v->setDescription("TX path is ready");
   v->setTrueFalse();

   addVariable(v = new Variable("rxCount",   Variable::Status));
   v->setDescription("RX counter");

   addVariable(v = new Variable("cellErrorCount",   Variable::Status));
   v->setDescription("Cell error counter");

   addVariable(v = new Variable("linkDownCount",   Variable::Status));
   v->setDescription("Link down counter");

   addVariable(v = new Variable("linkErrorCount",   Variable::Status));
   v->setDescription("Link error counter");

   addVariable(v = new Variable("fifoError",   Variable::Status));
   v->setDescription("FIFO error status");
   v->setTrueFalse();

   addVariable(v = new Variable("remData",   Variable::Status));
   v->setDescription("Remote data value");

   addVariable(v = new Variable("remBuffStatus",   Variable::Status));
   v->setDescription("Remote buffer status");

   if ( evrEn_ ) {

      addVariable(v = new Variable("evrLinkErrors", Variable::Status));
      v->setDescription("EVR link error counter");
      
      addVariable(v = new Variable("evrLinkUp", Variable::Status));
      v->setDescription("EVR link is up");

      addVariable(v = new Variable("evrRunStatus", Variable::Status));
      v->setDescription("EVR lane run status");
      v->setTrueFalse();

      addVariable(v = new Variable("evrSeconds", Variable::Status));
      v->setDescription("EVR timestamp");

      addVariable(v = new Variable("evrRunCounter", Variable::Status));
      v->setDescription("EVR lane run counter");

      addVariable(v = new Variable("evrAcceptCounter", Variable::Status));
      v->setDescription("EVR lane accept counter");

      addVariable(v = new Variable("evrEnable", Variable::Configuration));
      v->setDescription("EVR global enable");

      addVariable(v = new Variable("evrLaneRunEn", Variable::Configuration));
      v->setDescription("EVR lane run enable");
      v->setTrueFalse();

      addVariable(v = new Variable("evrLaneSyncEn", Variable::Configuration));
      v->setDescription("EVR lane sync enable");
      v->setTrueFalse();

      addVariable(v = new Variable("evrLaneSyncSel", Variable::Configuration));
      v->setDescription("EVR lane sync sel");
      v->setTrueFalse();

      addVariable(v = new Variable("evrLaneHeaderMask", Variable::Configuration));
      v->setDescription("EVR lane header mask");
      v->setTrueFalse();

      addVariable(v = new Variable("evrLaneSyncWord", Variable::Configuration));
      v->setDescription("EVR lane sync word");

      addVariable(v = new Variable("evrLaneRunCode", Variable::Configuration));
      v->setDescription("EVR lane run code");

      addVariable(v = new Variable("evrLaneRunDelay", Variable::Configuration));
      v->setDescription("EVR lane run delay");

      addVariable(v = new Variable("evrLaneAcceptCode", Variable::Configuration));
      v->setDescription("EVR lane accept code");

      addVariable(v = new Variable("evrLaneAcceptDelay", Variable::Configuration));
      v->setDescription("EVR lane accept delay");
   }

   locData_  = 0;
   loopBack_ = 0;

   pollEnable(true);
   getVariable("Enabled")->setHidden(true);
}

// Deconstructor
PgpCardLane::~PgpCardLane ( ) { }

// Method to read status registers and update variables
void PgpCardLane::readStatus ( ) {
   PgpStatus    pgpStatus;
   PgpEvrStatus evrStatus;

   pgpGetStatus(fd_,index_,&pgpStatus);
   pgpGetEvrStatus(fd_,index_,&evrStatus);

   loopBack_ = pgpStatus.loopBack;

   getVariable("locLinkready")->setInt(pgpStatus.locLinkReady);
   getVariable("remLinkready")->setInt(pgpStatus.remLinkReady);
   getVariable("rxReady")->setInt(pgpStatus.rxReady);
   getVariable("txReady")->setInt(pgpStatus.txReady);
   getVariable("rxCount")->setInt(pgpStatus.rxCount);
   getVariable("cellErrorCount")->setInt(pgpStatus.cellErrCnt);
   getVariable("linkDownCount")->setInt(pgpStatus.linkDownCnt);
   getVariable("linkErrorCount")->setInt(pgpStatus.linkErrCnt);
   getVariable("fifoError")->setInt(pgpStatus.fifoErr);
   getVariable("remData")->setInt(pgpStatus.remData);
   getVariable("remBuffStatus")->setInt(pgpStatus.remBuffStatus);
   getVariable("evrLinkErrors")->setInt(evrStatus.linkErrors);
   getVariable("evrLinkUp")->setInt(evrStatus.linkUp);
   getVariable("evrRunStatus")->setInt(evrStatus.runStatus);
   getVariable("evrSeconds")->setInt(evrStatus.evrSeconds);
   getVariable("evrRunCounter")->setInt(evrStatus.runCounter);
   getVariable("evrAcceptCounter")->setInt(evrStatus.acceptCounter);
}

void PgpCardLane::pollStatus ( ) {
   readStatus();
}

void PgpCardLane::readConfig ( ) {
   PgpEvrControl evrControl;

   pgpGetEvrControl(fd_,index_,&evrControl);

   getVariable("loopBack")->setInt(loopBack_);
   getVariable("locData")->setInt(locData_);
   getVariable("evrEnable")->setInt(evrControl.evrEnable);
   getVariable("evrLaneRunEn")->setInt(evrControl.laneRunMask);
   getVariable("evrLaneSyncEn")->setInt(evrControl.evrSyncEn);
   getVariable("evrLaneSyncSel")->setInt(evrControl.evrSyncSel);
   getVariable("evrLaneHeaderMask")->setInt(evrControl.headerMask);
   getVariable("evrLaneSyncWord")->setInt(evrControl.evrSyncWord);
   getVariable("evrLaneRunCode")->setInt(evrControl.runCode);
   getVariable("evrLaneRunDelay")->setInt(evrControl.runDelay);
   getVariable("evrLaneAcceptCode")->setInt(evrControl.acceptCode);
   getVariable("evrLaneAcceptDelay")->setInt(evrControl.acceptDelay);
}

void PgpCardLane::writeConfig ( bool force ) {
   PgpEvrControl evrControl;

   loopBack_ = getVariable("loopBack")->getInt();
   locData_  = getVariable("locData")->getInt();

   pgpSetLoop(fd_,index_,loopBack_);
   pgpSetData(fd_,index_,locData_);

   evrControl.evrEnable   = getVariable("evrEnable")->getInt();
   evrControl.laneRunMask = getVariable("evrLaneRunEn")->getInt();
   evrControl.evrSyncEn   = getVariable("evrLaneSyncEn")->getInt();
   evrControl.evrSyncSel  = getVariable("evrLaneSyncSel")->getInt();
   evrControl.headerMask  = getVariable("evrLaneHeaderMask")->getInt();
   evrControl.evrSyncWord = getVariable("evrLaneSyncWord")->getInt();
   evrControl.runCode     = getVariable("evrLaneRunCode")->getInt();
   evrControl.runDelay    = getVariable("evrLaneRunDelay")->getInt();
   evrControl.acceptCode  = getVariable("evrLaneAcceptCode")->getInt();
   evrControl.acceptDelay = getVariable("evrLaneAcceptDelay")->getInt();

   pgpSetEvrControl(fd_,index_,&evrControl);
}

void PgpCardLane::softReset() {
   pgpResetEvrCount(fd_,index_);
   Device::softReset();
}

