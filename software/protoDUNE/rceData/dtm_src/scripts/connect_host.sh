#!/bin/sh

ATCA_IP=/u1/cob/sdk_linux/bin/atca_ip
SHELF=STANDALONE
SLOT=1
IFACE=eth1

HOST="$SHELF/$SLOT/$1/$2"

echo ""
echo "---------------------------------------"
echo "Connecting to $HOST"
echo "---------------------------------------"

echo "Looking for $HOST"
IP=`$ATCA_IP $HOST --ifname $IFACE`

if [ "$IP" != "" ]
then

   echo "Found $HOST at IP $IP"

   ssh -X root@$IP
fi

