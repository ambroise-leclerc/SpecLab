# AGENTS.md

Compact, tool-neutral guidance for coding agents and contributors working in SpecLab. See `README.md`
for the design narrative and usage examples; this file records only facts that
are easy to get wrong or that contradict the docs.

## Toolchain (strict)

- CMake **4.0–4.4**, C++23, no extensions (`CMAKE_CXX_EXTENSIONS OFF`, set before `project()`).
  CMake 4.5+ is refused at configure time until its `import std` gate is added (see below).
- Compiler minimums enforced in `CMakeLists.txt`: MSVC 19.40+ (17.10), GCC 16.1+,
  Clang 20+. Other compilers abort configure. **GCC 15 is not supported** (see
  issue #9): it does not round-trip libstdc++'s `std` module through a
  second-level BMI, so `speclab.runners.testrunner` fails to compile.
  **GCC 16.2.0 is not usable either** (issue #17: it corrupts this tree's module
  BMIs); CI pins `gcc:16.1.0`.
- CI legs (`.github/workflows/ci.yml`): GCC 16.1.0 × CMake 4.1.1 / 4.3.1 / 4.4.2,
  Clang 21 + libc++ × CMake 4.3.1, macOS 15 arm64 with Clang 21.1.8 + libc++ × CMake 4.3.1,
  and MSVC on `windows-2022` (x64, x86) and `windows-latest` (x64). These mirror what MduX,
  the main consumer, builds with.
- Default build type is `RelWithDebInfo` (set in `cmake/StandardProjectSettings.cmake`,
  only when SpecLab is the top-level project).
- `import std` is uniform across compilers — there is no per-compiler path and no
  manually precompiled std module anywhere in the build:
  - `CMAKE_EXPERIMENTAL_CXX_IMPORT_STD` is set *before* `project()`, with the
    value for the running CMake series: `d0edc3af-4c50-42ea-a356-e2862fe7a444`
    (4.0–4.2), `451f2fe2-a8a2-47c3-bc32-94786d8fc91b` (4.3),
    `f35a9ac6-8463-4d38-8eec-5d6008153e7d` (4.4). Values come from each release's
    `Help/dev/experimental.rst`; add a series there and extend the CI matrix in the
    same diff. It cannot be guarded on `CMAKE_CXX_COMPILER_ID`: that variable is
    only populated by compiler detection inside `project()`.
  - Availability is checked with `23 IN_LIST CMAKE_CXX_COMPILER_IMPORT_STD`, and
    `CMAKE_CXX_MODULE_STD ON` (directory scope) gives every SpecLab target
    `CXX_MODULE_STD`. Do not test or link `__CMAKE::CXX23` by hand: it is an internal
    name, CMake 4.3 no longer creates it at configure time, and a manual link once
    leaked it into the install export set.
  - No `build_std_module` target exists — the old GCC one precompiled *header
    units* (`import <vector>;`), which is a different feature from `import std;`,
    and it was removed. Do not reintroduce a hand-rolled `std.pcm` for Clang
    either: it is a second definition of module `std`, and the `std.cppm` path is
    distro-specific.
  - **Clang** works with **libc++** only: configure with
    `--toolchain cmake/toolchains/linux-clang21-libcxx.cmake` (needs `clang-21`,
    `libc++-21-dev`, `libc++abi-21-dev` from apt.llvm.org; override the root with
    `SPECLAB_LLVM_ROOT`). The toolchain file selects `-stdlib=libc++` and points
    `CMAKE_CXX_STDLIB_MODULES_JSON` at `libc++.modules.json`. Same setup MduX uses.
  - **macOS**: Apple Silicon only, with upstream LLVM/Clang (Homebrew `llvm@21`, verified
    21.1.8) and libc++: `--toolchain cmake/toolchains/macos-arm64-llvm.cmake`
    (`SPECLAB_LLVM_ROOT` overrides the LLVM root). This is MduX's ADR-013 configuration.
    **AppleClang is refused** by `CMakeLists.txt` because it has no `import std` module surface.
    GCC on macOS is refused too, and so is any macOS target architecture other than arm64. The
    architecture check reads `CMAKE_OSX_ARCHITECTURES`, falling back to the processor when it is
    empty, so x86_64 and universal `arm64;x86_64` builds are rejected even on an Apple Silicon
    host. Another upstream Clang only gets a warning.

## Consumers (MduX and others)

- `speclab_options` and `speclab_warnings` are linked **privately** (and wrapped in
  `$<BUILD_INTERFACE:>`), so nothing from SpecLab's build — warnings, `-Werror`,
  `/permissive-`, `NOMINMAX`, `SPECLAB_*` definitions — reaches a target that
  links `speclab::speclab`. Keep it that way: a `PUBLIC` link here once injected
  `MDUX_VERSION_*=0.1.0` into every MduX test TU.
- Instrumented builds are the exception, and `enable_sanitizers(speclab_options speclab)`
  sets them up:
  - `-fsanitize=…` is **PUBLIC on `speclab`**, for both compiling and linking. A
    translation unit that does `import speclab;` compiles inline code from SpecLab's
    module interfaces as those interfaces were built. If the importer is not
    instrumented to match, GCC 16 fails with an internal compiler error
    (`expand_UBSAN_NULL`).
  - `--coverage` is compiled privately (through `speclab_options`) and only its link
    flag is exported.
  - Never route any of these through `speclab_options` alone: `BUILD_INTERFACE`
    would drop them from the installed package (`undefined reference to __asan_*`).
- In-tree targets (examples, tests) opt in explicitly by linking
  `speclab_options speclab_warnings`.
- `SPECLAB_WARNINGS_AS_ERRORS` defaults to `PROJECT_IS_TOP_LEVEL`: `-Werror` in
  SpecLab's own build, not when SpecLab is a subproject.
- Settings that write cache entries (`CMAKE_BUILD_TYPE`,
  `CMAKE_EXPORT_COMPILE_COMMANDS`) are applied only when top-level.

## Releases

- A release is a `vX.Y.Z` tag pushed on `develop`. `.github/workflows/release.yml`
  builds the packages and creates the GitHub release. The release notes are the
  `## [X.Y.Z]` section of `CHANGELOG.md` plus a fixed disclaimer. The workflow fails if
  that section is missing, so the CHANGELOG is also where the release text is written. Bump `SPECLAB_PROJECT_VERSION`'s
  default in `CMakeLists.txt` and add a `CHANGELOG.md` entry in a PR *before*
  tagging, so that the tagged sources declare their own version.
- **Tags are immutable**: never move or re-push a published tag, and ship a fix as a
  new patch release. MduX pins SpecLab by SHA and treats the tag as documentation;
  v0.1.0 was moved once and MduX recorded it as a supplier risk.
- Before tagging, dry-run the release workflow on `develop`
  (`gh workflow run release.yml --ref develop`). It builds every package but creates no
  release, because the publish jobs only run on a tag push.

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

- `SPECLAB_VERSION_*` and `SPECLAB_MEDICAL_DEVICE_COMPLIANCE=1` are defined by
  `configure_speclab_version` (see `cmake/CompilerSettings.cmake`), for SpecLab's
  own translation units only. Don't hardcode them from the docs.
- No `ASSERT_*`/`FAIL` macros exist: a `#define` in a module interface unit is not
  exported by `import speclab;`. Use `speclab::core::Assertions::…` directly.
- New `.cppm` modules must be added to the `target_sources` `FILE_SET` list in
  root `CMakeLists.txt` or they will not be compiled/exported.
- `*.mod` is `.gitignore`'d (Fortran convention), but C++ module BMI files
  (`.ifc`, `.module.json`) are also ignored — don't commit them.

## Repository guidance

- `AGENTS.md` is the canonical shared instruction file. Keep tool-specific assistant settings local
  and ignored; do not add a second repository instruction source.
- Commit messages, PR descriptions, and code comments contain no assistant attribution.
