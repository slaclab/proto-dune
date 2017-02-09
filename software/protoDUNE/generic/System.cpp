//-----------------------------------------------------------------------------
// File          : System.cpp
// Author        : Ryan Herbst  <rherbst@slac.stanford.edu>
// Created       : 04/12/2011
// Project       : General Purpose
//-----------------------------------------------------------------------------
// Description :
// Generic system level container
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
// 04/12/2011: created
//-----------------------------------------------------------------------------
#include <System.h>
#include <CommLink.h>
#include <Command.h>
#include <sstream>
#include <iostream>
#include <fstream>
#include <iomanip>
#include <string.h>
#include <Variable.h>
#include <time.h>
#include <stdint.h>
using namespace std;

// Constructor
System::System ( string name, CommLink *commLink ) : Device(0,0,name,0,NULL) {
   swRunEnable_    = false;
   swRunning_      = false;
   hwRunning_      = false;
   allStatusReq_   = false;
   topStatusReq_   = false;
   defaults_       = "defaults.xml";
   configureMsg_   = "System Is Not Configured.\nSet Defaults Or Load Settings!\n";
   swRunPeriod_    = 0;
   swRunCount_     = 0;
   swRunRetState_  = "Stopped";
   swRunError_     = "";
   errorBuffer_    = "";
   errorFlag_      = false;
   configureFlag_  = false;
   lastFileCount_  = 0;
   lastDataCount_  = 0;
   lastTime_       = 0;
   pollTime_       = 0;
   commLink_       = commLink;

   xmlInitParser();

   xmlId_ = 0;
   pthread_mutex_init(&xmlMutex_,NULL);

   Command  *c;
   Variable *v;

   // Commands
   addCommand(c = new Command("SetDefaults"));
   c->setDescription("Read XML defaults file.");
   c->setHidden(true);

   addCommand(c = new Command("ReadXmlFile"));
   c->setDescription("Read XML command or config file from disk. Pass filename as arg.");
   c->setHidden(true);
   c->setHasArg(true);

   addCommand(c = new Command("WriteConfigXml"));
   c->setDescription("Write configuration to disk. Pass filename as arg.");
   c->setHidden(true);
   c->setHasArg(true);

   addCommand(c = new Command("WriteStatusXml"));
   c->setDescription("Write status to disk. Pass filename as arg.");
   c->setHidden(true);
   c->setHasArg(true);

   addCommand(c = new Command("OpenDataFile"));
   c->setDescription("Open data file.");
   c->setHidden(true);

   addCommand(c = new Command("CloseDataFile"));
   c->setDescription("Close data file.");
   c->setHidden(true);

   addCommand(c = new Command("ReadConfig"));
   c->setDescription("Read configuration.");
   c->setHidden(true);

   addCommand(c = new Command("ReadStatus"));
   c->setDescription("Read status.");
   c->setHidden(true);

   addCommand(c = new Command("VerifyConfig"));
   c->setDescription("Verify configuration");
   c->setHidden(true);

   addCommand(c = new Command("ResetCount"));
   c->setDescription("Reset top level counters.");
   c->setHidden(true);

   addCommand(c = new Command("SetRunState"));
   c->setDescription("Set run state");
   c->setHidden(true);

   addCommand(c = new Command("HardReset"));
   c->setDescription("Hard reset System.");
   c->setHidden(true);

   addCommand(c = new Command("SoftReset"));
   c->setDescription("Soft reset System.");
   c->setHidden(true);

   addCommand(c = new Command("RefreshState"));
   c->setDescription("Refresh System State.");
   c->setHidden(true);

   // Variables
   addVariable(v = new Variable("DataFileCount",Variable::Status));
   v->setDescription("Number of events written to the data file");
   v->setHidden(true);

   addVariable(v = new Variable("DataFile",Variable::Configuration));
   v->setDescription("Data File For Write");
   v->setHidden(true);

   addVariable(v = new Variable("HideDataControl",Variable::Configuration));
   v->setDescription("Hide Data Control In GUI");
   v->setTrueFalse();
   v->setHidden(true);
   v->set("False");

   addVariable(v = new Variable("MaxFileSize",Variable::Configuration));
   v->setDescription("Maximum File Size");
   v->setInt(0);
   v->setHidden(true);   

   addVariable(v = new Variable("DataRxCount",Variable::Status));
   v->setDescription("Number of events received");
   v->setHidden(true);

   addVariable(v = new Variable("RegRxCount",Variable::Status));
   v->setDescription("Number of register responses received");
   v->setHidden(true);

   addVariable(v = new Variable("UnexpectedCount",Variable::Status));
   v->setDescription("Number of unexpected receive packets");
   v->setHidden(true);

   addVariable(v = new Variable("TimeoutCount",Variable::Status));
   v->setDescription("Number of timeout errors");
   v->setHidden(true);

   addVariable(v = new Variable("ErrorCount",Variable::Status));
   v->setDescription("Number of errors");
   v->setHidden(true);

   addVariable(v = new Variable("DataOpen",Variable::Status));
   v->setDescription("Data file is open");
   v->setTrueFalse();
   v->setHidden(true);

   addVariable(v = new Variable("RunRate",Variable::Configuration));
   v->setDescription("Run rate");
   v->setHidden(true);
   vector<string> rates;
   rates.resize(6);
   rates[0] = "1Hz";
   rates[1] = "10Hz";
   rates[2] = "100Hz";
   rates[3] = "120Hz";
   rates[4] = "1000Hz";
   rates[5] = "2000Hz";
   v->setEnums(rates);

   addVariable(v = new Variable("HideRunControl",Variable::Configuration));
   v->setDescription("Hide Run Control In GUI");
   v->setTrueFalse();
   v->setHidden(true);
   v->set("False");

   addVariable(v = new Variable("RunCount",Variable::Configuration));
   v->setDescription("SW Run Count");
   v->setHidden(true);
   v->setInt(1000);

   addVariable(v = new Variable("RunState",Variable::Status));
   v->setDescription("Run state");
   v->setHidden(true);
   vector<string> states;
   states.resize(2);
   states[0] = "Stopped";
   states[1] = "Running";
   v->setEnums(states);

   addVariable(v = new Variable("RunProgress",Variable::Status));
   v->setDescription("Run Total");
   v->setHidden(true);

   addVariable(v = new Variable("DebugEnable",Variable::Configuration));
   v->setDescription("Enable console debug messages.");
   v->setTrueFalse();
   v->set("False");

   addVariable(v = new Variable("DebugCmdTime",Variable::Configuration));
   v->setDescription("Enable showing command execution time.");
   v->setTrueFalse();
   v->set("True");

   addVariable(v = new Variable("PollPeriod",Variable::Configuration));
   v->setDescription("Poll Period In Seconds, Set To Zero To Disable");
   v->setRange(0,3600);
   v->setInt(0);

   addVariable(v = new Variable("SystemState",Variable::Status));
   v->setDescription("Current system state.");
   v->setHidden(true);

   addVariable(v = new Variable("RunNumber",Variable::Status));
   v->setDescription("Run Number Since Start");
   v->setInt(0);

   addVariable(v = new Variable("SystemStatus",Variable::Status));
   v->setDescription("Current System Status");
   v->setHidden(true);
   v->setInt(0);

   vector<string> status;
   states.resize(4);
   states[0] = "Error";
   states[1] = "Unconfigured";
   states[2] = "Warning";
   states[3] = "Ready";
   v->setEnums(status);

   addVariable(v = new Variable("WorkingDir",Variable::Status));
   v->setHidden(true);
   v->setDescription("Working Directory");

   char cwd[500]; 
   getcwd(cwd,500);
   v->set(cwd);

#ifdef MAKE_SW_VERSION
   addVariable(v = new Variable("SwVersion",Variable::Status));
   v->setDescription("Software version");
   v->set(MAKE_SW_VERSION);
#endif

   // System is always enabled
   getVariable("Enabled")->setHidden(true);
}

