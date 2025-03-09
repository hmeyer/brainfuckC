#!/bin/bash

BUILD_DIR=$(dirname "$0")/build
mkdir -p $BUILD_DIR
cd $BUILD_DIR
cmake ..
make

mkdir -p examples

for file in ../examples/*.simple; do
    echo "Running $file"
    ./bfs $file examples/$(basename $file .simple).bf
    ./bfi examples/$(basename $file .simple).bf
done
