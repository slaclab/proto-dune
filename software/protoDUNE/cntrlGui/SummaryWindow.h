//-----------------------------------------------------------------------------
// File          : SummaryWindow.h
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
#ifndef __SUMMARY_WINDOW_H__
#define __SUMMARY_WINDOW_H__

#include <QWidget>
#include <QDomDocument>
#include <QTableWidgetItem>
#include <QGroupBox>
#include <QTabWidget>
#include <QPushButton>
#include <QComboBox>
#include <QSpinBox>
#include <QObject>
#include <QTextEdit>
#include <QProgressBar>
#include "MainWindow.h"
using namespace std;

class CommandHolder;

class SummaryWindow : public QWidget {
   
   Q_OBJECT

      // Window groups
      QGroupBox *statusBox();
      QGroupBox *configBox();
      QGroupBox *runBox();

      // Objects
      QPushButton      *setDefaults_;
      QPushButton      *configRead_;
      QPushButton      *refreshState_;
      QPushButton      *softReset_;
      QPushButton      *hardReset_;
      QPushButton      *tb_;
      QPushButton      *pgp_;

      QComboBox        *runState_;

      // Status Objects
      QTableWidget     *statTable_;
      QTableWidgetItem **systemName_;
      QTableWidgetItem **sysRunState_;
      QTableWidgetItem **sysStatus_;
      QTableWidgetItem **sysTimeouts_;
      QTableWidgetItem **sysRegRx_;
      QTableWidgetItem **sysDataRx_;
      QTableWidgetItem **sysErrors_;

      // Holders
      QString lastLoadSettings_;
      QString workingDir_;

      int count_;
      MainWindow *wins_;

   public:

      // Creation Class
      SummaryWindow ( int count );

      // Delete
      ~SummaryWindow ( );

      // Readonly
      void setReadOnly(bool ro);

   private slots:

      void cellSelected(int,int);
      void setDefaultsPressed();
      void configReadPressed();
      void refreshStatePressed();
      void resetCountPressed();
      void runStateActivated(const QString &);
      void hardResetPressed();
      void softResetPressed();
      void pgpPressed();

   public slots:

      void updateSummaryField (uint32_t idx, QString field, QString value);
      void setRunStateEnums(uint32_t idx, vector<QString> enums);

   signals:

      void openPgp();
      void sendSummaryCommand(QString cmd);
      void windowSelected(uint32_t);
};

#endif

