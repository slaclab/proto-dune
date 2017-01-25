# Load RUCKUS environment and library
source -quiet $::env(RUCKUS_DIR)/vivado_proc.tcl

# Load common and sub-module ruckus.tcl files
loadRuckusTcl $::env(PROJ_DIR)/../../submodules/surf
loadRuckusTcl $::env(PROJ_DIR)/../../submodules/rce-gen3/DtmCore
loadRuckusTcl $::env(PROJ_DIR)/../../common/dtm

# Load local source Code
loadSource -dir  "$::DIR_PATH/hdl/"
loadSource -path "$::DIR_PATH/Version.vhd"

# Load Constraints
loadConstraints -path "$::DIR_PATH/../../common/rce-gen3/DtmCore/hdl/DtmCore.xdc"
loadConstraints -path "$::DIR_PATH/hdl/ProtoDuneDtm.xdc"
