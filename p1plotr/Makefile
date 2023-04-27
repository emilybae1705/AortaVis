##
## plt and plotr: SciVis-2023 Project 1
## Copyright (C)  2023 University of Chicago. All rights reserved.
##
## This is only for students and instructors in the 2023 CMSC 23710/33710
## ("SciVis") class, for use in that class. It is not licensed for open-source
## or any other kind of re-distribution. Do not allow this file to be copied
## or downloaded by anyone outside the 2023 SciVis class.

## Modifying this Makefile will not help you finish your work.
## Complete your work within the framework of existing files.

## Still, reading through this Makefile once will help you understand how your
## project code will be built, and tell you how to invoke "make" to modify
## compilation during your development (to, say, turn off optimization, or
## turn off debugging). The word "NOTE" will signal actionable info.

## If you are curious, see
## https://www.gnu.org/software/make/manual/html_node/Introduction.html or
## https://www.oreilly.com/library/view/managing-projects-with/0596006101/ or
## https://makefiletutorial.com/ or anything else you can find to learn more
## about how GNU Make works. This Makefile exploits many features particular
## to the GNU variant of make.

# ensure that failure of first command in a pipe is a show-stopper
SHELL=bash -o pipefail

# we try to tell Make to skip its mostly outdated "implicit rules", and we
# simulate calling make with "-r" which turns them off
.SUFFIXES:
ifeq ($(filter -r,$(MAKEFLAGS)),) # goo.gl/xMRkAW
MAKEFLAGS += -r
endif
# these targets are not actually files
.PHONY:integrity clean shlib clean.shlib clean.rr

# These variables identify (in way consistent across projects) what to build
LIB := plt
CMD := plotr
CMDGRD := grade/plotr
LLIBA := libplt.a
GLLIBA := grade/libgplt.a

# With shared (dynamically linked) libraries, things depend more on OS
# than with static; name of library ends with .dylib or .so
ifeq ($(shell uname -s),Darwin)
  SHEXT := dylib
else
  SHEXT := so
endif
# set name of shared library to create
LLIBSH := py/libplt.$(SHEXT)
ifeq ($(shell uname -s),Darwin)
  # On Mac, need to also assert name of library with "@rpath" prefix
  INSTNAME := -install_name @rpath/libplt.$(SHEXT)
else
  INSTNAME :=
endif

## These next three test functions check (1) that environment variable SCIVIS
## is set, (2) that its value does not contain spaces (because GNU make
## doesn't play well with paths containing spaces), and (3) that it seems to
## refer to an actual checkout of scivis-2023. Exactly how they work
## involves some odd tricks specific to GNU make, the details of which are
## not important for you to understand.
test1SCIVIS := $(if $(SCIVIS),,\
$(warning *)\
$(warning *)\
$(warning * Environment variable SCIVIS is not set. Set this to the path)\
$(warning * of your svn checkout of the scivis-2023 repository; that is, )\
$(warning * "echo $$SCIVIS" should show a path ending with "/scivis-2023" )\
$(warning *)\
$(warning *)\
$(error Make quitting))
$(test1SCIVIS)

# h/t https://www.gnu.org/software/make/manual/html_node/Syntax-of-Functions.html
empty:=
space:= $(empty) $(empty)
test2SCIVIS := $(if $(findstring $(space),$(SCIVIS)),\
$(warning *)\
$(warning *)\
$(warning * Value of environment variable SCIVIS:)\
$(warning * $(SCIVIS))\
$(warning * contains a space, which GNU make cannot handle correctly.)\
$(warning * Find a way to create a different path to the same directory,)\
$(warning * perhaps with symlinks (aliases), or by putting your scivis-2023)\
$(warning * checkout in a different path (such as /tmp) without spaces.)\
$(warning *)\
$(warning *)\
$(error Make quitting),)
$(test2SCIVIS)

test3SCIVIS := $(if $(and \
$(wildcard $(SCIVIS)/tpz),\
$(wildcard $(SCIVIS)/tpz/include/teem/air.h),\
$(wildcard $(SCIVIS)/tpz/include/teem/biff.h),\
$(wildcard $(SCIVIS)/tpz/include/teem/nrrd.h),\
$(wildcard $(SCIVIS)/tpz/bin-osx/unu),\
$(wildcard $(SCIVIS)/tpz/lib-osx/libtpz.a),\
$(wildcard $(SCIVIS)/tpz/bin-linux/unu),\
$(wildcard $(SCIVIS)/tpz/lib-linux/libtpz.a)),,\
$(warning *)\
$(warning *)\
$(warning * Value of environment variable SCIVIS:)\
$(warning * $(SCIVIS))\
$(warning * does not seem to be the path of a checkout of scivis-2023; it is)\
$(warning * missing tpz/include and tpz/{bin,lib}-{osx,linux} subdirs containing)\
$(warning * expected header, binary, and library files.)\
$(warning *)\
$(warning *)\
$(error Make quitting))
$(test3SCIVIS)

