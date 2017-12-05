#!/bin/bash
#clang-4.0 -I . -DIN_CRACKLIB -emit-llvm -c fascist.c 
clang-4.0 -I . -DIN_CRACKLIB -emit-llvm -c packlib.c
mv packlib.bc packlib_orig.bc
clang-4.0 -I . -DINGORE_MAGIC_CHECK -DIN_CRACKLIB -emit-llvm -c packlib.c
mv packlib.bc packlib_nomagic.bc
clang-4.0 -I . -DIN_CRACKLIB -emit-llvm -c rules.c
clang-4.0 -I . -DIN_CRACKLIB -emit-llvm -c stringlib.c
llvm-link-4.0 -o lib_orig.bc packlib_orig.bc rules.bc stringlib.bc
llvm-link-4.0 -o lib_nomagic.bc packlib_nomagic.bc rules.bc stringlib.bc
clang-4.0 -I . -emit-llvm -o packer.bc -c packer.c
clang-4.0 -I . -emit-llvm -o unpacker.bc -c unpacker.c
llvm-link-4.0 -o clpacker.bc packer.bc lib_orig.bc
llvm-link-4.0 -o clunpacker.bc unpacker.bc lib_orig.bc
llvm-link-4.0 -o clpacker_nomagic.bc packer.bc lib_nomagic.bc
llvm-link-4.0 -o clunpacker_nomagic.bc unpacker.bc lib_nomagic.bc
clang-4.0 -o clpacker clpacker.bc -lz
