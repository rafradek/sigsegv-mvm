#!/bin/bash

CONFIGURE=$(realpath configure.py)
PROJECT_DIR=$(dirname $(realpath $0))
echo $PROJECT_DIR
PATHS="--hl2sdk-root=$PROJECT_DIR/../alliedmodders --mms-path=$PROJECT_DIR/../alliedmodders/mmsource-1.10 --sm-path=$PROJECT_DIR/../alliedmodders/sourcemod"

mkdir -p build
cd build
 	CC=gcc CXX=g++ $CONFIGURE $PATHS --sdks=tf2 --enable-debug --exclude-mods-debug --enable-optimize --exclude-mods-visualize --exclude-vgui
cd ..

mkdir -p build/release
pushd build/release
	CC=gcc CXX=g++ $CONFIGURE $PATHS --sdks=tf2 --enable-optimize --exclude-mods-debug --exclude-mods-visualize --exclude-vgui
popd

# mkdir -p build/clang
# pushd build/clang
# 	CC=clang CXX=clang++ $CONFIGURE $PATHS --sdks=tf2 --enable-debug
# popd
