#!/bin/bash
#

make -C src
valgrind --leak-check=full --show-leak-kinds=all --trace-children=yes --track-origins=yes ./src/asynch_bellman_ford ./test/connectivity_0.txt

