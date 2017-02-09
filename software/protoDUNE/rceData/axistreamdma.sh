#!/bin/sh

# Switch to C-shell
chsh -s /bin/tcsh

# remove old driver
/usr/bin/rmmod -s AxiStreamDmaModule

# Make host directory
mkdir -p /mnt/host

##########################
# ProtoDune Configurations
##########################
mount -t nfs 192.168.2.1:/u1/DUNE/cob_nfs /mnt/host
insmod /mnt/host/protoDUNE/AxiStreamDma/driverV3_4.00/AxiStreamDmaModule.ko cfgRxSize=2048,2048,327680,0 cfgRxCount=8,8,800,0 cfgTxCount=8,8,0,0 cfgRxAcp=0,0,1,0
chmod a+rw /dev/axi*
/mnt/host/protoDUNE/rceScripts/start_server.csh

#####################
# LBNE Configurations
#####################
#mount -t nfs 192.168.2.1:/u1/lbne/cob_nfs /mnt/host
#insmod /mnt/host/35ton/AxiStreamDma/driverV3_4.00/AxiStreamDmaModule.ko cfgRxSize=2048,2048,327680,0 cfgRxCount=8,8,800,0 cfgTxCount=8,8,0,0 cfgRxAcp=0,0,1,0
#chmod a+rw /dev/axi*
#/mnt/host/35ton/rceScripts/start_server.csh

############################
# Example RCE Configurations
############################
#mount -t nfs 192.168.2.1:/u1/ExampleRceProject/software/ /mnt/host
#insmod /mnt/host/Common/AxiStreamDmaModule.ko cfgRxSize=327680,0,0,0 cfgRxCount=32,0,0,0 cfgTxSize=327680,0,0,0 cfgTxCount=32,0,0,0 cfgRxAcp=0,0,0,0
#chmod a+rw /dev/axi*

