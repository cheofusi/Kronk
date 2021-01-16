#!/bin/bash

input_file="$1"
# first compile(). Question: How do we check if the return code to see if it compiled sucessfully
(bin/kronkc < $input_file) 2> ir.ll 

# then pass to the optimizer (#TODO check if opt exists and get its name)
opt-10 --O2 ir.ll -o optim.bc

# execute using the jit (#TODO check if lli exists and get its name)
#clear
lli-10 -load=bin/rt/libkronkrt.so -jit-kind=orc-lazy optim.bc

# clean up (#TODO add time it took above command to complete)
#rm ir.ll 
rm optim.bc