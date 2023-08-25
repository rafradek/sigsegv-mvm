#!/bin/bash
cd /mnt/sigsegv-mvm
dpkg --add-architecture i386
apt update
apt install -y autoconf libtool pip nasm libiberty-dev:i386 libelf-dev:i386 libboost-dev:i386 libbsd-dev:i386 libunwind-dev:i386 lib32stdc++-7-dev lib32z1-dev libc6-dev-i386 linux-libc-dev:i386 g++-multilib
apt install -y git wget curl
apt install -y nano

# apt install p7zip-full git ca-certificates build-essential g++-multilib -y --no-install-recommends
# lib32stdc++-10-dev lib32z1-dev libc6-dev-i386 linux-libc-dev:i386
# cd /mnt/rafmod/ || exit
git config --global --add safe.directory "*"

rm -rf build
mkdir build
cd build

# mkdir build/scripting   -p
# mkdir build/plugins     -p

# cd ..
mkdir -p alliedmodders
cd alliedmodders
git clone --recursive https://github.com/alliedmodders/sourcemod --branch 1.11-dev
git clone --mirror https://github.com/alliedmodders/hl2sdk hl2sdk-proxy-repo
git clone hl2sdk-proxy-repo hl2sdk-sdk2013 -b sdk2013
git clone hl2sdk-proxy-repo hl2sdk-tf2     -b tf2
git clone https://github.com/alliedmodders/metamod-source -b 1.11-dev
git clone https://github.com/alliedmodders/ambuild


pip install ./ambuild
echo 'export PATH=~/.local/bin:$PATH' >> ~/.bashrc
source ~/.bashrc


apt install -y python2 python-is-python2

cd /mnt/sigsegv-mvm
git submodule init
git submodule update
cd libs/udis86
./autogen.sh
./configure
make
cd ../..

cd libs
wget https://www.lua.org/ftp/lua-5.4.4.tar.gz
tar -xf lua-*.tar.gz
rm lua-*.tar.gz
mv lua-* lua
cd lua
make MYCFLAGS='-m32' MYLDFLAGS='-m32'
cd ../..

apt install -y python-is-python3

cd /mnt/sigsegv-mvm
bash autoconfig_docker.sh

cd build/release/optimize-only
ambuild
bash
