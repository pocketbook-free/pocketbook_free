#!/bin/sh

if [ "$1" != "menuconfig" -a "$1" != "zImage" -a "$1" != "modules" -a "$1" != "clean" ] ; then
	echo "usage: make.sh {menuconfig|zImage|modules|clean}"
	exit 1
fi

#export PATH=/opt/freescale/usr/local/gcc-4.1.2-glibc-2.5-nptl-3/arm-none-linux-gnueabi/bin:$PATH
#export PATH=/opt/arm-2008q3/bin:$PATH
export PATH=/usr/local/arm/4.3.1-eabi-armv6/usr/bin:$PATH

[ $1 = "zImage" ] && rm -f zImage
[ $1 = "modules" ] && rm -rf modules
[ $1 = "modules" ] && find . -name "*.ko" -exec rm -f \{} \;

#make ARCH=arm CROSS_COMPILE=arm-none-linux-gnueabi- "$@"
#make ARCH=arm CROSS_COMPILE=arm-none-eabi- "$@"
make ARCH=arm CROSS_COMPILE=arm-linux- "$@"

[ $1 = "modules" ] && mkdir -p modules
[ $1 = "modules" ] && find . -name "*.ko" -exec cp -f \{} modules/ \;

