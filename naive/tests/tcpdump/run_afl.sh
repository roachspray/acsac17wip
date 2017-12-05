#!/bin/bash

timeout -sKILL 1h afl-fuzz -i input -o outd3 -- ./dep3_afl -r @@
timeout -sKILL 1h afl-fuzz -i input -o outd4 -- ./dep4_afl -r @@
timeout -sKILL 1h afl-fuzz -i input -o outd5 -- ./dep5_afl -r @@
timeout -sKILL 1h afl-fuzz -i input -o outd6 -- ./dep6_afl -r @@
