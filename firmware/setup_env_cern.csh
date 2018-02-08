
# Setup the Xilinx software 
set KERNAL_TYPE = `uname -m`
if ( $KERNAL_TYPE == "x86_64" ) then
	#printf  "Using 64-bit Xilinx\n"
   # TBD#
   ######
	# source /afs/slac/g/reseng/xilinx/vivado_2016.4/Vivado/2016.4/settings64.csh
else
	printf  "Firmware can only be compiled on 64-bit OS\n"
endif


setenv XILINXD_LICENSE_FILE "2112@lxlicen01,2112@lxlicen02,2112@lxlicen03"
