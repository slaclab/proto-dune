# -*-Mode: makefile;-*-


# -*-Mode: makefile;-*-

#
#  THE PROBLEM
#  -----------
#  The original makefile had problems with the -include *.d directive.
#  This directive implicitly caused all the *.d targets to be updated.
#  Normally, at worst, for example, in the case of executing a clean
#  target, this is just a waste of time. However in the case that the
#  the .d cannot be remade, say if you have pulled a new version from
#  the repository and the new version no longer includes one of the 
#  source files, the make file will abort execution because it cannot 
#  remake the .d.  One could go in and by hand do the clean, but this
#  sort of defeats the purpose of having a clean target. It should hide
#  these sort of details.
#
#  THE STANDARD SOLUTION
#  ---------------------
#  As one might expect, this is not the first time this problem has been
#  encountered and there is a recommended solution which uses the make
#  variable MAKECMDGOALS. This variable gives the list of goals as
#  preented on the command line. Here is the suggested solution
#
#  ifneq ($(MAKECMDGOALS), clean)
#   -include *.d
#  endif
#
#  This works like a champ if on types
#
#  $ gmake clean
#
#  The -include is not executed, thus avoiding the problem. However if
#  one updates a package to a new release, one of the more common make
#  commands might be
#
#  $ gmake clean all
#
#  Cleaning up the old stuff, then rebuilding everything. In this case
#  MAKECMDGOALS is now "clean all" and this does not match "clean", so
#  the protection afforded by the if is lost.
#
#  THE SOLUTION
#  ------------
#  The chosen solution is to recurse the make iff the target is not
#  clean and, only in the recursed make do the -include *.d. The clean
#  target was changed from deleting each individual file in the 
#  dependency directory to just deleting the contents of the dependency
#  directory. Personally, I think this is a cleaner solution, lessening
#  the effects of the files in a new release being a different set than
#  the previous release.
#
#  The implementation uses the internal MAKELEVEL variable to distinguish
#  whether one is in the top level or recursed make. This tactic means
#  only one makefile is needed. Thus this makefile is broken into three
#  sections
#        1. The stuff that is common to the top level execution
#           and the recursed execution
#        2. The stuff specific to the top level execution
#        3. The stuff specific to the recursed executions
#
# OTHER CHANGES
# -------------
# Have moved the directory (previously called WORKDIR) to BINDIR. This
# keeps the intermediate build products with the final build product.
# These should not be hung off the source tree.
#
# Moved the WORKDIR make to the original spot off the source tree. This is
# the way the offline wishes to work.
#
# Placed the .so in /lib directory, not the /bin directory as requested by
# the offline.
#
#     DATE   WHO WHAT
# ---------- --- ----------------------------------------------------------- 
# 2016.02.19 jjr Renamed PKGROOT -> PRJROOT, it is more appropriate
#                Directed .d and .o's into BLDROOT/<pkg>/*.d and *.o
#                This required reworking the update_dependency procedures
#                to appropriately name the .o files in the .d files
# 2016.01.16 jjr Cloned from EXOBinary package
#
#
# SECTION: Common
# ===============
# 


# ======================================================================
#
# SECTION: Common to all levels
#
# ----------------------------------------------------------------------


# ----------------------
# Disable implicit rules
# ---------------------
.SUFFIXES:
#----------------------



# ----------------------------------------------------------------------
#
# VERSION CHECK
# -------------
#
# The driving requirement is the 'eval' function which was not supported
# until version 3.81 of make.  
#
# NOTE:
# Many examples on the WEB use an '=' when using 'define'. This is buggy
# in 3.81. It was fixed in 3.82.  Since using 'define' without the '='
# means the same thing, this file avoids it usage, sacrificing a bit of
# clearity to use an older version of make.
# ----------------------------------------------------------------------
MIN_VERSION := 3.81
VERSION_OK  := $(filter $(MIN_VERSION), \
               $(firstword $(sort $(MAKE_VERSION) $(MIN_VERSION))))