# These variables control how the source files are compiled into a library
# and into an executable. In GNU make, "?=" means only set this variable if
# it is not already set by the environment or on the command-line.
#
# NOTE: If you want to change the optimization level to -O0 for debugging,
# you can compile with:
#     make DASHO=
# so that DASHO is set to an empty string prior to processing the Makefile
# contents. On the other hand, in GNU make, ":=" with "override" means
# append to any existing definition. So: if you compile with:
#     make CFLAGS=-DNDEBUG
# then "-DNDEBUG" will be added to the end of CFLAGS, and the C preprocessor
# will then evaluate "#ifdef NDEBUG" to true (or "#ifndef NDEBUG" to false).
# That will make the pltVerbose variable to be #define'd (at compile-time)
# to 0, which will turn off debugging messages conditioned on pltVerbose.

TPZ ?= $(SCIVIS)/tpz
ifeq ($(shell uname -s),Darwin)
  OS := osx
else
  OS := linux
endif
CC ?= cc
DASHO ?= -O2
DASHG ?= -g
# this way, "make CFLAGS=FOO" will put FOO *after* -Wall -Wextra, etc
# e.g. "make CC=clang CFLAGS=-fsanitize=undefined" to detect undefined behavior
override CFLAGS := -Wall -Wextra -std=c99 -pedantic $(DASHO) $(DASHG) $(CFLAGS)
# can set can DASHLTPZ to full path to libtpz.a to bypass linker defaults
DASHLTPZ ?= -ltpz
IPATH ?= -I$(TPZ)/include
LPATH ?= -L$(TPZ)/lib-$(OS)
SHLPATH ?= $(TPZ)/shlib
ARRR ?= ar crs
# Because we're linking with static libraries (to have more predictably
# useful and self-contained executables), we try to reduce the executable
# size by excluding code and data sections that aren't actually used
ifeq ($(shell uname -s),Darwin)
  DEADSTRIP := -dead_strip
else
  # the following option benefits from compiling the constituents sources in
  # libraries with: -ffunction-sections -fdata-sections
  DEADSTRIP := -Wl,-gc-sections
endif

# These are the header and source files involved.
#
# NOTE: Do not try to create new files to complete the project, unless
# specifically told otherwise. plotr_foo.c is scratch space you can use
# however you want (as long as it compiles). "./rplotr about" lists which
# files should be modified to complete the project, and how to test it.
HDR := plt.h  pltPrivate.h
SRCS := util.c poly.c convo.c pplot.c common.c data.c misc.c kernel.c \
 kparse.c plotr_cmpld.c plotr_env.c plotr_todo.c plotr_about.c plotr_lerp.c \
 plotr_itow.c plotr_wtoi.c plotr_klist.c plotr_keval.c plotr_peval.c \
 plotr_psamp.c plotr_delta.c plotr_ceval.c plotr_csamp.c plotr_plot.c \
 plotr_fp.c plotr_foo.c
# object files created to build the regular plotr in this directory
OBJS := $(SRCS:.c=.o)
# object files created specifically for grading
GOBJS := $(addprefix grade/,$(SRCS:.c=.o))

# isolate different kinds of sources, objects
LIBSRCS := $(filter-out plotr%.c,$(SRCS))
LIBOBJS := $(LIBSRCS:.c=.o)
SHOBJS := $(addprefix py/,$(LIBOBJS))
CMDSRCS := $(filter plotr_%.c,$(SRCS))
CMDOBJS := $(CMDSRCS:.c=.o)

# "make" will run integrity checks,
# then compile the (static) library and the executable
all: integrity $(LLIBA) $(CMD)
# "shlib" is the OS-independent target name for shared library
# so you can "make shlib" just make the shared library
# (and "make clean.shlib" cleans it up, and the associated .o files)
shlib: $(LLIBSH)

# all objects depend on headers
$(OBJS) $(GOBJS) $(SHOBJS) $(CMDOBJS): $(HDR)

# from .c to .o files
$(OBJS): %.o: %.c
	$(CC) -c $(CFLAGS) $(IPATH) -o $@ $<

# from .c to .o files for shared library: add -fPIC
$(SHOBJS): py/%.o: %.c
	$(CC) -c $(CFLAGS) -fPIC $(IPATH) -o $@ $<

# from .c to the (different) .o files for grading
$(GOBJS): grade/%.o: %.c
	$(CC) -c $(CFLAGS) -DNDEBUG -DSCIVIS_GRADE $(IPATH) -o $@ $<

# collect object files into (static) library
$(LLIBA): $(LIBOBJS)
	$(ARRR) $(LLIBA) $(LIBOBJS)

# collect object files for grading into library
$(GLLIBA): $(GOBJS)
	$(ARRR) $(GLLIBA) $(GOBJS)

