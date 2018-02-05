# How to build the firmware

> Setup your Xilinx Vivado:

>> If you are on the SLAC AFS network:

```$ source atlas-ftk-data-formatter/firmware/setup_slac.sh```

>> Else you will need to install Vivado and install the Xilinx Licensing

> Go to the firmware's target directory:

```$ cd atlas-ftk-data-formatter/firmware/targets/AtlasFtkDkFull```

> Option#1 to build the .bit/.mcs files: Batch mode

```$ make prom```

> Option#2 to build the .bit/.mcs files: GUI mode

```$ make gui```

> The ruckus build system will automatically dumped the .bit/.mcs files into atlas-ftk-data-formatter/firmware/targets/AtlasFtkDkFull/images/ directory 

# How to build the rogue software for reprogramming the FPGA's PROM

> Go to the software base directory

```cd atlas-ftk-data-formatter/software/```