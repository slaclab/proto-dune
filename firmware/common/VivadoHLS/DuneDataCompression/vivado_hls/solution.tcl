##############################################################################
## This file is part of 'DUNE Data compression'.
## It is subject to the license terms in the LICENSE.txt file found in the 
## top-level directory of this distribution and at: 
##    https://confluence.slac.stanford.edu/display/ppareg/LICENSE.html. 
## No part of 'DUNE Data compression', including this file, 
## may be copied, modified, propagated, or distributed except according to 
## the terms contained in the LICENSE.txt file.
##############################################################################
## Set default part for Vivado 2015.3 (or ealier)
set_part {xc7z045ffg900-2}

## Set default clock (units of ns)
create_clock -period 8 -name clk

## Set default clock uncertainty (units of ns)
set_clock_uncertainty 0.1

## Configure the RTL to be one-shot encode and reset all logic on reset
config_rtl -encoding onehot -reset all
