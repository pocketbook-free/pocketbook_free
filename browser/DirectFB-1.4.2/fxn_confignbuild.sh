#!/bin/sh

. ../setup_fxn.sh

$CC --version
$CXX --version

#export LIBS="-linkview -ljpeg"
export LDFLAGS="-linkview -ljpeg"

./configure --prefix=$INSTALL_PATH --build=i686-pc-linux-gnu --host=arm-none-linux-gnueabi --target=arm-none-linux-gnueabi --enable-debug=no --with-gfxdrivers=none --with-inputdrivers=zytronic --enable-x11=no && make && make install
