#!/bin/sh

. ../../setup_fxn.sh

$CC --version
$CXX --version

../configure --prefix=${INSTALL_PATH} --build=i686-pc-linux-gnu --host=arm-none-linux-gnueabi --enable-static --with-unicode-backend=icu --with-target=directfb \
--enable-javascript-debugger=no --enable-video=no --enable-jit=no \
--enable-gtk-doc=no --enable-gtk-doc-html=no --enable-gtk-doc-pdf=no && make -j 4 && make install
