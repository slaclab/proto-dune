
# Setup the Xilinx software 
set KERNAL_TYPE = `uname -m`
if ( $KERNAL_TYPE == "x86_64" ) then
	printf  "Using 64-bit Xilinx\n"
	source /afs/slac/g/reseng/xilinx/vivado_2016.4/Vivado/2016.4/settings64.csh
else
	printf  "Firmware can only be compiled on 64-bit OS\n"
endif

# Setup the VCS software
source  /afs/slac.stanford.edu/g/reseng/synopsys/vcs-mx/M-2017.03-1/settings.csh
source /afs/slac.stanford.edu/g/reseng/synopsys/ns/G-2012.06/settings.csh
source /afs/slac.stanford.edu/g/reseng/synopsys/CosmosScope/D-2010.03/settings.csh