// Deconstructor
System::~System ( ) {
   xmlCleanupParser();
   xmlMemoryDump();
}


// Set comm link
CommLink * System::commLink() {
   return(commLink_);
}

// Thread Routines
void *System::swRunStatic ( void *t ) {
   System *ti;
   ti = (System *)t;
   ti->swRunThread();
   pthread_exit(NULL);
   return(NULL);
}

// Default software run thread. Should be overridden for custom designs
void System::swRunThread() {
   struct timespec tme;
   ulong           ctime;
   ulong           ltime;
   uint32_t        runTotal;

   swRunning_ = true;
   swRunError_  = "";
   clock_gettime(CLOCK_REALTIME,&tme);
   ltime = (tme.tv_sec * 1000000) + (tme.tv_nsec/1000);

   // Get run attributes
   runTotal  = 0;

   if ( debug_ ) {
      cout << "System::runThread -> Name: " << name_ 
           << ", Run Started"
           << ", RunCount=" << dec << swRunCount_
           << ", RunPeriod=" << dec << swRunPeriod_ << endl;
   }

   // Run
   while ( swRunEnable_ && (runTotal < swRunCount_ || swRunCount_ == 0 )) {

      // Delay
      do {
         usleep(1);
         clock_gettime(CLOCK_REALTIME,&tme);
         ctime = (tme.tv_sec * 1000000) + (tme.tv_nsec/1000);
      } while ( (ctime-ltime) < swRunPeriod_ );

      // Execute command
      ltime = ctime;
      commLink_->queueRunCommand();
      runTotal++;
      if ( swRunCount_ == 0 ) getVariable("RunProgress")->setInt(0);
      else getVariable("RunProgress")->setInt((uint32_t)(((double)runTotal/(double)swRunCount_)*100.0));
   }

   if ( debug_ ) {
      cout << "System::runThread -> Name: " << name_ 
           << ", Run Stopped, RunTotal = " << dec << runTotal << endl;
   }

   sleep(1);

   // Set run
   if ( swRunCount_ == 0 ) getVariable("RunProgress")->setInt(100);
   else getVariable("RunProgress")->setInt((uint32_t)(((double)runTotal/(double)swRunCount_)*100.0));
   getVariable("RunState")->set(swRunRetState_);
   swRunning_ = false;
}

