//-----------------------------------------------------------------------------
// File          : CommandWindow.cpp
// Author        : Ryan Herbst  <rherbst@slac.stanford.edu>
// Created       : 10/04/2011
// Project       : General purpose
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
#include "CommandWindow.h"
#include "CommandHolder.h"
#include "MainWindow.h"
using namespace std;

// Constructor
CommandWindow::CommandWindow ( MainWindow *main ) : GenericWindow (main) {

   QVBoxLayout *vbox = new QVBoxLayout;
   setLayout(vbox);

   tree_ = new QTreeWidget();
   vbox->addWidget(tree_);

   tree_->setColumnCount(3);
   QStringList hdr;
   hdr << "Command" << "Arg" << "Execute" << "Info";
   tree_->setHeaderLabels(hdr);

   top_ = new QTreeWidgetItem(tree_);
   top_->setText(0,"Commands");
   tree_->addTopLevelItem(top_);
   top_->setExpanded(true);
   readOnly_ = false;
}


// Delete
CommandWindow::~CommandWindow ( ) { 

}

void CommandWindow::setReadOnly(bool ro) {
   vector<CommandHolder *>::iterator i;

   readOnly_ = ro;

   for(i=cmdList_.begin() ; i < cmdList_.end(); i++ ) {
      (*i)->setEnabled(not ro);
   }
}

// Create a device 
void CommandWindow::createCommand ( CommandHolder *cmd ) {
   QTreeWidgetItem * item;
   QTreeWidgetItem * subItem;

   item = findPath(cmd->path());

   subItem = new QTreeWidgetItem(item);
   cmd->setupItem(subItem);
   item->addChild(subItem);

   cmd->setEnabled(not readOnly_);

   cmdList_.push_back(cmd);

   connect(cmd,SIGNAL(commandPressed(QString)),main_,SLOT(commandPressed(QString)));
   connect(cmd,SIGNAL(helpPressed(QString,QString)),main_,SLOT(helpPressed(QString,QString)));
}


// Create done
void CommandWindow::createDone() {
   tree_->expandAll();
   tree_->resizeColumnToContents(0);
}

