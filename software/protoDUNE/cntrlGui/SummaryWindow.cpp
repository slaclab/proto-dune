//-----------------------------------------------------------------------------
// File          : SummaryWindow.cpp
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
#include "SummaryWindow.h"
#include "VariableHolder.h"
using namespace std;

// Create group box for status
QGroupBox *SummaryWindow::statusBox () {
   int x;

   QGroupBox *gbox = new QGroupBox("System State");

   QVBoxLayout *vbox = new QVBoxLayout;
   gbox->setLayout(vbox);

   statTable_ = new QTableWidget;
   statTable_->setRowCount(count_);
   statTable_->setColumnCount(7);

   QStringList header;
   header << "Device Name" << "Run State" << "Status" << "Reg Rx" << "Timeouts" << "Data/Event" << "Errors";
   statTable_->setHorizontalHeaderLabels(header);
   statTable_->verticalHeader()->setVisible(false);
   statTable_->setEditTriggers(QAbstractItemView::NoEditTriggers);
   statTable_->setSelectionMode(QAbstractItemView::SingleSelection);
   statTable_->setShowGrid(false);
   statTable_->horizontalHeader()->setStretchLastSection(false);
   statTable_->horizontalHeader()->setResizeMode(0, QHeaderView::ResizeToContents);
   statTable_->setMinimumHeight(400);
   statTable_->setMinimumWidth(600);

   for (x=0; x < count_; x++) {

      systemName_[x] = new QTableWidgetItem("Not Connected!");
      statTable_->setItem(x, 0, systemName_[x]);

      sysRunState_[x] = new QTableWidgetItem("");
      statTable_->setItem(x, 1, sysRunState_[x]);

      sysStatus_[x] = new QTableWidgetItem("");
      statTable_->setItem(x, 2, sysStatus_[x]);

      sysRegRx_[x] = new QTableWidgetItem("");
      statTable_->setItem(x, 3, sysRegRx_[x]);

      sysTimeouts_[x] = new QTableWidgetItem("");
      statTable_->setItem(x, 4, sysTimeouts_[x]);

      sysDataRx_[x] = new QTableWidgetItem("");
      statTable_->setItem(x, 5, sysDataRx_[x]);

      sysErrors_[x] = new QTableWidgetItem("");
      statTable_->setItem(x, 6, sysErrors_[x]);
   }

   connect( statTable_, SIGNAL( cellDoubleClicked (int, int) ), this, SLOT( cellSelected( int, int ) ) );

   vbox->addWidget(statTable_); 

   return(gbox);
}

// Create group box for config file read
QGroupBox *SummaryWindow::configBox () {

   QGroupBox *gbox = new QGroupBox("Configuration And Control");

   QVBoxLayout *vbox = new QVBoxLayout;
   gbox->setLayout(vbox);

   QHBoxLayout *hbox1 = new QHBoxLayout;
   vbox->addLayout(hbox1);

   hardReset_ = new QPushButton("HardReset");
   hbox1->addWidget(hardReset_);

   softReset_ = new QPushButton("SoftReset");
   hbox1->addWidget(softReset_);

   refreshState_ = new QPushButton("RefreshState");
   hbox1->addWidget(refreshState_);

   QHBoxLayout *hbox2 = new QHBoxLayout;
   vbox->addLayout(hbox2);

   setDefaults_ = new QPushButton("Set Defaults");
   hbox2->addWidget(setDefaults_);

   configRead_ = new QPushButton("Load Settings");
   hbox2->addWidget(configRead_);

   tb_ = new QPushButton("Reset Counters");
   hbox2->addWidget(tb_);

   pgp_ = new QPushButton("Pgp Link Status");
   hbox2->addWidget(pgp_);

   connect(setDefaults_,SIGNAL(pressed()),this,SLOT(setDefaultsPressed()));
   connect(configRead_,SIGNAL(pressed()),this,SLOT(configReadPressed()));
   connect(refreshState_,SIGNAL(pressed()),this,SLOT(refreshStatePressed()));
   connect(softReset_,SIGNAL(pressed()),this,SLOT(softResetPressed()));
   connect(hardReset_,SIGNAL(pressed()),this,SLOT(hardResetPressed()));
   connect(tb_,SIGNAL(pressed()),this,SLOT(resetCountPressed()));
   connect(pgp_,SIGNAL(pressed()),this,SLOT(pgpPressed()));
  
   return(gbox);
}


// Create group box for software run control
QGroupBox *SummaryWindow::runBox () {

   QGroupBox *gbox = new QGroupBox("Run Control");

   QVBoxLayout *vbox = new QVBoxLayout;
   gbox->setLayout(vbox);

   QFormLayout *form = new QFormLayout;
   form->setRowWrapPolicy(QFormLayout::DontWrapRows);
   form->setFormAlignment(Qt::AlignHCenter | Qt::AlignTop);
   form->setLabelAlignment(Qt::AlignRight);
   vbox->addLayout(form);

   runState_ = new QComboBox;
   form->addRow(tr("Run State:"),runState_);

   connect(runState_,SIGNAL(activated(const QString &)),this,SLOT(runStateActivated(const QString &)));

   return(gbox);
}


