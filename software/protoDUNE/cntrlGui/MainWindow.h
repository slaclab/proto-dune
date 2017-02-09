//-----------------------------------------------------------------------------
// File          : MainWindow.h
// Author        : Ryan Herbst  <rherbst@slac.stanford.edu>
// Created       : 10/04/2011
// Project       : General purpose
//-----------------------------------------------------------------------------
// Description :
// Top level control window
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
#ifndef __CONTROL_GUI_H__
#define __CONTROL_GUI_H__

#include <stdint.h>
#include <QWidget>
#include <QDomDocument>
#include <QTreeWidgetItem>
#include <QThread>
#include "XmlClient.h"
using namespace std;

class SystemWindow;
class CommandWindow;
class VariableWindow;
class CommandHolder;
class VariableHolder;

class MainWindow : public QWidget {
   
   Q_OBJECT

      uint32_t idx_;

      // Host and port
      QString  host_;
      uint32_t port_;

      // Window
      SystemWindow   * systemWindow_;
      CommandWindow  * commandWindow_;
      VariableWindow * statusWindow_;
      VariableWindow * configWindow_;

      // XML client
      XmlClient xmlClient_;

      QThread thread_;

   public:

      // Creation Class
      MainWindow ( uint32_t idx, QString host, uint32_t port );

      // Delete
      ~MainWindow ( );

      // Local Methdods
      void localCommand(QString cmd);
      void localConfig(QString cmd);
      void localConfigCommand(QString cfg, QString cmd);
      void setReadOnly(bool ro);

   private slots:

      void openPgpPressed ();
      void lostConnectionSlot ();
      void cfgReadPressed ();
      void cfgWritePressed ();
      void cfgVerifyPressed ();
      void statReadPressed ();
      void helpPressed (QString, QString);
      void commandPressed (QString);
      void createDescription ( QString );
      void createCommand ( CommandHolder * );
      void createVariable ( VariableHolder * );
      void createDone();
      void updateVariable ( bool, QString, QString );
      void updateDone(bool,bool);
      void gotError(QString);

   public slots:

      void externalCommand(QString cmd);
      void windowSelected(uint32_t);

   signals:

      void openServer(QString host, int port);
      void sendCommand(QString cmd);
      void sendConfig(QString cmd);
      void sendConfigCommand(QString cfg, QString cmd);
      void updateSummaryField(uint32_t idx, QString field, QString value);
      void setRunStateEnums(uint32_t idx, vector<QString> enums);
      void openPgp();
      void addPgpRow(QString path);
      void addPgpDone();
      void updatePgpField(QString id, QString value);
      void lostConnection();

};

#endif
