#!/bin/sh

cc "$@" -ljansson -lcurl src/main.c -o bin
./bin
