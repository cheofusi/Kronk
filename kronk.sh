#!/bin/bash

build/kronkc < $1 2> ir.ll 
llvm-link-10 ir.ll runtimeLib.ll | llvm-dis-10 -o finalIr.ll