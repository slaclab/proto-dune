//-----------------------------------------------------------------------------
// File          : VariableWindow.h
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
#ifndef __VARIABLE_WINDOW_H__
#define __VARIABLE_WINDOW_H__

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
#include "VariableHolder.h"
#include "GenericWindow.h"
using namespace std;

class MainWindow;

class VariableWindow : public GenericWindow {
   
   Q_OBJECT

      // Type
      QString type_;

      // Read status button
      QPushButton *read_;
      QPushButton *write_;
      QPushButton *verify_;

      // Variables
      VarMap varList_;

   public:

      // Creation Class
      VariableWindow ( QString type, MainWindow *main );

      // Delete
      ~VariableWindow ( );

      // Readonly
      void setReadOnly(bool ro);

      // Create a device 
      void createVariable ( VariableHolder * );

      // Create done
      void createDone();

      // Update Variable
      QString updateVariable ( QString, QString );

      // Update done
      void updateDone();

      // Get xml string
      QString getConfig();

};

#endif
