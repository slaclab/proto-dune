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
