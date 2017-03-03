//-----------------------------------------------------------------------------
// File          : Device.cpp
// Author        : Ryan Herbst  <rherbst@slac.stanford.edu>
// Created       : 04/12/2011
// Project       : General Purpose
//-----------------------------------------------------------------------------
// Description :
// Generic device container
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



/* ---------------------------------------------------------------------- *\
   
   HISTORY
   -------
  
   DATE       WHO WHAT
   ---------- --- ---------------------------------------------------------
   2017.03.02 jjr With version 5.2 of g++ and std=c0x11, one appears to
                  no longer be able to pass a stringstream to cout. You
                  must pass stream.str().
  
\* ---------------------------------------------------------------------- */



#include <Device.h>
#include <System.h>
#include <Register.h>
#include <Command.h>
#include <Variable.h>
#include <RegisterLink.h>
#include <CommLink.h>
#include <sstream>
#include <iostream>
#include <iomanip>
#include <string.h>
#include <stdint.h>
using namespace std;

// Write register
void Device::writeRegister ( Register *reg, bool force, bool wait ) {
   stringstream msg;
   msg.str("");

   // Set register stale if force is set, that way if not enabled it will be written eventually
   if ( force ) reg->setStale();

   if ( getInt("Enabled") == 0 || !reg->stale() ) return;

   // Call function to set register value
   system_->commLink()->queueRegister(linkConfig_,reg,true,wait);

   msg << "Device::writeRegister -> ";
   if ( reg->status() != 0 ) msg << "Status Error! ";
   msg << "Name: " << name_ << " Index: " << dec << index_;
   msg << ", Write Register: " << reg->name();
   msg << ", LinkConfig: 0x" << hex << setw(8) << setfill('0') << linkConfig_;
   msg << ", Address: 0x" << hex << setw(8) << setfill('0') << reg->address();
   msg << ", Value: 0x" << hex << setw(8) << setfill('0') << reg->get();
   msg << ", Status: " << reg->status() << endl;

   if ( debug_ ) cout << msg.str();

   if ( reg->status() != 0 ) {
      cout << msg.str() << endl;
      throw(msg.str());
   }
}

// Read register
void Device::readRegister ( Register *reg ) {
   stringstream msg;
   msg.str("");

   if ( getInt("Enabled") == 0 ) return;

   // Call function to get register value
   system_->commLink()->queueRegister(linkConfig_,reg,false,true);

   msg << "Device::readRegister -> ";
   if ( reg->status() != 0 ) msg << "Status Error! ";
   msg << "Name: " << name_ << " Index: " << dec << index_;
   msg << ", Read Register: " << reg->name();
   msg << ", LinkConfig: 0x" << hex << setw(8) << setfill('0') << linkConfig_;
   msg << ", Address: 0x" << hex << setw(8) << setfill('0') << reg->address();
   msg << ", Value: 0x" << hex << setw(8) << setfill('0') << reg->get();
   msg << ", Status: " << reg->status() << endl;

   if ( debug_ ) cout << msg.str();

   if ( reg->status() != 0 ) {
      cout << msg.str() << endl;
      throw(msg.str());
   }
}

// Verify register
void Device::verifyRegister ( Register *reg, bool warnOnly, uint32_t mask ) {
   stringstream msg;
   Register     *temp;
   bool         match;
   uint32_t     x;
   bool         err;

   if ( getInt("Enabled") == 0 ) return;

   // Cal function to read register value
   temp = new Register(reg);
   system_->commLink()->queueRegister(linkConfig_,temp,false,true);

   // Single register
   if ( reg->size() == 1 ) {
      if ( reg->get(0,mask) != temp->get(0,mask) ) match = false;
      else match = true;
   }

   // Block register
   else {

      // Verify register
      if ( memcmp(temp->data(),reg->data(),reg->size()*4) == 0 ) match = true;
      else match = false;
   }

   // Check for status error
   if ( temp->status() != 0 ) err = true;
   else err = false;

   // Debug message
   msg.str("");
   msg << "Device::verifyRegister ->";
   if ( temp->status() != 0 ) msg << "Status Error! ";
   msg << "Name: " << name_ << " Index: " << dec << index_;
   msg << ", Verify Register: " << reg->name();
   msg << ", LinkConfig: 0x" << hex << setw(8) << setfill('0') << linkConfig_;
   msg << ", Address: 0x" << hex << setw(8) << setfill('0') << reg->address();
   msg << ", Mask: 0x" << hex << setw(8) << setfill('0') << mask;
   msg << ", Exp: 0x" << hex << setw(8) << setfill('0') << reg->get(0, mask);
   msg << ", Got: 0x" << hex << setw(8) << setfill('0') << temp->get(0, mask);
   msg << ", Match: " << match;
   msg << ", Status: " << temp->status() << endl;

   // Generate verify status
   if ( debug_ ) {
      cout << msg.str();
      if ( reg->size() > 1 ) {
         cout << "Device::verifyRegister -> Details: " << endl;
         for ( x=0; x < reg->size(); x++ ) {
            if ( reg->getIndex(x) != temp->getIndex(x) ) {
               cout << "   Idx: 0x" << hex << setw(4) << setfill('0') << x
                    << " Got: 0x" << hex << setw(4) << setfill('0') << temp->getIndex(x)
                    << " Exp: 0x" << hex << setw(4) << setfill('0') << reg->getIndex(x) << endl;
            }
         }
      }
   }
   delete temp;

   // throw message on error or verify failure
   if ( err || (!match) ) {
      cout << msg.str() << endl;
      if ( !warnOnly ) throw(msg.str());
   }
}

