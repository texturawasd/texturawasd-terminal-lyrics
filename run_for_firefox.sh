#!/bin/sh

cc "$@" -D_FIREFOX_EXTENSION_BRIDGE -ljansson -lcurl src/main.c -o bin
cc "$@" -D_FIREFOX_EXTENSION_BRIDGE -D_FIREFOX_EXTENSION_BRIDGE_SERVER -ljansson -lcurl src/main.c -o server

./server &
./bin
pkill server