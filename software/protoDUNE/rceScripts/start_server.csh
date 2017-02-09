#!/bin/tcsh

echo "Killing existing process"
killall rceServer

echo "Pausing for 1 second"
sleep 1

echo "Starting new process"
setenv SWDIR /mnt/host/protoDUNE
source $SWDIR/setup_cob.csh
cd $SWDIR; nohup bin/rceServer > /dev/null 
