#!/usr/bin/env python
#-----------------------------------------------------------------------------
# Title      : PyRogue feb Module
#-----------------------------------------------------------------------------
# File       : _feb.py
# Created    : 2017-02-15
# Last update: 2017-02-15
#-----------------------------------------------------------------------------
# Description:
# PyRogue Feb Module
#-----------------------------------------------------------------------------
# This file is part of the 'Development Board Examples'. It is subject to 
# the license terms in the LICENSE.txt file found in the top-level directory 
# of this distribution and at: 
#    https://confluence.slac.stanford.edu/display/ppareg/LICENSE.html. 
# No part of the 'Development Board Examples', including this file, may be 
# copied, modified, propagated, or distributed except according to the terms 
# contained in the LICENSE.txt file.
#-----------------------------------------------------------------------------

import pyrogue             as pr
import surf.protocols.ssi  as ssi
import surf.protocols.rssi as rssi
import surf.ethernet.udp   as udp

class Fpga(pr.Device):                         
    def __init__( self,       
        name        = 'Fpga',
        description = 'Fpga Container',
        **kwargs):
        
        super().__init__(name=name,description=description, **kwargs)
        
        #############
        # Add devices
        #############
        
        self.add(rssi.RssiCore(
            offset = 0xA4010000,
            # expand = False,
        ))          
        
        self.add(ssi.SsiPrbsTx(
            offset = 0xA4020000,
            # expand = False,
        )) 

        self.add(ssi.SsiPrbsRx(
            offset = 0xA4030000,
            # expand = False,
        ))
        