// to set variable values from xml tree
bool Device::setXmlConfig( xmlNode *node ) {
   DeviceMap::iterator    devMapIter;
   DeviceVector::iterator devIter;
   DeviceVector           *dev;
   xmlNode                *childNode;
   xmlNode                *valueNode;
   const char             *nodeName;
   char                   *nodeValue;
   char                   *attrValue;
   int32_t                nodeIndex;
   char                   *eptr;
   bool                   update;

   update = false;

   // Look for child nodes
   for ( childNode = node->children; childNode; childNode = childNode->next ) {
      if ( childNode->type == XML_ELEMENT_NODE ) {

         // Extract name
         nodeName  = (const char *)childNode->name;

         // Extract value
         valueNode = childNode->children;
         if ( valueNode == NULL ) nodeValue = (char *)"";
         else {
            nodeValue = (char *)valueNode->content;
            if ( nodeValue == NULL ) nodeValue = (char *)"";
         }

         // Extract index attribute
         attrValue = (char *)xmlGetProp(childNode,(const xmlChar*)"index");
         if ( attrValue == NULL ) nodeIndex = -1;
         else {
            nodeIndex = (uint32_t)strtoul(attrValue,&eptr,0);
            if ( *eptr != '\0' || eptr == attrValue ) nodeIndex = -1;
         }

         // Look for matching device
         devMapIter = devices_.find(nodeName);

         // Element is a device
         if ( devMapIter != devices_.end() ) {
            dev = devMapIter->second;

            // All devices
            if ( nodeIndex == -1 ) {
               for ( devIter = dev->begin(); devIter != dev->end(); devIter++ ) 
                  if ((*devIter) != NULL && (*devIter)->setXmlConfig(childNode)) update = true;
            }

            // Specific device
            else {
               if ( nodeIndex < (int32_t)dev->size() ) {
                  if ( dev->at(nodeIndex) != NULL && dev->at(nodeIndex)->setXmlConfig(childNode) ) update = true;
               }
            }
         }

         // Element is a variable
         else {
            set(nodeName,nodeValue);
            update = true;
         }
      }
   }
   return(update);
}

// Method to return variables in xml string
string Device::getXmlConfig(bool top, bool common, bool hidden, uint32_t level) {
   DeviceMap::iterator    devMapIter;
   DeviceVector           *dev;
   DeviceVector::iterator devIter;
   stringstream           loc;
   stringstream           tmp;
   VariableMap::iterator  varIter;
   uint32_t               locLevel = level;;
   bool                   foundOne;
   Device               * first;

   loc.str("");
   tmp.str("");

   // Start device tag if not top level
   if ( !top ) {
      if ( level != 0 ) {
         for (uint32_t l=0; l < (level*3); l++) loc << " ";
         locLevel++;
      }
      loc << "<" << name_;
      if ( ! common ) loc << " index=\"" << dec << index_ << "\"";
      loc << ">" << endl;
   }

   // Each local variable
   for (varIter=variables_.begin(); varIter != variables_.end(); ++varIter) {

      // Return all config variables at the top, return variables based upon the hidden variable at other levels
      if ( varIter->second->type() != Variable::Status && (!varIter->second->noConfig()) && ((!varIter->second->hidden()) || top || hidden )) {
         if ( common == true || varIter->second->perInstance() ) {
            if ( locLevel != 0 ) for (uint32_t l=0; l < (locLevel*3); l++) tmp << " ";
            tmp << "<" << varIter->first << ">";
            tmp << varIter->second->get();
            tmp << "</" << varIter->first << ">" << endl;
         }
      }
   }

   // Get each sub device type
   for ( devMapIter = devices_.begin(); devMapIter != devices_.end(); devMapIter++ ) {
      dev = devMapIter->second;

      first = NULL;
      foundOne = false;

      // Device entries
      for ( devIter = dev->begin(); devIter != dev->end(); devIter++ ) {
         if ( (*devIter) != NULL ) {

            // Save the first one we found
            if ( first == NULL ) first = (*devIter);

            // Return first enabled device in common mode, if not common mode return all
            if ( (! common) || (*devIter)->getVariable("Enabled")->getInt() ) {
               tmp << (*devIter)->getXmlConfig(false,common,hidden,locLevel);
               foundOne = true; // We found an enabled device
               if ( common ) break;
            }
         }
      }

      // If we are in common mode and walked the array of a device type and did not find an enabled entry, 
      // use the config from the first valid device we found
      if ( common && first != NULL && !foundOne ) tmp << first->getXmlConfig(false,common,hidden,locLevel);
   }

   // End device tag
   loc << tmp.str();
   if ( !top ) {
      if ( level != 0 ) for (uint32_t l=0; l < (level*3); l++) loc << " ";
      loc << "</" << name_ << ">" << endl;
   }
 
   // Return empty string if there where no local variables
   if ( tmp.str() != "" ) return(loc.str());
   else return("");
}