# collect object files into shared library; the rpath to libtpz is *not* set
# here, since we don't know what it is for any particular student's setup
$(LLIBSH): $(SHOBJS)
	$(CC) -shared $(CFLAGS) -o $(LLIBSH) $(SHOBJS) $(INSTNAME) \
	  -L$(SHLPATH) -ltpz

# create executable by linking with library
$(CMD): $(CMD).c $(CMDOBJS) $(LLIBA)
	$(CC) $(DEADSTRIP) $(CFLAGS) $(IPATH) $(CMD).c $(CMDOBJS) -o $(CMD) \
	  -L. $(LPATH) -l$(LIB) $(DASHLTPZ) \
	  -lm  2>&1 | (grep -v "built for newer" ||:)
# (the grep -v "built for newer" is to suppress an annoying warning on Macs)

# create grading executable by linking with BOTH the library of your
# code, and, the reference library containing all the reference code
# (don't depend on integrity because that will always retrigger compilation)
$(CMDGRD): $(CMD).c $(GLLIBA) librplt
	@./.integrity.sh
	$(CC) $(DEADSTRIP) $(CFLAGS) -DNDEBUG -DSCIVIS_GRADE \
	  $(IPATH) $(CMD).c -o $(CMDGRD) \
	  -L. -Lgrade $(LPATH) -lg$(LIB) `./librplt` \
	  $(DASHLTPZ) \
	  -lm  2>&1 | (grep -v "built for newer" ||:)

## The ./integrity.sh script ensures that all the required files are present,
## that only certain files have been modified, and that they have been
## modified only within the indicated regions.
##
## *** Skipping or altering this integrity check is a bad idea ***
##
## NOTE: Grading will use the Makefile, .integrity.sh, and .strip.sh files
## originally distributed to you, ignoring any changes (svn committed or not)
## you make to them. If the integrity check fails, then "make" fails. If the
## graders can't compile your work with "make", you may get a zero grade on
## the assignment.
integrity:
	@./.integrity.sh

clean:
	rm -f $(OBJS) $(LLIBA) $(SHOBJS) $(LLIBSH) \
	   $(CMD) $(GOBJS) $(GLLIBA) $(CMDGRD)
	rm -rf $(CMD).dSYM $(CMDGRD).dSYM

clean.shlib:
	rm -f $(SHOBJS) $(LLIBSH)

#############################################################################
# Everything below here about *.rr.* files and .RR variables, is for
# generating code with randomized rounding direction in floating-point (FP)
# computations, for empirically measuring numerical stability
LIB.RR := plt.rr
LLIB.RRA := libplt.rr.a
LIB.RROBJS := $(LIBSRCS:.c=.rr.o)

# Prevent automatic deletion of some intermediate files, so that we can
# better see exactly how random rounding was implemented
# h/t https://unix.stackexchange.com/a/517196
.PRECIOUS: %.rr.c %.rr0.s

# FP random rounding is implemented by adding a buffer of rounding modes and
# various variables to plotr.c (via rr.py cmd), transforming delimiters of
# student code in foo.c into compile-able markers in foo.rr.c (via rr.py cc),
# compiling foo.rr.c to assembly foo.rr0.s, injecting rounding mode controls
# (via rr.py asm) to make foo.rr1.s, and compiling to binary object foo.rr.o
# for linking to create plotr.rr executable
$(CMD)-rr.c: $(CMD).c
	./rr.py cmd $< $@

%.rr.c: %.c
	./rr.py cc $< $@

# Using clang specifically for cross-platform controls on code generation.
# But found that clang -S (v15.0.5) sometimes puts garbage characters in
# comments, so using tr to clean it up.
# HEY: put -ffp-model=strict back in
%.rr0.s: %.rr.c
	clang -S $(CFLAGS) $(IPATH) -o - $< | (export LC_ALL=C; tr -d '\000\200') > $@

%.rr1.s: %.rr0.s
	./rr.py asm $< $@

$(LIB.RROBJS): %.rr.o: %.rr1.s
	$(CC) -c $(CFLAGS) -o $@ $<

$(LLIB.RRA): $(LIB.RROBJS)
	$(ARRR) $(LLIB.RRA) $(LIB.RROBJS)

$(CMD).rr: $(CMD)-rr.c $(CMDOBJS) $(LLIB.RRA)
	$(CC) $(DEADSTRIP) $(CFLAGS) $(IPATH) $(CMD)-rr.c $(CMDOBJS) -o $(CMD).rr \
	  -L. $(LPATH) -l$(LIB.RR) $(DASHLTPZ) \
	  -lm  2>&1 | (grep -v "built for newer" ||:)

# "make clean.rr" to clean up all random rounding stuff
# (separate from "make clean" to reduce noise)
clean.rr:
	rm -rf $(LLIB.RRA) $(LIB.RROBJS) $(LIB.RROBJS:.o=.c) \
	  $(CMD)-rr.c $(CMD).rr $(CMD).rr.dSYM \
	  $(LIB.RROBJS:.rr.o=.rr0.s) $(LIB.RROBJS:.rr.o=.rr1.s)
