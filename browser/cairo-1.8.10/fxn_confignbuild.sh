#!/bin/sh

. ../setup_fxn.sh

$CC --version
$CXX --version

./configure --prefix=${INSTALL_PATH} --host=arm-none-linux-gnueabi --build=i686-linux-gnu --enable-xlib=no --enable-directfb=yes && make && make install