// Start Run
void System::setRunState(string state) {
   stringstream err;
   stringstream tmp;
   uint32_t     toCount;
   uint32_t     runNumber;

   std::cout << "Setting run state: " << state << std::endl;

   // Stopped state is requested
   if ( state == "Stopped" ) {

      if ( swRunEnable_ ) {
         swRunEnable_ = false;
         pthread_join(swRunThread_,NULL);
      }

      allStatusReq_ = true;
      getVariable("RunState")->set(state);
      addRunStop();
   }

   // Running state is requested
   else if ( state == "Running" && !swRunning_ ) {

      // Set run command here when re-implemented
      //device->setRuncommand("command");

      // Increment run number
      runNumber = getVariable("RunNumber")->getInt() + 1;
      getVariable("RunNumber")->setInt(runNumber);

      addRunStart();

      swRunRetState_ = get("RunState");
      swRunEnable_   = true;
      getVariable("RunState")->set("Running");

      // Setup run parameters
      swRunCount_ = getInt("RunCount");
      if      ( get("RunRate") == "2000Hz") swRunPeriod_ =     500;
      else if ( get("RunRate") == "1000Hz") swRunPeriod_ =    1000;
      else if ( get("RunRate") == "120Hz") swRunPeriod_ =    8333;
      else if ( get("RunRate") == "100Hz") swRunPeriod_ =   10000;
      else if ( get("RunRate") ==  "10Hz") swRunPeriod_ =  100000;
      else if ( get("RunRate") ==   "1Hz") swRunPeriod_ = 1000000;
      else swRunPeriod_ = 1000000;


      // Start thread
      if ( swRunCount_ == 0 || pthread_create(&swRunThread_,NULL,swRunStatic,this) ) {
         err << "System::startRun -> Failed to create runThread" << endl;
         if ( debug_ ) cout << err.str();
         getVariable("RunState")->set(swRunRetState_);
         throw(err.str());
      }
#ifdef ARM
      pthread_setname_np(swRunThread_,"systemRunThread");
#endif

      // Wait for thread to start
      toCount = 0;
      while ( !swRunning_ ) {
         usleep(100);
         toCount++;
         if ( toCount > 1000 ) {
            swRunEnable_ = false;
            err << "System::startRun -> Timeout waiting for runthread" << endl;
            if ( debug_ ) cout << err.str();
            getVariable("RunState")->set(swRunRetState_);
            throw(err.str());
         }
      }
   }
}

