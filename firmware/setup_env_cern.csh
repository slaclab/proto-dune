
# Setup the Xilinx software 
source /opt/Xilinx/Vivado/2016.4/settings64.csh

# License Server
if ($?LM_LICENSE_FILE) then
   setenv LM_LICENSE_FILE 2112@lxlicen01:2112@lxlicen02:2112@lxlicen03:${LM_LICENSE_FILE}
else
   setenv LM_LICENSE_FILE 2112@lxlicen01:2112@lxlicen02:2112@lxlicen03
endif
