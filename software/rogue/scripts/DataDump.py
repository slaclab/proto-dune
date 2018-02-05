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
import pyrogue.utilities.fileio
import pyrogue.protocols
import argparse
import sys
import os

#################################################################

# Set the argument parser
parser = argparse.ArgumentParser()

# Add arguments
parser.add_argument(
    "--ip", 
    type     = str,
    required = True,
    help     = "RCE's IP address",
) 

# Get the arguments
args = parser.parse_args()

#################################################################

# Set base
system = pr.Root(name='System',description='Front End Board')

# File writer
dataWriter = pr.utilities.fileio.StreamWriter(name='dataWriter')
system.add(dataWriter)

# Create the ETH interface @ IP Address = args.dev
ethLink = pr.protocols.UdpRssiPack(host=args.ip,port=8192,size=1400)    

print( "RSSI Link Up!" )

# Add data stream to file as channel 0 to tDest = 0x0
pr.streamConnect(ethLink.application(0),dataWriter.getChannel(0x0))

# Start the system
system.start(pollEn=False)    

os.system("""bash -c 'read -s -n 1 -p "Press any key to continue..."'""")
