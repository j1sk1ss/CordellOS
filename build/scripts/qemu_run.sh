#!/bin/bash
set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROOT_DIR="$(cd "$SCRIPT_DIR/../.." && pwd)"
cd "$ROOT_DIR"

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
    scons 
    "$SCRIPT_DIR/${build_command#./}"
fi

qemu_args=(-drive file=disk.img,format=raw)

if [[ -f build/.qemu-kernel-boot ]]; then
    qemu_args=(-kernel build/CordellOS/boot/kernel/kernel.elf "${qemu_args[@]}")
fi

if [[ -n "${QEMU_DISPLAY:-}" ]]; then
    qemu_args+=(-display "$QEMU_DISPLAY")
elif [[ -z "${DISPLAY:-}" ]]; then
    qemu_args+=(-display none -vnc 0.0.0.0:0)
    echo "No DISPLAY found; QEMU VNC display is available on port 5900."
fi

qemu-system-x86_64 "${qemu_args[@]}"
