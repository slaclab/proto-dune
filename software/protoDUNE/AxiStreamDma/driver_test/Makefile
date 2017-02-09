AXIDMA_HOME := $(shell pwd)
AXIDMA_KVER := $(shell uname -r)

obj-m := AxiStreamDmaModule.o

all:
	make ARCH=arm CROSS_COMPILE=arm-xilinx-linux-gnueabi- -C /u1/xilinx/zynq/linux-xlnx M=$(AXIDMA_HOME) modules

clean:
	make -C /u1/xilinx/zynq/linux-xlnx M=$(AXIDMA_HOME) clean
