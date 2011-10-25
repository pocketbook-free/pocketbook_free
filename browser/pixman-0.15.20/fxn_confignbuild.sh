#!/bin/sh

. ../setup_fxn.sh

$CC --version
$CXX --version

./configure --prefix=${INSTALL_PATH} --build=i686-linux-gnu --host=arm-none-linux-gnueabi --disable-arm-simd --enable-gtk=no && make && make install