# -----------------------------------------------------------------



# -----------------------------------------------------------
# VERSION_OK will be blank if this version of make is too old
# otherwise  will be MIN_VERSION is okay
# -----------------------------------------------------------
ifeq ($(VERSION_OK),)
   space :=
   space += 
   spc   := $(space)$(space)$(space)$(space)$(space)$(space)
   $(info ERROR: Make version $(MAKE_VERSION) is insufficent)
   $(info $(spc) Minimum required version is $(MIN_VERSION))
   $(error Aborting..)
endif
# ----------------------------------------------------------------------




# ================================================================
# BEGIN: Convenience procedures
#
# The following procedures encapsulate functionality that is used
# in many places.
#
# -----------------------------------------------------------------
#
# derive
# ------
# Procedure to derive a related file aname
#
# PARAMETERS:
#         $1: the prefix to add
#         $2: the starting file name
#         $3: the suffix to add
#
#
# The directory is and file suffix are stripped for the starting
# file name and replaced by the specified prefix and suffix.
#
# Example
# -------
# To derive the corresponding object file from src/myfile.cpp
#
# $(call derive obj,src/myfile.cpp,.o) -> obj/myfile.o
#
# Note:
#  The directory slash is provided (don't need obj/), but
#  The . separator for the suffix (need .o, not o) is not
#
#  Seems a little inconsistent, but it reads somewhat clearer
#
# -----------------------------------------------------------------
derive = $(addprefix $(1)/,              \
         $(addsuffix $(3),               \
         $(basename $(notdir $(2)))))
# -----------------------------------------------------------------


# -----------------------------------------------------------------
#
# Create all the dependency and object directories. These 
# directories are creaeted on a per constituent basis
#
# The sort is used to weed out duplicates.  There may be cheaper
# ways to do this, since the sort is unnecessary; really just
# using it for the side effect of removing duplicates.
#
# -----------------------------------------------------------------
CONSTITUENTS := $(RELOCATABLES) $(EXECUTABLES)
DEPDIRS      := $(sort $(foreach c,$(CONSTITUENTS),$($c_DEPDIR)))
OBJDIRS      := $(sort $(foreach c,$(CONSTITUENTS),$($c_OBJDIR)))
# -----------------------------------------------------------------



# -----------------------------------------------------------------
#
# objfile
# -------
# Derives the corresponding object file from a dependency file and
# is used when constructing the rules for a dependency file.
#
# PARAMETERS:
#         $1: The dependency file name
#
# Example
# $(call objfile,bld/pkg/dep/myfile.d) -> bld/obj/myfile.o
#
# -----------------------------------------------------------------
objfile = $(patsubst %.d,%.o, $(subst /dep/,/obj/,$(1)))
# -----------------------------------------------------------------



# -----------------------------------------------------------------
#
# pkgfile
# -------
# Procedure to strip away the project root.  This is strictly for
# display purposes, cutting down on excessive verbiage.  Basically
# for pretty printing.
#
# PARAMETERS:
#         $1: The file to strip the project root string from
#
# Example:
# --------
# If PRJROOT = /u/ey/nobel
# Then
#
# $(call pkgfile, /u/ey/nobel/myfile.cpp) -> myfile.cpp
#
# Note:
#  The directory slash is provided.  If should not be part of
#  PRJROOT.  Because of the way PRJROOT is likely derived, this 
#  is the most natural thing thing to do.
#
# -----------------------------------------------------------------
pkgfile = $(subst $(PRJROOT)/,,$1)
# -----------------------------------------------------------------



# -----------------------------------------------------------------
#
# binfile
# -------
# Procedure to strip away the build root.  This is strictly for
# display purposes, cutting down on excessive verbiage.  Basically
# for pretty printing. It serves the same purpose for binary files
# as pkgfile does for package source files.
#
# PARAMETER:
#        $1: The file to strip the build root string from
#
# Example:
# If BLDROOT = /u/ey/nobel/build
# $(call binfile,/u/ey/nobel/build/bin/myfile.exe) -> bin/myfile.exe 
#
# Note:
#  The directory slash is provided.  If should not be part of
#  BLDROOT.  Because of the way BLDROOT is likely derived, this 
#  is the most natural thing thing to do.
#
# -----------------------------------------------------------------
binfile = $(subst $(BLDROOT)/,"",$(1))
# -----------------------------------------------------------------



