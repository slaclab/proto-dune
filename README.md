# Setup 

#### Pre-requirements before cloning
https://confluence.slac.stanford.edu/display/ppareg/Proto-DUNE%3A+How+to+setup+a+temporary+tunnel+for+github

#### Checkout the GIT reposotiry from github 
$ git clone --recursive git@github.com:slaclab/proto-dune

> Note: firmware/submodules/timing-board-firmware requires a CERN account for git cloning

#### Make a symbolic link to the output build directory (typically a local hardware, not network drive)
$ ln -s /path/to/build proto-dune/firmware/build

### If on the SLAC AFS network, source this script to setup the Vivado environment and licensing  
$ source proto-dune/firmware/setup_env_slac.csh

### If on the CERN AFS network, source this script to setup the Vivado environment and licensing  
TBD 

# Building the DTM firmware

#### Go to the DTM firmware target directory
$ cd proto-dune/firmware/targets/ProtoDuneDtm

#### Build the firmware in batch mode
$ make

#### Optional: Open up Vivado in GUI and see the batch mode results
$ make gui
