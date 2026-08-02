# Copilot Instructions for SpecLab

## Project Overview

SpecLab is a C++23 modules-based testing framework for medical devices. It provides BDD testing with built-in compliance for IEC 62304, ISO 13485, and ISO 14971 through audit trails, risk classification, and requirement traceability.

## Build Commands

**Prerequisites:** CMake 4.0+, and one of: MSVC 17.14+, GCC 16+, or Clang 20+. GCC 15 cannot build this project (see issue #9): it does not round-trip libstdc++'s `std` module through a second-level BMI, so compiling `speclab.runners.testrunner` fails. CI builds on GCC 16 only.

```bash
# Configure and build
cmake -B build -DSPECLAB_BUILD_EXAMPLES=ON -DSPECLAB_BUILD_TESTS=ON
cmake --build build

# Run tests (currently disabled in CMakeLists.txt)
cd build && ctest

# Format code
find . -name "*.cpp" -o -name "*.cppm" | xargs clang-format -i

# Static analysis
clang-tidy -p build include/**/*.cppm
```

Tests are currently disabled in the root `CMakeLists.txt` (`if(SPECLAB_BUILD_TESTS AND FALSE)`). When re-enabled, use `ctest` from the build directory.

## Architecture

### C++23 Modules — Zero Dependencies

The entire framework uses C++23 modules (`export module`) with only `import std` as a dependency. There are no `#include` directives or macro-based APIs by design.

### Module Dependency Chain

The build order matters — modules must be compiled in dependency order:

1. `speclab.core.testresult` — Base result types, `TestStatus` enum, `TestResult` struct
2. `speclab.core.assertions` — Assertion framework with `AssertionFailure` exception
3. `speclab.core.testcase` — Abstract `TestCase` base, `FunctionTestCase`, `ParameterizedTestCase`, `BenchmarkTestCase`
4. `speclab.core.testsuite` — `TestSuite` container with `TestResultCollection`
5. `speclab.core.functionalapi` — BDD fluent builders: `Test()`, `ParameterizedTest()`
6. `speclab.core.requirementapi` — `Requirement()`, `Feature()`, `IEC62304Process()` builders
7. `speclab.core.requirements` — `RequirementRegistry` singleton, traceability matrix, coverage gate
8. `speclab.medical.*` — Medical test cases, compliance validator, medical functional API
9. `speclab.reporters.*` / `speclab.runners.*` — Output and execution
10. `speclab` (SpecLab.cppm) — Re-exports everything under `SpecLab::` namespace

All module files live under `include/speclab/` and are registered in the root `CMakeLists.txt` via `FILE_SET cxx_modules`.

### Two API Styles

**Class-based:** Subclass `TestCase` or `MedicalTestCase`, override `run()`.

**Functional BDD (recommended):** Fluent builder with `Given`/`When`/`Then` lambdas:
```cpp
speclab::Test("TEST_ID")
    .Given("precondition", []() { /* setup */ })
    .When("action", []() { /* act */ })
    .Then("expected outcome", []() { /* assert */ })
    .Execute();
```

### Requirements Traceability System

Requirements are registered globally via `RegisterRequirement()` and linked to tests via `LinkTestRequirement()`. After suite execution:
- `TestResult.requirementIds` is auto-populated from links
- A synthetic `REQUIREMENT_COVERAGE` result is appended (Passed/Failed/Critical based on uncovered HIGH/CRITICAL requirements)
- `ExportTraceMatrixCSV()` and `ExportTraceMatrixHTML()` generate reports
- `SetRequirementsConfig({.abortOnCriticalGaps=true, .riskBasedOrdering=true})` enables risk-weighted ordering and pre-execution abort

### Medical Compliance Layer

- **Risk levels** (ISO 14971): `RiskLevel::Low`, `Medium`, `High`, `Critical`
- **Safety classes** (IEC 62304): `SafetyClass::ClassA`, `ClassB`, `ClassC` — ClassB/C auto-enable audit trails
- **Specialized assertions**: `assertSafetyCritical()`, `assertComplianceRequirement()`, `assertMedicalPerformance()`
- `MedicalTestContext` carries device ID, component name, risk level, safety class, and compliance standard

### CMake Module Setup

C++23 `import std` is handled by CMake via `CMAKE_EXPERIMENTAL_CXX_IMPORT_STD` (a UUID tied to the CMake release), set unconditionally before `project()`. CMake then synthesises the `__CMAKE::CXX23` target for every supported compiler and links it into `speclab`. There is **no** manually precompiled std module fallback — the previous GCC `build_std_module` target precompiled *header units* (`import <vector>;`), not `import std;`, so it never provided what it claimed; it is gone. A manually precompiled Clang `std.pcm` path is deliberately not added either (it would be a second definition of module `std` alongside `__CMAKE::CXX23`, and the `std.cppm` path is distro-specific).

When adding a new `.cppm` file, add it to `target_sources(speclab ... FILE_SET cxx_modules ...)` in the root `CMakeLists.txt` in the correct position respecting the dependency chain.

## Conventions

### Naming

| Element | Style | Example |
|---|---|---|
| Classes/structs/enums | `UpperCamelCase` | `TestResult`, `SafetyClass` |
| Functions/methods/variables | `lowerCamelCase` | `addResult()`, `testId` |
| Namespaces | `lower_case` | `speclab::core`, `speclab::medical` |
| Module files | `UpperCamelCase.cppm` | `TestResult.cppm` |
| Source files (examples/tests) | `UpperCamelCase.cpp` or `snake_case.cpp` | `basic_example.cpp` |
| Enum values | `UpperCamelCase` in code, `lowerCamelCase` in clang-tidy | `TestStatus::Passed` |

### Code Style

- 4-space indentation, no tabs
- Pointer/reference symbols next to type: `int* ptr`, `const std::string& name`
- `nullptr` instead of `NULL`
- Doxygen with `@brief`, `@param`, `@return`
- No macros — use C++23 features (`constexpr`, `concepts`, modules)

### Medical Device Patterns

- Requirement IDs use canonical prefixes: `REQ-###`, `SRS-###`, or domain prefixes (`ALARM-001`)
- Test IDs are short uppercase identifiers: `BASIC_001`, `MED_001`, `PERF_001`
- `TestStatus::Critical` is reserved for safety/compliance failures, distinct from `Failed`
- All `MedicalTestCase` subclasses must provide a `MedicalTestContext` with device ID, component name, risk level, and safety class

## Commit Policy

- Never commit LLM instruction files, AI configuration files, or AI assistant context files (e.g., `CLAUDE.md`, `AGENTS.md`, `.cursorrules`, `.windsurfrules`, `copilot-instructions.md`, etc.) to the repository.
- Commit messages and code comments must not mention AI, LLMs, Copilot, or any AI assistant. All contributions should read as if written entirely by a human developer.
