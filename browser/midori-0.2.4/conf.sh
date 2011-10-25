#!/bin/sh

. ../setup.sh

$CC --version
$CXX --version

./waf configure && ./waf build



