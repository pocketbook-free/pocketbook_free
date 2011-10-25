#!/bin/sh

. ../setup_fxn.sh

$CC --version
$CXX --version

make && make install