// Method to return status in xml string
string Device::getXmlStatus(bool top, bool hidden, uint32_t level, bool compact, bool recursive) {
   DeviceMap::iterator    devMapIter;
   DeviceVector           *dev;
   DeviceVector::iterator devIter;
   stringstream           tmp;
   stringstream           loc;
   VariableMap::iterator  varIter;
   uint32_t               locLevel = level;
   string                 value;
   bool                   include;

   loc.str("");
   tmp.str("");

   // Start device tag if not top level
   if (!top ) {
      if ( level != 0 ) {
         for (uint32_t l=0; l < (level*3); l++) loc << " ";
         locLevel++;
      }
      loc << "<" << name_ << " index=\"" << dec << index_ << "\">" << endl;
   }

   // Each local variable, status only
   for (varIter=variables_.begin(); varIter != variables_.end(); ++varIter) {

      // Return all status variables at the top, other levels depends on hidden flag
      if ( varIter->second->type() == Variable::Status && (!varIter->second->hidden() || top || hidden )) {

         // Get value, check if changed
         value = varIter->second->get(compact,&include);

         // Update if include flag is true
         if ( top || include ) {
            if ( locLevel != 0 ) for (uint32_t l=0; l < (locLevel*3); l++) tmp << " ";
            tmp << "<" << varIter->first << ">";
            tmp << value;
            tmp << "</" << varIter->first << ">" << endl;
         }
      }
   }

   // Get each sub device
   if ( recursive ) {
      for ( devMapIter = devices_.begin(); devMapIter != devices_.end(); devMapIter++ ) {
         dev = devMapIter->second;

         // Device entries
         for ( devIter = dev->begin(); devIter != dev->end(); devIter++ ) {
            if ((*devIter) != NULL) tmp << (*devIter)->getXmlStatus(false,hidden,locLevel,compact,true);
         }
      }
   }

   // End device tag
   loc << tmp.str();
   if (!top ) {
      if ( level != 0 ) for (uint32_t l=0; l < (level*3); l++) loc << " ";
      loc << "</" << name_ << ">" << endl;
   }

   // Return empty string if there where no local entries
   if ( tmp.str() != "" ) return(loc.str());
   else return("");
}

// Method to execute commands from xml tree
void Device::execXmlCommand ( xmlNode *node ) {
   DeviceMap::iterator    devMapIter;
   DeviceVector::iterator devIter;
   DeviceVector           *dev;
   xmlNode                *childNode;
   xmlNode                *valueNode;
   const char             *nodeName;
   char                   *nodeValue;
   char                   *attrValue;
   int32_t                nodeIndex;
   char                   *eptr;

   // Look for child nodes
   for ( childNode = node->children; childNode; childNode = childNode->next ) {
      if ( childNode->type == XML_ELEMENT_NODE ) {

         // Extract name
         nodeName  = (const char *)childNode->name;

         // Extract value
         valueNode = childNode->children;
         if ( valueNode == NULL ) nodeValue = (char *)"";
         else {
            nodeValue = (char *)valueNode->content;
            if ( nodeValue == NULL ) nodeValue = (char *)"";
         }

         // Extract index attribute
         attrValue = (char *)xmlGetProp(childNode,(const xmlChar*)"index");
         if ( attrValue == NULL ) nodeIndex = 0;
         else {
            nodeIndex = (uint32_t)strtoul(attrValue,&eptr,0);
            if ( *eptr != '\0' || eptr == attrValue ) nodeIndex = 0;
         }

         // Look for matching device
         devMapIter = devices_.find(nodeName);

         // Element is a device
         if ( devMapIter != devices_.end() ) {
            dev = devMapIter->second;

            // Execute command
            if ( nodeIndex < (int32_t)dev->size() && dev->at(nodeIndex) != NULL ) dev->at(nodeIndex)->execXmlCommand(childNode);
         }

         // Element is a command
         else command(nodeName,nodeValue);
      }
   }
}

