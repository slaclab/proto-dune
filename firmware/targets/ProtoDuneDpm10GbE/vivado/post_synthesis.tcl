##############################################################################
## This file is part of 'DUNE Development Firmware'.
## It is subject to the license terms in the LICENSE.txt file found in the 
## top-level directory of this distribution and at: 
##    https://confluence.slac.stanford.edu/display/ppareg/LICENSE.html. 
## No part of 'DUNE Development Firmware', including this file, 
## may be copied, modified, propagated, or distributed except according to 
## the terms contained in the LICENSE.txt file.
##############################################################################

# Bypass the debug chipscope generation
return

##############################
# Get variables and procedures
##############################
source -quiet $::env(RUCKUS_DIR)/vivado_env_var.tcl
source -quiet $::env(RUCKUS_DIR)/vivado_proc.tcl

############################
## Open the synthesis design
############################
open_run synth_1

###############################
## Set the name of the ILA core
###############################
set ilaName0 u_ila_0

##################
## Create the core
##################
CreateDebugCore ${ilaName0}

#######################
## Set the record depth
#######################
set_property C_DATA_DEPTH 1024 [get_debug_cores ${ilaName0}]

#################################
## Set the clock for the ILA core
#################################
SetDebugCoreClk ${ilaName0} {U_App/U_Timing/cdrClk}

#######################
## Set the debug Probes
#######################
ConfigProbe ${ilaName0} {U_App/U_Timing/cdrRst}
ConfigProbe ${ilaName0} {U_App/U_Timing/timingBus[*}

##########################
## Write the port map file
##########################
WriteDebugProbes ${ilaName0} ${PROJ_DIR}/images/debug_probes.ltx