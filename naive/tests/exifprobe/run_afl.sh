#!/bin/bash

timeout -sKILL 1h afl-fuzz -i input -o outd1 -- ./dep1_afl -c @@
timeout -sKILL 1h afl-fuzz -i input -o outd2 -- ./dep2_afl -c @@
timeout -sKILL 1h afl-fuzz -i input -o outd3 -- ./dep3_afl -c @@
timeout -sKILL 1h afl-fuzz -i input -o outd4 -- ./dep4_afl -c @@
timeout -sKILL 1h afl-fuzz -i input -o outd5 -- ./dep5_afl -c @@
timeout -sKILL 1h afl-fuzz -i input -o outd6 -- ./dep6_afl -c @@
timeout -sKILL 1h afl-fuzz -i input -o outd7 -- ./dep7_afl -c @@
timeout -sKILL 1h afl-fuzz -i input -o outd8 -- ./dep8_afl -c @@
timeout -sKILL 1h afl-fuzz -i input -o outd9 -- ./dep9_afl -c @@
