//-----------------------------------------------------------------------------
// File          : CntrlGui.cpp
// Author        : Ryan Herbst  <rherbst@slac.stanford.edu>
// Created       : 03/22/2011
// Project       : General purpose
//-----------------------------------------------------------------------------
// Description :
// Main program
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
// 03/22/2011: created
//-----------------------------------------------------------------------------
#include <iostream>
#include <iomanip>
#include <sstream>
#include <string>
#include <signal.h>
#include <unistd.h>
#include <QApplication>
#include <QErrorMessage>
#include <QObject>
#include "XmlClient.h"
#include "MainWindow.h"
#include "PgpLinkWindow.h"
#include "SystemWindow.h"
#include "CommandWindow.h"
#include "VariableWindow.h"
#include "SummaryWindow.h"
using namespace std;

// Main Function
int main ( int argc, char **argv ) {
   int x;

   // Determine the number of hosts
   int count = (argc > 3)?argc-2:1;

   QString host[count];
   int     port;

   // Start application
   QApplication a( argc, argv );

   if ( argc < 2 ) {
      host[0] = "localhost";
      port    = 8090;
   }
   else if ( argc > 2 ) {
      host[0] = argv[1];
      port    = atoi(argv[2]);

      for (x=1; x < count; x++) host[x] = argv[x+2];
   }
   else exit(1);

   MainWindow  * mainWin[count];
   SummaryWindow sumWin(count);
   PgpLinkWindow pgpWin(count);
   
   string ppath = argv[0];
   string pname = ppath.substr( ppath.find_last_of( '/' ) + 1 );
   bool readOnly;

   if ( pname != "cntrlGui" ) {
      printf("Starting in readonly mode!\n");
      readOnly = true;
   }
   else readOnly = false;

   if ( readOnly ) {
      sumWin.setReadOnly(true);
      pgpWin.setReadOnly(true);
   }

   for (x=0; x < count; x++) {

      mainWin[x] = new MainWindow(x,host[x],port);

      if ( readOnly ) {
         mainWin[x]->setReadOnly(true);
      }

      // Exit on lost connection
      QObject::connect(mainWin[x],SIGNAL(lostConnection()),&a,SLOT(closeAllWindows()));

      QObject::connect(&sumWin,SIGNAL(sendSummaryCommand(QString)),
                       mainWin[x],SLOT(externalCommand(QString)));

      QObject::connect(&sumWin,SIGNAL(windowSelected(uint32_t)),
                       mainWin[x],SLOT(windowSelected(uint32_t)));

      QObject::connect(mainWin[x],SIGNAL(updateSummaryField(uint32_t,QString,QString)),
                       &sumWin,SLOT(updateSummaryField(uint32_t,QString,QString)));

      QObject::connect(mainWin[x],SIGNAL(setRunStateEnums(uint32_t,vector<QString>)),
                       &sumWin,SLOT(setRunStateEnums(uint32_t,vector<QString>)));

      QObject::connect(mainWin[x],SIGNAL(openPgp()),&pgpWin,SLOT(show()));

      QObject::connect(mainWin[x],SIGNAL(addPgpRow(QString)),&pgpWin,SLOT(addPgpRow(QString)));

      QObject::connect(mainWin[x],SIGNAL(addPgpDone()),&pgpWin,SLOT(addPgpDone()));

      QObject::connect(mainWin[x],SIGNAL(updatePgpField(QString,QString)),
                       &pgpWin,SLOT(updatePgpField(QString,QString)));

      QObject::connect(&pgpWin,SIGNAL(sendCommand(QString)),mainWin[x],SLOT(externalCommand(QString)));
   }

   // Summary opening pgp window
   QObject::connect(&sumWin,SIGNAL(openPgp()),&pgpWin,SLOT(show()));

   // Exit on last window closed
   QObject::connect(&a,SIGNAL(lastWindowClosed()), &a, SLOT(quit())); 

   if ( count == 1 ) mainWin[0]->show();
   else sumWin.show();

   // Run application
   return(a.exec());
}

