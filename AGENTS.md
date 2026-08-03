# AGENTS.md

Compact guidance for OpenCode sessions working in SpecLab. See `README.md`
for the design narrative and usage examples; this file records only facts that
are easy to get wrong or that contradict the docs.

## Toolchain (strict)

- CMake **4.0+**, C++23, no extensions (`CMAKE_CXX_EXTENSIONS OFF`).
- Compiler minimums enforced in `CMakeLists.txt`: MSVC 19.40+ (17.10), GCC 15+,
  Clang 20+. Other compilers abort configure.
- Default build type is `RelWithDebInfo` (set in `cmake/StandardProjectSettings.cmake`).
- `import std` is handled per-compiler — do not assume a uniform path:
  - **MSVC**: `CMAKE_EXPERIMENTAL_CXX_IMPORT_STD` UUID
    `d0edc3af-4c50-42ea-a356-e2862fe7a444` is required. CI passes it on the
    command line; `CMakeLists.txt` sets it internally too.
  - **GCC**: `CMakeLists.txt` deliberately leaves `CMAKE_EXPERIMENTAL_CXX_IMPORT_STD`
    unset (synthesizing `__CMAKE::CXX23` references nonexistent `std.cc` on some
    libstdc++ setups). Instead a custom `build_std_module` target precompiles
    system headers with `-fmodules-ts`; `speclab` depends on it.
  - **Clang**: experimental flag is left empty; Clang is not currently building
    cleanly (CI's clang job is disabled due to libc++/`import std` cyclic deps).

## Build / verify commands

Configure from a `build/` subdir (in-source builds are blocked by
`cmake/PreventInSourceBuilds.cmake`):

```bash
cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Release \
  -DSPECLAB_BUILD_EXAMPLES=ON -DSPECLAB_BUILD_TESTS=ON
cmake --build build --parallel
```

Run the only verifiable artifacts (example binaries):

```bash
./build/examples/basic_example
./build/examples/pulse_oximeter_example
```

CI runs exactly these two executables as its "test" step.

### Tests are effectively disabled

Despite `README.md` saying `ctest` runs tests, `CMakeLists.txt` gates the
`tests/` subdirectory with `if(SPECLAB_BUILD_TESTS AND FALSE)` and
`tests/CMakeLists.txt` is a stub (`# Tests will be added here`). There is no
test target today. **Do not claim `ctest` validates anything; no tests are
registered.** When adding real tests, drop the `AND FALSE` guard and populate
`tests/CMakeLists.txt`. Also note CI runs with `-DSPECLAB_BUILD_TESTS=OFF`.

### Examples that exist but are NOT built

`examples/functional_api_examples.cpp` and `examples/requirements_example.cpp`
are present in the tree but `examples/CMakeLists.txt` only wires
`basic_example` and `pulse_oximeter_example`. If you edit or run the other two,
add them to `examples/CMakeLists.txt` first; otherwise they silently rot.

## Code quality tools — actual state

- `.clang-tidy` exists and is comprehensive (187 lines) — apply it manually.
- `.clang-format` is an **empty 0-byte file**; `clang-format` will fall back to
  defaults. Do not rely on it for the style described in `README.md`. Follow
  the written conventions (4-space indent, pointer/reference glued to type,
  `UpperCamelCase` types, `lowerCamelCase` functions) by hand.
- Sanitizers are opt-in via `ENABLE_SANITIZER_{ADDRESS,LEAK,UNDEFINED_BEHAVIOR,
  THREAD,MEMORY}` and `ENABLE_COVERAGE` (GCC/Clang only); see
  `cmake/Sanitizers.cmake`.
- `CMAKE_EXPORT_COMPILE_COMMANDS` is always on for clang-based tooling.

## Module layout — trust the build, not the docs

`README.md`'s module tree is incomplete. The authoritative file list is
`target_sources(... FILE_SET cxx_modules ...)` in `CMakeLists.txt`. Modules
actually present under `include/speclab/`:

- `core/`: `TestResult`, `Assertions`, `TestCase`, `TestSuite`, `Requirements`,
  `RequirementAPI`, `FunctionalAPI`
- `medical/`: `MedicalTestCase`, `ComplianceValidator`, `MedicalFunctionalAPI`
- `reporters/`: `Reporter`, `ConsoleReporter`
- `runners/`: `TestRunner`
- `SpecLab.cppm` (top-level aggregator)

Do not invent imports from `speclab.cppm`, `validators/`, or `utils/` — none
of these exist in the repo.

## Requirements/traceability API (Phases 1–3 in `README.md`)

Lives in `speclab.core.requirements` (module `core/Requirements.cppm` /
`core/RequirementAPI.cppm`). Key free functions: `RegisterRequirement`,
`LinkTestRequirement`, `ExportTraceMatrixCSV`, `ExportTraceMatrixHTML`,
`SetRequirementsConfig`, `TestRiskScore`, `HasUncoveredCriticalRequirements`.
Suite execution auto-appends synthetic `REQUIREMENT_COVERAGE` (and, when
`abortOnCriticalGaps` is set, `REQUIREMENT_COVERAGE_PRE`) results — these are
framework-emitted, not tests you register.

## Conventions that bite

- `MDUX_MEDICAL_DEVICE_COMPLIANCE=1` is defined by
  `configure_medical_compliance` (see `cmake/CompilerSettings.cmake`). Don't
  hardcode it from the docs.
- New `.cppm` modules must be added to the `target_sources` `FILE_SET` list in
  root `CMakeLists.txt` or they will not be compiled/exported.
- `*.mod` is `.gitignore`'d (Fortran convention), but C++ module BMI files
  (`.ifc`, `.module.json`) are also ignored — don't commit them.