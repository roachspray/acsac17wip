#!/bin/bash

C="opt-4.0 -load $PSKIN_ROOT/build/lib/TraceInject.so"

if [ "$#" -ne 3 ]; then
    echo "usage: inject_trays <logFileName> <inBitcode> <outBitcode>"
	exit 1
fi

PATH=$PATH:.:.. $C -pskin-inject -pskin-log-file $1 $2 -o $3

