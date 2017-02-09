#!/bin/sh

ATCA_IP=/u1/cob/sdk_linux/bin/atca_ip
SHELF=STANDALONE
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

      echo "Mouting Host Filesystem On $num"
      ssh root@$IP 'mount -t nfs -o nolock ppa-pc87444.slac.stanford.edu:/u1/cob /mnt/host'

      echo "Killing Existing Process On $num"
      ssh root@$IP 'killall pgp_slave'
      sleep 1;


      echo "Reset PGP Slave On $num"
      ssh root@$IP 'cd /mnt/host/dtm_src; nohup bin/pgp_reset'
   fi
done

