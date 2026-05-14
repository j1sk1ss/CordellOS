#!/bin/bash
set -e

scons
./build/scripts/grub2_build_image.sh
./build/scripts/qemu_run.sh
