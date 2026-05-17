# CordellOS

CordellOS is a small educational operating system for x86 machines. It is built to make low-level systems work visible: boot, interrupts, memory, filesystems, user programs, networking, and the tooling needed to compile code for bare metal.

The project is also a hardware-facing test bed for CPL, a custom language with an i386 backend. CordellOS gives the compiler a real target: not only "does this compile?", but "does the generated code run inside a kernel, cross boundaries, and survive real machine constraints?"

## What It Is For

- Learning operating system internals without hiding the sharp edges.
- Testing compilers, assemblers, linkers, and cross-toolchain behavior.
- Running experiments on emulators and real x86 hardware.
- Keeping a compact kernel where changes can still be understood end to end.

## Current Shape

CordellOS includes a freestanding kernel, boot image generation, a FAT-backed virtual filesystem, ELF loading, user programs, paging, tasking, interrupts, VBE graphics work, input devices, and early networking pieces.

It is not a production operating system. It is a workshop: useful because the machinery is exposed.

## Documentation Map

- [Getting Started](getting-started.md): quick orientation.
- [Build Guide](build.md): container and SCons build flow.
- [Architecture](architecture.md): boot, kernel, userspace, and drivers.
- [CPL Toolchain](cpl.md): planned CPL compiler integration.
- [Project Goals](goals.md): why this project exists.
