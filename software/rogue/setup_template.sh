
# Setup environment
source /afs/slac/g/reseng/rogue/master/setup_env.sh

# Python Package directories
export SURF_DIR=${PWD}/../../firmware/submodules/surf/python

# Setup python path
export PYTHONPATH=${SURF_DIR}:${PYTHONPATH}

