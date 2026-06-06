# CPL Toolchain

CPL is the custom language used by CordellOS experiments. It has an i386 backend and is being prepared as a first-class build input for kernel and toolchain testing.

## Compiler Interface

The current compiler is available in the development environment as:

```text
build/cplc
```

It reports itself as `ccpl 3.5`. The useful options for CordellOS are:

- Input files such as `*.cpl`.
- Include directories with `-I <dir>`.
- Assembly output with `--emit-asm`.
- Stopping after assembly output with `--no-compile`.
- Assembly output naming with `--asm-output <file>`.
- i386 target selection with `--arch i386 --sys-type i386`.
- Output naming with `--output <file>` for compile/link outputs.
- Assembler selection with `--asm-compiler <tool>`.
- Assembler format selection with `--asm-format elf32`.
- Linker selection with `--linker <tool>`.

The SCons integration uses `--emit-asm --no-compile --asm-output <target>` for kernel CPL sources.

## SCons Integration

CPL support is enabled by default because several low-level i386 routines are now written in `.cpl`.

The default build uses:

```bash
scons
```

Use assembly as the intermediate output:

```bash
scons enable_cpl=1 cpl=build/cplc cpl_object_mode=asm
```

By default, the build asks CPL for assembly, then lets the normal NASM path assemble it. This keeps CPL integration close to the rest of the kernel build and avoids depending on CPL's own assembler/linker stage for kernel objects.

The default CPL flags are:

```bash
--arch i386 --sys-type i386
```

The build passes kernel include directories to CPL, including:

```text
src/kernel
src/kernel/include
src/libs/include
```

## Why CPL Belongs Here

An operating system is a useful compiler target because it exercises code generation in hostile conditions:

- No hosted standard library.
- Strict ABI expectations.
- Inline assembly and privileged instructions.
- Linker-script-controlled memory layout.
- Interrupts, syscalls, and user/kernel transitions.
- Real hardware and emulator differences.

CordellOS is small enough to understand, but real enough to punish vague compiler assumptions.