# -----------------------------------------------------------------
#
# rofile
# -------
# Procedure to strip away the build root for .ro files.  This is
# strictly fordisplay purposes, cutting down on excessive verbiage.
# Basicallyfor pretty printing. It serves the same purpose for 
# relocatable files as pkgfile does for package source files.
#
# PARAMETER:
#        $1: The file to strip the build root string from
#
# Example:
# If BLDROOT = /u/ey/nobel/build
# $(call rofile,/u/ey/nobel/build/obj/mypkg.ro) -> obj/mypkg.exe 
#
# Note:
#  The directory slash is provided.  If should not be part of
#  BLDROOT.  Because of the way BLDROOT is likely derived, this 
#  is the most natural thing thing to do.
#
# -----------------------------------------------------------------
rofile  = $(subst $(BLDROOT)/,"",$(1))
# -----------------------------------------------------------------
#
# END: Convenience procedures
# ================================================================



# ======================================================================
# SECTION: Definition Phase, i.e. not BUILDING
# --------------------------------------------
#
# These are definitions needed to implement the targets (currently
# clean and help that get executed during non BUILD phase. Some of these
# definitions are also needed in the BUILD phase 1, so they are exported.
#
# Only those defintions needed for non-BUILD phase targets should be 
# defined here.
#
# ----------------------------------------------------------------------
ifndef BUILD

.PHONY: all clean help

.DEFAULT_GOAL := all

space  := 
space  +=
comma  := ,
extract_1 = $(patsubst $1:%,%,$(filter-out $(patsubst $1:%,,$2),$2))
extract   = $(subst $(comma),$(space),$(call extract_1,all,$2) $(call extract_1,$1,$2))


# --------------------------------------------
# Extract the name of the original 
# make file so that it can be re-invoked
# ---------------------------------------
makefile      = $(firstword $(MAKEFILE_LIST))
# --------------------------------------------

# ------------------------------------------------------------
# Taylor the options for phase (BUILD or non-BUILD)
MAKEOPTIONS  = --no-print-directory
# ------------------------------------------------------------


# -------------------------------------------------------------------
# Taylor the differed goals
# -------------------------
ifeq ($(MAKECMDGOALS),)
   # -- Nothing specified, default to all
   deferred_goals := all
else
   # -- Set recursed goal list to filter out the non-recursed goals
   deferred_goals := $(filter-out clean help,$(MAKECMDGOALS))
endif
# -------------------------------------------------------------------



# ---------------------------------------------------------------------------
# Only trigger the recursed make for the first goal
# -------------------------------------------------
first_goal := $(word 1,$(deferred_goals))
$(first_goal): 
	@$(MAKE) -r -f $(makefile) $(MAKEOPTIONS) BUILD=1 $(deferred_goals)
# ---------------------------------------------------------------------------




# ------------------------------------------------------------------------
# -- Have to neuter the remaining goals to prevent noisy print out
# -- I've tried 
#      - Not putting in anything  -> generates target is up-to-date
#      - Using a empty recipe (;) -> generates Nothing to be done
#
# -- Settled on just returning 'true' as the recipe.
#      - This is crude since it demands a shell to be spawned.
#        but taking this as a better alternative than a misleading message
# ------------------------------------------------------------------------
$(filter-out $(first_goal),$(deferred_goals)):
	@true
# ------------------------------------------------------------------------



