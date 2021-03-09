#!/bin/bash

# transparently pass all arguments to kronkc
#!/bin/bash

start=`date +%s.%N`
bin/kronkc "$@" 
end=`date +%s.%N`

printf "\n"
printf "Time elapsed.. "
dt=$(echo "$end - $start" | bc -l)
printf "%.3f%s\n" "$dt" "s" 
