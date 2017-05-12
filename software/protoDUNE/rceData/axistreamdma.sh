#!/bin/sh

# Switch to C-shell
chsh -s /bin/tcsh

# remove old driver
/usr/bin/rmmod -s rcestream

# Make host directory
mkdir -p /mnt/host

#set kernel=`uname -r`
#@ length = ( `expr index $kernel "-"` - 1 )
#set version=`expr substr $kernel 1 $length`
#set driver_version=${version}.arm
#set driver_dir=/mnt/host/proto-dune/software/aes-stream-drivers/install/${driver_version}
#set driver=${driver_dir}/rcestream.ko
#echo kernel         = $kernel
#echo length         = $length
#echo version        = $version
#echo driver_version = $driver_version
#echo driver_dir     = $driver_dir
#echo driver         = $driver

##########################
# ProtoDune Configurations
##########################
mount -t nfs 192.168.2.1:/u1/DUNE/cob_nfs /mnt/host
#insmod ${driver} cfgTxCount0=8 cfgRxCount0=8 cfgSize0=327680 cfgTxCount1=8 cfgRxCount1=8 cfgSize1=4096 cfgTxCount2=8 cfgRxCount2=800 cfgSize2=327680 cfgMode2=20
insmod /mnt/host/proto-dune/software/aes-stream-drivers/install/4.0.0.arm/rcestream.ko cfgTxCount0=8 cfgRxCount0=8 cfgSize0=327680 cfgTxCount1=8 cfgRxCount1=8 cfgSize1=4096 cfgTxCount2=8 cfgRxCount2=800 cfgSize2=327680 cfgMode2=20
chmod a+rw /dev/axi*
#exit 
/mnt/host/proto-dune/software/protoDUNE/rceScripts/start_server.csh

