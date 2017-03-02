# -*-Mode: Makefile;-*-

# ======================================================================
#
# Abstract: Defines the tool set needed for make
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
# ======================================================================




# ----------------------------------------------------------------------
# HISTORY
# 
#       DATE WHO  WHAT
# ---------- ---  ------------------------------------------------------
# 2016.11.14 jjr  Removed XML defines
# 2016.09.27 jjr  Added checked for cross-development tools
# ----------------------------------------------------------------------



ifeq ($(HOST_CPU),arm_CA9)

   # -----------------------------------
   # Building on the embedded target
   # -----------------------------------
   ifneq ($(TARGET_CPU),arm_CA9)

        # -----------------------------------
        # Target can only be the host machine
        # -----------------------------------
        $(error "Cannot cross build for on an <$(HOST)> for target <$(TARGET)>")

   endif

   #---------------------------------------------------------------
   # Tools for the embedded environment on the embedded environment
   #---------------------------------------------------------------   
   CC  := g++
   CXX := g++
   LD  := ld
   AS  := as

else 


   ifeq ($(TARGET_CPU), arm_CA9)

      # -----------------------
      # Cross development tools
      # -----------------------
      CC  := arm-linux-gnueabihf-gcc
      CXX := arm-linux-gnueabihf-g++
      LD  := arm-linux-gnueabihf-g++
      AS  := arm-linux-gnueabihf-as

      # ------------------------------------------------------------
      # Check for access to the cross development tools
      # This only needs be done once
      # It is assumed that they don't go away in between
      # ------------------------------------------------------------
      ifeq ($(cc_path),)
        cc_path := $(shell which $(CC) 2>/dev/null)
        ifeq ($(cc_path),)

              define newline


              endef

              define margin
      
              endef

              $(info $(newline)Error: No path to the cross development tools$(newline) \
                $(margin)CC : $(CC)$(newline)  \
                $(margin)CXX: $(CXX)$(newline) \
                $(margin)LD : $(LD)$(newline)  \
                $(margin)AS : $(AS)$(newline))
             $(error $(newline))
        endif

        export cc_path

      endif

   else

      # ------------
      # Native tools
      # ------------
      CC  ?= g++
      CXX ?= g++
      LD  ?= $(CXX)
      AS  ?= as

   endif

endif



ifeq ($(TARGET_CPU), arm_CA9)

    arm_stdflags  := -Wno-psabi                     \
                     -fno-zero-initialized-in-bss   \
                     -march=armv7-a                 \
                     -mtune=cortex-a9               \
                     -mcpu=cortex-a9                \
                     -mfpu=neon                     \
                     -mfloat-abi=hard 

   CPPFLAGS_DEF := -DARM -D __ARM_PCS_VFP
   CFLAGS_DEF   := -Wall -Werror $(arm_stdflags) -g -std=gnu99
   CXXFLAGS_DEF := -Wall -Werror $(arm_stdflags) -g -std=c++0x


   LDFLAGS     := 
   EXT_TARS    := 


else

   ifeq ($(TARGET_CPU),x86_64)

      # Server Build
      CPPFLAGS_DEF := 
      CFLAGS_DEF   := -Wall -Werror -g -std=gnu99
      CXXFLAGS_DEF := -Wall -Werror -g -std=c++0x

      LDFLAGS      := 
      EXT_TARS     := gui 
      CLN          := server_clean

  else

     ifeq ($(TARGET_CPU),x86_32)
       $(error ERROR: Building for x86_32 targets not yet supported)
     else
       $(error ERROR: Unknown target = $(TARGET))
     endif


  endif

endif



# -------------------------------------------------------------
#
# Define debugging symbols. These print out the rules that are
# being executed.
#
# Example
#   make -cmp=1     -> Print out all the compile commands
#   make -cmp_cxx=1 -> Print out only c++ compile commands 
# -------------------------------------------------------------
ifneq ($(MAKELEVEL), 0)
   DBG_DEP_ASM  := $(if $(strip $(dep_asm)  $(dep)  $(dbg)),,@)
   DBG_DEP_C    := $(if $(strip $(dep_c)    $(dep)  $(dbg)),,@)
   DBG_DEP_CXX  := $(if $(strip $(dep_cxx)  $(dep)  $(dbg)),,@)
   DBG_CMP_ASM  := $(if $(strip $(cmp_asm)  $(cmp)  $(dbg)),,@)
   DBG_CMP_C    := $(if $(strip $(cmp_c)    $(cmp)  $(dbg)),,@)
   DBG_CMP_CXX  := $(if $(strip $(cmp_cxx)  $(cmp)  $(dbg)),,@)
   DBG_LINK_RO  := $(if $(strip $(link_ro)  $(link) $(dbg)),,@)
   DBG_LINK_SO  := $(if $(strip $(link_so)  $(link) $(dbg)),,@)
   DBG_LINK_EXE := $(if $(strip $(link_exe) $(link) $(dbg)),,@)
endif
# -------------------------------------------------------------
