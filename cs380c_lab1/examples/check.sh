#!/usr/bin/env bash

for PROGRAM in collatz.c gcd.c hanoifibfac.c loop.c mmm.c prime.c \
    regslarge.c struct.c sort.c sieve.c
do
    ./check-one.sh ${PROGRAM}
done
echo "md5sum hash of outputs"
md5sum *.txt
