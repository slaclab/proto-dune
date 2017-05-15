#!/bin/csh

set kernel=`uname -r`
@ length = ( `expr index $kernel "-"` - 1 )
set version=`expr substr $kernel 1 $length`
set driver_version=${version}.arm
set driver_dir=/mnt/host/aes-stream-drivers/install/${driver_version}
set driver=${driver_dir}/rcestream.ko

echo kernel         = $kernel
echo length         = $length
echo version        = $version
echo driver_version = $driver_version
echo driver_dir     = $driver_dir
echo driver         = $driver
 
insmod ${driver} cfgTxCount0=8 cfgRxCount0=8 cfgSize0=327680 cfgTxCount1=8 cfgRxCount1=8 cfgSize1=4096 cfgTxCount2=8 cfgRxCount2=800 cfgSize2=327680 cfgMode2=20
chmod a+rw /dev/axi*

