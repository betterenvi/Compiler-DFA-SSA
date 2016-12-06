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
gcc $PROGRAM -o ${BASENAME}.gcc.bin
${THREE_ADDR_TO_C_TRANSLATOR} < ${BASENAME}.3addr > ${BASENAME}.3addr.c
gcc ${BASENAME}.3addr.c -o ${BASENAME}.3addr.bin
./${BASENAME}.gcc.bin > ${BASENAME}.gcc.txt
./${BASENAME}.3addr.bin > ${BASENAME}.3addr.txt
md5sum ${BASENAME}.gcc.txt ${BASENAME}.3addr.txt
