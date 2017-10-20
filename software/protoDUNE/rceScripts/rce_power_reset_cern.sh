#!/bin/sh

if [ $# -ne 3 ]
then
   echo "Usage: rce_power_reset.sh slot bay element"
   exit;
fi

HOST="np04-sm-1/$1/$2/$3"

echo ""
echo "---------------------------------------"
echo "Reseting $HOST"
echo "---------------------------------------"

cob_rce_reset $HOST
echo "Reset executed"
