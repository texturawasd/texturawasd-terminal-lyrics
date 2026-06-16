#!/bin/sh

# Separate compiler flags (starting with -D) from binary arguments
compiler_flags=""
binary_args=""

for arg in "$@"; do
    case "$arg" in
        -D*|-O*)
            compiler_flags="$compiler_flags $arg"
            ;;
        *)
            binary_args="$binary_args $arg"
            ;;
    esac
done

cc $compiler_flags -D_FIREFOX_EXTENSION_BRIDGE -D_OPTS -ljansson -lcurl src/main.c -o bin
cc $compiler_flags -D_FIREFOX_EXTENSION_BRIDGE -D_FIREFOX_EXTENSION_BRIDGE_SERVER -D_OPTS -ljansson -lcurl src/main.c -o server

./server &
./bin $binary_args
pkill server