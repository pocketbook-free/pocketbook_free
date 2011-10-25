#!/bin/sh

. ../setup.sh

$CC --version
$CXX --version

export LIBS=-linkview

./configure --prefix=`pwd`/output --host=arm-linux --enable-debug=no --with-gfxdrivers=none --with-inputdrivers=zytronic --enable-x11=no && make && make install