# ------------------------------------------------------------
# TARGET: help
#
# Provides rudimentary explanation of the various targets that
# are standardly available
# ------------------------------------------------------------
help:
	@echo -e                                                              \
        "Targets are"                                                         \
      "\n"                                                                    \
      "\n    all........................ Build all targets"                   \
      "\n    clean...................... Clean all targets"                   \
      "\n"                                                                    \
      "\n    make_directories........... Creates the output directories"      \
      "\n    copy_includes...............Copies the public include files"     \
      "\n"                                                                    \
      "\n    help....................... Print all targets"                   \
      "\n    print_tools................ Print the compiler/load tools used"  \
      "\n    print_flags................ Print the various flags"             \
      "\n    print_dependencyfiles...... Print the list of dependency files"  \
      "\n    print_directories.......... Print the input & output directories"\
      "\n"

# ------------------------------------------------------------
# TARGET: clean
#
# Cleans all the product files by deleting the contents of their
# directories. The strategy of cleaning only files that are 
# produced by this session of make leads to stale files. Imaging
# a file x.c that produces x.o.  If x.c is deemed unnecessart
# and eliminated. x.o may linger indefinitely.  Some make
# conveniences refer to what is being done here as really_clean
# but I don't see any advantage of having a clean and a 
# really clean.
# ------------------------------------------------------------
clean:
	@echo "Start clean of project ........... $(PRJNAME) for $(TARGET)"
	@echo "    Clean files in     dep   dir.. $(DEPROOT)"
	@rm -rf $(DEPROOT)
	@echo "    Clean files in     obj   dir.. $(OBJROOT)"
	@rm -rf $(OBJROOT)
	@echo "    Clean products in lib    dir.. $(LIBDIR)"
	@rm -rf $(LIBDIR)
	@echo "    Clean products in binary dir.. $(BINDIR)"
	@rm -rf $(BINDIR)
	@$(MAKE) -r -f $(makefile) $(MAKEOPTIONS) BUILD=1 server_clean
	@echo "End   clean of project ........... $(PRJNAME)"



# ======================================================================
else
# ======================================================================



# ----------------------------------------------------------------------
# SECTION: BUILD PHASE
# --------------------
#
# This is where the real action happens.  Much of this is 
# defining various lists of files and target and the canned
# recipes that do the actual building for the products.
#


# ================================================================
# BEGIN: Rule templates
#
# These templates implement the make rules for creating targets
# from their sources. The all take 1 parameter, the constituent
# stem's name.  This name acts like the object name in C++. It
# is concatenated with a number of other strings, which function
# like field member names, to define the variables.
#
# Example:
# If one is building the dependencies for the constituent FOO, 
# the following strings would be accessed
#
#  FOO_SRCFILES  :  The list of FOO's source files
#  FOO_CCDEPFILES:  The list of dependency files
#  FOO_DEPDIR    :  The dependency directory (where they live)
#  FOO_INCPATHS  :  Any include paths needed to compile the
#                   source files
#
# This method limits the specification granularity to a constituent,
# that is the same flags and list of inclueds is used for each
# source file.  This should probably be improved.
#
# -----------------------------------------------------------------



# ----------------------------------------------------------------------
# CANNED RECIPE: _FLAGS
# ---------------------
# Captures the syntax of modular versions of the various flags passed
# to tools, e.g. the preprocessor, compilers, linker. The flags are
# composed of 3 pieces capturing 4 scopes
#
# A constituent file scope    <constituent.<file>_CXXFLAGS
# A constituent      scope    <constituent>_CXXFLAGS
# A file             scope    <file>__CXXFLAGS
# A package wide     scope    CXXFLAGS
#
# They are added in this order to the command line. For the most part
# this is done as the most restrictive first.  One could argue whether
# constituent which should come first 
#   -> <file>__flags or <constituent>_flags
#
# ----------------------------------------------------------------------
define _FLAGS
 $($2.$(*F)_$1) $($2_$1) $($(*F)__$1) $($1)
endef
# ----------------------------------------------------------------------



