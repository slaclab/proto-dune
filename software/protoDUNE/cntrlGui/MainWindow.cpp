//-----------------------------------------------------------------------------
// File          : MainWindow.cpp
// Author        : Ryan Herbst  <rherbst@slac.stanford.edu>
// Created       : 10/04/2011
// Project       : General purpose
//-----------------------------------------------------------------------------
// Description :
// Top level control window
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
// 10/04/2011: created
//-----------------------------------------------------------------------------
#include <iostream>
#include <sstream>
#include <string>
#include <QObject>
#include <QTabWidget>
#include <QVBoxLayout>
#include "MainWindow.h"
#include "SystemWindow.h"
#include "CommandWindow.h"
#include "CommandHolder.h"
#include "VariableWindow.h"
#include "VariableHolder.h"
#include <QMessageBox>
using namespace std;

// Constructor
MainWindow::MainWindow ( uint32_t idx, QString host, uint32_t port ) : QWidget () {

   idx_  = idx;
   host_ = host;
   port_ = port;

   thread_.start();
   xmlClient_.moveToThread(&thread_);

   // Top level
   QVBoxLayout *base = new QVBoxLayout;
   this->setLayout(base);

   QTabWidget *tab = new QTabWidget;
   base->addWidget(tab);

   // System Window Tab
   systemWindow_ = new SystemWindow(this,host=="localhost");
   tab->addTab(systemWindow_,"System");

   // Command Window Tab
   commandWindow_ = new CommandWindow(this);
   tab->addTab(commandWindow_,"Commands");

   // Status Variable Window
   statusWindow_ = new VariableWindow("Status",this);
   tab->addTab(statusWindow_,"Status");

   // Config Variable Window
   configWindow_ = new VariableWindow("Configuration",this);
   tab->addTab(configWindow_,"Configuration");

   // XML Interface signals
   connect(&xmlClient_,SIGNAL(lostConnection()),this,SLOT(lostConnectionSlot()));
   connect(&xmlClient_,SIGNAL(createDescription(QString)),this,SLOT(createDescription(QString)));
   connect(&xmlClient_,SIGNAL(createCommand(CommandHolder *)),this,SLOT(createCommand(CommandHolder *)));
   connect(&xmlClient_,SIGNAL(createVariable(VariableHolder *)),this,SLOT(createVariable(VariableHolder *)));
   connect(&xmlClient_,SIGNAL(createDone()),this,SLOT(createDone()));
   connect(&xmlClient_,SIGNAL(updateVariable(bool, QString, QString)),this,SLOT(updateVariable(bool, QString, QString)));
   connect(&xmlClient_,SIGNAL(updateDone(bool,bool)),this,SLOT(updateDone(bool,bool)));
   connect(&xmlClient_,SIGNAL(gotError(QString)),this,SLOT(gotError(QString)));

   // XML Interface slots
   connect(this,SIGNAL(openServer(QString,int)),&xmlClient_,SLOT(openServer(QString,int)));
   connect(this,SIGNAL(sendCommand(QString)),&xmlClient_,SLOT(sendCommand(QString)));
   connect(this,SIGNAL(sendConfig(QString)),&xmlClient_,SLOT(sendConfig(QString)));
   connect(this,SIGNAL(sendConfigCommand(QString,QString)),&xmlClient_,SLOT(sendConfigCommand(QString,QString)));

   // Connect
   printf("Connecting window %i to host %s, Port %i\n",idx_,qPrintable(host),port);
   openServer(host,(int)port);
}


// Delete
MainWindow::~MainWindow ( ) { }

void MainWindow::setReadOnly(bool ro) {
   systemWindow_->setReadOnly(ro);
   commandWindow_->setReadOnly(ro);
   statusWindow_->setReadOnly(ro);
   configWindow_->setReadOnly(ro);
}

void MainWindow::localCommand(QString cmd) {
   sendCommand(cmd);
}


void MainWindow::localConfig(QString cmd) {
   sendConfig(cmd);
}


void MainWindow::localConfigCommand(QString cfg, QString cmd) {
   sendConfigCommand(cfg,cmd);
}


void MainWindow::openPgpPressed () {
   openPgp();
}


void MainWindow::lostConnectionSlot () {
   lostConnection();
}


void MainWindow::cfgReadPressed () {
   QString cmd;
   cmd = "<ReadConfig/>\n";
   sendCommand(cmd);
}


void MainWindow::cfgWritePressed () {
   QString cmd = configWindow_->getConfig();
   sendConfig(cmd);
}


void MainWindow::cfgVerifyPressed () {
   QString cmd;
   cmd = "<VerifyConfig/>\n";
   sendCommand(cmd);
}


void MainWindow::statReadPressed () {
   QString cmd;
   cmd = "<ReadStatus/>\n";
   sendCommand(cmd);
}


void MainWindow::helpPressed (QString name, QString desc) {
   QMessageBox::information(this,name,desc,QMessageBox::Ok);
}


void MainWindow::commandPressed (QString xml) {
   sendCommand(xml);
}


void MainWindow::createDescription ( QString desc ) {
   setWindowTitle(desc);
   updateSummaryField(idx_,"DeviceName",desc);
}


void MainWindow::createCommand ( CommandHolder *cmd ) {
   if ( ! cmd->isHidden() ) {
      commandWindow_->createCommand (cmd);
   }
}


void MainWindow::createVariable ( VariableHolder *var ) {
   if ( ! var->isHidden() ) {
      
      // Config Window
      if ( var->typeMatch("Configuration") ) configWindow_->createVariable(var);

      // Status Window
      else if ( var->typeMatch("Status") ) statusWindow_->createVariable(var);
   }

   if ( var->shortName() == "RunRate" ) systemWindow_->runRateEnums(var->getEnums());
   else if ( var->shortName() == "RunState" ) {
      systemWindow_->runStateEnums(var->getEnums());
      setRunStateEnums(idx_,var->getEnums());
   }

   if ( var->getId().contains("Pgp2bAxi") && var->shortName() == "RxLocalLinkReady" ) 
      addPgpRow(var->path());
}


void MainWindow::createDone() {
   configWindow_->createDone();
   statusWindow_->createDone();
   commandWindow_->createDone();
   addPgpDone();
}


void MainWindow::updateVariable ( bool status, QString id, QString value ) {
   QString decValue;

   if ( status ) {
      decValue = statusWindow_->updateVariable (id,value);

      if ( ! id.contains(":") ) {
         if ( systemWindow_->setStatusVariable(id,value) )
            updateSummaryField(idx_,id,value);
      }

      if ( id.contains("Pgp2bAxi") ) updatePgpField(id, decValue);
   }

   else {
      configWindow_->updateVariable (id,value);

      if ( ! id.contains(":") ) systemWindow_->setConfigVariable(id,value);
   }
}


void MainWindow::updateDone(bool status, bool config) {
   if ( status ) statusWindow_->updateDone();
   if ( config ) configWindow_->updateDone();
   systemWindow_->refresh(config);
}


void MainWindow::gotError(QString error) {
   QMessageBox::warning(NULL,QString("Error from host ").append(host_).append(" !"),error,QMessageBox::Ok);
}


void MainWindow::externalCommand(QString cmd) {
  sendCommand(cmd);
}


void MainWindow::windowSelected(uint32_t idx) {
   if ( idx == idx_ ) show();
}

