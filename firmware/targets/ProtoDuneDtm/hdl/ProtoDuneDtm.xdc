##############################################################################
## This file is part of 'DUNE Development Firmware'.
## It is subject to the license terms in the LICENSE.txt file found in the 
## top-level directory of this distribution and at: 
##    https://confluence.slac.stanford.edu/display/ppareg/LICENSE.html. 
## No part of 'DUNE Development Firmware', including this file, 
## may be copied, modified, propagated, or distributed except according to 
## the terms contained in the LICENSE.txt file.
##############################################################################

###########################
# User Application IO Types
###########################
set_property IOSTANDARD LVDS_25  [get_ports dtmToRtmLsP]
set_property IOSTANDARD LVDS_25  [get_ports dtmToRtmLsM]

set_property DIFF_TERM true [get_ports {dtmToRtmLsP[0]}]
set_property DIFF_TERM true [get_ports {dtmToRtmLsM[0]}]

set_property DIFF_TERM true [get_ports {dtmToRtmLsP[1]}]
set_property DIFF_TERM true [get_ports {dtmToRtmLsM[1]}]

set_property DIFF_TERM true [get_ports {dpmFbP[*]}]
set_property DIFF_TERM true [get_ports {dpmFbM[*]}]

#########################
# User Timing Constraints
#########################

create_clock -name recClk -period  4.000 [get_ports {dtmToRtmLsP[0]}]
create_clock -name emuClk -period  4.000 [get_pins {U_App/U_DPM_INTF/U_BUFGMUX/O}]

create_generated_clock -name cdrClk [get_pins  {U_App/U_Timing/clkgen/mmcm/CLKOUT0}] 

set_clock_groups -asynchronous -group [get_clocks {sysClk125}] -group [get_clocks {recClk}] 
set_clock_groups -asynchronous -group [get_clocks {sysClk125}] -group [get_clocks {cdrClk}] 
set_clock_groups -asynchronous -group [get_clocks {sysClk200}] -group [get_clocks {recClk}] 
set_clock_groups -asynchronous -group [get_clocks {sysClk200}] -group [get_clocks {cdrClk}] 

set_case_analysis 1 [get_pins {U_App/U_DPM_INTF/U_BUFGMUX/S}]
