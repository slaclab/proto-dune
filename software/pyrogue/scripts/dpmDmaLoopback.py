#!/usr/bin/env python3
#-----------------------------------------------------------------------------
# This file is part of the 'Development Board Examples'. It is subject to 
# the license terms in the LICENSE.txt file found in the top-level directory 
# of this distribution and at: 
#    https://confluence.slac.stanford.edu/display/ppareg/LICENSE.html. 
# No part of the 'Development Board Examples', including this file, may be 
# copied, modified, propagated, or distributed except according to the terms 
# contained in the LICENSE.txt file.
#-----------------------------------------------------------------------------

import pyrogue as pr
import Pyro4.naming
import pyrogue.utilities.prbs
import rogue.hardware.rce
import rogue.hardware.axi
import sys
import time

#################################################################

vcPrbs = rogue.hardware.axi.AxiStreamDma("/dev/axi_stream_dma_2",2,1)
 
#################################################################    

# Set base
dpmTest = pr.Root(name='DpmLoopbackTest',description='DPM Loopback Testing')

prbsRx = pyrogue.utilities.prbs.PrbsRx(name='PrbsRx')
pyrogue.streamConnect(vcPrbs,prbsRx)
dpmTest.add(prbsRx)  
    
prbTx = pyrogue.utilities.prbs.PrbsTx(name="PrbsTx")
pyrogue.streamConnect(prbTx, vcPrbs)
dpmTest.add(prbTx)  
    
#################################################################    

# Start the system
dpmTest.start(
    pollEn   = True,
    initRead = True,
    pyroGroup= 'rce', 
    pyroAddr = '192.168.2.111', # RCE IP   
)

# Close window and stop polling
def stop():
    dpmTest.stop()
    exit()

# Start with ipython -i scripts/rceGuiClient.py
print("Started rogue mesh server. To exit type stop()")

while True:
    time.sleep(1)
    
