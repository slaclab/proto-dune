# Template setup_env.csh script. You should make a copy of this and 
# rename it to setup_env.csh after checkout

# Base directory
export BASE=${PWD}

# QT Base Directory
export QTDIR=/usr/lib64/qt4
export QWTDIR=/usr/lib64

# Root base directory
#export ROOTSYS=/afs/slac/g/reseng/root/root_5.20_x64

# Python search path
export PYTHONPATH=${BASE}/python/lib/python/:${BASE}/python/lib64/python/


# Setup path
if [ -n "${PATH}" ]; then
    export PATH=${BASE}/bin:${ROOTSYS}/bin:${QTDIR}/bin:${PATH};
else
    export PATH=${BASE}/bin:${ROOTSYS}/bin:${QTDIR}/bin;
fi    
# Setup library path
if [ -n "${LD_LIBRARY_PATH}" ]; then
    export LD_LIBRARY_PATH=${ROOTSYS}/lib:${QTDIR}/lib:${QWTDIR}/lib:${LD_LIBRARY_PATH};
else
    export LD_LIBRARY_PATH=${ROOTSYS}/lib:${QTDIR}/lib:${QWTDIR}/lib:;
fi

