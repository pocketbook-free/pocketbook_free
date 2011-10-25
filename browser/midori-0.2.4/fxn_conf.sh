#!/bin/sh

. ../setup_fxn.sh

$CC --version
$CXX --version

./waf configure && ./waf build



