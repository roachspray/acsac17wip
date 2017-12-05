#!/bin/bash

if [ "$#" -ne 2 ]; then
    echo "usage: post_prune_delim <inBitcode> <outBitcode>"
	exit 1
fi
opt-4.0 -adce -dce -strip-dead-prototypes -loop-deletion -deadargelim -o $2 < $1
