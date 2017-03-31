##############################################################################
## This file is part of 'DUNE Data compression'.
## It is subject to the license terms in the LICENSE.txt file found in the 
## top-level directory of this distribution and at: 
##    https://confluence.slac.stanford.edu/display/ppareg/LICENSE.html. 
## No part of 'DUNE Data compression', including this file, 
## may be copied, modified, propagated, or distributed except according to 
## the terms contained in the LICENSE.txt file.
##############################################################################
## Get variables and Custom Procedures
set RUCKUS_DIR $::env(RUCKUS_DIR)
source ${RUCKUS_DIR}/vivado_hls_env_var.tcl
source ${RUCKUS_DIR}/vivado_hls_proc.tcl 

## Set the top level module
set_top DuneDataCompressionCore

## Add source code
add_files ${PROJ_DIR}/src/DuneDataCompressionCore.cpp -cflags ${CFLAGS}

## Add testbed files
add_files -tb ${PROJ_DIR}/src/DuneDataCompressionCore_test.cpp -cflags ${CFLAGS}
