//-----------------------------------------------------------------------------
// File          : CommandWindow.h
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
#ifndef __COMMAND_WINDOW_H__
#define __COMMAND_WINDOW_H__

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
#include "CommandHolder.h"
#include "GenericWindow.h"
using namespace std;

class MainWindow;

class CommandWindow : public GenericWindow {
   
   Q_OBJECT

      // Command list
      vector<CommandHolder *> cmdList_;

      // Last path
      QString           lastPath_;
      QTreeWidgetItem * lastItem_;
      bool readOnly_;

   public:

      // Creation Class
      CommandWindow ( MainWindow *main );

      // Delete
      ~CommandWindow ( );

      // Create a device 
      void createCommand ( CommandHolder * );

      // Create done
      void createDone();

      void setReadOnly(bool ro);

};

#endif
