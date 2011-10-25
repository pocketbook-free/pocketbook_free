#!/bin/sh

. ../setup_fxn.sh

$CC --version
$CXX --version

export LIBS="-linkview -ljpeg"

./configure --prefix=${INSTALL_PATH} --host=arm-none-linux-gnueabi --disable-glibtest --enable-static && make && make install
