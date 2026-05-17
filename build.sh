#!/bin/bash
set -e

# Connect to a docker environment and build the project
# with a linked tool chain. You can skip this if there is
# no need to build a new .rlf files.
docker run -v "$PWD":/home/os-dev/project \
  -w /home/os-dev/project \
  --privileged=true \
  --rm ghcr.io/j1sk1ss/os-dev-env:v02 \
  scons

# Build the final .img file with FAT32 file system at its start
# and grub2. Will only mount and unmount a new image without
# launch of the OS.
./build/scripts/grub2_build_image.sh

# Launch the OS
./build/scripts/qemu_run.sh
