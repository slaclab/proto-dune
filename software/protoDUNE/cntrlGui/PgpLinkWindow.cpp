//-----------------------------------------------------------------------------
// File          : PgpLinkWindow.cpp
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
#include <iomanip>
#include <QDomDocument>
#include <QObject>
#include <QDateTime>
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
#include <QTextStream>
#include <QInputDialog>
#include <QFormLayout>
#include <QComboBox>
#include <QLabel>
#include "PgpLinkWindow.h"
using namespace std;


// Constructor
PgpLinkWindow::PgpLinkWindow (int count ) : QWidget () {
   QString tmp;

   gotCount_ = 0;
   expCount_ = count;

   setWindowTitle("PGP Link Status");

   QVBoxLayout *vbox = new QVBoxLayout;
   this->setLayout(vbox);

   statTable_ = new QTableWidget;
   statTable_->setColumnCount(7);

   QStringList header;
   header << "Device Name" << "LinkReady" << "RxCount" << "FrameErrorCount" << "CellErrorCount" << "LinkErrorCount" << "LinkDownCount";
   statTable_->setHorizontalHeaderLabels(header);
   statTable_->verticalHeader()->setVisible(false);
   statTable_->setEditTriggers(QAbstractItemView::NoEditTriggers);
   statTable_->setSelectionMode(QAbstractItemView::SingleSelection);
   statTable_->setShowGrid(false);
   statTable_->horizontalHeader()->setStretchLastSection(false);
   statTable_->horizontalHeader()->setResizeMode(0, QHeaderView::ResizeToContents);
   statTable_->setMinimumHeight(400);
   statTable_->setMinimumWidth(1000);

   vbox->addWidget(statTable_);

   QHBoxLayout *hbox1 = new QHBoxLayout;
   vbox->addLayout(hbox1);

   refreshState_ = new QPushButton("RefreshState");
   hbox1->addWidget(refreshState_);

   countReset_ = new QPushButton("CounterReset");
   hbox1->addWidget(countReset_);

   connect(refreshState_,SIGNAL(pressed()),this,SLOT(refreshStatePressed()));
   connect(countReset_,SIGNAL(pressed()),this,SLOT(countResetPressed()));

   QHBoxLayout *hbox2 = new QHBoxLayout;
   vbox->addLayout(hbox2);

   hbox2->addWidget(new QLabel("Enable Logging:"));

   logEnable_ = new QCheckBox();
   hbox2->addWidget(logEnable_);

   hbox2->addWidget(new QLabel("Log File:"));

   logFile_ = new QLineEdit();
   hbox2->addWidget(logFile_);

   connect(logEnable_,SIGNAL(stateChanged(int)),this,SLOT(logEnableChanged(int)));

   connect(&logTimer_,SIGNAL(timeout()),this,SLOT(timeout()));
}


PgpLinkWindow::~PgpLinkWindow () {
}


void PgpLinkWindow::refreshStatePressed() {
   sendCommand("<RefreshState/>");
}


void PgpLinkWindow::countResetPressed() {
   sendCommand("<ResetCount/>");
}

// set readonly
void PgpLinkWindow::setReadOnly (bool ro) {
   refreshState_->setEnabled(not ro);
   countReset_->setEnabled(not ro);
}

void PgpLinkWindow::logEnableChanged(int state) {
   if ( state ) {
      logFile_->setEnabled(false);
      logTimer_.setInterval(5 * 60 * 1000); // 5 minutes
      logTimer_.start();
      timeout();
   }
   else {
      logFile_->setEnabled(true);
      logTimer_.stop();
   }
}


void PgpLinkWindow::timeout() {

   if ( ! logEnable_->isChecked() ) return;

   QFile file(logFile_->text());

   QTextStream data( &file );
   QStringList strList;
   int         row,col;
   int         rowCnt;
   int         colCnt;
   int         len;
  
   if (! file.open(QFile::WriteOnly | QFile::Append)) {
      printf("Failed to write file %s\n",qPrintable(logFile_->text()));
      return;
   }

   rowCnt = statTable_->rowCount();
   colCnt = statTable_->columnCount();
   int maxCol[colCnt];

   QDateTime tm = QDateTime::currentDateTime();

   data << "\n";
   data << "-------------------------------\n";
   data << tm.toString() << "\n";
   data << "-------------------------------\n";

   for ( col=0; col < colCnt; col++ ) {
      maxCol[col] = statTable_->horizontalHeaderItem(col)->text().length() + 4;

      for ( row=0; row < rowCnt; row++ ) {
         len = statTable_->item(row,col)->text().length() + 4;
         if ( len > maxCol[col] ) maxCol[col] = len;
      }

      data.setFieldWidth(maxCol[col]);
      data << statTable_->horizontalHeaderItem(col)->text();
   }
   data << "\n";

   for ( row = 0; row < rowCnt; row++ ) {
      for ( col=0; col < colCnt; col++ ) {
         data.setFieldWidth(maxCol[col]);
         data << statTable_->item(row,col)->text();
      }
      data << "\n";
   }

   file.close();
}


void PgpLinkWindow::addPgpRow(QString path) {
   int     row;
   QString id;
   QTableWidgetItem * item;

   row = statTable_->rowCount();
   statTable_->setRowCount(row+1);

   id = path;
   id.append("    ");
   statTable_->setItem(row, 0, new QTableWidgetItem(id));

   id = path;
   id.append(":RxCellErrorCount");
   item = new QTableWidgetItem("");
   statTable_->setItem(row, 4, item);
   fieldMap_.insert(pair<QString,QTableWidgetItem*>(id,item));
   
   id = path;
   id.append(":RxLinkDownCount");
   item = new QTableWidgetItem("");
   statTable_->setItem(row, 6, item);
   fieldMap_.insert(pair<QString,QTableWidgetItem*>(id,item));
   
   id = path;
   id.append(":RxLinkErrorCount");
   item = new QTableWidgetItem("");
   statTable_->setItem(row, 5, item);
   fieldMap_.insert(pair<QString,QTableWidgetItem*>(id,item));
   
   id = path;
   id.append(":RxFrameErrorCount");
   item = new QTableWidgetItem("");
   statTable_->setItem(row, 3, item);
   fieldMap_.insert(pair<QString,QTableWidgetItem*>(id,item));

   id = path;
   id.append(":RxFrameCount");
   item = new QTableWidgetItem("");
   statTable_->setItem(row, 2, item);
   fieldMap_.insert(pair<QString,QTableWidgetItem*>(id,item));
   
   id = path;
   id.append(":RxLocalLinkReady");
   item = new QTableWidgetItem("");
   statTable_->setItem(row, 1, item);
   fieldMap_.insert(pair<QString,QTableWidgetItem*>(id,item));
}


void PgpLinkWindow::addPgpDone() {
   if ( ++gotCount_ == expCount_ ) statTable_->sortByColumn(0,Qt::AscendingOrder);
}


void PgpLinkWindow::updatePgpField(QString id, QString value) {
   FieldMap::iterator iter;

   if ( (iter = fieldMap_.find(id)) != fieldMap_.end() ) 
      iter->second->setText(value);
}