# ----------------------------------------------------------------------
# CANNED RECIPE: _CPPFLAGS
# -------------------------
# Captures the full preprocessor flags.  This consists of the user
# defined CPPFLAGS + INCPATHS.  The INCPATHS could be bundled with the
# CPPFLAGS, but it seemed convenient to allow the include directories 
# not to include the -I.  Of course the user is free to put -I <inc-dir>
# in the CPPFLAGS.  The INCPATHS syntax is encouraged, but can also be
# be considered just a convenience.
# ----------------------------------------------------------------------
define _CPPFLAGS
-I $($1_SRCDIR)/ \
 $(foreach i,$(call _FLAGS,INCPATHS,$1),$(if $i,-I $i,)) \
 $(call _FLAGS,CPPFLAGS,$1)
endef
# ----------------------------------------------------------------------



# -----------------------------------------------------------------
#
# CDEP_template
# -------------
# Rule template to create the dependency files for .cpp source files
#
# PARAMETERS:
#         $1: The constituent stem
#
# -----------------------------------------------------------------
define CDEP_template
$($1_CDEPFILES) : | $($1_DEPDIR)
$($1_CDEPFILES) :   $($1_DEPDIR)/%.d : $($1_SRCDIR)/%.$(CSuf)
	@echo "    Update dependencies........ $$(call pkgfile,$$<)"
	$(DBG_DEP_C)$(CC) -MM  -MT $$@ -MT $$(call objfile,$$@)  \
                                    $$(call _CPPFLAGS,$1)        \
                                    $$(call _FLAGS,CFLAGS,$1)    \
                                    $$< > $$@
endef
# -----------------------------------------------------------------



# -----------------------------------------------------------------
#
# COBJ_template
# --------------
# Rule template to create static object files from .c source files
#
# PARAMETERS:
#         $1: The constituent stem
#
# ----------------------------------------------------------------- 
define COBJ_template
$($1_COBJFILES) : | $($1_OBJDIR)
$($1_COBJFILES) :   $($1_OBJDIR)/%.o : $($1_SRCDIR)/%.$(CSuf)       \
                                       $($1_DEPDIR)/%.d
	@echo "    Compile.................... $$(call pkgfile,$$<)"
	$(DBG_CMP_C)$(CC)  -c  $$(call _CPPFLAGS,$1)               \
                               $$(call _FLAGS,CFLAGS,$1) -o $$@ $$<
endef
# -----------------------------------------------------------------




# ----------------------------------------------------------------------
#
# CXXDEP_template
# --------------
# Rule template to create the dependency files for .cpp source files
#
# PARAMETERS:
#         $1: The constituent stem
#
# -----------------------------------------------------------------
define CXXDEP_template
$($1_CXXDEPFILES) : | $($1_DEPDIR)
$($1_CXXDEPFILES) :   $($1_DEPDIR)/%.d : $($1_SRCDIR)/%.$(CXXSuf)
	@echo "    Update dependencies........ $$(call pkgfile,$$<)"
	$(DBG_DEP_CXX)$(CXX) -MM  -MT $$@ -MT                     \
                                      $$(call objfile,$$@)         \
                                      $$(call _CPPFLAGS,$1)        \
                                      $$(call _FLAGS,CXXFLAGS,$1)  \
                                      $$< > $$@
endef
# -----------------------------------------------------------------



# ------------------------------------------------------------------
#
# CXXOBJ_template
# --------------
# Rule template to create static object files from .cpp source files
#
# PARAMETERS:
#         $1: The constituent stem
#
# ------------------------------------------------------------------
define CXXOBJ_template
$($1_CXXOBJFILES): | $($1_OBJDIR)
$($1_CXXOBJFILES): $($1_OBJDIR)/%.o : $($1_SRCDIR)/%.$(CXXSuf) \
                                      $($1_DEPDIR)/%.d
	@echo "    Compile.................... $$(call pkgfile,$$<)"
	$(DBG_CMP_CXX)$(CXX)  -c  $$(call _CPPFLAGS,$1)       \
                                  $$(call _FLAGS,CXXFLAGS,$1) \
                                 -o $$@ $$<
endef
# ------------------------------------------------------------------





