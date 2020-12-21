#!/bin/bash

# This script generates bitcode for the runtime library so the compiler i.e kronkc can check if a given
# function exists, get its prototype at once if it does or emit an error if it doesn't

find_binary() {
    binary=$(echo -e ${PATH//:/\\n} | 
            while read line; do find "$line" -name "$1*"; done | 
            sort -n | 
            head -n 1)
    echo $binary
}

clang_binary=$(find_binary clang++)
llvm_link_binary=$(find_binary llvm-link)

if [ -z $clang_binary ] || [ -z $llvm_link_binary ]; 
then 
    echo "clang toolchain not found, aborting.."
    exit 1
fi

$clang_binary -c -emit-llvm -w *.cpp
$llvm_link_binary *.bc -o libkronkrt.bc

mv libkronkrt.bc "$MESON_BUILD_ROOT"/src
#mv kronkrtbc.bc ../build/src/libkronkrt.bc
rm *.bc #clean up