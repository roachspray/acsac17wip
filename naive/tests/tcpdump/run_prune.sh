#!/bin/bash
timeout -sKILL 1m afl-fuzz -i tcpdump_input_dir -o tcpdump_out_prune_depth4 ./tcpdump_prune_afl -r @@
