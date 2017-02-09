//-----------------------------------------------------------------------------
// File          : SystemWindow.cpp
// Author        : Ryan Herbst  <rherbst@slac.stanford.edu>
// Created       : 10/04/2011
// Project       : General Purpose
//-----------------------------------------------------------------------------
// Description :
// System window in top GUI
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
#include <unistd.h>
#include <QDomDocument>
#include <QObject>
#include <QHeaderView>
#include <QMessageBox>
#include <QTabWidget>
#include <QTableWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <QLineEdit>
#include <QGroupBox>
#include <QFileDialog>
#include <QInputDialog>
#include <QFormLayout>
#include <QComboBox>
#include <QLabel>
#include "MainWindow.h"
#include "SystemWindow.h"
#include "CommandHolder.h"
#include "VariableHolder.h"
using namespace std;

// Counter table
QGroupBox *SystemWindow::counterBox() {
   QGroupBox *gb = new QGroupBox("Counters");

   QVBoxLayout *vbox = new QVBoxLayout;
   gb->setLayout(vbox);

   QHBoxLayout *hbox = new QHBoxLayout;
   vbox->addLayout(hbox);

   QFormLayout *form1 = new QFormLayout;
   form1->setRowWrapPolicy(QFormLayout::DontWrapRows);
   form1->setFieldGrowthPolicy(QFormLayout::FieldsStayAtSizeHint);
   form1->setFormAlignment(Qt::AlignHCenter | Qt::AlignTop);
   form1->setLabelAlignment(Qt::AlignRight);
   hbox->addLayout(form1);

   form1->addRow(tr("Register Rx:"),countRegRx_);
   form1->addRow(tr("Timeout:"),countTimeout_);
   form1->addRow(tr("Data/Event:"),countDataRx_);
   
   QFormLayout *form2 = new QFormLayout;
   form2->setRowWrapPolicy(QFormLayout::DontWrapRows);
   form2->setFieldGrowthPolicy(QFormLayout::FieldsStayAtSizeHint);
   form2->setFormAlignment(Qt::AlignHCenter | Qt::AlignTop);
   form2->setLabelAlignment(Qt::AlignRight);
   hbox->addLayout(form2);

   form2->addRow(tr("Error:"),countError_);
   form2->addRow(tr("Unexpected:"),countUnexp_);
   if ( ! hideData_ ) form2->addRow(tr("Data File:"),countDataFile_);

   QHBoxLayout *hbox2 = new QHBoxLayout;
   vbox->addLayout(hbox2);

   tb_ = new QPushButton("Reset Counters");
   hbox2->addWidget(tb_);
   connect(tb_,SIGNAL(pressed()),this,SLOT(resetCountPressed()));
   tb_->setEnabled(not readOnly_);

   pb_ = new QPushButton("Pgp Counters");
   hbox2->addWidget(pb_);
   connect(pb_,SIGNAL(pressed()),main_,SLOT(openPgpPressed()));

   return(gb);
}

// Create group box for config file read
QGroupBox *SystemWindow::configBox () {

   QGroupBox *gbox = new QGroupBox("Configuration and State");

   QVBoxLayout *vbox = new QVBoxLayout;
   gbox->setLayout(vbox);

   QFormLayout *form = new QFormLayout;
   vbox->addLayout(form);

   QFontMetrics m (stateLine_->font());
   int RowHeight = m.lineSpacing() ;
   stateLine_->setMinimumHeight(10 * RowHeight);

   form->addRow(tr("State:"),stateLine_);

   QHBoxLayout *hbox1 = new QHBoxLayout;
   vbox->addLayout(hbox1);

   hbox1->addWidget(hardReset_);
   hbox1->addWidget(softReset_);
   hbox1->addWidget(refreshState_);

   QHBoxLayout *hbox2 = new QHBoxLayout;
   vbox->addLayout(hbox2);

   hbox2->addWidget(setDefaults_);
   hbox2->addWidget(configRead_);
   hbox2->addWidget(configSave_);
  
   return(gbox);
}


