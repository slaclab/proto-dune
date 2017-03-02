# -*-Mode: makefile;-*-

# ======================================================================
#
# PURPOSE
# -------
# Just a convenience file to include the boilerplate used at the
# beginning of a package's makefile
#
# ======================================================================


# ------------------------------------------
#    This effectively loops over the command
#    line specified tags, i.e. targets=x,y,z
# ------------------------------------------
include $(MAKEUTILS_DIR)/targets.mk
# ------------------------------------------


ifdef CONTINUE
  # --------------------------------
  #    Now handling one specific tag
  # --------------------------------

  include $(MAKEUTILS_DIR)/define_target.mk
  include $(MAKEUTILS_DIR)/define_tools.mk
  include $(MAKEUTILS_DIR)/define_roots.mk
endif
