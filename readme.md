# DOS C Project Template

This repository is a minimal template for building a DOS executable with Open Watcom using either a local toolchain or Docker.

## Project layout

```text
src/             C/C++ source files
bin/             Generated executable output
obj/             Generated object files
err/             Build logs and compiler output
Dockerfile       Container image for Open Watcom
build.sh         POSIX build script
build.ps1        PowerShell build script
build-docker.sh  Dockerized build entry point
readme.md        Project documentation
```

## Requirements

- Docker (recommended, easiest cross-platform option)
- Open Watcom if you want to build locally
- A C/C++ source file in `src/`

## Build locally

From the project root:

```bash
./build.sh
```

This looks for source files under `src/`, compiles them for DOS, and writes the output to `bin/program.exe` by default.

Environment overrides are supported:

```bash
SRC_DIR=./src BIN_DIR=./bin OBJ_DIR=./obj ERR_DIR=./err OUTPUT_EXE=myapp.exe ./build.sh
```

## Build with Docker

```bash
./build-docker.sh
```

The Docker helper builds the Open Watcom image if needed and runs the build script inside the container.

## PowerShell build

On Windows, you can also run:

```powershell
./build.ps1
```

The PowerShell script respects the same environment variables:

```powershell
$env:SRC_DIR = ".\src"
$env:BIN_DIR = ".\bin"
$env:OBJ_DIR = ".\obj"
$env:ERR_DIR = ".\err"
$env:OUTPUT_EXE = "myapp.exe"
./build.ps1
```

## Notes

- The default target is DOS.
- The output executable is named `program.exe` unless overridden.
- Errors are written to `err/build_errors.log` when compilation fails.
- The Docker flow is the safest option for a portable template repository because it avoids local toolchain differences.

## Example source

A minimal project entry point looks like this:

```c
#include <stdio.h>

int main(void) {
    puts("Hello, world!");
    return 0;
}
```

Place this into `src/main.c` and run the build script.
