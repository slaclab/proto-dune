#!/usr/bin/python

import sys,socket,fcntl,os
from time import sleep
import xml.dom.minidom
from xml.dom.minidom import parseString

hosts = []

if len(sys.argv) < 2:
   print "Usage: find_variable.py <variable> [host list]"
   sys.exit()
elif len(sys.argv) > 2:
   for i in range(2,len(sys.argv)):
      hosts.append(sys.argv[i])
else:
   for i in range(0,2):
      hosts.append("dtm" + str(i))
   for i in range(0,15):
      hosts.append("dpm" + str(i))

target = sys.argv[1]

print "Looking for variables matching " + target

varData  = []
varValue = []
width    = 0

def getPath (pathIn, node):
   path = node.localName
  
   attr = node.getAttribute("index")

   if attr != "":
      path = path + "(" + attr + ")"

   if pathIn != "":
      path = path + "/" + pathIn

   if node.localName == "system":
      retPath = path
   else:
      retPath = getPath(path,node.parentNode)

   return(retPath)

def findLeaf (node):
   global width
   global varData
   global varValue
   cnt = 0

   for item in node.childNodes:
      if item.nodeType == item.ELEMENT_NODE:
         cnt = cnt + 1
         findLeaf(item)

   if cnt == 0:
      path = getPath("",node)
      if len(path) > width: width = len(path)

      if len(node.childNodes) > 0:
         varData.append(path)
         varValue.append(node.childNodes[0].nodeValue)

for host in hosts:
   sport = 8090
   ready = 0
   data  = ""
   count = 0

   s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
   s.settimeout(1)

   port = sport

   while ready == 0 and port < (sport+10):
      try: 
         s.connect((host, port))
         ready = 1
      except Exception, e: 
         print "Cannot establish a connectioan to " + host + ":" + str(port)
         port = port + 1 

   if ready == 0:
      print "Failed to connect"
      sys.exit()

   print "Connected to " + host + ":" + str(port)

   while "\f" not in data:
      data += s.recv(1024)

   print "Read " + str(len(data)) + " Bytes"
   s.close()

   dom = parseString(data[0:-2])

   searchNodes = dom.getElementsByTagName(target)

   for found in searchNodes:
      findLeaf(found)

print ""
print "Found the following matching variables:"

i = 0
for line in varData:
   print line.ljust(width+2) + " = " + varValue[i]
   i = i + 1