// Method to get device structure in xml form.
string Device::getXmlStructure (bool top, bool common, bool hidden, uint32_t level) {
   DeviceMap::iterator    devMapIter;
   DeviceVector           *dev;
   DeviceVector::iterator devIter;
   stringstream           tmp;
   stringstream           loc;
   VariableMap::iterator  varIter;
   CommandMap::iterator   cmdIter;
   uint32_t               locLevel = level;

   tmp.str("");
   loc.str("");

   // Start device tag, don't include for the top level device
   if ( !top ) {
      if ( level != 0 ) {
         for (uint32_t l=0; l < (level*3); l++) loc << " ";
         locLevel++;
      }
      loc << "<device>" << endl;
      if ( ! common ) {
         if ( level != 0 ) for (uint32_t l=0; l < (level*3); l++) loc << " ";
         loc << "<index>" << index_ << "</index>" << endl;
      }
   }

   // Return name and description for top level only when in common mode
   if ( common || !top ) {
      if ( locLevel != 0 ) for (uint32_t l=0; l < (locLevel*3); l++) tmp << " ";
      tmp << "<name>" << name_ << "</name>" << endl;
      if ( desc_ != "" ) {
         if ( locLevel != 0 ) for (uint32_t l=0; l < (locLevel*3); l++) tmp << " ";
         tmp << "<description>" << desc_ << "</description>" << endl;
      }
   }

   // Variables, common or specific
   // Return all top level variables
   if ( variables_.size() > 0 ) {

      // Each local variable
      for (varIter=variables_.begin(); varIter != variables_.end(); ++varIter) 
         if ( common != varIter->second->perInstance() ) tmp << varIter->second->getXmlStructure(hidden||top,locLevel);

   }

   // Commands always device specific
   // Return all top level variables
   if ( (!common) && commands_.size() > 0 ) {

      // Each local command
      for (cmdIter=commands_.begin(); cmdIter != commands_.end(); ++cmdIter) 
         tmp << cmdIter->second->getXmlStructure(hidden||top,locLevel);
   }

   // Sub devices
   if ( devices_.size() > 0 ) {

      // Get each sub device
      for ( devMapIter = devices_.begin(); devMapIter != devices_.end(); devMapIter++ ) {
         dev = devMapIter->second;

         // Device entries
         for ( devIter = dev->begin(); devIter != dev->end(); devIter++ ) {
            if ( (*devIter) != NULL ) {
               tmp << (*devIter)->getXmlStructure(false,common,hidden,locLevel);
               if ( common ) break; // Return first instance when in common mode
            }
         }
      }
   }

   // End device tag
   loc << tmp.str();
   if (!top) {
      if ( level != 0 ) for (uint32_t l=0; l < (level*3); l++) loc << " ";
      loc << "</device>" << endl;
   }

   // Return empty string if there where not local entries
   if ( tmp.str() != "" ) return(loc.str());
   else return("");
}

// Constructor
Device::Device ( uint32_t linkConfig, uint32_t baseAddress, string name, uint32_t index, Device *parent ) {
   Device   *nxt;
   Variable *v;
   Command  *c;

   //cout << "Making: " << name << "(" << index << ")" << " at address: " << hex << baseAddress << endl;

   linkConfig_     = linkConfig;
   baseAddress_     = baseAddress;
   name_            = name;
   index_           = index;
   debug_           = false;
   desc_            = "";
   pollEnable_      = false;

   variables_.clear();
   registers_.clear();
   devices_.clear();
   commands_.clear();

   // Add variable for enable, enable by default
   addVariable(v = new Variable("Enabled",Variable::Configuration));
   v->setPerInstance(true);
   v->setDescription("Set to true to enable device for physical access");
   v->setTrueFalse();
   v->set("True");

   // Hidden variable for register read result
   addVariable(v = new Variable("ReadRegisterResult",Variable::Status));
   v->setHidden(true);

   // Read and write commands
   addCommand(c = new Command("WriteRegister"));
   c->setHidden(true);

   addCommand(c = new Command("ReadRegister"));
   c->setHidden(true);

   // Parent and top level
   parent_ = parent;
   if ( parent_ == NULL ) system_ = (System *)this;
   else {
      nxt = parent_;
      while ( nxt != NULL ) {
         system_ = (System *)nxt;
         nxt = nxt->parent_;
      }
   }

   pthread_mutex_init(&mutex_,NULL);
}

// Deconstructor
Device::~Device ( ) {
   variables_.clear();
   registers_.clear();
   devices_.clear();
   commands_.clear();
   pthread_mutex_unlock(&mutex_);
}

// Set debug flag
void Device::setDebug( bool enable ) {
   DeviceMap::iterator    devMapIter;
   DeviceVector           *dev;
   DeviceVector::iterator devIter;

   // Get each sub device
   for ( devMapIter = devices_.begin(); devMapIter != devices_.end(); devMapIter++ ) {
      dev = devMapIter->second;

      // Device entries
      for ( devIter = dev->begin(); devIter != dev->end(); devIter++ ) {
         if ( (*devIter) != NULL ) (*devIter)->setDebug(enable);
      }
   }
   debug_ = enable;
}

// Add registers
void Device::addRegister(Register *reg) {
   registers_.insert(pair<string,Register*>(reg->name(),reg));
}

// Add variables
void Device::addVariable(Variable *var) {
   variables_.insert(pair<string,Variable*>(var->name(),var));
}

// Add Register Link
void Device::addRegisterLink(RegisterLink *regLink) {
   uint32_t x;

   addRegister(regLink->getRegister());

   for (x=0; x < regLink->getVariableCount(); x++) addVariable(regLink->getVariable(x));

   registerLinks_.push_back(regLink);
}

// Add devices
void Device::addDevice(Device *dev) {
   DeviceMap::iterator devMapIter;
   DeviceVector        *tdev;
   string              name;
   uint32_t            index;
   uint32_t            x;

   // Get device nand and index
   name  = dev->name();
   index = dev->index();

   // Look for device
   devMapIter = devices_.find(name);

   // Device was not found
   if ( devMapIter == devices_.end() ) {
      tdev = new DeviceVector;
      devices_.insert(pair<string,DeviceVector*>(name,tdev));
   }
   else tdev = devMapIter->second;

   // Look for index
   if ( index >= tdev->size() ) {
      tdev->resize(index+1);
      for (x=tdev->size(); x < index; x++) (*tdev)[x] = NULL;
   }

   // Set value
   (*tdev)[index] = dev;
}