// Method to process a command
void System::command ( string name, string arg ) {
   ofstream     os;
   stringstream tmp;
   struct timespec stme;
   struct timespec etme;
   time_t          ctme;

   clock_gettime(CLOCK_REALTIME,&stme);

   // Read defaults file
   if ( name == "SetDefaults" ) {
      parseXmlFile(defaults_);
      //softReset();
   }

   // Read and parse xml file
   else if ( name == "ReadXmlFile" ) {
      parseXmlFile(arg);
      //softReset();
   }

   // Write config xml dump
   else if ( name == "WriteConfigXml" ) {
      readConfig();
      os.open(arg.c_str(),ios::out | ios::trunc);
      if ( ! os.is_open() ) {
         tmp.str("");
         tmp << "System::command -> Error opening config xml file for write: " << arg << endl;
         if ( debug_ ) cout << tmp.str();
         throw(tmp.str());
      }
      os << "<system>" << endl << configString(true,true) << "</system>" << endl;
      os.close();
   }

   // Write status xml dump
   else if ( name == "WriteStatusXml" ) {
      readStatus();
      os.open(arg.c_str(),ios::out | ios::trunc);
      if ( ! os.is_open() ) {
         tmp.str("");
         tmp << "System::command -> Error opening status xml file for write: " << arg << endl;
         if ( debug_ ) cout << tmp.str();
         throw(tmp.str());
      }
      os << "<system>" << endl << statusString(true,true,false,true) << "</system>" << endl;
      os.close();
   }

   // Open data file
   else if ( name == "OpenDataFile" ) {
      command("CloseDataFile","");
      commLink_->openDataFile(getVariable("DataFile")->get(),getVariable("MaxFileSize")->getInt());
      commLink_->addConfig(configString(true,false));
      readStatus();
      commLink_->addStatus(statusString(true,false,false,true));
      getVariable("DataOpen")->set("True");
   }

   // Close data file
   else if ( name == "CloseDataFile" ) {
      if ( get("DataOpen") == "True" ) {
         readStatus();
         commLink_->addStatus(statusString(true,false,false,true));
         commLink_->closeDataFile();
         getVariable("DataOpen")->set("False");
      }
   }

   // Send config xml
   else if ( name == "ReadConfig" ) {
      readConfig();
      allConfigReq_ = true;
   }

   // Send status xml
   else if ( name == "ReadStatus" ) allStatusReq_ = true;

   // Send verify status
   else if ( name == "VerifyConfig" ) {
      verifyConfig();
      allStatusReq_ = true;
   }

   // Reset counters
   else if ( name == "ResetCount" ) countReset();

   // Start Run
   else if ( name == "SetRunState" ) setRunState(arg);

   // Hard reset
   else if ( name == "HardReset" ) hardReset();

   // Soft reset
   else if ( name == "SoftReset" ) softReset();

   else if ( name == "RefreshState" ) allStatusReq_ = true;

   else Device::command(name,arg);

   clock_gettime(CLOCK_REALTIME,&etme);

   time(&ctme);
   if ( get("DebugCmdTime") == "True" ) {
      cout << "System::command -> Command " << name << " time results: " << endl
           << "   Start Time: " << dec << stme.tv_sec << "." << stme.tv_nsec << endl
           << "     End Time: " << dec << etme.tv_sec << "." << etme.tv_nsec << " - " << ctime(&ctme);
   }
}

