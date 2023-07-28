#!/bin/bash



# we do this so that we can be agnostic about where we're invoked from

# meaning you can exec this script anywhere and it should work the same

thisiswhereiam=${BASH_SOURCE[0]}

# this should be /whatever/directory/structure/Open-Fortress-Source

script_folder=$( cd -- "$( dirname -- "${thisiswhereiam}" )" &> /dev/null && pwd )



# this should be /whatever/directory/structure/[sdkmod-source]/build

build_dir="build"

img="ubuntu:20.04"
echo ${thisiswhereiam}
echo ${script_folder}

podman run -it \
--mount type=bind,source=${script_folder}/../,target=/mnt/sigsegv-mvm \
${img} \
bash /mnt/sigsegv-mvm/ci/_docker_script.sh