// Add commands
void Device::addCommand(Command *cmd) {
   commands_.insert(pair<string,Command*>(cmd->name(),cmd));
}

// Return register, throws exception when not found
Register *Device::getRegister(string name) {
   RegisterMap::iterator regMapIter;
   stringstream          err;

   // Look for register
   regMapIter = registers_.find(name);

   // Register was not found
   if ( regMapIter == registers_.end() ) {
      err.str("");
      err << "Device::getRegister -> Name: " << name_ << " Index: " << dec << index_;
      err << ", Invalid Register: " << name << endl;
      if ( debug_ ) cout << err.str();
      throw(err.str());
   }
   return(regMapIter->second);
}

// Return variable, throw exception when not found
Variable *Device::getVariable(string name) {
   VariableMap::iterator varMapIter;
   stringstream          err;

   // Look for variable
   varMapIter = variables_.find(name);

   // Variable was not found
   if ( varMapIter == variables_.end() ) {
      err.str("");
      err << "Device::getVariable -> Name: " << name_ << " Index: " << dec << index_;
      err << ", Invalid Variable: " << name << endl;
      if ( debug_ ) cout << err.str();
      throw(err.str());
   }
   return(varMapIter->second);
}

// Return command, throw exception when not found
Command *Device::getCommand(string name) {
   CommandMap::iterator cmdMapIter;
   stringstream         err;

   // Look for command
   cmdMapIter = commands_.find(name);

   // Command was not found
   if ( cmdMapIter == commands_.end() ) {
      err.str("");
      err << "Device::getCommand -> Name: " << name_ << " Index: " << dec << index_;
      err << ", Invalid Command: " << name << endl;
      if ( debug_ ) cout << err.str();
      throw(err.str());
   }
   return(cmdMapIter->second);
}

// Get name
string Device::name() { return(name_); }

// Get index
uint32_t Device::index() { return(index_); }

// Method to get linkConfig
uint32_t Device::linkConfig() { return(linkConfig_); }

// Method to get base address
uint32_t Device::baseAddress() { return(baseAddress_); }

// Method to get device
Device * Device::device ( string name, uint32_t index ) {
   DeviceMap::iterator devMapIter;
   DeviceVector        *dev;
   stringstream        err;

   // Look for device
   devMapIter = devices_.find(name);

   // Device was not found
   if ( devMapIter == devices_.end() ) {
      err.str("");
      err << "Device::device -> Name: " << name_ << " Index: " << dec << index_;
      err << ", Device not found: " << name << endl;
      if ( debug_ ) cout << err.str();
      throw(err.str());
      return(NULL);
   }
   dev = devMapIter->second;

   // Get indexed element
   if ( index >= dev->size() ) {
      err.str("");
      err << "Device::device -> Name: " << name_ << " Index: " << dec << index_;
      err << ", Invalid device index. Device: " << name << ", Invalid index: " << dec << index << endl;
      if ( debug_ ) cout << err.str();
      throw(err.str());
      return(NULL);
   }
   return(dev->at(index));
}

// Method to get device count
uint32_t Device::deviceCount ( string name ) {
   DeviceMap::iterator devMapIter;
   DeviceVector        *dev;
   stringstream        err;

   // Look for device
   devMapIter = devices_.find(name);

   // Device was not found
   if ( devMapIter == devices_.end() ) {
      err.str("");
      err << "Device::device -> Name: " << name_ << " Index: " << dec << index_;
      err << ", Device not found: " << name << endl;
      if ( debug_ ) cout << err.str();
      throw(err.str());
      return(0);
   }
   dev = devMapIter->second;

   return((uint32_t)dev->size());
}

// Method to process a command
void Device::command ( string name, string arg ) {
   Command        *cmd;
   stringstream   tmp;
   const char     *sptr;
   char           *eptr;
   uint32_t       ret;
   string         tname;
   string         tvalue;

   // Device is not enabled
   if ( getInt("Enabled") == 0 ) return;

   // Register read
   if ( name == "ReadRegister" ) {
      readRegister(getRegister(arg));
      ret = getRegister(arg)->get();
      getVariable("ReadRegisterResult")->setInt(ret);
      return;
   }
   
   // Register write
   if ( name == "WriteRegister" ) {
      istringstream iss(arg,istringstream::in);
      iss >> tname;
      iss >> tvalue;

      sptr = tvalue.c_str();
      ret = (uint32_t)strtoul(sptr,&eptr,0);
      getRegister(tname)->set(ret);
      writeRegister(getRegister(tname),true,true);
      return;
   }

   cmd = getCommand(name);

   // Command is not internal
   if ( !cmd->internal() ) {

      if ( debug_ ) {
         cout << "Device::command -> Name: " << name_ << " Index: " << dec << index_
              << ", Sending Command: " << cmd->name() << endl;
      }

      // Call function to send command
      system_->commLink()->queueCommand(linkConfig_,cmd);
   }
}

