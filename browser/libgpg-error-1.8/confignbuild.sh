#!/bin/sh

. ../setup.sh

$CC --version
$CXX --version

./configure --prefix=`pwd`/output --build=i686-linux --host=arm-linux
