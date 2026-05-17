# CPL Toolchain

CPL is the custom language used by CordellOS experiments. It has an i386 backend and is being prepared as a first-class build input for kernel and toolchain testing.

## Intended Compiler Interface

The compiler binary should accept:

- Input files such as `*.cpl`.
- Include directories for source headers and imported files.
- A mode that emits assembly.
- A mode that compiles to object code.
- A mode that compiles and links.
- Explicit assembler and linker paths.
- Explicit assembler and linker flags.

## SCons Integration

CPL support is off by default so the normal C and assembly build continues to work without a CPL compiler installed.

Enable CPL sources with:

```bash
scons enable_cpl=1 cpl=/path/to/cpl
```

Use assembly as the intermediate output:

```bash
scons enable_cpl=1 cpl=/path/to/cpl cpl_object_mode=asm
```

Ask CPL to produce object files directly:

```bash
scons enable_cpl=1 cpl=/path/to/cpl cpl_object_mode=object
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
