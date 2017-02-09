//-----------------------------------------------------------------------------
// File          : SystemWindow.h
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
#ifndef __SYSTEM_WINDOW_H__
#define __SYSTEM_WINDOW_H__

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
#include <QVBoxLayout>
using namespace std;

class CommandHolder;
class MainWindow;

class SystemWindow : public QWidget {
   
   Q_OBJECT

      QVBoxLayout *top_;
      MainWindow  *main_;

      // Window groups
      QGroupBox *counterBox();
      QGroupBox *configBox();
      QGroupBox *dataBox();
      QGroupBox *cmdBox();

      // Objects
      QLineEdit        *countDataRx_;
      QLineEdit        *countDataFile_;
      QLineEdit        *countUnexp_;
      QLineEdit        *countRegRx_;
      QLineEdit        *countTimeout_;
      QLineEdit        *countError_;
      QLineEdit        *countRun_;
      QPushButton      *setDefaults_;
      QPushButton      *configRead_;
      QPushButton      *configSave_;
      QPushButton      *refreshState_;
      QTextEdit        *stateLine_;
      QPushButton      *softReset_;
      QPushButton      *hardReset_;
      QLineEdit        *dataFile_;
      QPushButton      *dataBrowse_;
      QPushButton      *dataOpen_;
      QPushButton      *dataClose_;
      QPushButton      *tb_;
      QPushButton      *pb_;
      QComboBox        *runRate_;
      QComboBox        *runState_;
      QSpinBox         *runCount_;
      QProgressBar     *runProgress_;
      bool             readOnly_;

      // Process response
      void cmdResStatus    (QDomNode node);
      void cmdResStructure (QDomNode node);
      void cmdResConfig    (QDomNode node);

      // Holders
      QString workingDir_;
      QString lastLoadSettings_;
      QString lastSaveSettings_;
      QString lastData_;

      // states
      QString stateMsg_;
      QString sysStatus_;

      // Local
      bool isLocal_;

      // Main Window Control
      bool hideData_;
      bool hideRun_;

      // Send command along with system window config
      void topConfigCommand(QString cmd);
      void updateState ();

   public:

      // Creation Class
      SystemWindow ( MainWindow *main, bool local );

      // Delete
      ~SystemWindow ( );

      // Manipulation functions
      void runStateEnums(vector<QString> enums);
      void runRateEnums(vector<QString> enums);
      void setConfigVariable(QString name, QString value);
      bool setStatusVariable(QString name, QString value);
      void refresh(bool status);
      void setReadOnly(bool ro);

   public slots:

      void setDefaultsPressed();
      void configReadPressed();
      void configSavePressed();
      void refreshStatePressed();
      void browseDataPressed();
      void openDataPressed();
      void closeDataPressed();
      void resetCountPressed();
      void runStateActivated(const QString &);
      void hardResetPressed();
      void softResetPressed();
};

#endif