# -----------------------------------------------------------------
#
# LINKRO_template
# ---------------
# Rule template to create partially linked object files from 
# statically compiled object files
#
# PARAMETERS:
#         $1: The constituent stem
#
# -----------------------------------------------------------------
define LINKRO_template

# -- Create alias
ifdef $1_ALIAS
   .PHONY: $($1_ALIAS)
   $($1_ALIAS): $($1_RO)
endif

# -- Create relocatable
$($1_RO) : | $$(dir $($1_OBJDIR))
$($1_RO) : $($1_CXXOBJFILES) $($1_COBJFILES)
	@echo "    Partial link............... $$(call rofile,$$@)"
	$(DBG_LINK_RO)$(LD)  -r -nostdlib -o $$@ $$^
endef
# -----------------------------------------------------------------




# -----------------------------------------------------------------
#
# LINKEXE_template
# --------------
# Rule template to create statically linked executable and, 
# optionally, an alias.
#
# PARAMETERS:
#         $1: The constituent stem
#
# ----------------------------------------------------------------- 
define LINKEXE_template

# -- Create alias
ifdef $1_ALIAS
   .PHONY: $($1_ALIAS)
   $($1_ALIAS): $($1_EXE)
endif

# -- Create exe
$($(1)_EXE) : | $$(dir $($1_EXE))
$($(1)_EXE) : $($1_CXXOBJFILES) $($1_COBJFILES) $($1_ROS) $($1_SOS)
	@echo "    Link....................... $$(call binfile,$$@)"
	$(DBG_LINK_EXE)$(CXX) -o $$@ $$^                 \
                              $($1_LDFLAGS)  $(LDFLAGS)  \
                              $($1_LOADLIBS) $(LOADLIBS) \
                              $($1_LDLIBS)   $(LDLIBS)
endef
# -----------------------------------------------------------------



# -----------------------------------------------------------------
#
# QT
#
# -----------------------------------------------------------------
define QT_template
.PHONY: $1
$1 :
	@echo Building gui................... $($1_QT)
	@cd $($1_QT); qmake
	@cd $($1_QT); make
endef
# -----------------------------------------------------------------



# -----------------------------------------------------------------



# -----------------------------------------------------------------
#
# define_files
# ------------
# Defines all the derived file names and list of file names using
# the constituents list of source files.
#
# PARAMETERS
#         $1: The constitent stem
#
# Note:
# This is exclusively used by the other procedures.  It is not meant
# to be called by the user.
#
# -----------------------------------------------------------------
define define_files
$(eval $$1_CXXDEPFILES := $$(call derive,$$($1_DEPDIR),$$($1_CXXSRCFILES),.d)) \
$(eval $$1_CXXOBJFILES := $$(call derive,$$($1_OBJDIR),$$($1_CXXSRCFILES),.o)) \
$(eval $$1_CDEPFILES   := $$(call derive,$$($1_DEPDIR),$$($1_CSRCFILES),.d))  \
$(eval $$1_COBJFILES   := $$(call derive,$$($1_OBJDIR),$$($1_CSRCFILES),.o))
endef
# -----------------------------------------------------------------



# -----------------------------------------------------------------
#
# build_ro
# --------
# Rule to build a partially linked static object
#
# PARAMETER:
#         $1: The constitent stem
#
# This procedure first defines all the related file names and lists
# of file names using the define_files procedure, then links the
# object files produced into the partically linked object file.
#
# -----------------------------------------------------------------
define build_ro
$(call define_files,$1)            \
$(eval $(call CXXDEP_template,$1)) \
$(eval $(call   CDEP_template,$1)) \
$(eval $(call CXXOBJ_template,$1)) \
$(eval $(call   COBJ_template,$1)) \
$(eval $(call LINKRO_template,$1))
endef
# -----------------------------------------------------------------





