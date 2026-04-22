# Getting Started

On Windows Visual Studio 2019/2022 is recommended. Clang is also supported via Ninja.

## <ins>**1. Downloading the repository:**</ins>

Start by cloning the repository with `git clone https://github.com/PrzodownikSW/Wandelt.git`.

## <ins>**2. Requirements:**</ins>

- **CMake** 3.25 or newer
- **MSVC** (Visual Studio 2019/2022) or **Clang** with **Ninja**
- **clang-format** and **clang-tidy** (optional, for code formatting and linting)

## <ins>**3. Generating solution:**</ins>

Run one of the setup scripts from the `scripts/` directory:

- `Windows (VS2019)` - Run [scripts/setup-vs2019.bat](scripts/setup-vs2019.bat)
- `Windows (VS2022)` - Run [scripts/setup-vs2022.bat](scripts/setup-vs2022.bat)
- `Windows (Clang + Ninja)` - Run [scripts/setup-clang.bat](scripts/setup-clang.bat)

This will generate all necessary projects and solution in the `build/` directory.

## <ins>**4. Building:**</ins>

Build via IDE or use the build scripts:

- [scripts/build-debug.bat](scripts/build-debug.bat) - No optimization, full debug symbols
- [scripts/build-release.bat](scripts/build-release.bat) - Full optimization, debug symbols
- [scripts/build-dist.bat](scripts/build-dist.bat) - Max optimization, LTO, no symbols (distribution)

Output binaries go to the `bin/` directory.


## <ins>**5. Other scripts:**</ins>

- [scripts/clean.bat](scripts/clean.bat) - Delete `build/` and `bin/` directories
- [scripts/coverage.bat](scripts/coverage.bat) - Build an instrumented coverage configuration, run the test suite, and generate LLVM coverage reports
- [scripts/run-clang-format.bat](scripts/run-clang-format.bat) - Format all source files

## <ins>**Documentation:**</ins>

- [docs/spec.md](docs/spec.md) - Language spec draft
- [docs/coding_standards.md](docs/coding_standards.md) - Coding standards
