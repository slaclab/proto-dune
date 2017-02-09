//-----------------------------------------------------------------------------
// File          : PgpLinkWindow.h
// Author        : Ryan Herbst  <rherbst@slac.stanford.edu>
// Created       : 10/04/2011
// Project       : General Purpose
//-----------------------------------------------------------------------------
// Description :
// PGP Link monitor window
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
#ifndef __PGP_LINK_WINDOW_H__
#define __PGP_LINK_WINDOW_H__

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
#include <QTreeWidget>
#include <QTreeWidgetItem>
#include <QCheckBox>
#include <QTimer>
#include "MainWindow.h"
#include "VariableHolder.h"
using namespace std;

typedef map<QString,QTableWidgetItem *> FieldMap;

class PgpLinkWindow : public QWidget {
   
   Q_OBJECT

      QLineEdit * logFile_;
      QCheckBox * logEnable_;
      QTimer      logTimer_;

      // Top level widget
      QTableWidget * statTable_;

      // Map of fields
      FieldMap fieldMap_;

      QPushButton * refreshState_;
      QPushButton * countReset_;

      int gotCount_;
      int expCount_;

   public:

      // Creation Class
      PgpLinkWindow ( int count );

      // Delete
      ~PgpLinkWindow ( );

      // Readonly
      void setReadOnly(bool ro);

   private slots:

      void refreshStatePressed();
      void countResetPressed();
      void logEnableChanged(int state);
      void timeout();

   public slots:

      void addPgpRow(QString path);
      void addPgpDone();
      void updatePgpField(QString id, QString value);

   signals:

      void sendCommand(QString cmd);

};

#endif
