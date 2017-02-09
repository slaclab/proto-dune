# Variables
MACHINE := $(shell uname -m)

# RCE Build
ifeq ($(MACHINE), armv7l)
   PYTHON=python2
	BASE=./arm
else
   PYTHON=python
	BASE=.
endif

all:
	${PYTHON} setup.py build   
	${PYTHON} setup.py install --home=${BASE}

clean:
	rm -rf build
	rm -rf ${BASE}/lib
