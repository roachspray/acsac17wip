#!/bin/bash

C="opt-4.0 -load $PSKIN_ROOT/build/lib/NaivePrune.so"

if [ "$#" -ne 4 ]; then
    echo "usage: naive_prune_interactive.sh <logFileName> <pruneDepth> <inBitcode> <outBitcode>"
	exit 1
fi

PATH=$PATH:.:.. $C -pskin-naive-prune -pskin-log-file $1 -pskin-depth $2  -pskin-interactive 1 $3 -o $4

