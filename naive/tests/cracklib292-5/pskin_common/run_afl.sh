#!/bin/bash

#timeout -sKILL 5h afl-fuzz -t 20000 -i input -o outd7 -- ./dep7_afl -r @@
#timeout -sKILL 5h afl-fuzz -t 20000 -i input -o outd8 -- ./dep8_afl -r @@
#timeout -sKILL 5h afl-fuzz -t 20000 -i input -o outd9 -- ./dep9_afl -r @@
timeout -sKILL 5h afl-fuzz -t 20000 -i input -o outd10 -- ./dep10_afl -r @@
