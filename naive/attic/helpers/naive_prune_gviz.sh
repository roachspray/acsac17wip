#!/bin/bash

c="opt-4.0 -load $PSKIN_ROOT/build/lib/GVizPrune.so"

if [ "$#" -ne 4 ]; then
    echo "usage: naive_prune_gviz <logFileName> <pruneDepth> <inBitcode> <outBitcode>"
	exit 1
fi

PATH=$PATH:.:.. $C -pskin-gviz-prune -pskin-log-file $1 -pskin-depth=$2 $3 $4

