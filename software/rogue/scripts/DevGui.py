#!/usr/bin/env python3
#-----------------------------------------------------------------------------
# This file is part of the 'DUNE Development Software'. It is subject to 
# the license terms in the LICENSE.txt file found in the top-level directory 
# of this distribution and at: 
#    https://confluence.slac.stanford.edu/display/ppareg/LICENSE.html. 
# No part of the 'DUNE Development Software', including this file, may be 
# copied, modified, propagated, or distributed except according to the terms 
# contained in the LICENSE.txt file.
#-----------------------------------------------------------------------------

import pyrogue as pr
import DevBoard as devBoard
import pyrogue.gui
import pyrogue.protocols
import pyrogue.utilities.prbs
import PyQt4.QtGui
import rogue
import sys
import argparse

# rogue.Logging.setLevel(rogue.Logging.Warning)
# # rogue.Logging.setFilter("pyrogue.SrpV3",rogue.Logging.Debug)
# # rogue.Logging.setLevel(rogue.Logging.Debug)

#################################################################

# Convert str to bool
argBool = lambda s: s.lower() in ['true', 't', 'yes', '1']

# Set the argument parser
parser = argparse.ArgumentParser()

# Add arguments
parser.add_argument(
    "--ip", 
    type     = str,
    required = True,
    help     = "IP address",
) 

parser.add_argument(
    "--pollEn", 
    type     = argBool,
    required = False,
    default  = True,
    help     = "auto-polling",
)  

parser.add_argument(
    "--initRead", 
    type     = argBool,
    required = False,
    default  = True,
    help     = "Enable read all variables at start",
)  

# Get the arguments
args = parser.parse_args()

#################################################################

# Create the ETH interface @ IP Address = args.dev
rudp = pr.protocols.UdpRssiPack(
        host    = args.ip,
        port    = 8192,
        packVer = 1) 
        
# Map the AxiStream.TDEST
vcData = rudp.application(0x00); # AxiStream.tDest = 0x00
vcSrp  = rudp.application(0x80); # AxiStream.tDest = 0x80
vcPrbs = rudp.application(0xFF); # AxiStream.tDest = 0xFF
        
#################################################################    

# Set base
rootTop = pr.Root(name='System',description='Front End Board')
    
# Connect to SRPv3
srp = rogue.protocols.srp.SrpV3()
pr.streamConnectBiDir(vcSrp,srp)  

# Loopback the PRBS data
pyrogue.streamConnect(vcPrbs,vcPrbs)
    
# Connect the SW RSSI monitor
rootTop.add(rudp)    
    
# Add registers
rootTop.add(devBoard.Fpga(memBase=srp))

#################################################################    

# Start the system
rootTop.start(
    pollEn   = args.pollEn,
    initRead = args.initRead,
)

# Create GUI
appTop = PyQt4.QtGui.QApplication(sys.argv)
guiTop = pr.gui.GuiTop(group='PyRogueGui')
guiTop.addTree(rootTop)
guiTop.resize(800, 1000)

print("Starting GUI...\n");

# Run gui
appTop.exec_()

#################################################################    

# Stop mesh after gui exits
rootTop.stop()
exit()

#################################################################