// Constructor
SummaryWindow::SummaryWindow (int count ) : QWidget () {
   QString tmp;

   count_     = count;

   systemName_  = (QTableWidgetItem **)malloc(sizeof(QTableWidgetItem *)*count);
   sysRunState_ = (QTableWidgetItem **)malloc(sizeof(QTableWidgetItem *)*count);
   sysStatus_   = (QTableWidgetItem **)malloc(sizeof(QTableWidgetItem *)*count);
   sysTimeouts_ = (QTableWidgetItem **)malloc(sizeof(QTableWidgetItem *)*count);
   sysRegRx_    = (QTableWidgetItem **)malloc(sizeof(QTableWidgetItem *)*count);
   sysDataRx_   = (QTableWidgetItem **)malloc(sizeof(QTableWidgetItem *)*count);
   sysErrors_   = (QTableWidgetItem **)malloc(sizeof(QTableWidgetItem *)*count);

   QVBoxLayout *top = new QVBoxLayout;
   setLayout(top); 

   top->addWidget(statusBox());
   top->addWidget(configBox());
   top->addWidget(runBox());

   lastLoadSettings_ = "";

   setWindowTitle("System Control");
}

// Delete
SummaryWindow::~SummaryWindow ( ) { 
   delete systemName_;
   delete sysRunState_;
   delete sysStatus_;
   delete sysTimeouts_;
   delete sysRegRx_;
   delete sysDataRx_;
   delete sysErrors_;
}

// set readonly
void SummaryWindow::setReadOnly (bool ro) {
   hardReset_->setEnabled(not ro);
   softReset_->setEnabled(not ro);
   refreshState_->setEnabled(not ro);
   setDefaults_->setEnabled(not ro);
   configRead_->setEnabled(not ro);
   runState_->setEnabled(not ro);
   tb_->setEnabled(not ro);
}

void SummaryWindow::cellSelected (int x, int) {
   windowSelected(x);
}


void SummaryWindow::setDefaultsPressed() {
   sendSummaryCommand("<SetDefaults/>");
}


void SummaryWindow::configReadPressed() {
   QString cmd;
   QString fileName;

   QString label = "Config File";
   for (int x=0; x < 150; x++) label += " ";

   fileName = QInputDialog::getText(this, tr("Config File"), label, QLineEdit::Normal, lastLoadSettings_,0 ,0);

   if ( fileName != "" ) {
      cmd = "<ReadXmlFile>";
      cmd.append(fileName);
      cmd.append("</ReadXmlFile>");
      sendSummaryCommand(cmd);
      lastLoadSettings_ = fileName;
   }
}


void SummaryWindow::refreshStatePressed() {
   sendSummaryCommand("<RefreshState/>");
}


void SummaryWindow::resetCountPressed() {
   sendSummaryCommand("<ResetCount/>");
}


void SummaryWindow::runStateActivated(const QString &state) {
   QString cmd;

   cmd = "<SetRunState>";
   cmd.append(state);
   cmd.append("</SetRunState>");
   sendSummaryCommand(cmd);
}


void SummaryWindow::hardResetPressed() {
   usleep(100);
   sendSummaryCommand("<HardReset/>");
}


void SummaryWindow::softResetPressed() {
   usleep(100);
   sendSummaryCommand("<SoftReset/>");
}


void SummaryWindow::pgpPressed() {
   openPgp();
}


void SummaryWindow::updateSummaryField (uint32_t idx, QString field, QString value) {
   int  x;
   bool ok;

   if ( field == "DeviceName" ) {
      systemName_[idx]->setText(value);
      statTable_->resizeColumnsToContents();
   }
   else if ( field == "RegRxCount"    ) {
      sysRegRx_[idx]->setText(QString().setNum(value.toUInt(&ok,0)).append(" "));
      statTable_->resizeColumnsToContents();
   }
   else if ( field == "TimeoutCount"  ) {
      sysTimeouts_[idx]->setText(QString().setNum(value.toUInt(&ok,0)).append(" "));
      statTable_->resizeColumnsToContents();
   }
   else if ( field == "DataRxCount"  ) {
      sysDataRx_[idx]->setText(QString().setNum(value.toUInt(&ok,0)).append(" "));
      statTable_->resizeColumnsToContents();
   }
   else if ( field == "ErrorCount"  ) {
      sysErrors_[idx]->setText(QString().setNum(value.toUInt(&ok,0)).append(" "));
      statTable_->resizeColumnsToContents();
   }
   else if ( field == "SystemStatus"    ) {
      value = value;

      if ( value == "Ready" ) sysStatus_[idx]->setBackground(QBrush(Qt::white));
      else if ( value == "Error" ) sysStatus_[idx]->setBackground(QBrush(Qt::red));
      else sysStatus_[idx]->setBackground(QBrush(Qt::yellow));

      sysStatus_[idx]->setText(value.append(" "));
      statTable_->resizeColumnsToContents();

   }
   else if ( field == "RunState" ) {

      if ( idx == 0 ) {
         x = runState_->findText(value);
         runState_->setCurrentIndex(x);
      }

      sysRunState_[idx]->setText(value.append(" "));
      statTable_->resizeColumnsToContents();

   }
   else if ( idx == 0 && field == "WorkingDir" ) {
      value = value;
      if ( lastLoadSettings_ == "" ) {
         lastLoadSettings_ = value;
         lastLoadSettings_.append("/config/defaults.xml");
      }
   }
   update();
}


void SummaryWindow::setRunStateEnums(uint32_t idx, vector<QString> enums) {
   uint32_t x;
   if ( idx == 0 ) {
      for (x=0; x < enums.size(); x++ ) runState_->addItem(enums[x]);
   }
}

