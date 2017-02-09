#!/usr/bin/env python
##############################################################################
## This file is part of 'SLAC Generic DAQ Software'.
## It is subject to the license terms in the LICENSE.txt file found in the 
## top-level directory of this distribution and at: 
##    https://confluence.slac.stanford.edu/display/ppareg/LICENSE.html. 
## No part of 'SLAC Generic DAQ Software', including this file, 
## may be copied, modified, propagated, or distributed except according to 
## the terms contained in the LICENSE.txt file.
##############################################################################

import datetime
import time
import threading
import socket
import errno
import xml.dom.minidom
import re
from xml.dom.minidom import parseString

__version__ = "1.0"

class DaqClientException(Exception):
    pass

class DaqClient(threading.Thread):
    """Class to handle daq updates and queries."""

    def __init__(self, dhost, dport, quiet=False):
        """ Create daq class.  """
        threading.Thread.__init__(self)

        self.quiet      = quiet
        self._host      = dhost
        self._port      = dport
        self._sLock     = threading.Lock()
        self._cbLock    = threading.Lock()
        self._sock      = None
        self._sready    = False
        self._tEn       = False
        self._cbacks    = {}
        self._ctime     = {}
        self._config    = {}
        self._status    = {}
        self._configCnt = 0
        self._statusCnt = 0

        # lock for node value resource
        self._nLock  =  threading.Lock()
        self.__nodeValues = {}

        for cb in ["config","status","structure","error"]:
            self._cbacks[cb] = []
            self._ctime[cb]  = None

        self.__initcbStatusFunctions()
    
    def __del__(self):
        self.disable()

    def enable(self):
        self._tEn = True
        self.start()

    def disable(self):
        if self._tEn :
            self._tEn = False
            self.join()

    # Register a structure callback
    def addStuctureCallback (self, function):
        self._cbLock.acquire()
        self._cbacks['structure'].append(function)
        self._cbLock.release()

    # Register a command callback
    def addStatusCallback (self, function):
        self._cbLock.acquire()
        self._cbacks['status'].append(function)
        self._cbLock.release()

    # Register a configuration callback
    def addConfigurationCallback (self, function):
        self._cbLock.acquire()
        self._cbacks['config'].append(function)
        self._cbLock.release()

    # Register an error callback
    def addErrorCallback (self, function):
        self._cbLock.acquire()
        self._cbacks['error'].append(function)
        self._cbLock.release()

    # Get status node value
    def daqReadStatusNode(self,node):
        """Get value for a given node.

        Adds a callback function to continously update the value going forward.

        Arguments:
        node - name of node to extract value for

        returns:
        None the first time and the value of the node otherwise
        """
        r = None
        self._nLock.acquire()
        if node in self.__nodeValues:
            r = self.__nodeValues[node]
        else:
            print("Add callback for \"" + node + "\"")
            self.addStatusCallback( self.__cbStatusFunctions[node] )
        self._nLock.release()
        return r
   
    def sendXml (self, xmlOut) : 
        self._sLock.acquire()
        self._sock.send(xmlOut)
        self._sLock.release()

    # Update a configuration
    def sendConfig (self, path, value):
        xmlStart="<system><config>"
        xmlEnd="</config></system>"

        subs = path.split(":")
        
        for sub in subs:
            flds = re.split('\(|\)',sub)

            xmlEnd = "</%s>%s" % (flds[0],xmlEnd)

            if len(flds) > 1:
                xmlStart = "%s<%s index=\"%s\">" % (xmlStart,flds[0],flds[1])
            else:
                xmlStart = "%s<%s>" % (xmlStart,flds[0])

        # Put value here
        xmlOut = xmlStart + str(value) + xmlEnd + "\n\f"
        self.sendXml(xmlOut)

    def readConfig (self):
        xmlStart="<system><command><ReadConfig/>"
        xmlEnd="</command></system>"
        xmlOut = xmlStart + xmlEnd + "\n\f"
        self.sendXml(xmlOut)

    def readStatus (self):
        xmlStart="<system><command><ReadStatus/>"
        xmlEnd="</command></system>"
        xmlOut = xmlStart + xmlEnd + "\n\f"
        self.sendXml(xmlOut)

    # Send a command
    def sendCommand (self, path, arg):
        xmlStart="<system><command>"
        xmlEnd="</command></system>"
        if path == '':
            xmlOut = xmlStart + str('<') + str(arg) + str('/>') + xmlEnd + "\n\f"

        else:
            subs = path.split(":")
       
            for sub in subs:
                flds = re.split('\(|\)',sub)
    
                xmlEnd = "</%s>%s" % (flds[0],xmlEnd)
    
                if len(flds) > 1:
                    xmlStart = "%s<%s index=\"%s\">" % (xmlStart,flds[0],flds[1])
                else:
                    xmlStart = "%s<%s>" % (xmlStart,flds[0])
    
            # Put value here
            xmlOut = xmlStart + str(arg) + xmlEnd + "\n\f"

        self.sendXml(xmlOut)

    # Send a hard reset
    def hardReset (self):
        xmlStart="<system><command>"
        xmlEnd="</command></system>"
        cmd="<HardReset/>"
        # Put value here
        xmlOut = xmlStart + str(cmd) + xmlEnd + "\n\f"
        print('Sending '+xmlOut)
        self.sendXml(xmlOut)

   # set the default config
    def setDefaults (self):
        xmlStart="<system><command>"
        xmlEnd="</command></system>"
        cmd="<SetDefaults/>"
        # Put value here
        xmlOut = xmlStart + str(cmd) + xmlEnd + "\n\f"
        print('Sending '+xmlOut)
        self.sendXml(xmlOut)

    # send a soft reset 
    def softReset (self):
        xmlStart="<system><command>"
        xmlEnd="</command></system>"
        cmd="<SoftReset/>"
        # Put value here
        xmlOut = xmlStart + str(cmd) + xmlEnd + "\n\f"
        print('Sending '+xmlOut)
        self.sendXml(xmlOut)

    def findConfigStatus (self,xmlType,pathIn,node):
        cnt = 0

        local = node.localName

        if local == "config" or local == "status":
            path = ""
        elif pathIn != "":
           path = pathIn + ":" + node.localName
        else:
           path = node.localName

        attr = node.getAttribute("index")

        if attr != "":
           path = path + "(" + attr + ")"

        for item in node.childNodes:
            if item.nodeType == item.ELEMENT_NODE:
                cnt = cnt + 1
                self.findConfigStatus(xmlType,path,item)

        if cnt == 0:
            if len(node.childNodes) == 1:

                # Update local state
                if xmlType == 'config':
                   self._config[path] = str(node.childNodes[0].nodeValue)
                   self._configCnt += 1
                elif xmlType == 'status':
                   self._status[path] = str(node.childNodes[0].nodeValue)
                   self._statusCnt += 1

                # Execute callbacks
                self._cbLock.acquire()
                for cb in self._cbacks[xmlType]:
                    try:
                        cb(path,node.childNodes[0].nodeValue)
                    except Exception,msg:
                        print("Callback exception: " + str(msg) + " in " + str(path))
                    except :
                        print("Unknown callback exception!")
                self._cbLock.release()

    # Process structure
    def rxStructureDevice ( self, pathIn, node):
        index = None
        for item in node.childNodes:
            if item.nodeType == item.ELEMENT_NODE:
                if item.localName == "index":
                    index = item.childNodes[0].nodeValue
                elif item.localName == "name":
                    path = pathIn + item.childNodes[0].nodeValue

                    if index:
                        path = path + "(%s):" % (index)
                    else:
                        path = path + ":"

                elif item.localName == "variable":
                    self.rxStructureVariable(path,item)
                elif item.localName == "command":
                    self.rxStructureCommand(path,item)
                elif item.localName == "device":
                    self.rxStructureDevice(path,item)

    # Process Variable
    def rxStructureVariable ( self, path, node):
        varData = {}
        varData['name']        = ""
        varData['type']        = ""
        varData['enum']        = ""
        varData['compA']       = ""
        varData['compB']       = ""
        varData['compC']       = ""
        varData['compUnits']   = ""
        varData['min']         = ""
        varData['max']         = ""
        varData['perInstance'] = "False"
        varData['hidden']      = "False"
        varData['description'] = ""

        for item in node.childNodes:
            if item.nodeType == item.ELEMENT_NODE:

                if len(item.childNodes) != 0:
                    val = item.childNodes[0].nodeValue
                else:
                    val = ""

                if item.localName == "name":
                    varData['name'] = path + val
                elif item.localName == "type":
                    varData['type'] = val
                elif item.localName == "enum":
                    if varData['enum'] != "":
                        varData['enum'] += ","
                    varData['enum'] += val
                elif item.localName == "compA":
                    varData['compA'] = val
                elif item.localName == "compB":
                    varData['compB'] = val
                elif item.localName == "compC":
                    varData['compC'] = val
                elif item.localName == "compUnits":
                    varData['compUnits'] = val
                elif item.localName == "min":
                    varData['min'] = val
                elif item.localName == "max":
                    varData['max'] = val
                elif item.localName == "perInstance":
                    varData['perInst'] = "True"
                elif item.localName == "hidden":
                    varData['hidden'] = "True"
                elif item.localName == "description":
                    varData['description'] = val

        if varData['type'] == "Status":
            xmlType = 'status'
            self._status[varData['name']] = ""
        else:
            xmlType = 'config'
            self._config[varData['name']] = ""

        # Execute callbacks
        self._cbLock.acquire()
        for cb in self._cbacks['structure']:
            try:
                cb(xmlType,varData)
            except Exception,msg:
                print("Callback exception: " + str(msg) + " in " + str(varData))
            except :
                print("Unknown callback exception!")
        self._cbLock.release()

    # Process structure
    def rxStructureCommand ( self, path, node):
        cmdData = {}
        cmdData['name']        = ""
        cmdData['hasArg']      = "False"
        cmdData['hidden']      = "False"
        cmdData['description'] = ""

        for item in node.childNodes:
            if item.nodeType == item.ELEMENT_NODE:

                if len(item.childNodes) != 0:
                    val = item.childNodes[0].nodeValue
                else:
                    val = ""

                if item.localName == "name":
                    cmdData['name'] = path + val
                elif item.localName == "hidden":
                    cmdData['hidden'] = "True"
                elif item.localName == "hasArg":
                    cmdData['hasArg'] = "True"
                elif item.localName == "description":
                    cmdData['description'] = val

        # Execute callbacks
        self._cbLock.acquire()
        for cb in self._cbacks['structure']:
            try:
                cb('command',cmdData)
            except Exception,msg:
                print("Callback exception: " + str(msg) + " in " + str(cmdData))
            except :
                print("Unknown callback exception!")
        self._cbLock.release()

    # Process structure
    def rxStructure ( self, node):
        for item in node.childNodes:
            if item.nodeType == item.ELEMENT_NODE:
                if item.localName == "device":
                    self.rxStructureDevice("",item)
                elif item.localName == "variable":
                    self.rxStructureVariable("",item)
                elif item.localName == "command":
                    self.rxStructureCommand("",item)

    # Process error
    def rxError ( self, node):
        val = node.childNodes[0].nodeValue

        self._cbLock.acquire()
        for cb in self._cbacks['error']:
            try:
                cb(val)
            except Exception,msg:
                print("Callback exception: " + str(val))
            except:
                print("Unknown callback exception!")
        self._cbLock.release()

    # Get configuration
    def getConfig(self,path):
        return self._config[path]

    def waitConfig(self,path):
        lCnt = self._configCnt
        while lCnt == self._configCnt:
           time.sleep(.1)
        return self._config[path]

    def waitConfigInt(self,path):
        return int( self.waitConfig(path) )

    # Get status
    def getStatus(self,path):
        return self._status[path]

    def waitStatus(self,path):
        lCnt = self._statusCnt
        while lCnt == self._statusCnt:
           time.sleep(.1)
        return self._status[path]

    # Process a received message
    def rxMessage(self,msg):
        try:
            #msg+=str('>')
            dom = parseString(msg)
            node = dom.documentElement

            for item in node.childNodes:
                if item.nodeType == item.ELEMENT_NODE:
                    if item.localName == "status":
                        self.findConfigStatus("status","",item)
                    elif item.localName == "config":
                        self.findConfigStatus("config","",item)
                    elif item.localName == "structure":
                        self.rxStructure(item)
                    elif item.localName == "error":
                        self.rxError(item)

        except:
            print("Error parsing XML")

    # Connect to the socket
    def connect(self):

        self._sLock.acquire()

        if self._sready:
            self._sLock.release()
            return True

        port = self._port

        while (not self._sready) and port < (self._port+10):
            try: 
                self._sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
                self._sock.settimeout(.1)
                self._sock.connect((self._host, port))
                self._sready = True
            except Exception, e: 
                print("Cannot establish a connection to " + self._host + ":" + str(port))
                port = port + 1 

        self._sLock.release()

        if not self._sready:
            print("Failed to connect")
            time.sleep(1)
            return False

        print("Connected to " + self._host + ":" + str(port))
        return True

    def __cbSetNodeValue(self,path,value,nodeName):
        """Set node value dict if the path matches"""
        if path == nodeName:
            self.__nodeValues[nodeName] = value
    
    # Polling thread
    def run(self):
        data = ""
        last = time.time()
        print("Start run thread at " + str(last))
        # Loop while thread is enabled
        i = 0
        while self._tEn == True:
            self.connect()

            pos = data.find("\f")
            while pos < 0 and self._sready:
                try:
                    self._sLock.acquire()
                    data += self._sock.recv(65536)
                    self._sLock.release()
                except socket.error,e:
                    self._sLock.release()
                pos = data.find("\f")

                #print "Got " + str(len(data)) + " after " + str(time.time() - last)
                #print "\"" + data + "\""

                if pos < 0 and (time.time() - last) > 5:
                    print("Message timeout")
                    self._sLock.acquire()
                    self._sready = False
                    last = time.time()
                    self._sLock.release()

            if pos >= 0:
                xml = data[0:pos]
                data = data[pos+1:]
                
                if i % 100 == 0 and not self.quiet:
                    print("[DaqClient]: Processed %i xml polls, last one with %i bytes and backlog = %i" % (i, len(xml),len(data)))
                self.rxMessage(xml)
                last = time.time()
                #with open('result_%d.xml' % i,'w') as f:
                #    f.write(xml)
                i += 1
    
    
    def __initcbStatusFunctions(self):
        """Status call back function map."""
        self.__cbStatusFunctions = {
            'RunState':self.__cbdaqGetRunState,
            'DataFileCount':self.__cbdaqGetDataFileCount
            }
    
    #---- Specific DAQ status call back function library
    def __cbdaqGetRunState(self, path, value):
        """Call back function"""
        self.__cbSetNodeValue(path,value,'RunState')

    def __cbdaqGetDataFileCount(self, path, value):
        """Call back function"""
        self.__cbSetNodeValue(path,value,'DataFileCount')
    #----

