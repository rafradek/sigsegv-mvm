#!/bin/bash

CONFIGURE=$(realpath configure.py)
PATHS="--hl2sdk-root=/opt/tfserver/alliedmodders --mms-path=/opt/tfserver/alliedmodders/mmsource-1.10 --sm-path=/opt/tfserver/alliedmodders/sourcemod"

mkdir -p build
cd build
 	CC=clang CXX=clang++ $CONFIGURE $PATHS --sdks=tf2 --enable-debug --enable-optimize --exclude-mods-visualize
cd ..

mkdir -p build/release
pushd build/release
	CC=clang CXX=clang++ $CONFIGURE $PATHS --sdks=tf2 --enable-optimize --exclude-mods-debug --exclude-mods-visualize
popd

# mkdir -p build/clang
# pushd build/clang
# 	CC=clang CXX=clang++ $CONFIGURE $PATHS --sdks=tf2 --enable-debug
# popd
