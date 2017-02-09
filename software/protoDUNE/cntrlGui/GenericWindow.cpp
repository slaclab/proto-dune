//-----------------------------------------------------------------------------
// File          : GenericWindow.cpp
// Author        : Ryan Herbst  <rherbst@slac.stanford.edu>
// Created       : 10/04/2011
// Project       : General Purpose
//-----------------------------------------------------------------------------
// Description :
// Generic Window
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
#include "GenericWindow.h"
#include "MainWindow.h"
using namespace std;

// Constructor
GenericWindow::GenericWindow ( MainWindow *main ) : QWidget () {
   main_     = main;
   lastItem_ = NULL;
   lastPath_ = "";
}

// Delete
GenericWindow::~GenericWindow ( ) { }


QTreeWidgetItem * GenericWindow::makePath  ( QTreeWidgetItem *base, QString basePath, QString subPath ) {
   QTreeWidgetItem * newItem;
   QTreeWidgetItem * item;
   QString           npath;
   uint32_t x;

   npath = basePath;
   item  = base;

   QStringList spath = subPath.split(":");

   for (x=0; x < (uint32_t)spath.size(); x++) {
      if ( npath != "" ) npath.append(":");
      npath.append(spath.at(x));

      newItem = new QTreeWidgetItem(item);
      item->addChild(newItem);
      newItem->setText(0,spath.at(x));
   
      treeList_.insert(pair<QString,QTreeWidgetItem*>(npath,newItem));

      item = newItem;
   }

   return(item);
}


QTreeWidgetItem * GenericWindow::findPath  ( QString path ) {
   TreeMap::iterator iter;

   // Repeat path
   if ( lastPath_ == path && lastItem_ != NULL ) return(lastItem_);
   lastPath_ = path;

   // Empty path
   if ( path == "" ) {
      lastItem_ = top_;
      return(top_);
   }

   // Path found
   if ( (iter = treeList_.find(path)) != treeList_.end() ) {
      lastItem_ = iter->second;
      return(iter->second);
   }

   QTreeWidgetItem * item = NULL;
   QString delim    = ":";
   QString basePath = path;
   QString subPath  = "";
   int idx = path.length();

   do {

      // Remove one device level from end of path
      idx = path.lastIndexOf(delim,idx-1);

      // No device breaks in path
      if ( idx <= 0 ) {
         basePath = "";
         subPath  = path;
         item     = top_;
      }
      else {

         // Generate new break
         basePath = path.mid(0,idx);
         subPath  = path.mid(idx+1);

         // Find path
         if ( (iter = treeList_.find(basePath)) != treeList_.end() ) item = iter->second;
      }
   } while ( item == NULL );

   item = makePath ( item, basePath, subPath );

   lastItem_ = item;

   return(item);
}

