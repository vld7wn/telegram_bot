#!/bin/bash
cd build
make VERBOSE=1 -j1 > build_log.txt 2>&1
cat build_log.txt | head -n 50
echo "..."
cat build_log.txt | grep "error:" | head -n 20
