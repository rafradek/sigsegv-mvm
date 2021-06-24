# sigsegv-mvm
gigantic, obese SourceMod extension library of sigsegv's TF2 mods (mostly MvM related)

# Tips

How to run a TF2 server on Windows using WSL: https://github.com/rafradek/sigsegv-mvm/wiki/Installing-on-Windows-with-WSL

# How to build

On Ubuntu 20.04:

1. Install packages:
```
autoconf libtool pip nasm libiberty-dev:i386 libelf-dev:i386 libboost-dev:i386 libbsd-dev:i386 libunwind-dev:i386 lib32stdc++-7-dev lib32z1-dev libc6-dev-i386 linux-libc-dev:i386 g++-multilib python2 python-is-python2
```

2. Init submodules:
```
git submodule init
git submodule update
cd lib/udis86
./autogen.sh
./configure
make
cd ../..
```

3. Install packages:
```
python-is-python3
```

4. Follow this guide, stop after finishing Downloading Source and Dependencies section. In checkout-deps.sh, you can remove mysql and sdks that are not tf2 and sdk2013 to save space : https://wiki.alliedmods.net/Building_SourceMod

5. Update autoconfig.sh with correct hl2sdk, metamod, sourcemod paths

6. Run autoconfig.sh

7. Build

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
