//-----------------------------------------------------------------------------
// File          : XmlClient.cpp
// Author        : Ryan Herbst  <rherbst@slac.stanford.edu>
// Created       : 10/04/2011
// Project       : General Purpose
//-----------------------------------------------------------------------------
// Description :
// XML client for server connections.
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
#include <QMessageBox>
#include "XmlClient.h"
using namespace std;

// Constructor
XmlClient::XmlClient ( ) {

   // Network setup
   tcpSocket_ = new QTcpSocket(this);
   connect(tcpSocket_, SIGNAL(readyRead()), this, SLOT(sockReady()));
   connect(tcpSocket_, SIGNAL(connected()), this, SLOT(sockConnected()));
   connect(tcpSocket_, SIGNAL(disconnected()), this, SLOT(sockDisconnected()));
   connect(tcpSocket_, SIGNAL(error(QAbstractSocket::SocketError)), 
           this,       SLOT(sockError(QAbstractSocket::SocketError)));

   tcpStream_ = new QTextStream(tcpSocket_);
   host_      = "";
   port_      = 0;
   tries_     = 0;
   xmlBuffer_ = "";
}


// Delete
XmlClient::~XmlClient ( ) { 
   delete tcpStream_;
   delete tcpSocket_;
}


void XmlClient::sockConnected() { 
   cout << "XmlClient::sockConnected -> Connected to host " << qPrintable(host_) << ":" << port_ << endl;
}


void XmlClient::sockDisconnected() {
   cout << "XmlClient::sockDisconnected -> Socket Disconnected for host " << qPrintable(host_) << endl;
   lostConnection();
}


void XmlClient::sockError(QAbstractSocket::SocketError) {
   cout << "XmlClient::sockError -> Socket error detected for host " << qPrintable(host_) << endl;
   if ( tries_++ < 5 ) {
      port_++;
      cout << "XmlClient::sockError -> Retry for host " << qPrintable(host_) << " port " << dec << port_ << endl;
      openServer (host_, (int)port_);
   }
   else lostConnection();
}


void XmlClient::sockReady() {
   bool statusDone = false;
   bool configDone = false;
   bool structDone = false;

   // Append buffer
   xmlBuffer_.append(tcpStream_->readAll());

   // Look for end character
   if ( xmlBuffer_.contains("\f") ) {

      // Parse
      QDomDocument doc("temp");
      doc.setContent(xmlBuffer_);
      xmlBuffer_ = "";

      // Get top level
      QDomElement elem = doc.documentElement();
      QDomNode node;

      // Process first child element
      node = elem.firstChild();
     
      // Process each sub sibling
      while ( ! node.isNull() ) {
         if ( node.isElement() ) {

            // Node is an error
            if ( node.nodeName() == "error" ) {
                  cout << "XmlClient::sockReady -> Found Error from host " << qPrintable(host_) << ": " 
                       << qPrintable(node.firstChild().nodeValue()) << endl;

               // Send error signal
               gotError(node.firstChild().nodeValue());
            }

            // Node is structure
            else if ( node.nodeName() == "structure" ) {
               cmdResStructure(node.firstChild(),true,NULL,NULL);
               structDone = true;
            }
            else if ( node.nodeName() == "config" ) {
               cmdResVariable(node.firstChild(),false,"");
               configDone = true;
            }
            else if ( node.nodeName() == "status" ) {
               cmdResVariable(node.firstChild(),true,"");
               statusDone = true;
            }
         }
         node = node.nextSibling();
      }
      if ( structDone ) createDone();
      if ( configDone || statusDone ) updateDone(statusDone,configDone);
   }
}


void XmlClient::openServer (QString host, int port) {
   host_ = host;
   port_ = port;

   cout << "XmlClient::openServer -> Connecting to host " << qPrintable(host_) << ":" << port_ << endl;
   tcpSocket_->connectToHost(host,port);
}


void XmlClient::closeServer() {
   cout << "XmlClient::closeServer -> Closing host " << qPrintable(host_) << endl;
   tcpSocket_->disconnectFromHost();
}


// Send commands
void XmlClient::sendCommand ( QString cmd ) {
   *tcpStream_ << QString("<system><command>");
   *tcpStream_ << cmd;
   *tcpStream_ << QString("</command></system>\n\f");
   tcpStream_->flush();
}


// Send config
void XmlClient::sendConfig ( QString cfg ) {
   *tcpStream_ << QString("<system><config>");
   *tcpStream_ << cfg;
   *tcpStream_ << QString("</config></system>\n\f");
   tcpStream_->flush();
}


// Send config & command
void XmlClient::sendConfigCommand(QString cfg, QString cmd) {
   *tcpStream_ << QString("<system><config>");
   *tcpStream_ << cfg;
   *tcpStream_ << QString("</config><command>");
   *tcpStream_ << cmd;
   *tcpStream_ << QString("</command></system>\n\f");
   tcpStream_->flush();
}


void XmlClient::cmdResStructure ( QDomNode node, bool top, CommandHolder * cmdHolder, VariableHolder * varHolder ) {
   CommandHolder  *  localCmd  = NULL;
   VariableHolder *  localVar  = NULL;

   while ( ! node.isNull() ) {
      if ( node.isElement() ) {

         // copy or create holders
         if ( cmdHolder == NULL ) localCmd = new CommandHolder; 
         else localCmd = new CommandHolder(cmdHolder);

         if ( varHolder == NULL ) localVar = new VariableHolder; 
         else localVar = new VariableHolder(varHolder);

         if ( top && node.nodeName() == "description" )
            createDescription(node.firstChild().nodeValue());

         // Device found
         else if ( node.nodeName() == "device" ) {
            localCmd->addDevice(node.firstChild());
            localVar->addDevice(node.firstChild());

            // Process next level
            cmdResStructure ( node.firstChild(), false, localCmd, localVar );
         }

         // Command found
         else if ( node.nodeName() == "command" ) {
            localCmd->addCommand(node.firstChild());
            createCommand(new CommandHolder(localCmd));
         }

         // Variable found
         else if ( node.nodeName() == "variable" ) {
            localVar->addVariable(node.firstChild());
            createVariable(new VariableHolder(localVar));
         }

         delete localCmd;
         delete localVar;
      }
      node = node.nextSibling();
   }
}


void XmlClient::cmdResVariable (QDomNode node, bool status, QString id) {
   QString          locId;
   QString          index;
   QString          value;
   VarMap::iterator iter;

   while ( ! node.isNull() ) {
      if ( node.isElement() ) {
         locId = id;

         // Append node name to id
         if ( locId != "" ) locId.append(":");
         locId.append(node.nodeName());

         // Node has index
         if ( node.hasAttributes() ) {
            index = node.attributes().namedItem("index").nodeValue();
            locId.append("(");
            locId.append(index);
            locId.append(")");
         }

         // Get value
         value = node.firstChild().nodeValue();

         // Update variable
         updateVariable(status,locId,value);

         // Process child
         cmdResVariable(node.firstChild(),status,locId);
      }
      node = node.nextSibling();
   }
}