// Parse XML string
bool System::parseXml ( string xml, bool force ) {
   xmlDocPtr    doc;
   xmlNodePtr   node;
   xmlNodePtr   childNode;
   const char   *childName;
   string       err;
   string       stat;
   bool         configUpdate;
   char         idString[200];

   // get unique xml id
   pthread_mutex_lock(&xmlMutex_);
   sprintf(idString,"%i",++xmlId_);
   pthread_mutex_unlock(&xmlMutex_);
   
   stat = "";

   // -----------------------------------------------------------------------
   // 2016.09.22 -- jjr
   // -----------------
   // Added initialization of doc. Without it, the compiler believes that
   // xmlFreeDoc (doc) (appearing lower in the code) may have doc unitialized.
   // I believe this is because xmlReadMemory may throw a string exception
   // leaving doc unitialized.
   // -----------------------------------------------------------------------
   doc  =  NULL;
   configUpdate = false;
   try {

      // Parse string
      doc = xmlReadMemory(xml.c_str(), strlen(xml.c_str()), idString, NULL, 0);
      if (doc == NULL) {
         err = "System::parseXml -> Failed to parse string\n";
         if ( debug_ ) cout << err;
         //throw(err);
         return false;
      }

      // get the root element node
      node = xmlDocGetRootElement(doc);

      // Look for child nodes
      for ( childNode = node->children; childNode; childNode = childNode->next ) {
         if ( childNode->type == XML_ELEMENT_NODE ) {
            childName  = (const char *)childNode->name;

            // Config
            if ( strcmp(childName,"config") == 0 ) {
               if ( setXmlConfig(childNode) ) {
                  writeConfig(force);
                  if ( force ) verifyConfig();
                  configUpdate = true;
               }
            }

            // Command
            else if ( strcmp(childName,"command") == 0 ) execXmlCommand(childNode);
         }
      }
   } catch ( string error ) { stat = error; }

   // Cleanup

   // --------------------------------------
   // 2016.09.22 -- jjr
   // -----------------
   // Added check in case doc never gets set
   // --------------------------------------
   if (doc) xmlFreeDoc(doc);

   if ( stat != "" ) throw(stat);
   return(configUpdate);
}


// Parse XML string
void System::parseXmlString ( string xml ) {
   try { 
      if ( parseXml(xml,false) ) allConfigReq_ = true;
   } catch ( string error ) { 
      errorBuffer_.append("<error>");
      errorBuffer_.append(error); 
      errorBuffer_.append("</error>\n");
      errorFlag_     = true;
      configureMsg_  = "A System Error Has Occured!\n";
      configureMsg_.append("Please HardReset and then configure!\n");
   }
   topStatusReq_ = true;
}

// Parse XML file
void System::parseXmlFile ( string file ) {
   uint32_t     idx;
   ifstream     is;
   stringstream tmp;
   stringstream buffer;
  
   // Stop run and close file
   setRunState("Stopped");
   command("CloseDataFile","");
 
   // Open file
   is.open(file.c_str());
   if ( ! is.is_open() ) {
      tmp.str("");
      tmp << "System::parseXmlFile -> Error opening xml file for read: " << file << endl;
      if ( debug_ ) cout << tmp.str();
      throw(tmp.str());
   }
   buffer.str("");
   buffer << is.rdbuf();
   is.close();

   // Parse string
   parseXml(buffer.str(),true);

   // Update message
   configureMsg_ = "System Configured From ";
   idx = file.find_last_of("/");
   configureMsg_.append(file.substr(idx+1));
   configureMsg_.append(".\n");
   configureFlag_ = true;
   allStatusReq_ = true;
   allConfigReq_ = true;
}

//! Method to perform soft reset
void System::softReset ( ) { 
   setRunState("Stopped");
   command("CloseDataFile","");
   Device::softReset();
   allStatusReq_ = true;
   System::countReset();
}

//! Method to perform hard reset
void System::hardReset ( ) { 
   errorFlag_ = false;
   configureFlag_ = false;
   configureMsg_ = "System Is Not Configured.\nSet Defaults Or Load Settings!\n";
   setRunState("Stopped");
   command("CloseDataFile","");
   Device::hardReset();
   topStatusReq_ = true;
}

//! Method to reset counters
void System::countReset() {
   commLink_->clearCounters();
   Device::countReset();
   allStatusReq_ = true;
}

//! return local state
string System::localState () {
   return("");
}

