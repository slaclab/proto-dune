
# Define target output
target: dcp

# Set TOP_DIR to be the base directory for which the build directory resides
export PROJ_DIR = $(abspath $(PWD))
export BASE_DIR = $(abspath $(PWD)/..)
export TOP_DIR  = $(abspath $(PWD)/../../../..)

# Set ARGV, CFLAGs, LDFLAGs
export ARGV      =
export CFLAGS    = -std=c++0x
export CXXFLAGS  = 
export LDFLAGS   =

print:
	@echo ${ARGV}
	@echo ${CFLAGS}
	@echo ${LDFLAGS}

# Use top level makefile
include $(TOP_DIR)/submodules/ruckus/system_vivado_hls.mk
