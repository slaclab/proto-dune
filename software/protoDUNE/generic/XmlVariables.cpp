//-----------------------------------------------------------------------------
// File          : XmlVariables.cpp
// Author        : Ryan Herbst  <rherbst@slac.stanford.edu>
// Created       : 06/05/2012
// Project       : General Purpose
//-----------------------------------------------------------------------------
// Description :
// Extract and store variables from XML string.
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
#include <XmlVariables.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sstream>
#include <stdlib.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <iostream>
#include <fstream>
#include <iomanip>
#include <libxml/tree.h>
#include <stdint.h>
using namespace std;

// Constructor
XmlVariables::XmlVariables ( ) {
   xmlInitParser();
   vars_.clear();  
}

// Deconstructor
XmlVariables::~XmlVariables ( ) { 
   xmlCleanupParser();
   xmlMemoryDump();
}

// Clear
void XmlVariables::clear() {
   vars_.clear();
}

// Remove whitespace and newlines
string XmlVariables::removeWhite ( string str ) {
   string temp;
   uint32_t   i;

   temp = "";

   for (i=0; i < str.length(); i++) {
      if ( str[i] != ' ' && str[i] != '\n' ) 
         temp += str[i];
   }
   return(temp);
}

// Process xml
bool XmlVariables::parse ( string type, const char *str ) {
   xmlDocPtr    doc;
   xmlNodePtr   node;
   string       name;

   // Parse string
   doc = xmlReadMemory(str, strlen(str), "string.xml", NULL, 0);
   if (doc == NULL) return (false);

   // get the root element node
   node = xmlDocGetRootElement(doc);
   name = (char *)node->name;

   // Process
   if ( name == type ) xmlLevel(node,"");

   // Cleanup
   xmlFreeDoc(doc);
   return(true);
}

// Process xml file
bool XmlVariables::parseFile ( string type, string file ) {
   ifstream     is;
   stringstream buffer;

   // Open file
   is.open(file.c_str());
   if ( ! is.is_open() ) return(false);
   buffer.str("");
   buffer << is.rdbuf();
   is.close();

   // Parse string
   return(parse(type,buffer.str().c_str()));
}

// Process level
void XmlVariables::xmlLevel( xmlNode *node, string curr ) {
   xmlNode    *childNode;
   const char *nodeName;
   char       *nodeValue;
   string     nameStr;
   string     valStr;
   char       *attrValue;
   string     idxStr;

   // Look for child nodes
   for ( childNode = node->children; childNode; childNode = childNode->next ) {

      // Copy incoming string
      nameStr = curr;

      if ( childNode->type == XML_ELEMENT_NODE ) {

         // Extract name
         nodeName  = (const char *)childNode->name;

         // Append name
         if ( nameStr != "" ) nameStr.append(":");
         nameStr.append(nodeName);

         // Append index
         attrValue = (char *)xmlGetProp(childNode,(const xmlChar*)"index");
         if ( attrValue != NULL ) {
            idxStr = attrValue;
            nameStr.append("(");
            nameStr.append(idxStr);
            nameStr.append(")");
         }

         // Process children
         xmlLevel(childNode,nameStr);
      }
      else if ( childNode->type == XML_TEXT_NODE ) {
         nodeValue = (char *)childNode->content;
         if ( nodeValue != NULL ) {
            valStr = nodeValue;
            if ( removeWhite(valStr) != "" ) vars_[nameStr] = valStr;
         }
      }
   }
}

// Get
string XmlVariables::get ( string var ) {
   VariableHolder::iterator varMapIter;

   // Look for variable
   varMapIter = vars_.find(var);

   // Variable was not found
   if ( varMapIter == vars_.end() ) return("");
   else return(varMapIter->second);
}

// Get
uint32_t XmlVariables::getInt ( string var ) {
   VariableHolder::iterator varMapIter;
   uint32_t                 ret;
   string                   value;
   const char               *sptr;
   char                     *eptr;

   // Look for variable
   varMapIter = vars_.find(var);

   // Variable was not found
   if ( varMapIter == vars_.end() ) return(0);

   value = varMapIter->second;
   sptr = value.c_str();
   ret = (uint32_t)strtoul(sptr,&eptr,0);
   if ( *eptr != '\0' || eptr == sptr ) ret = 0;
   return(ret);
}

// Get
double XmlVariables::getDouble ( string var ) {
   VariableHolder::iterator varMapIter;
   double                   ret;
   string                   value;
   const char               *sptr;
   char                     *eptr;

   // Look for variable
   varMapIter = vars_.find(var);

   // Variable was not found
   if ( varMapIter == vars_.end() ) return(0);

   value = varMapIter->second;
   sptr = value.c_str();
   ret = strtod(sptr,&eptr);
   if ( *eptr != '\0' || eptr == sptr ) ret = 0;
   return(ret);
}

