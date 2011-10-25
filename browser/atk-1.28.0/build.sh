#!/bin/sh

. ../setup.sh

$CC --version
$CXX --version

make && make install
