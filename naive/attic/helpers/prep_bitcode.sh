#!/bin/bash

if [ "$#" -ne 2 ]; then
	echo "usage: prep_bitcode.sh  <inBitcode> <outBitcode>"
	exit 1
fi


# Versions, my dear, versions!
opt-4.0 -mem2reg -constprop -ipconstprop -o $2 $1
