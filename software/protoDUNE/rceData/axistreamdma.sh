#!/bin/sh

# Switch to C-shell
chsh -s /bin/tcsh

# remove old driver
/usr/bin/rmmod -s rcestream

# Make host directory
mkdir -p /mnt/host

set kernel=`uname -r`
@ length = ( `expr index $kernel "-"` - 1 )
set version=`expr substr $kernel 1 $length`
set driver_version=${version}.arm
set driver_dir=/mnt/host/russell/proto-dune/software/aes-stream-drivers/install/${driver_version}
set driver=${driver_dir}/rcestream.ko
echo kernel         = $kernel
echo length         = $length
echo version        = $version
echo driver_version = $driver_version
echo driver_dir     = $driver_dir
echo driver         = $driver

##########################
# ProtoDune Configurations
##########################
mount -t nfs 192.168.2.1:/u1/DUNE/cob_nfs /mnt/host
#insmod /mnt/host/protoDUNE/AxiStreamDma/driverV3_4.00/AxiStreamDmaModule.ko-4.0.0-xilinx-11504-g1c2b753 cfgRxSize=2048,2048,327680,0 cfgRxCount=8,8,800,0 cfgTxCount=8,8,0,0 cfgRxAcp=0,0,1,0
insmod ${driver} cfgTxCount0=8 cfgRxCount0=8 cfgSize0=327680 cfgTxCount1=8 cfgRxCount1=8 cfgSize1=4096 cfgTxCount2=8 cfgRxCount2=800 cfgSize2=327680 cfgMode2=20
#insmod ${driver} cfgTxCount0=8 cfgRxCount0=800 cfgSize0=327680 cfgMode0=2
chmod a+rw /dev/axi*
exit 
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
