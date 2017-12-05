#!/bin/bash

C="opt-4.0 -load $PSKIN_ROOT/build/lib/NaivePrune.so"

if [ "$#" -ne 5 ]; then
    echo "usage: naive_prune_fnlist <logFileName> <pruneDepth> <fnList> <inBitcode> <outBitcode>"
	exit 1
fi

PATH=$PATH:.:.. $C -pskin-naive-prune -pskin-fnlist $3 -pskin-log-file $1 -pskin-depth $2 $4 -o $5

