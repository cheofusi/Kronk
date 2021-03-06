#!/bin/bash

# transparently pass all arguments to kronkc
start=`date +%s.%N`
bin/kronkc "$@" #2> optim.ll
end=`date +%s.%N`

printf "Time elapsed.. "
dt=$(echo "$end - $start" | bc -l)
printf "%.3f%s\n" "$dt" "s" 
