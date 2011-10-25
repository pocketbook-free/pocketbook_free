#!/bin/sh

. ../setup.sh

$CC --version
$CXX --version

./configure --prefix=`pwd`/output --host=arm-linux && make && make install
