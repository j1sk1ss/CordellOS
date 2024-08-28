#!/bin/bash

while [[ "$#" -gt 0 ]]; do
    case $1 in
        -r) 
            scons -c 
            ;;
        --limine) 
            build_command="./limine_build_image.sh"
            ;;
        --grub) 
            build_command="./grub2_build_image.sh"
            ;;
        *) 
            echo "Unknown arg: $1"
            exit 1
            ;;
    esac
    shift
done

if [[ -n "$build_command" ]]; then
    cd ..
    cd ..
    scons 
    cd build/scripts
    $build_command
fi

sudo qemu-system-x86_64 -hda disk.img

