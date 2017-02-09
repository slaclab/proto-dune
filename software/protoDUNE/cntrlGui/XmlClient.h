//-----------------------------------------------------------------------------
// File          : XmlClient.h
// Author        : Ryan Herbst  <rherbst@slac.stanford.edu>
// Created       : 10/04/2011
// Project       : General Purpose
//-----------------------------------------------------------------------------
// Description :
// XML client for server connections
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
#ifndef __XML_CLIENT_H__
#define __XML_CLIENT_H__

#include <stdint.h>
#include <QTcpSocket>
#include <QObject>
#include <QTimer>
#include <QDomDocument>
#include "VariableHolder.h"
#include "CommandHolder.h"
using namespace std;

class XmlClient : public QObject {
   
   Q_OBJECT

      // Host status
      bool hostOpen_;

      // Network
      QTcpSocket  *tcpSocket_;
      QTextStream *tcpStream_;

      // Receive buffer
      QString   xmlBuffer_;

      // Host and port
      QString  host_;
      uint32_t port_;

      // Try count
      int     tries_;

   public:

      // Creation Class
      XmlClient ( );

      // Delete
      ~XmlClient ( );

   private slots:

      void sockConnected();
      void sockDisconnected();
      void sockError(QAbstractSocket::SocketError socketError);
      void sockReady();

   public slots:

      // Open server
      void openServer (QString host, int port);

      // Close server
      void closeServer();

      // Send commands
      void sendCommand(QString cmd);

      // Send config
      void sendConfig(QString cfg);

      // Send config & command
      void sendConfigCommand(QString cfg, QString cmd);

   signals:

      // Create a description
      void createDescription ( QString );

      // Create a device 
      void createCommand ( CommandHolder * );

      // Create a Variable
      void createVariable ( VariableHolder * );

      // Create done
      void createDone();

      // Update Variable
      void updateVariable ( bool, QString, QString );

      // Update done
      void updateDone(bool,bool);

      // Lost connection
      void lostConnection ();

      // Got error
      void gotError(QString);

   private:

      // Structure result
      void cmdResStructure ( QDomNode node, bool top, CommandHolder *cmdHolder, VariableHolder *varHolder );

      // Variable result
      void cmdResVariable (QDomNode node, bool status, QString id);

};

#endif

