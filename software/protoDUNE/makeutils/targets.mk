# -*-Mode: makefile;-*-

# ======================================================================
#
# TARGET LOOP IMPLENTATION
# ------------------------
# This effectively implements the looping of the target. 
# Given the limited capabilities of make, this works in a somewhat
# awkward way.  Fortunately, almost all of this awkwardness is hidden
# within this file. The only user responsibility is to skip everything
# that comes after including this file if the variable CONTINUE is
# not defined.
#
# A single tag is peeled off of the command line variable TAG or TAGS
# (either is accepted) and the original Makefile is recursively called
# back with the CONTINUE variable set.  This continues until the list
# is exhausted.
# ======================================================================


HOST_CPU := $(shell uname -m)
HOST_OS  := $(shell uname -s)

ifeq ($(HOST_OS),Linux)
   HOST_OS := linux
endif

ifeq ($(HOST_CPU),armv7l)
   HOST_CPU = arm_CA9
endif


HOST := $(HOST_CPU)-$(HOST_OS)
#$(info HOST = $(HOST))

export HOST TARGET


# -----------------------------------------------------------------------
# See if target or targets (accept either) is present on the command line
# -----------------------------------------------------------------------
ifndef target
  ifdef targets
   TARGET = $(targets)
  endif
else
   TARGET = $(target)
endif

ifndef TARGET
TARGET = all
endif


ifeq ($(TARGET),all)

  # -----------------------------------------------------------------------
  # TARGET not define, default to the all targets valid for the host-cpu/os
  # ----------------------------------------------------------------------- 
  ALL_x86_64-linux  := x86_64-linux,arm_CA9-linux
  ALL_arm_CA9-linux := arm_CA9-linux

  TARGET            := $(ALL_$(HOST_CPU)-$(HOST_OS))

endif

space :=
 space += 
 comma :=,
 TARGET := $(subst $(comma),$(space),$(TARGET))
 ifneq ($(words $(TARGET)),1)

    makefile = $(firstword $(MAKEFILE_LIST))

    ifeq ($(MAKECMDGOALS),)
       # -- Nothing specified, default to all
       # -- This ensures that a per target rule gets generated
       goals := all
    else
       # -- Set recursed goal list to filter out the non-recursed goals
       goals := $(MAKECMDGOALS)
    endif


    # -------------------------------------------------------
    # -- Only trigger the per target makes for the first goal
    # -------------------------------------------------------
    first_goal = $(word 1,$(goals))
    $(first_goal):
	@$(foreach t,$(TARGET), $(MAKE) --no-print-directory -f $(makefile)\
           target=$(t) $(MAKECMDGOALS);)

     # -- Nop the remaining goals
     $(filter-out $(first_goal),$(goals)):
	@true

else
    # -- Only one target defined, just process it
    CONTINUE := 1
endif
