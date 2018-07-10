# Load RUCKUS environment and library
source -quiet $::env(RUCKUS_DIR)/vivado_proc.tcl

# Load common and sub-module ruckus.tcl files
loadRuckusTcl $::env(PROJ_DIR)/../../submodules/surf
loadRuckusTcl $::env(PROJ_DIR)/../../common/VivadoHLS/DuneDataCompression
loadRuckusTcl $::env(PROJ_DIR)/../../submodules/rce-gen3-fw-lib/RceG3

DcpToVerilogSim DuneDataCompressionCore

loadSource -path  $::env(PROJ_DIR)/../../common/dpm/wib/EthernetCRCD16B.vhd
loadSource -path  $::env(PROJ_DIR)/../../common/dpm/rtl/ProtoDuneDpmPkg.vhd
loadSource -path  $::env(PROJ_DIR)/../../common/dpm/rtl/ProtoDuneDpmHls.vhd
loadSource -path  $::env(PROJ_DIR)/../../common/dpm/rtl/ProtoDuneDpmHlsMon.vhd
loadSource -path  $::env(PROJ_DIR)/../../common/dpm/rtl/ProtoDuneDpmEmuData.vhd
loadSource -path  $::env(PROJ_DIR)/../../common/dpm/rtl/ProtoDuneDpmEmuTxFramer.vhd
loadSource -path  $::env(PROJ_DIR)/../../common/dpm/rtl/ProtoDuneDpmWibRxFramer.vhd
loadSource -path  $::env(PROJ_DIR)/../../common/dpm/rtl/ProtoDuneDpmWibRxFramerReg.vhd

# Load local Source Code and constraints
loadSource -sim_only -dir "$::DIR_PATH/rtl/"

set_property top {compression_tb} [get_filesets sim_1]