//! Method to return state string
string System::poll(ControlCmdMemory *cmem) {
   uint32_t     curr;
   uint32_t     rate;
   stringstream msg;
   time_t       currTime;
   bool         send;
   string       stateIn;
   bool         pollStatusReq;
   uint32_t     pollPer;
   string       cfgString;
   string       statString;

   time(&currTime);
   pollStatusReq = false;

   // Detect run stop
   if ( swRunEnable_ && !swRunning_ ) {
      swRunEnable_ = false;
      pthread_join(swRunThread_,NULL);
      addRunStop();
      allStatusReq_ = true;
      if ( swRunError_ != "" ) {
         errorBuffer_.append("<error>");
         errorBuffer_.append(swRunError_); 
         errorBuffer_.append("</error>\n");
         errorFlag_ = true;
         configureMsg_ = "A System Error Has Occured!\n";
         configureMsg_.append("Please HardReset and then configure!\n");
      }
   }

   try {

      // Once a second updates
      if ( currTime != lastTime_ ) {
         lastTime_ = currTime;

         // Add timestamp to data file if running
         if ( swRunning_ || hwRunning_ ) addRunTime();

         // File counters
         getVariable("RegRxCount")->setInt(commLink_->regRxCount());
         getVariable("TimeoutCount")->setInt(commLink_->timeoutCount());
         getVariable("ErrorCount")->setInt(commLink_->errorCount());
         getVariable("UnexpectedCount")->setInt(commLink_->unexpectedCount());

         curr = commLink_->dataFileCount();
         if ( curr < lastFileCount_ ) rate = 0;
         else rate = curr - lastFileCount_;
         lastFileCount_ = curr;
         msg.str("");
         msg << dec << curr << " - " << dec << rate << " Hz";
         getVariable("DataFileCount")->set(msg.str());
      
         curr = commLink_->dataRxCount();
         if ( curr < lastDataCount_ ) rate = 0;
         else rate = curr - lastDataCount_;
         lastDataCount_ = curr;
         msg.str("");
         msg << dec << curr << " - " << dec << rate << " Hz";
         getVariable("DataRxCount")->set(msg.str());

         // Top status request
         topStatusReq_ = true;

         // Polling Period
         pollPer = getInt("PollPeriod");
         if ( (pollPer != 0 ) && ((currTime - pollTime_) >= (int)pollPer) ) {
            pollStatusReq = true;
            pollTime_ = currTime;
         }
      }

      // Read status if requested
      if ( allStatusReq_ ) readStatus();
      else if (pollStatusReq) pollStatus();

      // State update if needed
      if ( errorBuffer_ != "" || allConfigReq_ || topStatusReq_ || allStatusReq_ || pollStatusReq ) {

         // Set top level state, may override below in local state
         if ( errorFlag_ ) getVariable("SystemStatus")->set("Error");
         else if ( ! configureFlag_ ) getVariable("SystemStatus")->set("Unconfigured");
         else getVariable("SystemStatus")->set("Ready");
         // Warning is open for use by the custom application

         // User state
         stateIn = localState();

         // Update state message
         msg.str("");
         msg << configureMsg_;
         msg << "System is is in run state '" << get("RunState") << "'" << endl;
         msg << stateIn;
         getVariable("SystemState")->set(msg.str());
      }

   } catch (string error ) {
      errorBuffer_.append("<error>");
      errorBuffer_.append(error); 
      errorBuffer_.append("</error>\n");
      errorFlag_ = true;
      configureMsg_ = "A System Error Has Occured!\n";
      configureMsg_.append("Please HardReset and then configure!\n");
   }

   // Config string is needed
   if ( allConfigReq_ ) cfgString = configString(false,false);

   // Status string is needed
   if ( topStatusReq_ || allStatusReq_ || pollStatusReq ) 
      statString = statusString(false,false,(!allStatusReq_),(allStatusReq_||pollStatusReq)); 

   // Generate outgoing message
   send = false;
   msg.str("");
   msg << "<system>" << endl;
   if ( errorBuffer_ != "" ) { msg << errorBuffer_; send = true; }
   if ( topStatusReq_ || allStatusReq_ || pollStatusReq ) { msg << statString; send=true; }
   if ( allConfigReq_ ) { msg << cfgString; send=true; }
   msg << "</system>" << endl;

   // Do we add configuration updates to file?
   if ( allConfigReq_ ) commLink_->addConfig(configString(true,false));
   if ( allStatusReq_ || allConfigReq_ ) commLink_->addStatus(statusString(true,false,false,true));

   // Update Shared Memory
   if ( cmem != NULL ) {
      if ( errorBuffer_ != "" ) controlCmdSetError(cmem,errorBuffer_.c_str());
      if ( allStatusReq_ ) {
         controlCmdSetStatus(cmem,statString.c_str());
         controlCmdSetPerStatus(cmem,"");
      }
      else if ( topStatusReq_ || pollStatusReq ) controlCmdSetPerStatus(cmem,statString.c_str());
      if ( allConfigReq_ ) controlCmdSetConfig(cmem,cfgString.c_str());
   }

   // Clear send requests
   errorBuffer_  = "";
   topStatusReq_ = false; 
   allStatusReq_ = false; 
   allConfigReq_ = false; 

   // Send message
   if ( send ) return(msg.str());
   else return("");
}

