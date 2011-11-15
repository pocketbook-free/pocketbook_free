#!/bin/sh
make clean
cmake -DCMAKE_BUILD_TYPE=Release -DTARGET_TYPE=ARM
make

