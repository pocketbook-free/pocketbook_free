#!/bin/sh

. ../setup.sh

$CC --version
$CXX --version

./configure --prefix=`pwd`/output --host=arm-linux --enable-directfb=yes && make && make install
