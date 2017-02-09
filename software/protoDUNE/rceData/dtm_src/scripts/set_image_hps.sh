#!/bin/sh

ATCA_IP=/u1/cob/sdk_linux/bin/atca_ip
SHELF=STANDALONE
SLOT=1
IFACE=eth1

for num in $SHELF/$SLOT/0/0 $SHELF/$SLOT/0/2 $SHELF/$SLOT/1/0 $SHELF/$SLOT/1/2 $SHELF/$SLOT/2/0 $SHELF/$SLOT/2/2 $SHELF/$SLOT/3/0
do

   echo ""
   echo "---------------------------------------"
   echo "Connecting to $num"
   echo "---------------------------------------"

   echo "Looking for $num"
   IP=`$ATCA_IP $num --ifname $IFACE`

   if [ "$IP" != "" ]
   then

      echo "Found $num at IP $IP"

      scp $1 root@$IP:/mnt/boot/fpga.bit
      ssh root@$IP 'sync; ls -l /mnt/boot/fpga.bit'
   fi
done

for num in $SHELF/$SLOT/3/2
do

   echo ""
   echo "---------------------------------------"
   echo "Connecting to $num"
   echo "---------------------------------------"

   echo "Looking for $num"
   IP=`$ATCA_IP $num --ifname $IFACE`

   if [ "$IP" != "" ]
   then

      echo "Found $num at IP $IP"

      scp $2 root@$IP:/mnt/boot/fpga.bit
      ssh root@$IP 'sync; ls -l /mnt/boot/fpga.bit'
   fi
done

