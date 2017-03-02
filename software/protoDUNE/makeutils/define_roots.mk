# -*-Mode: makefile;-*-

PRJROOT     := $(shell cd $(dir $(word 1, $(MAKEFILE_LIST))); pwd)
BLDROOT     := $(if $(DUNEBUILD),$(DUNEBUILD),$(PRJROOT)/build/$(TARGET))
LIBDIR      := $(BLDROOT)/lib
BINDIR      := $(BLDROOT)/bin
DEPROOT     := $(BLDROOT)/dep
OBJROOT     := $(BLDROOT)/obj
