#!/bin/sh

. ../../setup_fxn.sh

$CC --version
$CXX --version

./configure --prefix=${INSTALL_PATH} --build=i686-pc-linux-gnu --host=arm-none-linux-gnueabi --enable-shared --with-cross-build=~/projects/FRSCSDK/arm-none-linux-gnueabi/sysroot/usr&& make && make install
