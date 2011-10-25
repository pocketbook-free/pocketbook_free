#!/bin/sh

. ../setup.sh

$CC --version
$CXX --version

./configure --prefix=`pwd`/output --build=i686-linux --with-gdktarget=directfb --host=arm-linux \
               --disable-modules --without-libtiff \
               --without-libjasper --disable-glibtest --disable-cups --disable-papi && make && make install
