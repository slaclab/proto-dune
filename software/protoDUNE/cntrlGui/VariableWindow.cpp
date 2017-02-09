//-----------------------------------------------------------------------------
// File          : VariableWindow.cpp
// Author        : Ryan Herbst  <rherbst@slac.stanford.edu>
// Created       : 10/04/2011
// Project       : General Purpose
//-----------------------------------------------------------------------------
// Description :
// Command window in top GUI
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
#include <QDomDocument>
#include <QObject>
#include <QHeaderView>
#include <QMessageBox>
#include <QTabWidget>
#include <QTableWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <QToolButton>
#include <QLineEdit>
#include <QGroupBox>
#include <QFileDialog>
#include <QFormLayout>
#include <QComboBox>
#include <QLabel>
#include <QTreeWidget>
#include <QTreeWidgetItem>
#include "VariableWindow.h"
#include "VariableHolder.h"
#include "MainWindow.h"
using namespace std;

// Constructor
VariableWindow::VariableWindow ( QString type, MainWindow *main ) : GenericWindow (main) {

   type_ = type;

   QVBoxLayout *vbox = new QVBoxLayout;
   setLayout(vbox);

   tree_ = new QTreeWidget();
   vbox->addWidget(tree_);

   tree_->setColumnCount(3);
   QStringList hdr;
   hdr << "Variable" << "Value" << "Decode" << "Info";
   tree_->setHeaderLabels(hdr);

   top_ = new QTreeWidgetItem(tree_);
   top_->setText(0,type_);
   tree_->addTopLevelItem(top_);
   top_->setExpanded(true);

   QHBoxLayout *hbox = new QHBoxLayout;
   vbox->addLayout(hbox);

   read_ = new QPushButton(QString("Read ").append(type_));
   hbox->addWidget(read_);

   if ( type_ == "Status" ) {
      connect(read_,SIGNAL(pressed()),main_,SLOT(statReadPressed()));
   } else {
      connect(read_,SIGNAL(pressed()),main_,SLOT(cfgReadPressed()));

      write_ = new QPushButton(QString("Write ").append(type_));
      hbox->addWidget(write_);
      connect(write_,SIGNAL(pressed()),main_,SLOT(cfgWritePressed()));

      verify_ = new QPushButton(QString("Verify ").append(type_));
      hbox->addWidget(verify_);
      connect(verify_,SIGNAL(pressed()),main_,SLOT(cfgVerifyPressed()));
   }
}

// Delete
VariableWindow::~VariableWindow ( ) { }

// set readonly
void VariableWindow::setReadOnly (bool ro) {
   read_->setEnabled(not ro);

   if ( type_ != "Status" ) {
      write_->setEnabled(not ro);
      verify_->setEnabled(not ro);
   }
}

// Create a device 
void VariableWindow::createVariable ( VariableHolder *var ) {
   QTreeWidgetItem * item;
   QTreeWidgetItem * subItem;

   item = findPath(var->path());

   subItem = new QTreeWidgetItem(item);
   var->setupItem(subItem);
   item->addChild(subItem);

   connect(var,SIGNAL(helpPressed(QString,QString)),main_,SLOT(helpPressed(QString,QString)));

   varList_.insert(pair<QString,VariableHolder*>(var->getId(),var));
}


// Create done
void VariableWindow::createDone() {
   tree_->expandAll();
   tree_->resizeColumnToContents(0);
}


// Update Variable
QString VariableWindow::updateVariable ( QString id, QString value ) {
   VarMap::iterator iter;

   if ( (iter = varList_.find(id)) != varList_.end() ) {
      iter->second->updateValue(value);
      return(iter->second->getValue());
   }
   return("");
}


// Update done
void VariableWindow::updateDone() {

}


// Get xml string
QString VariableWindow::getConfig() {
   QString cmd;
   VarMap::iterator iter;

   for (iter = varList_.begin(); iter != varList_.end(); iter++) 
      cmd.append(iter->second->getXml());

   return(cmd);
}

