#!/bin/sh

ATCA_IP=/u1/cob/sdk_linux/bin/atca_ip
SHELF=STANDALONE
SLOT=1
IFACE=eth1

DTM="$SHELF/$SLOT/4/0"

echo ""
echo "---------------------------------------"
echo "Connecting to $DTM"
echo "---------------------------------------"

echo "Looking for $DTM"
IP=`$ATCA_IP $DTM --ifname $IFACE`

if [ "$IP" != "" ]
then

   echo "Found $DTM at IP $IP"

   scp $1 root@$IP:/mnt/boot/fpga.bit
   ssh root@$IP 'sync; ls -l /mnt/boot/fpga.bit'
fi

