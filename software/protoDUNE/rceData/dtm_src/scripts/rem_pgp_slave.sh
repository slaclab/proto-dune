#!/bin/sh

ATCA_IP=/afs/slac/u/ey/rherbst/projects/rce/sdk/linux/bin/atca_ip
SHELF=ritter
SLOT=1
IFACE=eth1

for num in $SHELF/$SLOT/4/0 $SHELF/$SLOT/0/0 $SHELF/$SLOT/0/2 $SHELF/$SLOT/1/0 $SHELF/$SLOT/1/2 $SHELF/$SLOT/2/0 $SHELF/$SLOT/2/2 $SHELF/$SLOT/3/0 $SHELF/$SLOT/3/2
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

      echo "Killing Existing Process On $num"
      ssh root@$IP 'killall pgp_slave'
      sleep 1;

      echo "Running PGP Slave On $num"
      ssh root@$IP "cd /mnt/host/dtm_src; nohup bin/pgp_slave 192.168.1.255 1 > /dev/null &" &
   fi
done

