# sigsegv-mvm
gigantic, obese SourceMod extension library of sigsegv's and rafradek's TF2/Source mods (mostly MvM related)
For other Source games, only optimize-only package is provided

# Tips

How to run a TF2 server on Windows using WSL: https://github.com/rafradek/sigsegv-mvm/wiki/Installing-on-Windows-with-WSL

# Features
### Optimize-Only Package
* Reduce server cpu usage by ~50%
* [A list of all configurable features](https://github.com/rafradek/sigsegv-mvm/blob/master/cfg/sigsegv_convars_optimize_only.cfg)
### No-MvM Package
* Reduce server cpu usage by ~50%
* Extra player slots
* Interact with SourceTV spectators
* Reduce the amount of networked entities and automatically remove disposable entities when the entity limit is met
* [Hundreds of new attributes](https://sigwiki.potato.tf/index.php/List_of_custom_attributes)
* [New entity inputs, outputs and keyvalues](https://sigwiki.potato.tf/index.php/Entity_Additions)
* [A list of all configurable features](https://github.com/rafradek/sigsegv-mvm/blob/master/cfg/sigsegv_convars_no_mvm.cfg)
### Full Package
* All features from above
* More popfile features documented in [demonstrative popfile](https://github.com/rafradek/sigsegv-mvm/blob/master/scripts/mvm_bigrock_sigdemo.pop) and [wiki](https://sigwiki.potato.tf/)
* [A list of all configurable features](https://github.com/rafradek/sigsegv-mvm/blob/master/cfg/sigsegv_convars.cfg)

# Installing
Download a package (optimize-only, no-mvm, or full) from releases and extract it into server tf directory. Edit cfg/sigsegv_convars.cfg to enable or disable features

# How to build

This extension requires gcc 13 to build. For TF2, special modified hl2sdk-tf2 is used from https://github.com/rafradek/hl2sdk-tf2

Ubuntu 20.04 docker image with gcc 13 and other dependencies already installed (skip to step 3): rafradek/ubuntu2004dev:latest 

On Ubuntu 24.04:

1. Add x86 architecture if not installed yet
```
dpkg --add-architecture i386
apt update
```

2. Install packages:
```
autoconf libtool pip nasm libiberty-dev libiberty-dev:i386 libelf-dev:i386 libboost-dev:i386 libbsd-dev:i386 libunwind-dev:i386 lib32z1-dev libc6-dev-i386 linux-libc-dev:i386 g++-multilib
```

3. Clone Sourcemod, Metamod, SDK repositories, and AMBuild
```
cd ..
mkdir -p alliedmodders
cd alliedmodders
git clone --recursive https://github.com/alliedmodders/sourcemod --depth 1 -b 1.11-dev
git clone https://github.com/alliedmodders/hl2sdk --depth 1 -b sdk2013 hl2sdk-sdk2013
git clone https://github.com/alliedmodders/hl2sdk --depth 1 -b tf2 hl2sdk-tf2
git clone https://github.com/alliedmodders/hl2sdk --depth 1 -b css hl2sdk-css
git clone https://github.com/alliedmodders/metamod-source --depth 1 -b 1.11-dev
git clone https://github.com/alliedmodders/ambuild --depth 1
```

4. Install AMBuild. Also add ~/.local/bin to PATH variable (Not needed if ambuild is installed as root)
```
pip install ./ambuild
echo 'export PATH=~/.local/bin:$PATH' >> ~/.bashrc
source ~/.bashrc
```

6. Init submodules:
```
cd ../sigsegv-mvm
git submodule init
git submodule update --depth 1
cd libs/udis86
./autogen.sh
./configure --enable-static=yes
make CFLAGS="-m32" LDFLAGS="-m32"
mv libudis86/.libs/libudis86.a ../libudis86.a
make clean
make CFLAGS="-fPIC"
mv libudis86/.libs/libudis86.a ../libudis86x64.a
cd ../..
```

7. Install lua:
```
cd libs
wget https://www.lua.org/ftp/lua-5.4.4.tar.gz
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
```

9. If hl2sdk, metamod, sourcemod directory is placed in a custom location, Update autoconfig.sh with correct paths

10. Run autoconfig.sh

11. Build

Release:
```
mkdir -p build/release
cd build/release
ambuild
```

Debug (libbsd-dev:i386 libunwind-dev:i386 is required to load the extension):
```
mkdir -p build
cd build
ambuild
```
Build output is created in the current directory

Build and create full, no-mvm, optimize-only packages (they can be found in build/release):
```
./multibuild.sh
```
