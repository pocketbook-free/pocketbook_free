#!/bin/sh

. ../setup_fxn.sh

$CC --version
$CXX --version

./configure --prefix=${INSTALL_PATH} --build=i686-linux --host=arm-none-linux-gnueabi --disable-gtktest && make && make install
