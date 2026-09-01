# AGENTS.md

Tool-neutral guidance for contributors and coding agents working in SpecLab.
Use `README.md` for the project overview and public API documentation.

## Toolchain

- CMake 4.0 or newer and C++23 are required.
- The minimum compiler versions enforced by this branch are MSVC 19.40,
  GCC 15, and Clang 20.
- `CMAKE_EXPERIMENTAL_CXX_IMPORT_STD` must remain set before `project()`:
  compiler detection has not populated `CMAKE_CXX_COMPILER_ID` at that point.
- Configure out of source; in-source builds are rejected.

## Build and verification

```bash
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release \
  -DSPECLAB_BUILD_EXAMPLES=ON -DSPECLAB_BUILD_TESTS=OFF
cmake --build build --parallel
./build/examples/basic_example
./build/examples/pulse_oximeter_example
```

The tests directory is currently disabled by the `AND FALSE` condition in the
root `CMakeLists.txt`; do not claim that `ctest` validates this branch. Only
`basic_example` and `pulse_oximeter_example` are wired into the build.

## Repository conventions

- Module sources live under `include/speclab/`. Add every new `.cppm` file to
  the root `target_sources(... FILE_SET cxx_modules ...)` list.
- Apply `.clang-tidy` manually. `.clang-format` is empty, so follow the written
  style: four-space indentation, `UpperCamelCase` types, and `lowerCamelCase`
  functions.
- Preserve the public compliance terminology, traceability model, and medical
  safety metadata described in `README.md`.
- `AGENTS.md` is the canonical shared instruction file. Keep tool-specific
  assistant settings local and ignored; do not add a second repository
  instruction source.
- Do not add assistant attribution to commits, PR descriptions, or comments.
