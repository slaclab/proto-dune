
# Setup the Xilinx software 
source /opt/Xilinx/Vivado/2016.4/settings64.sh

# License Server
if [ -n $LM_LICENSE_FILE ]
then
   LM_LICENSE_FILE=2112@lxlicen01:2112@lxlicen02:2112@lxlicen03:${LM_LICENSE_FILE}
else
   LM_LICENSE_FILE=2112@lxlicen01:2112@lxlicen02:2112@lxlicen03
fi
export LM_LICENSE_FILE
