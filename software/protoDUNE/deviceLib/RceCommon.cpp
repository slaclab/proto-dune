//-----------------------------------------------------------------------------
// File          : RceCommon.h
// Author        : Ryan Herbst <rherbst@slac.stanford.edu>
// Created       : 06/19/2014
// Project       : 
//-----------------------------------------------------------------------------
// Description :
//    Common registers for the RCE
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
// 06/19/2014: created
//-----------------------------------------------------------------------------
#include <RceCommon.h>
#include <Register.h>
#include <RegisterLink.h>
#include <Variable.h>
#include <sstream>
#include <iostream>
#include <string>
#include <iomanip>
#include <MappedMemory.h>

using namespace std;

#include <stdio.h>
#include <unistd.h>
#include <string.h> /* for strncpy */
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <netinet/in.h>
#include <net/if.h>
#include <arpa/inet.h>


// Constructor
RceCommon::RceCommon ( uint32_t linkConfig, uint32_t index, Device *parent ) : 
                        Device(linkConfig,0x0,"RceCommon",index,parent) {
  
   RegisterLink *rl;
   Variable     *v;

   // Description
   desc_ = "RCE Common Registers.";
   pollEnable(true);

   addRegisterLink(rl = new RegisterLink("FpgaVersion", 0x80000000, Variable::Status));
   rl->getVariable()->setDescription("FPGA Version");

   addRegisterLink(rl = new RegisterLink("RceVersion", 0x80000008, Variable::Status));
   rl->getVariable()->setDescription("RCE Version");

   addRegisterLink(rl = new RegisterLink("DeviceDna", 0x80000020, 2, Variable::Status));
   rl->getVariable()->setDescription("Xilinx Device DNA Value");

   addRegisterLink(rl = new RegisterLink("EFuseValue", 0x80000030, Variable::Status));
   rl->getVariable()->setDescription("E-Fuse Value");

   addRegisterLink(rl = new RegisterLink("EthMode", 0x80000034, Variable::Status));
   rl->getVariable()->setDescription("Eth Mode Value");

   addRegisterLink(rl = new RegisterLink("Heartbeat", 0x80000038, Variable::Status));
   rl->getVariable()->setDescription("A constantly incrementing counter.");
   rl->setPollEnable(true);

   addRegisterLink(rl = new RegisterLink("BuildString", 0x80001000, 64, Variable::Status));
   rl->getVariable()->setDescription("FPGA Build String");
   rl->getVariable()->setString();

   addRegisterLink(rl = new RegisterLink("SerialNumber",0x84000140, 2, Variable::Status));
   rl->getVariable()->setDescription("Serial Number");

   addRegisterLink(rl = new RegisterLink("Cluster", 0x84000148, 1, 3,
                                         "AtcaSlot",   Variable::Status, 16, 0xFF,
                                         "CobBay",     Variable::Status, 8, 0xFF,
                                         "CobElement", Variable::Status, 0, 0xFF));
            
   rl->getVariable(0)->setDescription("ATCA Slot Number");
   rl->getVariable(1)->setDescription("COB Bay Numer");
   rl->getVariable(2)->setDescription("COB Element Numer");


   // Set and hide enabled
   v = getVariable("Enabled");
   v->set("True");
   v->setHidden(true);

   // Set the IP address for RSSI
   MappedMemory *mem = new MappedMemory (1, 0xB0000000, 0x00001000);
   mem->open();
   mem->write(0xB000001C,getIpAddr()); 
   // cout << hex << mem->read(0xB000001C) << endl; 
   mem->close();
   delete mem;
}

// Deconstructor
RceCommon::~RceCommon ( ) { }

//! Static method to return bay/element for memory mapped RCE
void RceCommon::getRcePosition ( uint32_t *slot, uint32_t *bay, uint32_t *element ) {
   MappedMemory *mem = new MappedMemory (1, 0x84000000, 0x00001000);
   mem->open();

   uint32_t cluster = mem->read(0x84000148);

   *slot = (cluster >> 16) & 0xFF;
   *bay = (cluster >> 8) & 0xFF;
   *element = cluster & 0xFF;

   printf("Slot=%i, Bay=%i, Element=%i\n",*slot,*bay,*element);


   // Slot = 0 is invalid, no BSI, return DTM
   if ( *slot == 0 ) {
      *slot    = 1;
      *bay     = 4;
      *element = 0;
   }

   printf("Slot=%i, Bay=%i, Element=%i\n",*slot,*bay,*element);

   mem->close();
   delete mem;
}

//! Read status regsiters and dump values to stdout
void RceCommon::dumpDebug () {
   this->readStatus();

   printf("-------------- RceCommon ------------\n");
   printf("  FpgaVersion : %s\n",get("FpgaVersion").c_str());
   printf("   RceVersion : %s\n",get("RceVersion").c_str());
   printf("    DeviceDna : %s\n",get("DeviceDna").c_str());
   printf("   EFuseValue : %s\n",get("EFuseValue").c_str());
   printf("      EThMode : %s\n",get("EthMode").c_str());
   printf("    Heartbeat : %s\n",get("Heartbeat").c_str());
   printf("  BuildString : %s\n",get("BuildString").c_str());
   printf(" SerialNumber : %s\n",get("SerialNumber").c_str());
   printf("      Cluster : %s\n",get("Cluster").c_str());
   printf("     AtcaSlot : %s\n",get("AtcaSlot").c_str());
   printf("       CobBay : %s\n",get("CobBay").c_str());
   printf("   CobElement : %s\n",get("CobElement").c_str());
}

//! Convert the IP address string to a uint
uint32_t RceCommon::ipToUint (const char * ip) {
    // The return value. 
    uint32_t v = 0;
    // The count of the number of bytes processed. 
    uint32_t i;
    // A pointer to the next digit to process. 
    const char * start;

    start = ip;
    for (i = 0; i < 4; i++) {
        // The digit being processed.
        char c;
        // The value of this byte. 
        uint32_t n = 0;
        while (1) {
            c = * start;
            start++;
            if (c >= '0' && c <= '9') {
                n *= 10;
                n += c - '0';
            }
            // We insist on stopping at "." if we are still parsing
            //   the first, second, or third numbers. If we have reached
            //   the end of the numbers, we will allow any character.
            else if ((i < 3 && c == '.') || i == 3) {
                break;
            }
            else {
                return 0;
            }
        }
        if (n >= 256) {
            return 0;
        }
        v *= 256;
        v += n;
    }
    return ntohl(v);
}

//! Get the local IP address 
uint32_t  RceCommon::getIpAddr () {
   int32_t  fd;
   int32_t  myIp;
   struct ifreq ifr;
   fd = socket(AF_INET, SOCK_DGRAM, 0);
   ifr.ifr_addr.sa_family = AF_INET;
   strncpy(ifr.ifr_name, "eth0", IFNAMSIZ-1);
   ioctl(fd, SIOCGIFADDR, &ifr);
   close(fd);
   char *ip = inet_ntoa(((struct sockaddr_in *)&ifr.ifr_addr)->sin_addr);
   // cout << ip << endl;
   myIp = ipToUint(ip);
   //  cout << hex << myIp << endl;
   return myIp;
}

