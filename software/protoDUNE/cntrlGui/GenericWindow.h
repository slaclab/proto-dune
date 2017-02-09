//-----------------------------------------------------------------------------
// File          : GenericWindow.h
// Author        : Ryan Herbst  <rherbst@slac.stanford.edu>
// Created       : 10/04/2011
// Project       : General purpose
//-----------------------------------------------------------------------------
// Description :
// Generic window
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
#ifndef __GENERIC_WINDOW_H__
#define __GENERIC_WINDOW_H__

#include <QWidget>
#include <QDomDocument>
#include <QTableWidgetItem>
#include <QGroupBox>
#include <QTabWidget>
#include <QPushButton>
#include <QComboBox>
#include <QSpinBox>
#include <QTreeWidget>
#include <QTreeWidgetItem>
using namespace std;

class MainWindow;

typedef map<QString,QTreeWidgetItem *> TreeMap;

class GenericWindow : public QWidget {
   
   Q_OBJECT

   protected:

      MainWindow * main_;

      QString           lastPath_;
      QTreeWidgetItem * lastItem_;

      // Top branch
      QTreeWidgetItem * top_;
      QTreeWidget     * tree_;

      // Tree Widget Map
      TreeMap treeList_;

      // Find and create path
      QTreeWidgetItem * findPath  ( QString path );
      QTreeWidgetItem * makePath  ( QTreeWidgetItem *base, QString basePath, QString subPath );

   public:

      // Creation Class
      GenericWindow ( MainWindow *main );

      // Delete
      ~GenericWindow ( );

};

#endif
