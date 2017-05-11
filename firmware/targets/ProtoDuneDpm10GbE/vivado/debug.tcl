##############################################################################
## This file is part of 'LCLS2 LLRF Firmware'.
## It is subject to the license terms in the LICENSE.txt file found in the 
## top-level directory of this distribution and at: 
##    https://confluence.slac.stanford.edu/display/ppareg/LICENSE.html. 
## No part of 'LCLS2 LLRF Firmware', including this file, 
## may be copied, modified, propagated, or distributed except according to 
## the terms contained in the LICENSE.txt file.
##############################################################################

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
set ilaName1 u_ila_1

##################
## Create the core
##################
CreateDebugCore ${ilaName0}
# CreateDebugCore ${ilaName1}

#######################
## Set the record depth
#######################
set_property C_DATA_DEPTH 1024 [get_debug_cores ${ilaName0}]
# set_property C_DATA_DEPTH 1024 [get_debug_cores ${ilaName1}]

#################################
## Set the clock for the ILA core
#################################
SetDebugCoreClk ${ilaName0} {U_App/U_Hls/dmaClk}
# SetDebugCoreClk ${ilaName1} {U_App/U_Hls/axilClk}

#######################
## Set the debug Probes
#######################

ConfigProbe ${ilaName0} {U_DpmCore/U_RceG3Top/U_RceG3Dma/U_AxisV2DmaGen.U_RceG3DmaAxisV2/U_Gen2Dma[2].U_RceG3DmaAxisChan/U_AxiStreamDmaV2/U_ChanGen[0].U_DmaWrite/axiWriteCtrl[pause]}
ConfigProbe ${ilaName0} {U_DpmCore/U_RceG3Top/U_RceG3Dma/U_AxisV2DmaGen.U_RceG3DmaAxisV2/U_Gen2Dma[2].U_RceG3DmaAxisChan/U_AxiStreamDmaV2/U_ChanGen[0].U_DmaWrite/r[ackCount][*]}
ConfigProbe ${ilaName0} {U_DpmCore/U_RceG3Top/U_RceG3Dma/U_AxisV2DmaGen.U_RceG3DmaAxisV2/U_Gen2Dma[2].U_RceG3DmaAxisChan/U_AxiStreamDmaV2/U_ChanGen[0].U_DmaWrite/r[awlen][*]}
ConfigProbe ${ilaName0} {U_DpmCore/U_RceG3Top/U_RceG3Dma/U_AxisV2DmaGen.U_RceG3DmaAxisV2/U_Gen2Dma[2].U_RceG3DmaAxisChan/U_AxiStreamDmaV2/U_ChanGen[0].U_DmaWrite/r[continue]}
ConfigProbe ${ilaName0} {U_DpmCore/U_RceG3Top/U_RceG3Dma/U_AxisV2DmaGen.U_RceG3DmaAxisV2/U_Gen2Dma[2].U_RceG3DmaAxisChan/U_AxiStreamDmaV2/U_ChanGen[0].U_DmaWrite/r[dmaWrTrack][dropEn]}
ConfigProbe ${ilaName0} {U_DpmCore/U_RceG3Top/U_RceG3Dma/U_AxisV2DmaGen.U_RceG3DmaAxisV2/U_Gen2Dma[2].U_RceG3DmaAxisChan/U_AxiStreamDmaV2/U_ChanGen[0].U_DmaWrite/r[dmaWrTrack][inUse]}
ConfigProbe ${ilaName0} {U_DpmCore/U_RceG3Top/U_RceG3Dma/U_AxisV2DmaGen.U_RceG3DmaAxisV2/U_Gen2Dma[2].U_RceG3DmaAxisChan/U_AxiStreamDmaV2/U_ChanGen[0].U_DmaWrite/r[dmaWrTrack][overflow]}
ConfigProbe ${ilaName0} {U_DpmCore/U_RceG3Top/U_RceG3Dma/U_AxisV2DmaGen.U_RceG3DmaAxisV2/U_Gen2Dma[2].U_RceG3DmaAxisChan/U_AxiStreamDmaV2/U_ChanGen[0].U_DmaWrite/r[dmaWrTrack][dest][*]}
ConfigProbe ${ilaName0} {U_DpmCore/U_RceG3Top/U_RceG3Dma/U_AxisV2DmaGen.U_RceG3DmaAxisV2/U_Gen2Dma[2].U_RceG3DmaAxisChan/U_AxiStreamDmaV2/U_ChanGen[0].U_DmaWrite/r[reqCount][*]}
ConfigProbe ${ilaName0} {U_DpmCore/U_RceG3Top/U_RceG3Dma/U_AxisV2DmaGen.U_RceG3DmaAxisV2/U_Gen2Dma[2].U_RceG3DmaAxisChan/U_AxiStreamDmaV2/U_ChanGen[0].U_DmaWrite/r[result][*]}
ConfigProbe ${ilaName0} {U_DpmCore/U_RceG3Top/U_RceG3Dma/U_AxisV2DmaGen.U_RceG3DmaAxisV2/U_Gen2Dma[2].U_RceG3DmaAxisChan/U_AxiStreamDmaV2/U_ChanGen[0].U_DmaWrite/r[slave][tReady]}
ConfigProbe ${ilaName0} {U_DpmCore/U_RceG3Top/U_RceG3Dma/U_AxisV2DmaGen.U_RceG3DmaAxisV2/U_Gen2Dma[2].U_RceG3DmaAxisChan/U_AxiStreamDmaV2/U_ChanGen[0].U_DmaWrite/r[state][*]}
ConfigProbe ${ilaName0} {U_DpmCore/U_RceG3Top/U_RceG3Dma/U_AxisV2DmaGen.U_RceG3DmaAxisV2/U_Gen2Dma[2].U_RceG3DmaAxisChan/U_AxiStreamDmaV2/U_ChanGen[0].U_DmaWrite/r[wMaster][wlast]}
ConfigProbe ${ilaName0} {U_DpmCore/U_RceG3Top/U_RceG3Dma/U_AxisV2DmaGen.U_RceG3DmaAxisV2/U_Gen2Dma[2].U_RceG3DmaAxisChan/U_AxiStreamDmaV2/U_ChanGen[0].U_DmaWrite/r[wMaster][wvalid]}
ConfigProbe ${ilaName0} {U_DpmCore/U_RceG3Top/U_RceG3Dma/U_AxisV2DmaGen.U_RceG3DmaAxisV2/U_Gen2Dma[2].U_RceG3DmaAxisChan/U_AxiStreamDmaV2/U_ChanGen[0].U_DmaWrite/r[wMaster][wstrb][*]}
ConfigProbe ${ilaName0} {U_DpmCore/U_RceG3Top/U_RceG3Dma/U_AxisV2DmaGen.U_RceG3DmaAxisV2/U_Gen2Dma[2].U_RceG3DmaAxisChan/U_AxiStreamDmaV2/U_ChanGen[0].U_DmaWrite/r[wMaster][awvalid]}
ConfigProbe ${ilaName0} {U_DpmCore/U_RceG3Top/U_RceG3Dma/U_AxisV2DmaGen.U_RceG3DmaAxisV2/U_Gen2Dma[2].U_RceG3DmaAxisChan/U_AxiStreamDmaV2/U_ChanGen[0].U_DmaWrite/axiWriteCtrl[pause]}

