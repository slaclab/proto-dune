#!/usr/bin/env python3
#-----------------------------------------------------------------------------
# Title      : GUI Client
#-----------------------------------------------------------------------------
# File       : guiClient.py
# Created    : 2016-09-29
#-----------------------------------------------------------------------------
# Description:
# Generic GUI client for rogue
#-----------------------------------------------------------------------------
# This file is part of the rogue_example software. It is subject to 
# the license terms in the LICENSE.txt file found in the top-level directory 
# of this distribution and at: 
#    https://confluence.slac.stanford.edu/display/ppareg/LICENSE.html. 
# No part of the rogue_example software, including this file, may be 
# copied, modified, propagated, or distributed except according to the terms 
# contained in the LICENSE.txt file.
#-----------------------------------------------------------------------------
import pyrogue.gui
import PyQt4.QtGui
import getopt
import sys

# Set host= to the address of a network interface to secificy the network to use
# Set ns= to the address of the nameserver(optional)
client = pyrogue.PyroClient(
    group= 'rce', 
    ns   = '192.168.2.111',
)

# Create GUI
appTop = PyQt4.QtGui.QApplication(sys.argv)
appTop.setStyle('Fusion')
guiTop = pyrogue.gui.GuiTop(group='rootMesh')
guiTop.addTree(client.getRoot(name='DpmLoopbackTest'))
guiTop.resize(800, 1000)

print("Starting GUI...\n");

# Run gui
appTop.exec_()

client.stop()