// Create group box for data file write
QGroupBox *SystemWindow::dataBox () {

   QGroupBox *gbox = new QGroupBox("Data File");

   QVBoxLayout *vbox = new QVBoxLayout;
   gbox->setLayout(vbox);

   dataFile_ = new QLineEdit;
   vbox->addWidget(dataFile_);

   QHBoxLayout *hbox = new QHBoxLayout;
   vbox->addLayout(hbox);

   hbox->addWidget(dataBrowse_);
   hbox->addWidget(dataOpen_);
   hbox->addWidget(dataClose_);

   return(gbox);
}

// Create group box for software run control
QGroupBox *SystemWindow::cmdBox () {

   QGroupBox *gbox = new QGroupBox("Run Control");

   QVBoxLayout *vbox = new QVBoxLayout;
   gbox->setLayout(vbox);

   QFormLayout *form = new QFormLayout;
   form->setRowWrapPolicy(QFormLayout::DontWrapRows);
   form->setFormAlignment(Qt::AlignHCenter | Qt::AlignTop);
   form->setLabelAlignment(Qt::AlignRight);
   //form->setFieldGrowthPolicy(QFormLayout::FieldsStayAtSizeHint);
   vbox->addLayout(form);

   if ( !hideRun_ ) form->addRow(tr("Run Rate:"),runRate_);
   if ( !hideRun_ ) form->addRow(tr("Run Count:"),runCount_);
   form->addRow(tr("Run State:"),runState_);
   if ( !hideRun_ ) vbox->addWidget(runProgress_);

   return(gbox);
}

// Constructor
SystemWindow::SystemWindow ( MainWindow *main, bool local ) : QWidget () {
   QString tmp;

   top_     = NULL;
   main_    = main;
   isLocal_ = local;

   // Create objects
   countRegRx_ = new QLineEdit;
   countRegRx_->setReadOnly(true);

   countDataRx_ = new QLineEdit;
   countDataRx_->setReadOnly(true);

   countDataFile_ = new QLineEdit;
   countDataFile_->setReadOnly(true);
   
   countTimeout_ = new QLineEdit;
   countTimeout_->setReadOnly(true);

   countError_ = new QLineEdit;
   countError_->setReadOnly(true);

   countUnexp_ = new QLineEdit;
   countUnexp_->setReadOnly(true);

   stateLine_ = new QTextEdit();
   stateLine_->setReadOnly(true);

   hardReset_    = new QPushButton("HardReset");
   softReset_    = new QPushButton("SoftReset");
   refreshState_ = new QPushButton("RefreshState");
   setDefaults_  = new QPushButton("Set Defaults");
   configRead_   = new QPushButton("Load Settings");
   configSave_   = new QPushButton("Save Settings");

   dataFile_ = new QLineEdit;

   dataBrowse_ = new QPushButton("Browse");
   dataOpen_   = new QPushButton("Open");
   dataClose_  = new QPushButton("Close");

   runRate_  = new QComboBox;
   runCount_ = new QSpinBox;
   runCount_->setMinimum(0);
   runCount_->setMaximum(99999999);

   runState_    = new QComboBox;
   runProgress_ = new QProgressBar;

   connect(runState_,SIGNAL(activated(const QString &)),
           this,SLOT(runStateActivated(const QString &)));

   connect(dataBrowse_,SIGNAL(pressed()),this,SLOT(browseDataPressed()));
   connect(dataOpen_,SIGNAL(pressed()),this,SLOT(openDataPressed()));
   connect(dataClose_,SIGNAL(pressed()),this,SLOT(closeDataPressed()));
   connect(setDefaults_,SIGNAL(pressed()),this,SLOT(setDefaultsPressed()));
   connect(configRead_,SIGNAL(pressed()),this,SLOT(configReadPressed()));
   connect(configSave_,SIGNAL(pressed()),this,SLOT(configSavePressed()));
   connect(refreshState_,SIGNAL(pressed()),this,SLOT(refreshStatePressed()));
   connect(softReset_,SIGNAL(pressed()),this,SLOT(softResetPressed()));
   connect(hardReset_,SIGNAL(pressed()),this,SLOT(hardResetPressed()));

   lastLoadSettings_ = "";
   lastSaveSettings_ = "";
   lastData_ = "";

   stateMsg_ = "";
   hideData_ = false;
   hideRun_  = false;
   readOnly_ = false;
}

// Delete
SystemWindow::~SystemWindow ( ) { 
}

