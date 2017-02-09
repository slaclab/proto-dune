AXIDMA_HOME := $(shell pwd)
AXIDMA_KVER := $(shell uname -r)
KERNEL := /afs/slac.stanford.edu/g/reseng/arm-linux/linux-xlnx

obj-m := AxiStreamDmaModule.o

all:
	make ARCH=arm CROSS_COMPILE=arm-xilinx-linux-gnueabi- -C $(KERNEL) M=$(AXIDMA_HOME) modules

clean:
	make -C $(KERNEL) M=$(AXIDMA_HOME) clean
