#!/bin/bash

C="opt-4.0 -load $PSKIN_ROOT/build/lib/NaivePrune.so"

if [ "$#" -ne 5 ]; then
    echo "usage: naive_prune <logFileName> <logBlockRemove> <pruneDepth> <inBitcode> <outBitcode>"
	exit 1
fi

PATH=$PATH:.:.. $C -pskin-naive-prune -pskin-log-file $1  \
  -pskin-log-block-removal $2.lbt -pskin-depth $3 $4 -o $5

