#!/bin/bash

# Set LLVM directory
export LLVM_DIR=/usr/lib/llvm-19

# Clean and create build directory
echo "Cleaning and creating build directory..."
rm -rf build
mkdir build
cd build

# Build the pass
echo "Building the pass..."
cmake -DLT_LLVM_INSTALL_DIR=$LLVM_DIR ..
make

# Return to root directory
cd ..

# Compile the test file with value names preserved
echo "Compiling test.c to LLVM bitcode..."
clang-19 -c -emit-llvm -fno-discard-value-names -O0 test.c -o test.bc

# Run the pass
echo "Running the pass..."
opt-19 -load-pass-plugin build/lib/libHelloWorld.so -passes=hello-world -disable-output test.bc