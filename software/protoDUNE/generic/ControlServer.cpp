//-----------------------------------------------------------------------------
// File          : ControlServer.cpp
// Author        : Ryan Herbst  <rherbst@slac.stanford.edu>
// Created       : 08/29/2011
// Project       : General Purpose
//-----------------------------------------------------------------------------
// Description :
// Control server class
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
// 08/29/2011: created
//-----------------------------------------------------------------------------
#include <System.h>
#include <ControlServer.h>
#include <XmlVariables.h>
#include <sys/select.h>
#include <stdint.h>
#include <inttypes.h>
#include <signal.h>
using namespace std;

// Constructor
ControlServer::ControlServer ( ) {
   uint32_t x;

   debug_      = false;
   servFd_     = -1;
   port_       = 0;
   system_     = NULL;

   for ( x=0; x < MaxClients_; x++ ) {
      connFd_[x] = -1;
      rxData_[x].str("");
   }
   memset((char *) (&servAddr_),0,sizeof(servAddr_));
   memset((char *) (&connAddr_),0,sizeof(connAddr_));

   smem_ = NULL;

   signal(SIGPIPE, SIG_IGN);
}

// DeConstructor
ControlServer::~ControlServer ( ) {
   if ( smem_ != NULL ) controlCmdClose(smem_);
   stopListen();
}

// Enable shared 
void ControlServer::enableSharedMemory ( string system, uint32_t id ) {

   // Attempt to open and init shared memory
   if ( (smemFd_ = controlCmdOpenAndMap ( &smem_ , system.c_str(), id )) < 0 ) {
      smem_ = NULL;
      throw string("ControlServer::ControlServer -> Failed to open shared memory");
   }

   // Init shared memory
   controlCmdInit(smem_);
}

// Set debug flag
void ControlServer::setDebug ( bool debug ) { 
   debug_ = debug;
}

// Set top level device
void ControlServer::setSystem ( System *system ) {
   system_    = system;
}

// Start tcpip listen socket
int32_t ControlServer::startListen ( int32_t port ) {
   stringstream err;

   // Init structures
   servFd_ = socket(AF_INET, SOCK_STREAM, 0);
   memset((char *)&servAddr_,0,sizeof(servAddr_));
   servAddr_.sin_family = AF_INET;
   servAddr_.sin_addr.s_addr = INADDR_ANY;

   // port number if passed
   if ( port != 0 ) {
      servAddr_.sin_port = htons(port);

      // Attempt to bind socket
      if ( bind(servFd_, (struct sockaddr *) &servAddr_, sizeof(servAddr_)) < 0 ) {
         err.str("");
         err << "ControlServer::startListen -> Failed to bind socket " << dec << port << endl;
         servFd_ = -1;
         if ( debug_ ) cout << err.str();
         throw(err.str());
      }
   }

   // Find a free port
   else {
      for ( port = 8090; port < 8100; port++ ) {
         servAddr_.sin_port = htons(port);
         if ( bind(servFd_, (struct sockaddr *) &servAddr_, sizeof(servAddr_)) >= 0 ) break;
      }

      // Port not found
      if ( port == 8100 ) {
         err.str("");
         err << "ControlServer::startListen -> Failed to find free socket" << endl;
         servFd_ = -1;
         if ( debug_ ) cout << err.str();
         throw(err.str());
      }
   }

   // Start listen
   listen(servFd_,5);

   // Debug
   if ( debug_ ) 
      cout << "ControlServer::startListen -> Listening on port " << dec << port << endl;

   return(port);
}

// Close tcpip listen socket
void ControlServer::stopListen ( ) {
   uint32_t x;

   for ( x=0; x < MaxClients_; x++ ) {
      if ( connFd_[x] >= 0 ) close(connFd_[x]);
      connFd_[x] = -1;
   }
   if ( servFd_ >= 0 ) close(servFd_);
   servFd_ = -1;

   // Debug
   if ( servFd_ >= 0 && debug_ ) 
      cout << "ControlServer::startListen -> Stopped listening" << endl;
}

