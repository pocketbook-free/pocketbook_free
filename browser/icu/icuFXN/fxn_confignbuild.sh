#!/bin/sh

. ../../setup_fxn.sh

$CC --version
$CXX --version

../source/configure --prefix=${INSTALL_PATH} --host=arm-none-linux-gnueabi --with-cross-build=~/projects/icu/icuLinux && make && make install