// Method to set run command
void Device::setRunCommand ( string name ) {
   Command       *cmd;
   stringstream  tmp;

   // Device is not enabled
   if ( getInt("Enabled") == 0 ) return;

   cmd = getCommand(name);

   if ( debug_ ) {
      cout << "Device::setRuncommand -> Name: " << name_ << " Index: " << dec << index_
           << ", Setting run command: " << cmd->name() << endl;
   }

   // Call function to send command
   system_->commLink()->setRunCommand(linkConfig_,cmd);
}

// Method to set variable
void Device::set ( string variable, string value ) {
   Variable              *var;

   try {
      var = getVariable(variable);
   } catch ( string error ) { return; } 

   if ( debug_ ) {
      cout << "Device::set -> Name: " << name_ << " Index: " << dec << index_
           << ", Setting variable: " << variable << ", Value: " << value << endl;
   }
   var->set(value);
}

// Method to get variable
string Device::get ( string variable ) {
   Variable              *var;

   try {
      var = getVariable(variable);
   } catch ( string error ) { return ""; } 

   return(var->get());
}

// Method to set variable
void Device::setInt ( string variable, uint32_t value ) {
   Variable              *var;

   try {
      var = getVariable(variable);
   } catch ( string error ) { return; } 

   if ( debug_ ) {
      cout << "Device::setInt -> Name: " << name_ << " Index: " << dec << index_
           << ", Setting variable: " << variable << ", Value: 0x" << hex << value << endl;
   }
   var->setInt(value);
}

// Method to get variable
uint32_t Device::getInt ( string variable ) {
   Variable              *var;

   try {
      var = getVariable(variable);
   } catch ( string error ) { return 0; } 

   return(var->getInt());
}

// Method to set variable
void Device::setFloat ( string variable, float value, const char * format ) {
   Variable              *var;

   try {
      var = getVariable(variable);
   } catch ( string error ) { return; } 

   //if ( var->type() != Variable::Configuration ) return;

   if ( debug_ ) {
      cout << "Device::setInt -> Name: " << name_ << " Index: " << dec << index_
           << ", Setting variable: " << variable << ", Value: " << value << endl;
   }
   var->setFloat(value,format);
}

// Method to get variable
float Device::getFloat ( string variable ) {
   Variable              *var;

   try {
      var = getVariable(variable);
   } catch ( string error ) { return 0; } 

   return(var->getFloat());
}

// Method to read a specific register
uint32_t Device::readSingle ( string name ) {
   RegisterMap::iterator regMapIter;
   stringstream          err;

   REGISTER_LOCK

   // Find command
   regMapIter = registers_.find(name);

   // Command was not found
   if ( regMapIter == registers_.end() ) {
      err.str("");
      err << "Device::readSingle -> Name: " << name_ << " Index: " << dec << index_;
      err << ", Invalid Register: " << name << endl;
      if ( debug_ ) cout << err.str() << endl;
      throw(err.str());
   }

   // Read register
   readRegister(regMapIter->second);

   REGISTER_UNLOCK

   return(regMapIter->second->get());
}

// Method to write a specific register
void Device::writeSingle ( string name, uint32_t value ) {
   RegisterMap::iterator regMapIter;
   stringstream          err;

   REGISTER_LOCK

   // Find register
   regMapIter = registers_.find(name);

   // Command was not found
   if ( regMapIter == registers_.end() ) {
      err.str("");
      err << "Device::writeSingle -> Name: " << name_ << " Index: " << dec << index_;
      err << ", Invalid Register: " << name << endl;
      if ( debug_ ) cout << err.str() << endl;
      throw(err.str());
   }

   // Set value
   regMapIter->second->set(value);

   // Write register
   writeRegister(regMapIter->second,true);

   REGISTER_UNLOCK
}

void Device::writeSingleV2 ( string name, uint32_t value ) {
   Register *r;
   REGISTER_LOCK
   r = getRegister(name);
   r->set(value);
   writeRegister(r, true);
   REGISTER_UNLOCK
}

// Method to read status registers from device
void Device::readStatus() {
   DeviceMap::iterator          devMapIter;
   DeviceVector               * dev;
   DeviceVector::iterator       devIter;
   RegisterLinkVector::iterator regLinkIter;
 
   if ( ! getInt("Enabled") ) return;

   REGISTER_LOCK

   // Read linked registers
   for (regLinkIter = registerLinks_.begin(); regLinkIter != registerLinks_.end(); regLinkIter++) {
      if ( (*regLinkIter)->hasType(Variable::Status) ) {
         readRegister((*regLinkIter)->getRegister());
         (*regLinkIter)->registerToVariables();
      }
   }

   REGISTER_UNLOCK

   // Debug
   if ( debug_ && devices_.size() > 0 ) {
      cout << "Device::readStatus -> name: " << name_
           << ", Index: 0x" << hex << setw(0) << index_ << " reading sub devices: " << endl;
   }

   for ( devMapIter = devices_.begin(); devMapIter != devices_.end(); devMapIter++ ) {
      dev = devMapIter->second;

      // Device entries
      for ( devIter = dev->begin(); devIter != dev->end(); devIter++ ) {
         if ( (*devIter) != NULL ) (*devIter)->readStatus();
      }
   }
}

