AXIDMA_HOME := $(shell pwd)
AXIDMA_KVER := $(shell uname -r)
KERNEL := /u1/xilinx/zynq/linux-xlnx_3.9/linux-xlnx

obj-m := AxiStreamDmaModule.o

all:
	make ARCH=arm CROSS_COMPILE=arm-xilinx-linux-gnueabi- -C $(KERNEL) M=$(AXIDMA_HOME) modules

clean:
	make -C $(KERNEL) M=$(AXIDMA_HOME) clean
