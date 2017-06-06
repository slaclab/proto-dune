##############################################################################
## This file is part of 'DUNE Development Firmware'.
## It is subject to the license terms in the LICENSE.txt file found in the 
## top-level directory of this distribution and at: 
##    https://confluence.slac.stanford.edu/display/ppareg/LICENSE.html. 
## No part of 'DUNE Development Firmware', including this file, 
## may be copied, modified, propagated, or distributed except according to 
## the terms contained in the LICENSE.txt file.
##############################################################################

set_property DIFF_TERM true [get_ports {dtmClkP[*]}]
set_property DIFF_TERM true [get_ports {dtmClkM[*]}]

# create_clock -name wibRefClk -period  4.000 [get_ports {locRefClkP}]
create_clock -name wibRefClk -period  4.000 [get_ports {dtmRefClkP}]

create_generated_clock -name wibRefClkDiv2 [get_pins  {U_App/U_Wib/U_IBUFDS_GTE2/ODIV2}]
create_generated_clock -name wibClk        [get_pins  {U_App/U_Wib/U_MMCM/MmcmGen.U_Mmcm/CLKOUT0}]

set_clock_groups -asynchronous -group [get_clocks {wibRefClk}] -group [get_clocks {wibClk}] -group [get_clocks {sysClk125}]

create_clock -name recClk -period  4.000 [get_ports {dtmClkP[0]}]

create_generated_clock -name cdrClk [get_pins  {U_App/U_Timing/U_Timing/clkgen/mmcm/CLKOUT0}] 

set_clock_groups -asynchronous -group [get_clocks {sysClk125}] -group [get_clocks {recClk}] 
set_clock_groups -asynchronous -group [get_clocks {sysClk125}] -group [get_clocks {cdrClk}] 
set_clock_groups -asynchronous -group [get_clocks {sysClk200}] -group [get_clocks {recClk}] 
set_clock_groups -asynchronous -group [get_clocks {sysClk200}] -group [get_clocks {cdrClk}] 

set_clock_groups -asynchronous -group [get_clocks {wibClk}] -group [get_clocks {recClk}] 
set_clock_groups -asynchronous -group [get_clocks {wibClk}] -group [get_clocks {cdrClk}] 
