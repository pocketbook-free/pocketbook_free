#!/bin/sh

. ../setup.sh

$CC --version
$CXX --version

export LIBS=-linkview

./configure --prefix=`pwd`/output --host=arm-linux --disable-glibtest && make && make install
