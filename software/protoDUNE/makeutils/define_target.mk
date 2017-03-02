# -*-Mode: makefile;-*-

# ======================================================================
#
# Author  : russell@slac.stanford.edu
#
# Facility: makeutils
#
# Date created: yyyy/mm/dd
#
# Credits: SLAC
#
#
#                               Copyright 2013
#                                    by
#                       The Board of Trustees of the
#                    Leland Stanford Junior University.
#                           All rights reserved.
#
#  PURPOSE
#  -------
#  Defines the host and target cpu and operating system. 
#  The host cpu and os are gotten from uname, wherea the target cpu and
#  os are specified by the variable TARGET.  If TARGET is not specified
#  it is defaulted to the host
# ======================================================================




# ----------------------------------------------------------------------
# HISTORY
# 
#       DATE WHO  WHAT
# ---------- ---  ------------------------------------------------------
# 2016.09.27 jjr  Spiffed up error message for invalid target
# ----------------------------------------------------------------------




# ---------------------------------------------
# Define and verify the host/target combination
# ---------------------------------------------
VALID_TARGET_SET := x86_32-linux x86_64-linux arm_CA9-linux arm_CA9-rtems
ifdef TARGET


   # ----------------------------
   # Check if have a valid target
   # ----------------------------
  ifeq ($(filter $(TARGET),$(VALID_TARGET_SET)),) 
    define newline


    endef


    define margin
      
    endef

    $(info  $(newline)Error: Invalid target$(newline) \
            $(margin)TARGET=<$(TARGET)> is not one of $(VALID_TARGET_SET)$(newline))
    $(error Aborting)
  endif

  # --- Change fields of the target to whitespace delimited list
  space :=
  space += 
  tmp = $(subst -,$(space),$(TARGET))
  TARGET_CPU := $(word 1,$(tmp))
  TARGET_OS  := $(word 2,$(tmp))

else

  # ----------------------------------------------
  # TARGET not define, default to the host machine
  # ----------------------------------------------
  TARGET = $(HOST_CPU)-$(HOST_OS)
  TARGET_CPU = $(HOST_CPU)
  TARGET_OS  = $(HOST_OS)

endif

