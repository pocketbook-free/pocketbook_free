#!/bin/sh

. ../setup.sh

$CC --version
$CXX --version

./configure --prefix=`pwd`/output --host=arm-linux --without-x --with-included-modules=yes --with-dynamic-modules=no --enable-debug=no && make && make install