// Method to read status registers from device
void Device::pollStatus() {
   DeviceMap::iterator          devMapIter;
   DeviceVector               * dev;
   DeviceVector::iterator       devIter;
   RegisterLinkVector::iterator regLinkIter;
 
   if ( ! getInt("Enabled") ) return;

   // Skip local reads if poll is set true and polling enable is false
   if ( pollEnable_ == true ) {

      REGISTER_LOCK

      // Read linked registers
      for (regLinkIter = registerLinks_.begin(); regLinkIter != registerLinks_.end(); regLinkIter++) {
         if ( (*regLinkIter)->pollEnable() && (*regLinkIter)->hasType(Variable::Status) ) {
            readRegister((*regLinkIter)->getRegister());
            (*regLinkIter)->registerToVariables();
         }
      }

      REGISTER_UNLOCK
   }

   // Debug
   if ( debug_ && devices_.size() > 0 ) {
      cout << "Device::pollStatus -> name: " << name_
           << ", Index: 0x" << hex << setw(0) << index_ << " reading sub devices: " << endl;
   }

   for ( devMapIter = devices_.begin(); devMapIter != devices_.end(); devMapIter++ ) {
      dev = devMapIter->second;

      // Device entries
      for ( devIter = dev->begin(); devIter != dev->end(); devIter++ ) {
         if ((*devIter) != NULL ) {
            try {
               (*devIter)->pollStatus();
            } catch (string err) {
               cout << "Device::pollStatus -> name: " << name_
                    << ", Ignoring sub-device " << (*devIter)->name() << " error: " << endl << "   " << err << endl;
            }
         }
      }
   }
}

// Method to read config registers from device
void Device::readConfig ( ) {
   DeviceMap::iterator          devMapIter;
   DeviceVector               * dev;
   DeviceVector::iterator       devIter;
   RegisterLinkVector::iterator regLinkIter;

   if ( ! getInt("Enabled") ) return;

   REGISTER_LOCK

   // Read linked registers
   for (regLinkIter = registerLinks_.begin(); regLinkIter != registerLinks_.end(); regLinkIter++) {
      if ( (*regLinkIter)->hasType(Variable::Configuration) ) {
         readRegister((*regLinkIter)->getRegister());
         (*regLinkIter)->registerToVariables();
      }
   }

   REGISTER_UNLOCK

   // Debug
   if ( debug_ && devices_.size() > 0 ) {
      cout << "Device::readConfig -> name: " << name_
           << ", Index: 0x" << hex << setw(0) << index_ << " reading sub devices: " << endl;
   }

   for ( devMapIter = devices_.begin(); devMapIter != devices_.end(); devMapIter++ ) {
      dev = devMapIter->second;

      // Device entries
      for ( devIter = dev->begin(); devIter != dev->end(); devIter++ ) {
         if ((*devIter) != NULL ) (*devIter)->readConfig();
      }
   }
}

// Method to write registers to device
void Device::writeConfig ( bool force ) {
   DeviceMap::iterator          devMapIter;
   DeviceVector               * dev;
   DeviceVector::iterator       devIter;
   RegisterLinkVector::iterator regLinkIter;

   // Exit if not enabled
   if ( ! getInt("Enabled") ) return;

   REGISTER_LOCK

   // Write linked registers
   for (regLinkIter = registerLinks_.begin(); regLinkIter != registerLinks_.end(); regLinkIter++) {
      if ( (*regLinkIter)->hasType(Variable::Configuration) ) {
         (*regLinkIter)->variablesToRegister();
         writeRegister((*regLinkIter)->getRegister(),force);
      }
   }

   REGISTER_UNLOCK

   // Debug
   if ( debug_ && devices_.size() > 0 ) {
      cout << "Device::write -> name: " << name_
           << ", Index: 0x" << hex << setw(0) << index_ << " writing sub devices: " << endl;
   }

   for ( devMapIter = devices_.begin(); devMapIter != devices_.end(); devMapIter++ ) {
      dev = devMapIter->second;

      // Device entries
      for ( devIter = dev->begin(); devIter != dev->end(); devIter++ ) {
         if ((*devIter) != NULL) (*devIter)->writeConfig(force);
      }
   }
}

// Method to verify hardware state of registers
void Device::verifyConfig( ) {
   DeviceMap::iterator          devMapIter;
   DeviceVector               * dev;
   DeviceVector::iterator       devIter;
   RegisterLinkVector::iterator regLinkIter;

   if ( ! getInt("Enabled") ) return;

   REGISTER_LOCK

   // Verify linked registers
   for (regLinkIter = registerLinks_.begin(); regLinkIter != registerLinks_.end(); regLinkIter++) {
      if ( (*regLinkIter)->hasType(Variable::Configuration) && (*regLinkIter)->verify() ) 
         verifyRegister((*regLinkIter)->getRegister(), false, (*regLinkIter)->getVerifyMask());
   }

   REGISTER_UNLOCK

   // Get each sub device
   for ( devMapIter = devices_.begin(); devMapIter != devices_.end(); devMapIter++ ) {
      dev = devMapIter->second;

      // Device entries
      for ( devIter = dev->begin(); devIter != dev->end(); devIter++ ) {
         if ((*devIter) != NULL ) (*devIter)->verifyConfig();
      }
   }
}

