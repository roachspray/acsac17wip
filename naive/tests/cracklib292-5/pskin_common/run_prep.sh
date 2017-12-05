#!/bin/bash
# Use all the inputs without differentiation. Not sure if this is correct.
#timeout -sKILL 5h afl-fuzz -t 20000 -i outd7/crashes -o prep_outsd7 -- ./prep_afl -r @@ -w 128
timeout -sKILL 13h afl-fuzz -t 20000 -i outd8/crashes -o prep_outsd8 -- ./prep_afl -r @@ -w 128
