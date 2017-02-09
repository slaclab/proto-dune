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

      ssh root@$IP 'mkdir .ssh; touch .ssh/authorized_keys'
      cat $HOME/.ssh/id_rsa.pub | `ssh root@$IP 'cat >> .ssh/authorized_keys'`
      ssh root@$IP 'echo nameserver 134.79.110.9 >> /etc/resolv.conf'
      ssh root@$IP 'echo domain     slac.stanford.edu >> /etc/resolv/conf'
      ssh root@$IP 'echo search     slac.stanford.edu >> /etc/resolv/conf'
      ssh root@$IP 'pacman -Sy --noconfirm'
      ssh root@$IP 'pacman -S nfs-utils libxml2 tcsh mariadb python'
      ssh root@$IP 'mkdir /mnt/host'
      scp mount.sh root@$IP:
   fi
done