// Receive and process data if ready
void ControlServer::receive ( uint32_t selectPeriod, uint32_t pollPeriod, bool *stop ) {
   fd_set         fdset;
   int32_t        maxFd;
   socklen_t      cliLen;
   struct timeval tval;
   int32_t        newFd;
   int32_t        ret;
   stringstream   msg;
   uint32_t       x;
   int32_t        y;
   string         pmsg;
   uint8_t        cmdType;
   char         * cmdArgA;
   char         * cmdArgB;
   bool           tcpPend;
   bool           indPend[MaxClients_];
   bool           shmPend;
   int32_t        pollCycles;
   int32_t        pollCount;

   if ( pollPeriod < selectPeriod ) pollCycles = 1;
   else  pollCycles = (pollPeriod/selectPeriod);
   pollCount = 0;

   /*fprintf (stderr, 
            "pollPeriod = %8" PRIu32 " selectPeriod = %8" PRIu32 " pollCycles = %8" PRIu32 "\n",
            pollPeriod,
            selectPeriod,
            pollCycles);*/

   do {

      pollCount++;
      tcpPend = false;
      shmPend = false;

      // Setup for listen call
      FD_ZERO(&fdset);
      maxFd = 0;
      if ( servFd_ >= 0 ) {
         FD_SET(servFd_,&fdset);
         if ( servFd_ > maxFd ) maxFd = servFd_;
      }
      for ( x=0; x < MaxClients_; x++ ) {
         if ( connFd_[x] >= 0 ) {
            FD_SET(connFd_[x],&fdset);
            if ( connFd_[x] > maxFd ) maxFd = connFd_[x];
            indPend[x] = false;
         }
         else rxData_[x].str("");
      }

      // Call select
      tval.tv_sec  = 0;
      tval.tv_usec = selectPeriod;
      ret = select(maxFd+1,&fdset,NULL,NULL,&tval);

      // Something is ready
      if ( ret > 0 ) {

         // server socket is ready
         if ( servFd_ >= 0 && FD_ISSET(servFd_,&fdset)  ) {

            // Accept new client
            cliLen = sizeof(connAddr_);

#ifdef SOCK_NONBLOCK
            newFd = accept4(servFd_,(struct sockaddr *)&connAddr_,&cliLen,SOCK_NONBLOCK);
#else
            newFd = accept(servFd_,(struct sockaddr *)&connAddr_,&cliLen);
#endif

            // Error on accept
            if ( newFd < 0 ) {
               //if ( debug_ ) 
                  //cout << "ControlServer::receive -> Failed to accept on socket" << endl;
            } else {

               // Find empty client
               for ( x=0; x < MaxClients_; x++ ) {
                  if ( connFd_[x] == -1 ) break;
               }

               // Out of clients
               if ( x == MaxClients_ ) {
                  //if ( debug_ ) cout << "ControlServer::receive -> Rejected connection" << endl;
                  close(newFd);
               }
               else {
                  //if ( debug_ ) cout << "ControlServer::receive -> Accepted new connection" << endl;
                  connFd_[x] = newFd;
                  quietMode_[x] = false;

                  msg.str("");
                  msg << "<system>" << endl;
                  msg << system_->structureString(false,false);
                  msg << system_->configString(false,false);
                  msg << system_->statusString(false,false,false,true);
                  msg << "</system>" << endl;
                  msg << "\f";
                  sendData(x,msg.str().c_str(),msg.str().length());
               }
            }
         }

         // client socket is ready
         for ( x=0; x < MaxClients_; x++ ) {
            if ( connFd_[x] >= 0 && FD_ISSET(connFd_[x],&fdset) ) {

               // Read data
               ret = read(connFd_[x],buffer_,9000);

               // Connection is lost
               if ( ret <= 0 ) {
                  //if ( debug_ ) cout << "ControlServer::receive -> Closing connection" << endl;

                  // Reset
                  rxData_[x].str("");

                  // Close socket
                  close(connFd_[x]);
                  connFd_[x] = -1;
               }

               // Process each byte
               for (y=0; y<ret; y++) {

                  if ( buffer_[y] == '\a' || buffer_[y] == 27 ) quietMode_[x] = true;

                  else if ( buffer_[y] == '\f' || buffer_[y] == 0x4 ) {

                     // Send to top level
                     if ( system_ != NULL ) {
                        if ( debug_ ) {
                           cout << "ControlServer::receive -> Processing message: " << endl;
                           cout << rxData_[x].str() << endl;
                        }
                        system_->parseXmlString (rxData_[x].str()); 
                        indPend[x] = true;

                        tcpPend = true;
                        if ( debug_ ) cout << "ControlServer::receive -> Done Processing message" << endl;
                     }
                     rxData_[x].str("");
                  }
                  else rxData_[x] << buffer_[y];
               }
            }
         }
      }

      // Shared memory commands
      if ( smem_ != NULL && controlCmdGetCommand(smem_,&cmdType,&cmdArgA,&cmdArgB) ) {
         if ( debug_ ) cout << "ControlServer::receive -> Processing shared memory command type " << dec << cmdType << endl;
         vars_.clear();

         // Process Command
         switch(cmdType) {

            // One arg,  XML string, No Result
            case CONTROL_CMD_TYPE_SEND_XML :
               system_->parseXmlString(cmdArgA);
               controlCmdSetResult(smem_,"");
               break;

            // Two args, Config Variable and String Value, No Result
            case CONTROL_CMD_TYPE_SET_CONFIG :
               sprintf(xmlCmd_,"<system><config>%s</config></system>\n",vars_.setXml(cmdArgA,cmdArgB).c_str());
               system_->parseXmlString(xmlCmd_);
               controlCmdSetResult(smem_,"");
               break;

            // One Arg, Config Variable, Result is Value
            case CONTROL_CMD_TYPE_GET_CONFIG :
               vars_.parse("config",controlCmdGetConfig(smem_));
               controlCmdSetResult(smem_,vars_.get(cmdArgA).c_str());
               break;

            // One Arg, Status Variable, Result is Value
            case CONTROL_CMD_TYPE_GET_STATUS :
               vars_.parse("status",controlCmdGetStatus(smem_));
               vars_.parse("status",controlCmdGetPerStatus(smem_));
               controlCmdSetResult(smem_,vars_.get(cmdArgA).c_str());
               break;

            // Two Args, Device Path and command, No Result
            case CONTROL_CMD_TYPE_EXEC_COMMAND  :
               sprintf(xmlCmd_,"<system><command>%s</command></system>\n",vars_.setXml(cmdArgA,cmdArgB).c_str());
               system_->parseXmlString(xmlCmd_);
               controlCmdSetResult(smem_,"");
               break;

            // Three Args, Device Path, register name and value, No Result
            case CONTROL_CMD_TYPE_SET_REGISTER :
               sprintf(regStr_,"%s:WriteRegister",cmdArgA);
               sprintf(xmlCmd_,"<system><command>%s</command></system>\n",vars_.setXml(regStr_,cmdArgB).c_str());
               system_->parseXmlString(xmlCmd_);
               controlCmdSetResult(smem_,"");
               break;

            // Two Args, Device Path, register name, Result is value
            case CONTROL_CMD_TYPE_GET_REGISTER :
               sprintf(regStr_,"%s:ReadRegister",cmdArgA);
               sprintf(xmlCmd_,"<system><command>%s</command></system>\n",vars_.setXml(regStr_,cmdArgB).c_str());
               system_->parseXmlString(xmlCmd_);
               vars_.parse("status",system_->statusString(true,false,true,true).c_str());
               sprintf(regStr_,"%s:ReadRegisterResult",cmdArgA);
               controlCmdSetResult(smem_,vars_.get(regStr_).c_str());
               break;

            default:
               controlCmdSetResult(smem_,"");
               break;
         }
         shmPend = true;
      }

      // Poll if timeout or xml or tcp command
      if ( shmPend || tcpPend || (pollCount >= pollCycles) ) {
         pmsg = system_->poll(smem_);
         pollCount = 0;

         // Ack pending command
         if ( shmPend ) {
            if ( debug_ ) cout << "ControlServer::receive -> Done Processing shared memory message" << endl;
            controlCmdAckCommand(smem_);
         }

         // Send message
         if ( pmsg != "" ) {
            msg.str("");
            msg << pmsg << "\f";

            for ( x=0; x < MaxClients_; x++ ) {
               if ( connFd_[x] >= 0 ) {
                  if ( (!quietMode_[x]) || indPend[x] ) sendData(x,msg.str().c_str(),msg.str().length());
               }
            }
         }
      }
   } while ( stop != NULL && *stop == false );
}


void ControlServer::sendData ( uint32_t idx, const char *buffer, uint32_t size ) {
   int32_t  ret;
   uint32_t sent;
   uint32_t retryCount;
   uint32_t loop;

   loop = 0;
   sent = 0;
   retryCount = 0;

   while ( (sent != size) && (retryCount < 1000) ) {

      ret = write(connFd_[idx],buffer + sent,size-sent);

      if ( ret > 0 ) {
         sent += ret;
         retryCount = 0;
      }
      else {
         retryCount++;
         usleep(100);
      }
      loop++;
   }

   if ( sent != size ) {
      printf("ControlServer::sendData -> Write error. Closing connection. Sent=%i, Size=%i, loop=%i\n",sent,size,loop);

      // Reset
      rxData_[idx].str("");

      // Close socket
      close(connFd_[idx]);
      connFd_[idx] = -1;
   }
}

