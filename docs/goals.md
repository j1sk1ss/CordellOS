# Project Goals

CordellOS exists to make systems programming concrete.

## Education

The project is a learning surface for:

- Boot flow.
- CPU descriptors and interrupts.
- Paging and allocators.
- Filesystems.
- ELF loading.
- Device drivers.
- User/kernel boundaries.

The code is meant to be read, changed, broken, and repaired.

## Compiler And Tool Testing

CordellOS is also a target for toolchain experiments. A compiler that can generate working kernel code has to respect calling conventions, object formats, symbol visibility, register behavior, stack layout, and linker constraints.

That makes the OS a practical test rig for CPL and related tools.

## Hardware Reality

Emulators are convenient, but the project is shaped around the idea that code should eventually face hardware. Drivers, timing, memory layout, and boot media all become more honest when the system can be tested outside a purely hosted environment.

## Non-Goals

CordellOS is not trying to be a general-purpose desktop OS. It is not trying to hide low-level details. It is a compact operating system for learning, experimentation, and building confidence in the toolchain.
