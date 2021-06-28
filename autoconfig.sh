#!/bin/bash

CONFIGURE=$(realpath configure.py)
PATHS="--hl2sdk-root=/mnt/user/dev/alliedmodders --mms-path=/mnt/user/dev/alliedmodders/mmsource-1.10 --sm-path=/mnt/user/dev/alliedmodders/sourcemod"

mkdir -p build
cd build
 	CC=gcc CXX=g++ $CONFIGURE $PATHS --sdks=tf2 --enable-debug --exclude-mods-debug --enable-optimize --exclude-mods-visualize
cd ..

mkdir -p build/release
pushd build/release
	CC=gcc CXX=g++ $CONFIGURE $PATHS --sdks=tf2 --enable-optimize --exclude-mods-debug --exclude-mods-visualize --exclude-vgui
popd

# mkdir -p build/clang
# pushd build/clang
# 	CC=clang CXX=clang++ $CONFIGURE $PATHS --sdks=tf2 --enable-debug
# popd
