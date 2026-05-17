# Architecture

CordellOS is organized around a small freestanding kernel and a simple userland environment. The system favors direct, readable mechanisms over abstraction-heavy design.

## Boot And Kernel

The boot path loads the kernel into an i386 environment. The kernel is linked with a custom linker script and initializes core CPU and platform facilities:

- GDT and IDT setup.
- Interrupt and IRQ handling.
- Physical and virtual memory management.
- Paging.
- Tasking and user mode transitions.
- Syscalls.

## Storage

The storage path currently centers on ATA PIO and FAT filesystems.

- ATA driver reads and writes sectors.
- VFS wraps mounted devices.
- FAT support handles boot sector parsing, cluster chains, directories, and file operations.

## Userland

User programs are compiled as ELF binaries and placed under the generated home tree. The kernel can load and execute ELF programs from the filesystem.

Current example applications live under:

```text
apps/shell
apps/std/calc
```

## Graphics And Input

The kernel includes early graphics and device support:

- VGA and VBE paths.
- Keyboard and mouse drivers.
- Basic graphics support libraries.

## Networking

Networking work exists as a practical systems test area:

- RTL8139 driver.
- Ethernet frames.
- ARP.
- UDP.
- DHCP.

This is intentionally low-level. It is useful for testing memory, interrupts, DMA-adjacent driver behavior, packet formats, and user/kernel boundaries.
