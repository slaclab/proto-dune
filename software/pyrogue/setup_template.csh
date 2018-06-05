###################
# Setup environment
###################
source /afs/slac/g/reseng/rogue/pre-release/setup_env.csh
#source /afs/slac/g/reseng/rogue/master/setup_env.csh
#source /afs/slac/g/reseng/rogue/v2.8.0/setup_env.csh

############################
# Python Package directories
############################
setenv SURF_DIR   ${PWD}/../../firmware/submodules/surf/python

###################
# Setup python path
###################
setenv PYTHONPATH ${PWD}/python:${SURF_DIR}:${PYTHONPATH}

