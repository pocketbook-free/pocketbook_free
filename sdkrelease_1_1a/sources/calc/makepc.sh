#!/bin/sh
make clean
cmake -DCMAKE_BUILD_TYPE=Debug -DTARGET_TYPE=Linux
make