# -----------------------------------------------------------------
#
# build_exe
# ---------
# Rule to build a statically linked executable
#
# PARAMETER:
#         $1: The constitent stem
#
# This procedure first defines all the related file names and lists
# of file names using the define_files procedure, then links the
# object files produced into the executable
#
# -----------------------------------------------------------------
define build_exe
$(call define_files,$1)            \
$(eval $(call CXXDEP_template,$1)) \
$(eval $(call   CDEP_template,$1)) \
$(eval $(call CXXOBJ_template,$1)) \
$(eval $(call   COBJ_template,$1)) \
$(eval $(call LINKEXE_template,$1))
endef
# -----------------------------------------------------------------


# -----------------------------------------------------------------
#
# build_qt
# --------
# Rule to build a QT project
#
# PARAMETER:
#        $1: The constituent stem
#
# -----------------------------------------------------------------
define build_qt
$(eval $(call QT_template,$1)) 
endef
# -----------------------------------------------------------------




# -----------------------------------------------------------------
#
# clean_qt
# --------
# Rule to clean a QT project
#
# -----------------------------------------------------------------
define clean_qt
.PHONY: $1_clean
$1_clean:
	@cd $($1_QT); qmake; make clean distclean
endef
#
# END: Template rules and other rules
#==================================================================



# --------------------------------
# Define all the top level targets
# --------------------------------
ALL_ROS         := $(foreach r,$(RELOCATABLES),$($r_RO))
ALL_EXES        := $(foreach x,$(EXECUTABLES),$($x_EXE))
ALL_QTS         := $(foreach q,$(QTS),$q)
ALL_TARGETS     :=  $(ALL_ROS)                  \
                    $(ALL_EXES)                 \
                    $(ALL_QTS)
ALL_QT_CLEAN    := $(foreach q,$(ALL_QTS),$q_clean)

# -----------------------------------------------
# Create the rules to build the top level targets
# -----------------------------------------------
$(eval $(foreach r,$(RELOCATABLES),$(call build_ro,$r)))
$(eval $(foreach x,$(EXECUTABLES),$(call build_exe,$x)))
$(eval $(foreach q,$(QTS),$(call build_qt,$q)))
$(eval $(foreach q,$(ALL_QTS),$(call clean_qt,$q)))


.PHONY: server_clean
server_clean: $(ALL_QT_CLEAN)


# --------------------------------------------------------------
# Create the list of all constituents and their dependency files
# --------------------------------------------------------------
ALL_CXXDEPFILES  := $(foreach c,$(CONSTITUENTS),$($c_CXXDEPFILES))
ALL_CDEPFILES    := $(foreach c,$(CONSTITUENTS),$($c_CDEPFILES))
ALL_ASMDEPFILES  := $(foreach c,$(CONSTITUENTS),$($c_ASMDEPFILES))
ALL_DEPFILES     := $(ALL_CXXDEPFILES) $(ALL_CDEPFILES) $(ALL_ASMDEPFILES)


ALL_CXXOBJFILES  := $(foreach c,$(CONSTITUENTS),$($c_CXXOBJFILES))
ALL_COBJFILES    := $(foreach c,$(CONSTITUENTS),$($c_COBJFILES))
ALL_ASMOBJFILES  := $(foreach c,$(CONSTITUENTS),$($c_ASMOBJFILES))
ALL_OBJFILES     := $(ALL_CXXOBJFILES) $(ALL_COBJFILES) $(ALL_ASMOBJFILES)


print_goals       := print_flags           \
                     print_dependencyfiles \
                     print_objectfiles     \
                     print_directories     \
                     print_tools           \
                     print_start_of_build  \
                     print_end_of_build




# ------------------------------------------
# Include the rules for the dependency files
# -- Only do this if there are make goals
#    that involve dependency goals
# ------------------------------------------
nondependency_goals := $(print_goals) server_clean
dependency_goals    := $(filter-out $(nondependency_goals), $(MAKECMDGOALS))


ifneq ($(dependency_goals),)
   -include $(ALL_DEPFILES)
endif




.PHONY: all 
.PHONY: make_directories
.PHONY: $(make_build_goals)
.PHONY: $(print_goals)
.PHONE: depend

