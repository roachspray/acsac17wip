#!/bin/bash
timeout -sKILL 1m afl-fuzz -i tcpdump_input_dir -o tcpdump_out_orig ./tcpdump_afl -r @@
