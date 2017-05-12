#!/bin/sh

# Switch to C-shell
chsh -s /bin/tcsh

# remove old driver
/usr/bin/rmmod -s rcestream

# Make host directory
mkdir -p /mnt/host

# Setup the NFS mount
mount -t nfs 192.168.2.1:/u1/DUNE/cob_nfs /mnt/host

# Load the DMA driver
/mnt/host/proto-dune/software/protoDUNE/rceScripts/start_dma.csh

# Start the RCE server
/mnt/host/proto-dune/software/protoDUNE/rceScripts/start_server.csh