all:                             \
	print_start_of_build     \
	make_directories         \
                                 \
	$(ALL_TARGETS)           \
                                 \
	print_end_of_build 

define make_directory
	@mkdir -p $@
endef
#### @chmod g+ws $@   Don't change in user's home directory system

make_directories:                                         \
	$(filter_out $(wildcard $(BLDROOT)), $(BLDROOT))  \
	$(filter_out $(wildcard $(DEPROOT)), $(DEPROOT))  \
	$(filter_out $(wildcard $(OBJROOT)), $(OBJROOT))  \
	$(filter-out $(wildcard $(DEPDIRS)), $(DEPDIRS))  \
	$(filter-out $(wildcard $(OBJDIRS)), $(OBJDIRS))  \
	$(filter-out $(wildcard $(BINDIR)),  $(BINDIR))   \
	$(filter-out $(wildcard $(LIBDIR)),  $(LIBDIR))


$(BLDROOT):
	@echo  "    Create build  root    dir .. $@"
	$(make_directory)

$(DEPROOT):
	@echo  "    Create depend root    dir .. $@"
	$(make_directory)

$(OBJROOT):
	@echo  "    Create object root    dir .. $@"
	$(make_directory)

$(DEPDIRS):
	@echo  "    Create dependency   dirs .. $@"
	$(make_directory)

$(OBJDIRS):
	@echo  "    Create object       dirs .. $@"
	$(make_directory)

$(BINDIR):
	@echo  "    Create binary  directory .. $@"
	$(make_directory)

$(LIBDIR):
	@echo  "    Create library directory .. $@"
	$(make_directory)


rceServer: $(RCESERVER)

#LbneDpmZeroSuppressionTest : $(PROTODUNEDPMZEROSUPPRESSIONTEST)



print_start_of_build:
	@echo    "Start build of project......... $(PRJNAME) for $(TARGET)"


print_dependencyfiles:
	@echo -e                                      \
               "----------------------------------"   \
              "\Dependency files for $(TARGET)"       \
              "\n----------------------------------"  \
	@echo $(ALL_DEPFILES) | sed 's/ /\n'/g

print_objectfiles:
	@echo -e                                      \
              "\n----------------------------------"  \
              "\nObject     files for $(TARGET)"      \
              "\n---------------------------------"
	@echo  $(ALL_OBJFILES) | sed 's/ /\n'/g

print_directories:
	@echo  -e                                                  \
                "-----------------------------------------------"  \
              "\nDirectories for $(TARGET)"                        \
              "\n-----------------------------------------------"  \
               "\nProject root................... $(PRJROOT)"      \
               "\nBuild root..................... $(BLDROOT)"      \
               "\nDependency   directory ........ $(DEPROOT)"      \
               "\nObject       directory ........ $(OBJROOT)"      \
               "\nLibrary      directory......... $(LIBDIR)"       \
               "\nBinary       directory......... $(BINDIR)"       \
               "\n"

print_end_of_build:
	@echo  "End   build of project......... $(PRJNAME) for $(TARGET)" 


print_flags:
	@echo -e                           \
                "------------------------" \
              "\nFLAGS for $(TARGET)"      \
              "\n------------------------" \
              "\nCPPFLAGS: $(CPPFLAGS)"    \
              "\nCFLAGS  : $(CFLAGS)"      \
              "\nCXXFLAGS: $(CXXFLAGS)"    \
              "\nLDFLAGS : $(LDFLAGS)"     \
              "\nINCLUDES: $(INCLUDES)"

print_tools:
	@echo -e                           \
	       "------------------------"  \
              "\nTools for $(TARGET)"      \
	      "\n------------------------" \
	      "\nC   compiler: $(CC)"      \
	      "\nC++ compiler: $(CXX)"     \
              "\nAS  asembler: $(AS)"      \
	      "\nLD  loader  : $(LD)"      \
	      "\nLD  flags   : $(LDFLAGS)"

endif