// Return status string
string System::statusString(bool hidden, bool indent, bool compact, bool recursive) {
   stringstream tmp;
   tmp.str("");
   if ( indent ) tmp << "   ";
   tmp << "<status>" << endl;
   tmp << getXmlStatus(true,hidden,((indent)?2:0),compact,recursive);
   if ( indent ) tmp << "   ";
   tmp << "</status>" << endl;
   return(tmp.str());
}

// Return config string
string System::configString(bool hidden, bool indent) {
   stringstream tmp;
   tmp.str("");
   if ( indent ) tmp << "   ";
   tmp << "<config>" << endl;
   tmp << getXmlConfig(true,true,hidden,((indent)?2:0));  // Common
   tmp << getXmlConfig(true,false,hidden,((indent)?2:0)); // Per-Instance
   if ( indent ) tmp << "   ";
   tmp << "</config>" << endl;
   return(tmp.str());
}

// Return structure string
string System::structureString (bool hidden, bool indent) {
   stringstream tmp;
   tmp.str("");
   if ( indent ) tmp << "   ";
   tmp << "<structure>" << endl;
   tmp << getXmlStructure(true,true,hidden,((indent)?2:0));  // General
   tmp << getXmlStructure(true,false,hidden,((indent)?2:0)); // Per-Instance
   if ( indent ) tmp << "   ";
   tmp << "</structure>" << endl;
   return(tmp.str());
}

// Method to write configuration registers
void System::writeConfig ( bool force ) {
   
   // Update debug
   setDebug(getVariable("DebugEnable")->getInt());
   commLink_->setDebug(getVariable("DebugEnable")->getInt());

   Device::writeConfig(force);
}

// Set debug flag
void System::setDebug( bool enable ) {
   if ( enable && !debug_ ) cout << "System::setDebug -> Name: " << name_ << " Enabling debug messages." << endl;
   if ( debug_ && !enable ) cout << "System::setDebug -> Name: " << name_ << " Disabling debug messages." << endl;

   getVariable("DebugEnable")->setInt(enable);
   Device::setDebug(enable);
}

// Generate timestamp from passed time value
string System::genTime( time_t tme ) {
   char   tstr[200];
   struct tm *timeinfo;
   string ret;

   timeinfo = localtime(&tme);

   strftime(tstr,200,"%Y_%m_%d_%H_%M_%S",timeinfo);
   ret = tstr;
   return(ret); 
}

void System::addRunStart() {
   stringstream    xml;



   xml.str("");
   xml << "<runStart>" << endl;
   xml << "<runNumber>" << getVariable("RunNumber")->getInt() << "</runNumber>" << endl;
   xml << "<timestamp>" << genTime(time(0)) << "</timestamp>" << endl;
   xml << "<unixtime>" << dec << time(0) << "</unixtime>" << endl;
   //xml << "<user>" << getlogin() << "</user>" << endl;
   xml << "</runStart>" << endl;

   std::cout << "addRunStart: " << xml << std::endl;
   commLink_->addRunStart(xml.str());
}

void System::addRunStop() {
   stringstream    xml;

   xml.str("");
   xml << "<runStop>" << endl;
   xml << "<runNumber>" << getVariable("RunNumber")->getInt() << "</runNumber>" << endl;
   xml << "<timestamp>" << genTime(time(0)) << "</timestamp>" << endl;
   xml << "<unixtime>" << dec << time(0) << "</unixtime>" << endl;
   //xml << "<user>" << getlogin() << "</user>" << endl;
   xml << "</runStop>" << endl;
   commLink_->addRunStop(xml.str());
}

void System::addRunTime() {
   stringstream    xml;

   xml.str("");
   xml << "<runTime>" << endl;
   xml << "<runNumber>" << getVariable("RunNumber")->getInt() << "</runNumber>" << endl;
   xml << "<timestamp>" << genTime(time(0)) << "</timestamp>" << endl;
   xml << "<unixtime>" << dec << time(0) << "</unixtime>" << endl;
   //xml << "<user>" << getlogin() << "</user>" << endl;
   xml << "</runTime>" << endl;
   commLink_->addRunTime(xml.str());
}

