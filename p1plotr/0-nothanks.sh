#!/bin/bash
# plt and plotr: SciVis-2023 Project 1
# Copyright (C)  2023 University of Chicago. All rights reserved.
#
# This is only for students and instructors in the 2023 CMSC 23710/33710
# ("SciVis") class, for use in that class. It is not licensed for open-source
# or any other kind of re-distribution. Do not allow this file to be copied
# or downloaded by anyone outside the 2023 SciVis class.

set -o errexit
set -o nounset
shopt -s expand_aliases
JUNK=""
function junk { JUNK="$JUNK $@"; }
function cleanup { rm -rf $JUNK; }
trap cleanup err exit int term

# disable any coloring
unset GREP_OPTIONS

LIB=plt
PROJ=p1plotr

# make sure $SCIVIS is set
if [ -z ${SCIVIS+x} ]; then
    echo "$0: ERROR:"
    echo "Environment variable \$SCIVIS is not defined, or is only an empty string"
    echo "SCIVIS should be set to path into:"
    echo "svn checkout https://phoenixforge.cs.uchicago.edu/svn/scivis-2023"
    exit 1
fi

if [[ ! -f $LIB.h ]]; then
    echo "$0: ERROR:"
    echo "$LIB.h not in current directory.  This should be run from"
    echo "directory containing your $PROJ work."
    exit 1;
fi

MOD=$(svn status | grep ^M)||:
if [ ! -z "$MOD" ]; then
    echo "$0: !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!"
    echo "$0: !!! WARNING:"
    echo "$0: !!! You have modified but uncommitted files:"
    echo "$MOD"
    echo "$0: !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!"
    echo ""
fi

URL=$(svn info | grep ^URL: | cut -d' ' -f 2)||:
echo "$0: =========================================================="
echo "$0: will do a fresh checkout of:"
echo "     $URL"

TMP=`mktemp -d 2>/dev/null || mktemp -d -t 'mytmpdir'`
echo "$0: =========================================================="
echo "$0: cd'ing to new tmp dir:"
echo "     $TMP"
junk $TMP
cd $TMP
echo "$0: =========================================================="
echo "$0: running: svn co $URL ."
svn co $URL .
echo "$0: =========================================================="
echo "$0: running: svn update $SCIVIS (to ensure work/$PROJ is there)"
svn update $SCIVIS
echo "$0: =========================================================="
echo "$0: getting original Makefile, .integrity.sh, .strip.sh"
cp $SCIVIS/work/$PROJ/Makefile .
cp $SCIVIS/work/$PROJ/.integrity.sh .
cp $SCIVIS/work/$PROJ/.strip.sh .

# before make, turn off errexit
set +o errexit
echo "$0: =========================================================="
echo "$0: running: make"
make
# get exit status
ret=$?
if [ $ret != 0 ]; then
    echo "$0: !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!"
    echo "$0: !!! ERROR: \"make\" FAILED (status $ret)"
    echo "$0: !!! You might get a ZERO if this is not fixed."
    echo "$0: !!! The current tmp directory ($TMP) "
    echo "$0: !!! will now be cleaned up but you should fix the above "
    echo "$0: !!! problems in your $PROJ directory prior to the deadline".
    echo "$0: !!! Good luck."
    echo "$0: !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!"
    exit 1
fi
# else

echo "$0: =========================================================="
echo "$0: \"make\" finished OK. Now trying \"make grade/plotr\""
make grade/plotr
# get exit status
ret=$?
if [ $ret != 0 ]; then
    echo "$0: !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!"
    echo "$0: !!! ERROR: \"make grade/plotr\" FAILED (status $ret)"
    echo "$0: !!! You might get a ZERO if this is not fixed."
    echo "$0: !!! The current tmp directory ($TMP) "
    echo "$0: !!! will now be cleaned up but you should fix the above "
    echo "$0: !!! problems in your $PROJ directory prior to the deadline".
    echo "$0: !!! Good luck."
    echo "$0: !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!"
    exit 1
fi
# else

echo "$0: =========================================================="
echo "$0: \"make grade/plotr\" finished OK.  Goodbye."
