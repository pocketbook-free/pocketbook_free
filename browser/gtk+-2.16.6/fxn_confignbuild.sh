#!/bin/sh

. ../setup_fxn.sh

$CC --version
$CXX --version

./configure --prefix=${INSTALL_PATH} --build=i686-linux --with-gdktarget=directfb --host=arm-none-linux-gnueabi \
               --disable-modules --without-libtiff \
               --without-libjasper --disable-glibtest --disable-cups --disable-papi --enable-static && make && make install