// set readonly
void SystemWindow::setReadOnly (bool ro) {
   readOnly_ = ro;
   setDefaults_->setEnabled(not ro);
   configRead_->setEnabled(not ro);
   configSave_->setEnabled(not ro);
   refreshState_->setEnabled(not ro);
   softReset_->setEnabled(not ro);
   hardReset_->setEnabled(not ro);
   dataBrowse_->setEnabled(not ro);
   dataOpen_->setEnabled(not ro);
   dataClose_->setEnabled(not ro);
   runRate_->setEnabled(not ro);
   runCount_->setEnabled(not ro);
   runState_->setEnabled(not ro);
   if (top_ != NULL ) tb_->setEnabled(not ro);
}

void SystemWindow::runStateEnums(vector<QString> enums) {
   uint32_t x;
   for (x=0; x < enums.size(); x++ ) runState_->addItem(enums[x]);
}

void SystemWindow::runRateEnums(vector<QString> enums) {
   uint32_t x;
   for (x=0; x < enums.size(); x++ ) runRate_->addItem(enums[x]);
}

void SystemWindow::setConfigVariable(QString name, QString value) {
   uint32_t idx;
   bool     ok;

   if ( name == "HideDataControl" && value == "True" ) hideData_ = true;
   else if ( name == "HideRunControl" && value == "True" ) hideRun_ = true;
   else if ( name == "RunRate" ) {
      idx = runRate_->findText(value);
      runRate_->setCurrentIndex(idx);
   }
   else if ( name == "RunCount" ) runCount_->setValue(value.toUInt(&ok,0));
   else if ( name == "DataFile" ) dataFile_->setText(value);
}

bool SystemWindow::setStatusVariable(QString name, QString value) {
   uint32_t idx;
   bool     ok;
   bool     ret = false;

   if      ( name == "DataFileCount"   ) countDataFile_->setText(value);
   else if ( name == "DataRxCount"     ) { countDataRx_->setText(value); ret=true; }
   else if ( name == "RegRxCount"      ) { countRegRx_->setText(QString().setNum(value.toUInt(&ok,0))); ret = true; }
   else if ( name == "UnexpectedCount" ) countUnexp_->setText(QString().setNum(value.toUInt(&ok,0)));
   else if ( name == "TimeoutCount"    ) { countTimeout_->setText(QString().setNum(value.toUInt(&ok,0))); ret = true; }
   else if ( name == "ErrorCount"      ) { countError_->setText(QString().setNum(value.toUInt(&ok,0))); ret = true; }
   else if ( name == "SystemState"     ) stateMsg_ = value;
   else if ( name == "SystemStatus"    ) { sysStatus_ = value; ret = true; }
   else if ( name == "WorkingDir" ) {
      if ( lastLoadSettings_ == "" ) {
         lastLoadSettings_ = value;
         lastLoadSettings_.append("/config/defaults.xml");
      }
      if ( lastSaveSettings_ == "" ) {
         lastSaveSettings_ = value;
         lastSaveSettings_.append("/config/configDump.xml");
      }
      if ( lastData_ == "" ) {
         lastData_ = value;
         lastData_.append("/data.bin");
      }
      ret = true;
   }
   else if ( name == "RunProgress"    ) {
      runProgress_->setRange(0,100);
      runProgress_->setValue(value.toUInt(&ok,0));
   }

   // File Status
   else if ( name == "DataOpen" ) {
      if ( readOnly_ ) {
         dataBrowse_->setEnabled(false);
         dataOpen_->setEnabled(false);
         dataClose_->setEnabled(false);
      }
      else if ( value == "False" ) {
         dataBrowse_->setEnabled(isLocal_);
         dataOpen_->setEnabled(true);
         dataClose_->setEnabled(false);
      } else {
         dataBrowse_->setEnabled(false);
         dataOpen_->setEnabled(false);
         dataClose_->setEnabled(true);
      }
   }

   // Run state
   else if ( name == "RunState" ) {
      idx = runState_->findText(value);
      runState_->setCurrentIndex(idx);
      ret = true;
   }
   return(ret);
}

void SystemWindow::refresh(bool config) {
   if ( config && (top_ == NULL) ) {
      top_ = new QVBoxLayout;
      setLayout(top_); 

      top_->addWidget(configBox());
      if ( ! hideData_ ) top_->addWidget(dataBox());
      top_->addWidget(cmdBox());
      top_->addWidget(counterBox());
   }
   updateState();
}