ConfigProbe ${ilaName0} {U_DpmCore/U_RceG3Top/U_RceG3Dma/U_AxisV2DmaGen.U_RceG3DmaAxisV2/U_Gen2Dma[2].U_RceG3DmaAxisChan/U_AxiStreamDmaV2/U_ChanGen[0].U_DmaWrite/axisMaster[tDest][*]}
ConfigProbe ${ilaName0} {U_DpmCore/U_RceG3Top/U_RceG3Dma/U_AxisV2DmaGen.U_RceG3DmaAxisV2/U_Gen2Dma[2].U_RceG3DmaAxisChan/U_AxiStreamDmaV2/U_ChanGen[0].U_DmaWrite/axisMaster[tKeep][*]}
ConfigProbe ${ilaName0} {U_DpmCore/U_RceG3Top/U_RceG3Dma/U_AxisV2DmaGen.U_RceG3DmaAxisV2/U_Gen2Dma[2].U_RceG3DmaAxisChan/U_AxiStreamDmaV2/U_ChanGen[0].U_DmaWrite/axisMaster[tLast]}
ConfigProbe ${ilaName0} {U_DpmCore/U_RceG3Top/U_RceG3Dma/U_AxisV2DmaGen.U_RceG3DmaAxisV2/U_Gen2Dma[2].U_RceG3DmaAxisChan/U_AxiStreamDmaV2/U_ChanGen[0].U_DmaWrite/axisMaster[tValid]}
ConfigProbe ${ilaName0} {U_DpmCore/U_RceG3Top/U_RceG3Dma/U_AxisV2DmaGen.U_RceG3DmaAxisV2/U_Gen2Dma[2].U_RceG3DmaAxisChan/U_AxiStreamDmaV2/U_ChanGen[0].U_DmaWrite/axisSlave[tReady]}

