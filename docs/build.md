# Build Guide

CordellOS uses SCons as the top-level build system. The build config selects the target architecture, image type, filesystem, and toolchain location.

## Basic Build

```bash
scons
```

Useful build variables:

```bash
scons config=debug
scons config=release
scons image_type=disk
scons image_type=floppy
scons image_file_system=fat32
scons tool_chain=/home/os-dev/tool_chain
```

## Toolchain

The current toolchain baseline is:

| Tool | Version |
| --- | --- |
| GCC | 11.2.0 |
| Binutils | 2.37 |
| NASM | Provided by the development image |
| CPL | 3.5, planned as a first-class build input |

The target compiler prefix is selected from the architecture. For `i686`, SCons expects tools such as:

```text
i686-elf-gcc
i686-elf-g++
i686-elf-ar
i686-elf-ranlib
```

## Kernel Build

The kernel build collects:

- C sources: `*.c`
- C++ sources: `*.cpp`
- NASM sources: `*.asm`
- CPL sources: `*.cpl` when CPL support is enabled

Kernel include paths include:

```text
src/kernel
src/kernel/include
src/libs/include
```

## Running

The repository contains helper scripts under `build/scripts` for image creation and QEMU workflows. The exact command depends on the image type and host environment, but the typical cycle is:

```bash
scons
./build/scripts/grub2_build_image.sh
./build/scripts/qemu_run.sh
```

When using loop devices or image mounting, run inside the privileged development container.
