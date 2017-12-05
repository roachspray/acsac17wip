#!/bin/bash

timeout -sKILL 3h afl-fuzz -i outd3/crashes -o origd3samples -- ./prep_afl -r @@
timeout -sKILL 3h afl-fuzz -i outd4/crashes -o origd4samples -- ./prep_afl -r @@
timeout -sKILL 3h afl-fuzz -i outd5/crashes -o origd5samples -- ./prep_afl -r @@
timeout -sKILL 3h afl-fuzz -i outd6/crashes -o origd6samples -- ./prep_afl -r @@
