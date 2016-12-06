#!/usr/bin/env bash

# $Id: check-one.sh 818 2007-09-02 17:45:21Z suriya $

C_SUBSET_COMPILER=../src/csc
THREE_ADDR_TO_C_TRANSLATOR=../3addr-to-c-converter/convert.py

[ $# -ne 1 ] && { echo "Usage $0 PROGRAM" >&2; exit 1; }

# set -v

PROGRAM=$1
BASENAME=`basename $PROGRAM .c`
echo $PROGRAM
${C_SUBSET_COMPILER} $PROGRAM > ${BASENAME}.3addr
${THREE_ADDR_TO_C_TRANSLATOR} < ${BASENAME}.3addr > ${BASENAME}.cfg
md5sum ${BASENAME}.cfg ${BASENAME}.ta.cfg
