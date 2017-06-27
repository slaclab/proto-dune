//-----------------------------------------------------------------------------
// File          : dataDpmServer.cpp
// Author        : Ryan Herbst  <rherbst@slac.stanford.edu>
// Created       : 08/15/2014
// Project       : HPS
//-----------------------------------------------------------------------------
// Description :
// Data DPM Server
//-----------------------------------------------------------------------------
// This file is part of 'DUNE Development Software'.
// It is subject to the license terms in the LICENSE.txt file found in the 
// top-level directory of this distribution and at: 
//    https://confluence.slac.stanford.edu/display/ppareg/LICENSE.html. 
// No part of 'DUNE Development Software', including this file, 
// may be copied, modified, propagated, or distributed except according to 
// the terms contained in the LICENSE.txt file.
// Proprietary and confidential to SLAC.
//-----------------------------------------------------------------------------
// Modification history :
// 08/15/2014: created
//----------------------------------------------------------------------------
#include <DataDpmSystem.h>
#include <TimingDtmSystem.h>
#include <ControlServer.h>
#include <iomanip>
#include <fstream>
#include <iostream>
#include <signal.h>
#include <RceCommon.h> 

using namespace std;

// Run flag for sig catch
bool stop;

// Function to catch cntrl-c
void sigTerm (int) { 
   cout << "Got Signal!" << endl;
   stop = true; 
}


int main (int argc, char **argv) {
   int32_t pid;
   int32_t opt;
   bool    doBg;
   string  defFile;


   defFile = "config/defaults.xml";
   doBg    = true;
   while ((opt = getopt (argc, argv, "dc:")) != -1)
      switch (opt) {
         case 'd':
            doBg = false;
            printf("Running in foreground\n");
            break;
         case 'c':
            defFile = optarg;
            break;
         case '?':
            if (optopt == 'c') fprintf (stderr, "Option -%c requires an argument.\n", optopt);
            else fprintf (stderr, "Usage: %s [-d] [-c configFile]\n", argv[0]);
            return 1;
            break;
      default:
        fprintf (stderr, "Usage: %s [-d] [-c configFile].\n", argv[0]);
        return 1;
        break;
   }

   printf("Using default file %s\n",defFile.c_str());

   if ( doBg ) {
      pid = fork();

      if ( pid < 0 ) {
         cout << "Error occured in fork!" << endl;
         return(1);
      }
      else if ( pid > 0 ) {
         printf("Server started with process id %i\n",pid);
         return(0);
      }
   }

   ControlServer  cntrlServer;
   int            port;
   stringstream   cmd;
   int            idx;
   uint           slot;
   uint           bay;
   uint           element;
   DataDpmSystem   *dataDpmSystem;
   TimingDtmSystem *timingDtmSystem;
   System          *system;


   // -----------------------------------------------------------------------
   // 2016.09.22 -- jjr
   // -----------------
   // Added initialization of system. Without it, the compiler believes that
   // delete system (appearing lower in the code) may have system unitialized.
   // I believe this is because the code in the try region may throw a string
   // exception leaving system unitialized.
   // -----------------------------------------------------------------------
   dataDpmSystem   = 0;
   timingDtmSystem = 0;

   // Catch signals
   signal (SIGINT,&sigTerm);
   
   try {

      RceCommon::getRcePosition(&slot,&bay,&element);

      // DTM
      if ( bay == 4 ) {
         idx = slot - 1;
         cout << "Detected trigger DTM idx " << idx << endl;
         system = timingDtmSystem = new TimingDtmSystem(defFile,idx);
      }

      // DPM
      else {
         idx = ((slot-1) * 8) + bay * 2;
         idx += (element==2)?1:0;
         cout << "Detected data DPM index " << dec << idx << endl;
         system = dataDpmSystem = new DataDpmSystem(defFile,idx);
      }

      // Setup control server
      cntrlServer.enableSharedMemory("protoDUNE",1);
      port = cntrlServer.startListen(0);
      cntrlServer.setSystem(system);
      //do some debugging...
      cntrlServer.setDebug(true);
      cout << "Starting server at port " << dec << port << endl;

      cntrlServer.receive(500,100000,&stop);

   } catch ( string error ) {
      cout << "Caught Error: " << endl;
      cout << error << endl;
   }

   cout << "Stopped server" << endl;
   cntrlServer.stopListen();
   if      (  dataDpmSystem) delete   dataDpmSystem;
   else if (timingDtmSystem) delete timingDtmSystem;
}


