# Changelog

All notable changes to SpecLab are recorded here.

**Release tags are immutable.** A published `vX.Y.Z` tag is never moved or re-pointed. A fix to
a release is a new patch release. Consumers such as MduX pin SpecLab by commit SHA and use the tag
name as documentation, and a tag that moves would silently change what that name refers to. That
happened once, to v0.1.0 (see below).

## [0.1.2] - 2026-09-19

### Added
- macOS support on Apple Silicon, in the configuration MduX verified in its ADR-013: upstream
  LLVM/Clang 21.1.8 with libc++ (Homebrew `llvm@21`), through
  `cmake/toolchains/macos-arm64-llvm.cmake`, and a `macos-15` arm64 CI job.
- AppleClang is now refused at configure time with a message pointing to that toolchain. It used
  to get only a warning and then fail later on a missing `std` module. On macOS, Intel and non-Clang
  compilers are refused as well, and an upstream Clang other than 21.1.8 gets a warning that it is
  untested.

### Changed
- README rewritten against the actual API. Every code example is compiled with
  `-Wall -Wextra -Werror` and run before publication. It has per-platform CI badges, a supported
  platforms table, the real CMake options and target name (`speclab::speclab`), consumption with
  FetchContent pinned by SHA, and a "Current limitations" section. Examples of APIs that do not
  exist (`Assert::…`, `.Parameters(…)`, `.TraceabilityMatrix(…)`) are gone. The
  `Requirement`/`Feature`/`IEC62304Process` builders are documented as metadata-only, since their
  `Execute()` runs no test.

### Fixed
- Sanitizer builds of consumers that do `import speclab;`. v0.1.1 compiled SpecLab with
  `-fsanitize=...` but no longer passed that flag to consumers. With GCC 16, a translation unit
  that imports an instrumented SpecLab module without being instrumented itself fails with an
  internal compiler error (`in expand_UBSAN_NULL, at internal-fn.cc`). MduX's ASan+UBSan job
  failed this way. `-fsanitize=...` is now PUBLIC on `speclab`, for compiling and linking, in the
  build tree and in the installed package. `--coverage` is unchanged: it is compiled privately and
  only its link flag is exported.

## [0.1.1] - 2026-09-19

Build and consumer-compatibility release, with no change to the test API. After this release MduX
no longer needs its SpecLab compatibility patch (`cmake/patches/speclab-cmake-4.3.patch`).

### Added
- CMake 4.3 and 4.4 support for `import std`. `CMAKE_EXPERIMENTAL_CXX_IMPORT_STD` is selected
  per CMake series (4.0–4.2, 4.3, 4.4), and 4.5+ is refused until someone qualifies it.
- Clang support with libc++ through `cmake/toolchains/linux-clang21-libcxx.cmake`, with a CI job
  on Clang 21.
- CI legs: GCC 16.1.0 with CMake 4.1.1, 4.3.1 and 4.4.2; MSVC on `windows-latest`.

### Changed
- `speclab_options` and `speclab_warnings` are linked privately. SpecLab's warnings, `-Werror`,
  `/permissive-`, `NOMINMAX` and definitions no longer reach targets that link
  `speclab::speclab`. The link flags of sanitizer and coverage builds are still exported,
  including in the installed package.
- Definitions renamed from `MDUX_*` to `SPECLAB_*`. `WARNINGS_AS_ERRORS` is renamed to
  `SPECLAB_WARNINGS_AS_ERRORS` and defaults to on only when SpecLab is the top-level project.
  clang-tidy and cppcheck follow it too.
- As a subproject, SpecLab no longer writes the consumer's `CMAKE_BUILD_TYPE`,
  `CMAKE_EXPORT_COMPILE_COMMANDS` or `SPECLAB_PROJECT_VERSION` cache entries.
- `import std` availability is detected through `CMAKE_CXX_COMPILER_IMPORT_STD` and enabled
  through `CXX_MODULE_STD`. The build no longer links `__CMAKE::CXX23` by hand.
- CI actions `lukka/get-cmake` and `actions/checkout` are pinned by commit SHA.

### Fixed
- SpecLab's `cmake/` helpers are included by path. When SpecLab was built as a subproject, a
  consumer's module of the same name was loaded instead. Inside MduX this stamped
  `MDUX_VERSION_*=0.1.0` onto MduX's own test targets.

### Removed
- The `ASSERT_*` / `FAIL` macros in `Assertions.cppm`. They were never reachable through
  `import speclab;`, because a `#define` in a module interface is not exported.
- Leftover MduX/MddLog build files (Vulkan definitions, `mddlogConfig.cmake.in`).

### Known issues
- GCC 16.2.0 cannot read this tree's module BMIs (#17). Use GCC 16.1.0.

## [0.1.0] - 2026-08-12

Initial release: BDD test cases and suites, medical test cases and compliance validation,
requirements traceability, console reporting, parallel execution, and examples.

The `v0.1.0` tag was first published on `ea7f2e3` and then moved to `37b51fc` to include a
release-packaging fix (#18). The immutable-tag rule above was adopted because of this.
