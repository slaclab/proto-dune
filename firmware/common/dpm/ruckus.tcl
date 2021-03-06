# Load RUCKUS library
source -quiet $::env(RUCKUS_DIR)/vivado_proc.tcl

## Check for version 2016.4 of Vivado
if { [VersionCheck 2016.4] < 0 } {
   close_project
   exit -1
}

# Load Source Code
loadSource -dir "$::DIR_PATH/rtl/"
loadSource -dir "$::DIR_PATH/wib/"
loadSource -sim_only -dir "$::DIR_PATH/tb/"
