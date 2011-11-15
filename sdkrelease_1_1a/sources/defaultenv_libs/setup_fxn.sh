#!/bin/sh

#export CC=arm-none-linux-gnueabi-gcc
#export CXX=arm-none-linux-gnueabi-g++
#export CFLAGS="-s -msoft-float"
#export CPPFLAGS="-s -msoft-float"
#export AR=arm-none-linux-gnueabi-ar
#export AS=arm-none-linux-gnueabi-as
#export LD=arm-none-linux-gnueabi-ld
#export RANLIB=arm-none-linux-gnueabi-ranlib
#export CC=arm-none-linux-gnueabi-gcc
#export NM=arm-none-linux-gnueabi-nm

TOOLCHAIN=`pwd`/../../../FRSCSDK
export INSTALL_PATH=$TOOLCHAIN/arm-none-linux-gnueabi/sysroot/usr
export PATH=$TOOLCHAIN/bin:$PATH
export PKG_CONFIG_PATH=$INSTALL_PATH/lib/pkgconfig
export PKG_CONFIG_LIBDIR=$INSTALL_PATH/lib/pkgconfig
#export PATH=$TOOLCHAIN/arm-none-linux-gnueabi/bin:$TOOLCHAIN/bin:$PATH
