# Load RUCKUS environment and library
source -quiet $::env(RUCKUS_DIR)/vivado_proc.tcl

# Load common and sub-module ruckus.tcl files
loadRuckusTcl $::env(PROJ_DIR)/../../submodules/surf
loadRuckusTcl $::env(PROJ_DIR)/../../submodules/rce-gen3-fw-lib/DpmCore
loadRuckusTcl $::env(PROJ_DIR)/../../common/dpm
loadRuckusTcl $::env(PROJ_DIR)/../../common/VivadoHLS/DuneDataCompression

# Load local Source Code and constraints
loadSource -dir       "$::DIR_PATH/hdl/"
loadSource -path      "$::DIR_PATH/Version.vhd"
loadConstraints -dir  "$::DIR_PATH/hdl/"
