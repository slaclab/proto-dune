//-----------------------------------------------------------------------------
// File          : CommandHolder.h
// Author        : Ryan Herbst  <rherbst@slac.stanford.edu>
// Created       : 10/04/2011
// Project       : General purpose
//-----------------------------------------------------------------------------
// Description :
// Command holder
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
#ifndef __COMMAND_HOLDER_H__
#define __COMMAND_HOLDER_H__
#include <QString>
#include <vector>
#include <QDomDocument>
#include <QObject>
#include <QToolButton>
#include <QLineEdit>
#include <QTreeWidget>
#include <QTreeWidgetItem>
using namespace std;

class CommandHolder : public QObject {

   Q_OBJECT

      // Tracking values
      QString fullName_; 
      QString shortName_; 
      QString path_; 

      bool    hasArg_;
      QString desc_;
      bool    hidden_;

      QLineEdit   *arg_;
      QToolButton *btn_;
      QToolButton *hlp_;

   public:

      // Creation Class
      CommandHolder ( );
      CommandHolder ( CommandHolder * );

      // Delete
      ~CommandHolder ( );

      // Has arg
      bool hasArg();

      // Is hidden
      bool isHidden();

      // Add device level information
      void addDevice ( QDomNode node );

      // Add command level information
      void addCommand ( QDomNode node );

      // Parse ID string
      void parseId ( QString id );

      // Get ID string
      QString getId ();

      // Get path name
      QString path ();

      // Get XML string
      QString getXml (QString arg);

      // Get short name
      QString shortName ();

      // Set enabled state
      void setEnabled(bool state);

      // Update tree widget item with fields
      void setupItem (QTreeWidgetItem *item);

   public slots:

      void pressed();
      void hPressed();

   signals:

      void commandPressed(QString xml);
      void helpPressed(QString name, QString desc);

};

#endif