// Hide all commands in the device 
void Device::hideAllCommands() {
   CommandMap::iterator cmdMapIter;
   
   for (cmdMapIter=commands_.begin(); cmdMapIter!=commands_.end(); cmdMapIter++) {
      ((Command*)(cmdMapIter->second))->setHidden(true);
   }
}

// Hide all variables in the device 
void Device::hideAllVariables() {
   VariableMap::iterator i;
   Variable* v;
   //   string s;
   hiddenVariables_.clear();
   
   for (i=variables_.begin(); i!=variables_.end(); i++) {
      //      s = (string)(i->first);
      v = (Variable*)(i->second);
      if (v->hidden()) {
         // Save list of variables that are already hidden so that they can remain hidden when
         // unhideDevice is called
         hiddenVariables_[(i->first)] = v;
      }
      v->setHidden(true);
   }
}

void Device::hideAllVariables(Variable::VariableType vt) {
   VariableMap::iterator i;
   Variable* v;
   
   hiddenVariables_.clear();
   for (i=variables_.begin(); i!=variables_.end(); i++) {
      //      s = (string)(i->first);
      v = (Variable*)(i->second);
      if (v->hidden()) {
         // Save list of variables that are already hidden so that they can remain hidden when
         // unhideDevice is called
         hiddenVariables_[(i->first)] = v;
      }
      v->setHidden(v->type()==vt);
   }
}

// Un-Hide all variables in the device 
void Device::unhideVariables() {
    VariableMap::iterator i;
    for (i=variables_.begin(); i!=variables_.end(); i++) {
       if (hiddenVariables_.count(i->first) == 0) {
          ((Variable*)(i->second))->setHidden(false);
       }
    }
}

// Set all registers stale
void Device::setAllStale(bool stale, bool recursive) {
   RegisterMap::iterator regIter;
   DeviceMap::iterator devMapIter;
   DeviceVector::iterator devIter;
   DeviceVector *dev;
   Register *r;
   
   if (stale) {
      for (regIter=registers_.begin(); regIter!=registers_.end(); regIter++) {
         r = ((Register*)(regIter->second));
         //      cout << "Setting Stale: " << name_ << "(" << index_ << "):" << r->name() << endl;
         r->setStale();
      }
      if (recursive) {
         for (devMapIter=devices_.begin(); devMapIter!=devices_.end(); devMapIter++) {
            dev = devMapIter->second;
            for (devIter=dev->begin(); devIter != dev->end(); devIter++) {
               if ((*devIter) != NULL) (*devIter)->setAllStale();
            }
         }
      }
   }
}
        
// Register read and write helper functions

void Device::regReadInt ( string nameReg, string nameVar, uint32_t bit, uint32_t mask) {
   Register * r;   
   r = getRegister(nameReg);
   readRegister(r);
   getVariable(nameVar)->setInt(r->get(bit, mask)); 
}

void Device::regReadInt ( string name, uint32_t bit, uint32_t mask ){
   regReadInt(name,name,bit,mask);
}

void Device::regWrite ( string nameReg, string nameVar, bool force, uint32_t bit, uint32_t mask ) {
   Register * r;
   r = getRegister(nameReg);
   r->set(getInt(nameVar), bit, mask);
   writeRegister(r, force); 
} 

void Device::regWrite ( string name, bool force, uint32_t bit, uint32_t mask ){
   regWrite(name,name,force,bit,mask);
}

void Device::pollEnable ( bool enable ) {
   pollEnable_ = enable;
}

void Device::hardReset () {
   DeviceMap::iterator          devMapIter;
   DeviceVector               * dev;
   DeviceVector::iterator       devIter;

   // Set all local registers as stale
   setAllStale(true,false);

   // Get each sub device
   for ( devMapIter = devices_.begin(); devMapIter != devices_.end(); devMapIter++ ) {
      dev = devMapIter->second;

      // Device entries
      for ( devIter = dev->begin(); devIter != dev->end(); devIter++ ) {
         if (((*devIter) != NULL) ) (*devIter)->hardReset();
      }
   }
}

void Device::softReset() {
   DeviceMap::iterator          devMapIter;
   DeviceVector               * dev;
   DeviceVector::iterator       devIter;

   // Get each sub device
   for ( devMapIter = devices_.begin(); devMapIter != devices_.end(); devMapIter++ ) {
      dev = devMapIter->second;

      // Device entries
      for ( devIter = dev->begin(); devIter != dev->end(); devIter++ ) {
         if (((*devIter) != NULL) ) (*devIter)->softReset();
      }
   }
}

void Device::countReset() {
   DeviceMap::iterator          devMapIter;
   DeviceVector               * dev;
   DeviceVector::iterator       devIter;

   // Get each sub device
   for ( devMapIter = devices_.begin(); devMapIter != devices_.end(); devMapIter++ ) {
      dev = devMapIter->second;

      // Device entries
      for ( devIter = dev->begin(); devIter != dev->end(); devIter++ ) {
         if (((*devIter) != NULL) ) (*devIter)->countReset();
      }
   }
}

