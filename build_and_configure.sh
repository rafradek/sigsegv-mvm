#!/bin/bash

if [[ $EUID -ne 0 ]]
then
    echo "Not running as root!"
    exit
fi

dpkg --add-architecture i386
apt update

apt install -y autoconf automake libtool pip python3-venv nasm libiberty-dev libiberty-dev:i386 libelf-dev:i386 libboost-dev:i386 libbsd-dev:i386 libunwind-dev:i386 lib32z1-dev libc6-dev-i386 linux-libc-dev:i386 g++-multilib

_INSTALL_DIR="/home/wsluser"
mkdir -p $_INSTALL_DIR/sigsegv-mvm

git clone "https://github.com/rafradek/sigsegv-mvm.git" --recursive

chmod -R 755 $_INSTALL_DIR/sigsegv-mvm
chown -R wsluser:wsluser $_INSTALL_DIR/sigsegv-mvm

mkdir -p alliedmodders
cd alliedmodders

# Clone or pull repositories
git clone --recursive https://github.com/alliedmodders/sourcemod.git --depth 1 -b 1.11-dev
git clone https://github.com/alliedmodders/hl2sdk.git --depth 1 -b sdk2013 hl2sdk-sdk2013
git clone https://github.com/rafradek/hl2sdk-tf2.git --depth 1 hl2sdk-tf2
git clone https://github.com/alliedmodders/hl2sdk.git --depth 1 -b css hl2sdk-css
git clone https://github.com/alliedmodders/metamod-source.git --depth 1 -b 1.11-dev
git clone https://github.com/alliedmodders/ambuild.git --depth 1

chmod -R 755 $_INSTALL_DIR/alliedmodders

cp -r --update=none sourcemod/public/libudis86 $_INSTALL_DIR/sigsegv-mvm/libs/udis86

cd ..
mkdir -p .venvs
python3 -m venv ./bin/python
python3 -m venv .venvs/ambuild
chmod -R 755 $_INSTALL_DIR/.venvs
chown -R wsluser:wsluser $_INSTALL_DIR/.venvs

.venvs/ambuild/bin/pip install alliedmodders/ambuild

#replace configure.py shebang line
sed -i '1s|^.*|#!/home/wsluser/.venvs/ambuild/bin/python|' $_INSTALL_DIR/sigsegv-mvm/configure.py

pathfile=$_INSTALL_DIR/.bashrc
pathvar='export PATH='$_INSTALL_DIR'/.venvs/ambuild/bin/:$PATH'

touch $pathfile
if ! grep -q -F -x "$pathvar" "$pathfile"; then
    echo "$pathvar" >> $pathfile
fi

cd $_INSTALL_DIR/sigsegv-mvm
git submodule init
git submodule update

chmod -R 755 $_INSTALL_DIR/sigsegv-mvm

cd libs/udis86
./autogen.sh
./configure --enable-static=yes
make clean
make CFLAGS="-m32" LDFLAGS="-m32"
mv libudis86/.libs/libudis86.a ../libudis86.a
make clean
make CFLAGS="-fPIC"
mv libudis86/.libs/libudis86.a ../libudis86x64.a
cd ../..

chmod -R 755 $_INSTALL_DIR/sigsegv-mvm/libs/udis86

cd $_INSTALL_DIR/sigsegv-mvm/libs
wget https://www.lua.org/ftp/lua-5.4.4.tar.gz

chmod -R 755 $_INSTALL_DIR/sigsegv-mvm/libs

tar -xf lua-*.tar.gz
rm lua-*.tar.gz
mv lua-* lua
cd lua
make CC=g++ MYCFLAGS='-m32' MYLDFLAGS='-m32'
mv src/liblua.a ../liblua.a
make clean
make CC=g++ MYCFLAGS="-fPIC"
mv src/liblua.a ../libluax64.a
cd ../..

chmod -R 755 $_INSTALL_DIR/sigsegv-mvm

chown -R wsluser:wsluser $_INSTALL_DIR/sigsegv-mvm

./autoconfig.sh

source /$_INSTALL_DIR/.bashrc
mkdir -p build/release
cd build/release
$_INSTALL_DIR/.venvs/ambuild/bin/ambuild

chmod -R 755 $_INSTALL_DIR/sigsegv-mvm/build

chown -R wsluser:wsluser $_INSTALL_DIR/sigsegv-mvm/build