# Getting Started

CordellOS targets i386-style development with a freestanding kernel and cross-toolchain. The recommended path is to use the prepared development container, because it already contains the expected compiler, assembler, image tools, and emulator.

## Requirements

- Docker or a compatible container runtime.
- A checkout of this repository.
- Privileged container access if you want to build bootable images with loop devices.

## Development Container

Run the OS development environment from the project root:

```bash
docker run -it \
  -v "$PWD":/home/os-dev/project \
  --privileged=true \
  --rm \
  ghcr.io/j1sk1ss/os-dev-env:v02
```

Inside the container, the cross toolchain is expected at:

```text
/home/os-dev/tool_chain
```

If you build outside the container, pass the toolchain path to SCons:

```bash
scons tool_chain=/path/to/tool_chain
```

## Repository Layout

| Path | Purpose |
| --- | --- |
| `src/kernel` | Kernel sources, linker script, architecture code, drivers |
| `src/kernel/include` | Kernel headers |
| `src/libs` | Freestanding support libraries for kernel and user programs |
| `apps` | Userland applications |
| `build` | Build helpers and generated image tree |
| `docs` | This docsify site |

## First Build

The shortest path is:

```bash
scons
```

The resulting kernel and image tree are produced under `build/CordellOS`.