// get list
string XmlVariables::getList ( string prefix ) {
   stringstream ret;
   ret.str("");

   VariableHolder::iterator varMapIter;

   for ( varMapIter = vars_.begin(); varMapIter != vars_.end(); varMapIter++ ) {
      ret << prefix << varMapIter->first << " = " << varMapIter->second << endl;
   }
   return(ret.str());
}

// Generate XML to transition from one variable to the next
string XmlVariables::genXmlString ( string prevName, string currName, string currValue, string nextName ) {
   uint32_t       level;
   uint32_t       x;
   uint32_t       y;
   uint32_t       high;
   uint32_t       last;
   stringstream   idx;
   stringstream   tag;
   stringstream   ret;
   bool           inIdx;

   ret.str("");

   tag.str("");
   idx.str("");
   inIdx = false;
   level = 0;
   last  = 0;
   high  = 0;

   // Walk up the current name string until it differs from previous string
   for (x=0; x < currName.length(); x++) {
      if ( x < prevName.length() ) {
         if ( currName.at(x) != prevName.at(x) ) break;
         if ( currName.at(x) == ':' ) {
            last = x+1;
            high = x-1;
            level++;
         }
      }
   }

   // Generate XML tags for the rest of the string
   for (x=last; x < currName.length(); x++) {
      if ( currName.at(x) == ':' ) {
         if ( tag.str() != "" ) {
            for (y=0; y < level + 2; y++) ret << "   ";
            ret << "<" << tag.str();
            if ( idx.str() != "" ) ret << " index=\"" << idx.str() << "\"";
            ret << ">" << endl;
         }
         inIdx = false;
         idx.str("");
         tag.str("");
         last = x;
         high = x-1;
         level++;
      }
      else if ( currName.at(x) == '(' ) {
         inIdx = true;
         idx.str("");
      }
      else if ( currName.at(x) == ')' ) inIdx = false;
      else if ( inIdx ) idx << currName.at(x);
      else tag << currName.at(x);
   }
         
   // Output values
   for (x=0; x < level + 2; x++) ret << "   ";
   ret << "<" << tag.str() << ">";
   ret << currValue;
   ret << "</" << tag.str() << ">" << endl;
   level--;

   // Walk up the current name string until it differs from next string
   last = 0;
   for (x=0; x < currName.length(); x++) {
      if ( x < nextName.length() ) {
         if ( currName.at(x) != nextName.at(x) ) break;
         if ( currName.at(x) == ':' ) last = x;
      }
   }

   // Work down and close each tag
   for (x=high; x >= last; x--) {
      if ( currName.at(x) == ':' ) {
         for (y=0; y < level + 2; y++) ret << "   ";
         ret << "</" << currName.substr(x+1,(high-x)) << ">" << endl;
         level--;
         high = x-1;
      }
      else if ( currName.at(x) == '(' ) high = x-1;
      if ( x == 0 ) break;
   }

   if ( x == 0 && high != 0 ) {
      ret << "      " << "</" << currName.substr(0,high+1) << ">" << endl;
   }

   return(ret.str());
}

// get xml
string XmlVariables::getXml ( ) {
   stringstream    ret;
   string          prevName;
   string          currName;
   string          currValue;
   string          nextName;
   string          nextValue;

   ret.str("");
   prevName = "";
   currName = "";
   nextName = "";

   VariableHolder::iterator varMapIter;

   for ( varMapIter = vars_.begin(); varMapIter != vars_.end(); varMapIter++ ) {
      nextName  = varMapIter->first;
      nextValue = varMapIter->second;

      if ( currName != "" ) ret << genXmlString(prevName,currName,currValue,nextName);

      prevName  = currName;
      currName  = nextName;
      currValue = nextValue;
   }

   if ( currName != "" ) ret << genXmlString(prevName,currName,currValue,"");

   return(ret.str());
}

// get xml
string XmlVariables::getXml ( string variable ) {
   VariableHolder::iterator varMapIter;
   stringstream    ret;
   string          currName;
   string          currValue;

   ret.str("");

   // Look for variable
   varMapIter = vars_.find(variable);

   // Variable was not found
   if ( varMapIter == vars_.end() ) return("");

   ret << genXmlString("",varMapIter->first,varMapIter->second,"");

   return(ret.str());
}

// set xml
string XmlVariables::setXml ( string variable, string value ) {
   string ret;
   ret = genXmlString("",variable,value,"");
   return(ret);
}

