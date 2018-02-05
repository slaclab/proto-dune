
# Setup environment
source /afs/slac/g/reseng/rogue/master/setup_env.csh

# Python Package directories
setenv SURF_DIR   ${PWD}/../../firmware/submodules/surf/python

# Setup python path
setenv PYTHONPATH ${SURF_DIR}:${PYTHONPATH}

