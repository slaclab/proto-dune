##############################################################################
## This file is part of 'DUNE Development Firmware'.
## It is subject to the license terms in the LICENSE.txt file found in the 
## top-level directory of this distribution and at: 
##    https://confluence.slac.stanford.edu/display/ppareg/LICENSE.html. 
## No part of 'DUNE Development Firmware', including this file, 
## may be copied, modified, propagated, or distributed except according to 
## the terms contained in the LICENSE.txt file.
##############################################################################

##############################
# Get variables and procedures
##############################
source -quiet $::env(RUCKUS_DIR)/vivado_env_var.tcl
source -quiet $::env(RUCKUS_DIR)/vivado_proc.tcl

# Bypass the debug chipscope generation
#return

############################
## Open the synthesis design
############################
open_run synth_1

###############################
## Set the name of the ILA core
###############################
set ilaName u_ila_0

##################
## Create the core
##################
CreateDebugCore ${ilaName}

#######################
## Set the record depth
#######################
set_property C_DATA_DEPTH 1024 [get_debug_cores ${ilaName}]

#################################
## Set the clock for the ILA core
#################################
SetDebugCoreClk ${ilaName} {U_App/U_Wib/WIB_LINK[0].U_RX/axilClk}

#######################
## Set the debug Probes
#######################

ConfigProbe ${ilaName} {U_App/U_Wib/WIB_LINK[0].U_RX/master[tUser][1]}
ConfigProbe ${ilaName} {U_App/U_Wib/WIB_LINK[0].U_RX/master[tUser][0]}
ConfigProbe ${ilaName} {U_App/U_Wib/WIB_LINK[0].U_RX/master[tLast]}
ConfigProbe ${ilaName} {U_App/U_Wib/WIB_LINK[0].U_RX/master[tValid]}
ConfigProbe ${ilaName} {U_App/U_Wib/WIB_LINK[0].U_RX/slave[tReady]}

ConfigProbe ${ilaName} {U_App/U_Wib/WIB_LINK[0].U_RX/wibMaster[tUser][1]}
ConfigProbe ${ilaName} {U_App/U_Wib/WIB_LINK[0].U_RX/wibMaster[tUser][0]}
ConfigProbe ${ilaName} {U_App/U_Wib/WIB_LINK[0].U_RX/wibMaster[tLast]}
ConfigProbe ${ilaName} {U_App/U_Wib/WIB_LINK[0].U_RX/wibMaster[tValid]}
ConfigProbe ${ilaName} {U_App/U_Wib/WIB_LINK[0].U_RX/wibSlave[tReady]}

ConfigProbe ${ilaName} {U_App/U_Hls/hlsMasters[0][tUser][1]}
ConfigProbe ${ilaName} {U_App/U_Hls/hlsMasters[0][tUser][0]}
ConfigProbe ${ilaName} {U_App/U_Hls/hlsMasters[0][tLast]}
ConfigProbe ${ilaName} {U_App/U_Hls/hlsMasters[0][tValid]}
ConfigProbe ${ilaName} {U_App/U_Hls/hlsSlaves[0][tReady]}

ConfigProbe ${ilaName} {U_App/U_Hls/ssiMasters[0][tUser][1]}
ConfigProbe ${ilaName} {U_App/U_Hls/ssiMasters[0][tUser][0]}
ConfigProbe ${ilaName} {U_App/U_Hls/ssiMasters[0][tLast]}
ConfigProbe ${ilaName} {U_App/U_Hls/ssiMasters[0][tValid]}
ConfigProbe ${ilaName} {U_App/U_Hls/ssiSlaves[0][tReady]}

##########################
## Write the port map file
##########################
WriteDebugProbes ${ilaName} debug.ltx
