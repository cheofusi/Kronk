#!/bin/bash

# transparently pass all arguments to kronkc
(bin/kronkc "$@") 2> optim.ll

#TODO add time it took above command to complete)