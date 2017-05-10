# Load RUCKUS environment and library
source -quiet $::env(RUCKUS_DIR)/vivado_proc.tcl

# Load common and sub-module ruckus.tcl files
loadRuckusTcl $::env(PROJ_DIR)/../../submodules/surf
loadRuckusTcl $::env(PROJ_DIR)/../../submodules/rce-gen3-fw-lib/DtmCore
loadRuckusTcl $::env(PROJ_DIR)/../../submodules/proto-dune-ep-timing-core
loadRuckusTcl $::env(PROJ_DIR)/../../common/dtm

# Load local Source Code and constraints
loadSource -dir       "$::DIR_PATH/hdl/"
loadConstraints -dir  "$::DIR_PATH/hdl/"
