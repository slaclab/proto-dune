# Load RUCKUS environment and library
source -quiet $::env(RUCKUS_DIR)/vivado_proc.tcl

# Load common and sub-module ruckus.tcl files
loadRuckusTcl $::env(PROJ_DIR)/../../submodules/surf
loadRuckusTcl $::env(PROJ_DIR)/../../submodules/rce-gen3-fw-lib/DpmCore
loadRuckusTcl $::env(PROJ_DIR)/../../common/dpm
loadRuckusTcl $::env(PROJ_DIR)/../../common/VivadoHLS/DuneDataCompression

# Load the CERN Timing Library
loadSource -dir $::env(PROJ_DIR)/../../submodules/timing-board-firmware/components/pdts/firmware/hdl
loadSource -dir $::env(PROJ_DIR)/../../submodules/timing-board-firmware/projects/endpoint/firmware/hdl
remove_files [get_files {pdts_endpoint_dummy.vhd}] 

# Load local Source Code and constraints
loadSource -dir       "$::DIR_PATH/hdl/"
loadConstraints -dir  "$::DIR_PATH/hdl/"

# Place and Route strategies 
set_property strategy Performance_Explore [get_runs impl_1]