void SystemWindow::setDefaultsPressed() {
   topConfigCommand("<SetDefaults/>");
}

void SystemWindow::configReadPressed() {
   QString cmd;
   QString fileName;

   QString label = "Config File";
   for (int x=0; x < 150; x++) label += " ";

   if ( isLocal_ ) 
      fileName = QFileDialog::getOpenFileName(this, tr("Load Config"), lastLoadSettings_, tr("XML Files (*.xml)"));
   else 
      fileName = QInputDialog::getText(this, tr("Config File"), label, QLineEdit::Normal, lastLoadSettings_,0 ,0);

   if ( fileName != "" ) {
      cmd = "<ReadXmlFile>";
      cmd.append(fileName);
      cmd.append("</ReadXmlFile>");
      topConfigCommand(cmd);
      lastLoadSettings_ = fileName;
   }
}

void SystemWindow::configSavePressed() {
   QString cmd;
   QString fileName;

   QString label = "Dump File";
   for (int x=0; x < 150; x++) label += " ";

   if ( isLocal_ ) fileName = QFileDialog::getSaveFileName(this, tr("Save Config"), lastSaveSettings_, tr("XML Files (*.xml)"));
   else fileName = QInputDialog::getText(this, tr("Dump File"), label, QLineEdit::Normal, lastSaveSettings_,0 ,0);

   if ( fileName != "" ) {
      cmd = "<WriteConfigXml>";
      cmd.append(fileName);
      cmd.append("</WriteConfigXml>");
      topConfigCommand(cmd);
      lastSaveSettings_ = fileName;
   }
}

void SystemWindow::refreshStatePressed() {
   stateMsg_ = "Updating State. Please Wait!\n";
   updateState();
   topConfigCommand("<RefreshState/>");
}

void SystemWindow::hardResetPressed() {
   usleep(100);
   stateMsg_ = "Sending Hard Reset. Please Wait!\n";
   main_->localCommand("<HardReset/>");
   updateState();
}

void SystemWindow::softResetPressed() {
   usleep(100);
   stateMsg_ = "Sending Soft Reset. Please Wait!\n";
   main_->localCommand("<SoftReset/>");
   updateState();
}

void SystemWindow::browseDataPressed() {
   QString fileName;

   fileName = QFileDialog::getSaveFileName(this, tr("Select Data File"), lastData_, tr("Data File (*.bin)"),0,QFileDialog::DontConfirmOverwrite);

   if ( fileName != "" ) {
      lastData_ = fileName;
      dataFile_->setText(fileName); 
   }
}

void SystemWindow::openDataPressed() {
   if ( dataFile_->text() != "" ) 
      topConfigCommand("<OpenDataFile/>");
}

void SystemWindow::closeDataPressed() {
   topConfigCommand("<CloseDataFile/>");
}

void SystemWindow::resetCountPressed() {
   topConfigCommand("<ResetCount/>");
}

void SystemWindow::runStateActivated(const QString &state) {
   QString cmd;

   cmd = "<SetRunState>";
   cmd.append(state);
   cmd.append("</SetRunState>");
   topConfigCommand(cmd);
}

// Send command along with system window config
void SystemWindow::topConfigCommand(QString cmd) {
   QString cfg;

   cfg = "<DataFile>";
   cfg.append(dataFile_->text());
   cfg.append("</DataFile>");
   cfg.append("<RunRate>");
   cfg.append(runRate_->currentText());
   cfg.append("</RunRate>");
   cfg.append("<RunCount>");
   cfg.append(QString().setNum(runCount_->value()));
   cfg.append("</RunCount>");

   main_->localConfigCommand(cfg,cmd);
}

void SystemWindow::updateState () {
   QString msg;

   msg = stateMsg_;
   if ( stateLine_->toPlainText() != msg ) stateLine_->setPlainText(msg);

   QPalette p = stateLine_->palette();

   if ( sysStatus_ == "Ready" ) p.setColor(QPalette::Base, Qt::white);
   else if ( sysStatus_ == "Error" ) p.setColor(QPalette::Base, Qt::red); // Error
   else p.setColor(QPalette::Base, Qt::yellow); // Warning or unconfigured or user defined value

   stateLine_->setPalette(p);
   update();
}

