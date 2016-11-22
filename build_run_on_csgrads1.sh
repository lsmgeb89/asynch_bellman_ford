#!/bin/bash
#

make -C src/ -f makefile_csgrads1
export LD_LIBRARY_PATH=/usr/local/gcc610/lib64/:$LD_LIBRARY_PATH
./src/asynch_bellman_ford ./test/connectivity_0.txt

