AXIDMA_HOME := $(shell pwd)
AXIDMA_KVER := $(shell uname -r)
#KERNEL := /afs/slac.stanford.edu/g/cci/volumes/vol1/xilinx/linux-xlnx

# ---------------------------
# -- Selects the 4.0.0 kernel
# ---------------------------
KERNEL := /afs/slac.stanford.edu/g/cci/volumes/vol1/xilinx/linux-xlnx-v2015.2.03

obj-m := AxiStreamDmaModule.o

all:
	make ARCH=arm CROSS_COMPILE=arm-xilinx-linux-gnueabi- -C $(KERNEL) M=$(AXIDMA_HOME) modules

clean:
	make -C $(KERNEL) M=$(AXIDMA_HOME) clean