ConfigProbe ${ilaName0} {U_DpmCore/U_RceG3Top/U_RceG3Dma/U_AxisV2DmaGen.U_RceG3DmaAxisV2/U_Gen2Dma[2].U_RceG3DmaAxisChan/axiWriteMaster[awvalid]}
ConfigProbe ${ilaName0} {U_DpmCore/U_RceG3Top/U_RceG3Dma/U_AxisV2DmaGen.U_RceG3DmaAxisV2/U_Gen2Dma[2].U_RceG3DmaAxisChan/axiWriteMaster[awid][*]}
ConfigProbe ${ilaName0} {U_DpmCore/U_RceG3Top/U_RceG3Dma/U_AxisV2DmaGen.U_RceG3DmaAxisV2/U_Gen2Dma[2].U_RceG3DmaAxisChan/axiWriteMaster[wid][*]}
ConfigProbe ${ilaName0} {U_DpmCore/U_RceG3Top/U_RceG3Dma/U_AxisV2DmaGen.U_RceG3DmaAxisV2/U_Gen2Dma[2].U_RceG3DmaAxisChan/axiWriteMaster[wlast]}
ConfigProbe ${ilaName0} {U_DpmCore/U_RceG3Top/U_RceG3Dma/U_AxisV2DmaGen.U_RceG3DmaAxisV2/U_Gen2Dma[2].U_RceG3DmaAxisChan/axiWriteMaster[wvalid]}
ConfigProbe ${ilaName0} {U_DpmCore/U_RceG3Top/U_RceG3Dma/U_AxisV2DmaGen.U_RceG3DmaAxisV2/U_Gen2Dma[2].U_RceG3DmaAxisChan/axiWriteMaster[wstrb][*]}
ConfigProbe ${ilaName0} {U_DpmCore/U_RceG3Top/U_RceG3Dma/U_AxisV2DmaGen.U_RceG3DmaAxisV2/U_Gen2Dma[2].U_RceG3DmaAxisChan/axiWriteSlave[awready]}
ConfigProbe ${ilaName0} {U_DpmCore/U_RceG3Top/U_RceG3Dma/U_AxisV2DmaGen.U_RceG3DmaAxisV2/U_Gen2Dma[2].U_RceG3DmaAxisChan/axiWriteSlave[bvalid]}
ConfigProbe ${ilaName0} {U_DpmCore/U_RceG3Top/U_RceG3Dma/U_AxisV2DmaGen.U_RceG3DmaAxisV2/U_Gen2Dma[2].U_RceG3DmaAxisChan/axiWriteSlave[wready]}

ConfigProbe ${ilaName0} {U_App/U_Hls/dmaIbMaster[tLast]}
ConfigProbe ${ilaName0} {U_App/U_Hls/dmaIbMaster[tValid]}
ConfigProbe ${ilaName0} {U_App/U_Hls/dmaIbSlave[tReady]}

ConfigProbe ${ilaName0} {U_App/U_Hls/dmaIbMasters[0][tLast]}
ConfigProbe ${ilaName0} {U_App/U_Hls/dmaIbMasters[0][tValid]}
ConfigProbe ${ilaName0} {U_App/U_Hls/dmaIbSlaves[0][tReady]}

ConfigProbe ${ilaName0} {U_App/U_Hls/dmaIbMasters[1][tLast]}
ConfigProbe ${ilaName0} {U_App/U_Hls/dmaIbMasters[1][tValid]}
ConfigProbe ${ilaName0} {U_App/U_Hls/dmaIbSlaves[1][tReady]}

# ConfigProbe ${ilaName1} {U_App/U_Hls/ibHlsMasters[0][tLast]}
# ConfigProbe ${ilaName1} {U_App/U_Hls/ibHlsMasters[0][tValid]}
# ConfigProbe ${ilaName1} {U_App/U_Hls/ibHlsSlaves[0][tReady]}

# ConfigProbe ${ilaName1} {U_App/U_Hls/ibHlsMasters[1][tLast]}
# ConfigProbe ${ilaName1} {U_App/U_Hls/ibHlsMasters[1][tValid]}
# ConfigProbe ${ilaName1} {U_App/U_Hls/ibHlsSlaves[1][tReady]}

# ConfigProbe ${ilaName1} {U_App/U_Hls/obHlsMasters[0][tLast]}
# ConfigProbe ${ilaName1} {U_App/U_Hls/obHlsMasters[0][tValid]}
# ConfigProbe ${ilaName1} {U_App/U_Hls/obHlsSlaves[0][tReady]}

# ConfigProbe ${ilaName1} {U_App/U_Hls/obHlsMasters[1][tLast]}
# ConfigProbe ${ilaName1} {U_App/U_Hls/obHlsMasters[1][tValid]}
# ConfigProbe ${ilaName1} {U_App/U_Hls/obHlsSlaves[1][tReady]}

##########################
## Write the port map file
##########################

# Delete the last unused port
delete_debug_port [get_debug_ports [GetCurrentProbe ${ilaName0}]]
# delete_debug_port [get_debug_ports [GetCurrentProbe ${ilaName1}]]

# Write the port map file
write_debug_probes -force ${PROJ_DIR}/images/debug_probes.ltx
