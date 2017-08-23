#!/bin/sh

mkdir -p build
cd build

PGM=$(which ninja)
if [ $? -eq 0 ]; then
    cmake -G Ninja .. && ninja
else
    NPROC=$(nproc --all 2>/dev/null || echo 1) # Find num cpus or use 1 as default
    cmake .. && make -j $NPROC
fi
