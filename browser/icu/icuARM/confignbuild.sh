#!/bin/sh

. ../../setup.sh

$CC --version
$CXX --version

../source/configure --prefix=`pwd`/output --host=arm-linux --with-cross-build=~/projects/icu/icuLinux && make && make install
