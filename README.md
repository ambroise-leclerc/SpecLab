# SpecLab - Medical Device Testing Framework

![Status](https://img.shields.io/badge/status-experimental-orange)
[![Version](https://img.shields.io/github/v/tag/ambroise-leclerc/SpecLab?label=version)](https://github.com/ambroise-leclerc/SpecLab/releases)
[![C++23](https://img.shields.io/badge/C%2B%2B-23%20modules-blue.svg)](https://en.cppreference.com/w/cpp/compiler_support)
[![CMake](https://img.shields.io/badge/CMake-4.0%E2%80%934.4-green.svg)](https://cmake.org/)
[![License: EUPL-1.2](https://img.shields.io/badge/License-EUPL--1.2-blue)](LICENSE)

[![Windows MSVC](https://img.shields.io/github/check-runs/ambroise-leclerc/SpecLab/develop?nameFilter=Windows%20MSVC%20%28windows-latest%2C%20x64%29&label=Windows%20MSVC)](https://github.com/ambroise-leclerc/SpecLab/actions/workflows/ci.yml?query=branch%3Adevelop)
[![Linux GCC 16.1](https://img.shields.io/github/check-runs/ambroise-leclerc/SpecLab/develop?nameFilter=Linux%20GCC%2016.1.0%20%28CMake%204.1.1%29&label=Linux%20GCC%2016.1)](https://github.com/ambroise-leclerc/SpecLab/actions/workflows/ci.yml?query=branch%3Adevelop)
[![Linux Clang 21](https://img.shields.io/github/check-runs/ambroise-leclerc/SpecLab/develop?nameFilter=Linux%20Clang%2021%20libc%2B%2B%20%28CMake%204.3.1%29&label=Linux%20Clang%2021)](https://github.com/ambroise-leclerc/SpecLab/actions/workflows/ci.yml?query=branch%3Adevelop)
[![macOS Apple Silicon](https://img.shields.io/github/check-runs/ambroise-leclerc/SpecLab/develop?nameFilter=macOS%2015%20arm64%20%28Clang%2021.1.8%2C%20libc%2B%2B%29&label=macOS%20Apple%20Silicon)](https://github.com/ambroise-leclerc/SpecLab/actions/workflows/ci.yml?query=branch%3Adevelop)

[![CI/CD](https://github.com/ambroise-leclerc/SpecLab/actions/workflows/ci.yml/badge.svg?branch=develop)](https://github.com/ambroise-leclerc/SpecLab/actions/workflows/ci.yml)
[![Medical Device Validation](https://github.com/ambroise-leclerc/SpecLab/actions/workflows/medical-validation.yml/badge.svg?branch=develop)](https://github.com/ambroise-leclerc/SpecLab/actions/workflows/medical-validation.yml)
[![Security](https://github.com/ambroise-leclerc/SpecLab/actions/workflows/security.yml/badge.svg?branch=develop)](https://github.com/ambroise-leclerc/SpecLab/actions/workflows/security.yml)
[![Release](https://github.com/ambroise-leclerc/SpecLab/actions/workflows/release.yml/badge.svg)](https://github.com/ambroise-leclerc/SpecLab/actions/workflows/release.yml)

SpecLab is a C++23 **named-modules** testing framework for medical device software. It offers
Given/When/Then tests without macros, parameterized and benchmark tests, medical-device metadata
(ISO 14971 risk level, IEC 62304 safety class), and a requirement registry that produces a
traceability matrix.

> **Experimental project.** SpecLab explores C++23 modules and `import std` on cutting-edge
> toolchains. Its compliance features are conceptual and educational: using SpecLab does not make
> software compliant with IEC 62304, ISO 13485 or ISO 14971, and it is not recommended for
> production use.

Its main consumer is [MduX](https://github.com/ambroise-leclerc/MduX), a C++23 modules UI library
for medical devices, which uses it as a test-only dependency for its Given/When/Then suites.

## Supported platforms

Every platform below is built and its examples run in CI (`.github/workflows/ci.yml`) on each
push and pull request.

| Platform | Compiler and standard library | CMake in CI | Configure with |
|---|---|---|---|
| Windows x64 / x86 | MSVC 17.10+ (`windows-2022`, `windows-latest`) | 4.1.1 | `-G Ninja` from a Developer prompt |
| Linux x64 | GCC 16.1.0, libstdc++ | 4.1.1, 4.3.1, 4.4.2 | `-G Ninja` |
| Linux x64 | Clang 21, libc++ | 4.3.1 | `--toolchain cmake/toolchains/linux-clang21-libcxx.cmake` |
| macOS Apple Silicon | upstream Clang 21.1.8, libc++ (`brew install llvm@21`) | 4.3.1 | `--toolchain cmake/toolchains/macos-arm64-llvm.cmake` |

Unsupported, and refused at configure time:
- **GCC 15** fails on libstdc++'s `std` module ([#9](https://github.com/ambroise-leclerc/SpecLab/issues/9)).
- **GCC 16.2.0** corrupts this tree's module BMIs ([#17](https://github.com/ambroise-leclerc/SpecLab/issues/17)). GCC 16.2.0 is not refused at configure time: its build fails.
- **AppleClang** has no `import std` module.
- **macOS x86_64 or universal (`arm64;x86_64`) targets** have never been tested. The check uses
  `CMAKE_OSX_ARCHITECTURES` when it is set.
- **CMake 4.5+** is refused until its `import std` gate has been qualified.

Clang needs libc++, because libc++ is the standard library whose `std` module CMake knows how to
build. The toolchain files select it and point CMake at `libc++.modules.json`.

## Quick start

```cpp
import std;
import speclab;

using speclab::core::Assertions;

int main() {
    // Steps are void() callables: state shared between Given, When and Then lives in an object
    // the lambdas capture, and must outlive Execute().
    struct State { int sum{0}; };
    auto state = std::make_shared<State>();

    auto result = speclab::Test("ADD-001")
        .Given("two operands", [state] { state->sum = 0; })
        .When("they are added", [state] { state->sum = 2 + 3; })
        .Then("the sum is 5", [state] { Assertions::assertEqual(5, state->sum, "2 + 3"); })
        .Execute();

    std::println("{}: {}", result.testId, speclab::core::toString(result.status));
    return result.passed() ? 0 : 1;
}
```

`Execute()` returns a `speclab::core::TestResult`. A failed assertion throws
`speclab::core::AssertionFailure`, which gives a `Failed` status with the message and source
location. Any other exception gives an `Error` status. The assertions are in
`speclab::core::Assertions`: `assertTrue`, `assertFalse`, `assertEqual`, `assertNotEqual`,
`assertNear`, `assertNull`, `assertNotNull`, `assertThrows`, `assertNoThrow`, `fail`, and the
medical-device ones `assertSafety`, `assertCompliance` and `assertPerformance`. Each one captures
its `std::source_location`.

All the examples in this README are compiled with `-Wall -Wextra -Werror` and run before they are
published.

### Benchmarks

```cpp
std::vector<double> samples;

auto result = speclab::Test("ECG-PERF")
    .Given("a buffer of samples", [&] { samples.assign(4096, 1.0); })
    // A When that returns the operation to time: it is run Benchmark(n) times.
    .When("the signal is filtered", speclab::PerformanceFunction{[&] {
        return std::function<void()>{[&] {
            for (auto& s : samples) { s *= 0.5; }
        }};
    }})
    // The Then receives the average duration.
    .Then("one pass takes less than 1 ms", [](std::chrono::nanoseconds average) {
        Assertions::assertPerformance(average, std::chrono::milliseconds(1), "filter pass");
    })
    .Benchmark(100)
    .Execute();
// result.metadata holds avg_duration_ns, min_duration_ns, max_duration_ns, benchmark_iterations.
```

### Parameterized tests

```cpp
struct Reading {
    double actual;
    double measured;
};

auto results = speclab::ParameterizedTest<Reading>("TEMP-ACCURACY", {
        {36.5, 36.52},  // normal
        {35.0, 34.95},  // hypothermia
        {38.0, 38.08},  // fever
    })
    .Then("the reading is within 0.1 °C", [](const Reading& r) {
        Assertions::assertNear(r.actual, r.measured, 0.1, "temperature");
    })
    .Execute();  // one TestResult per parameter: TEMP-ACCURACY[0], [1], [2]
```

### Medical-device tests

```cpp
using namespace speclab::medical;

bool alarmLatched = false;

// Keep the builder in a variable: Given/When/Then return the base TestBuilder&, and
// ExecuteMedical() is what adds the risk and compliance metadata to the result.
auto test = MedicalTest("ALARM-001", "MONITOR-01", "AlarmManager", RiskLevel::High);
test.SafetyClass(SafetyClass::ClassB)
    .SafetyCriticalAssertion("the alarm stays latched", [&] { return alarmLatched; });
test.Given("a monitor with the alarm armed", [&] { alarmLatched = false; })
    .When("SpO2 drops below the threshold", [&] { alarmLatched = true; })
    .Then("the alarm is raised", [&] { Assertions::assertSafety(alarmLatched, "missed desaturation alarm"); });

auto result = test.ExecuteMedical();  // result.riskLevel == "HIGH"
```

A failed `SafetyCriticalAssertion` gives a `Critical` status. The class-based API
(`speclab::core::TestCase`, `speclab::medical::MedicalTestCase`, `TestSuite`, `TestRunner`) is
shown in [`examples/basic_example.cpp`](examples/basic_example.cpp) and
[`examples/pulse_oximeter_example.cpp`](examples/pulse_oximeter_example.cpp), which CI builds and
runs on every platform.

## Requirements traceability

The requirement registry lives in `speclab.core.requirements` (`core/Requirements.cppm`) and is
exported by `import speclab;` in `speclab::core`.

```cpp
using namespace speclab::core;

RegisterRequirement({
    .id = "REQ-100",
    .description = "The system boots in less than 250 ms",
    .riskLevel = "HIGH",
    .safetyClass = "CLASS_B",
    .requiresAudit = true,
    .requiresValidation = true,
    .source = "SRS-Boot",
});

TestSuite suite("Boot");
suite.addTest("T_BootTime", [] { /* measure the boot time */ });
LinkTestRequirement("T_BootTime", "REQ-100");

auto results = suite.execute();
std::print("{}", ExportTraceMatrixCSV());
// RequirementID,RiskLevel,SafetyClass,RequiresValidation,RequiresAudit,TestIDs
// REQ-100,HIGH,CLASS_B,true,true,T_BootTime
```

A `Requirement` has:
- `id` and `description`;
- `riskLevel` (`LOW`, `MEDIUM`, `HIGH` or `CRITICAL`) and `safetyClass` (`CLASS_A`, `CLASS_B` or `CLASS_C`);
- `requiresAudit` and `requiresValidation`;
- `source` (an SRS section or a ticket key, for example) and `version` (`"1.0"` by default);
- a `metadata` map for anything else.

Use stable IDs (`REQ-###`, `ALARM-001`).

- **Coverage gate.** Every suite execution appends a synthetic `REQUIREMENT_COVERAGE` result,
  which is why the suite above reports two results for one test. That result is:
  - `Passed` when every HIGH or CRITICAL requirement has at least one linked test;
  - `Failed` when a HIGH requirement has none;
  - `Critical` when a CRITICAL requirement has none.

  Its `missing_requirements` metadata lists the uncovered IDs. Check it in CI to enforce
  coverage.
- **Risk-based execution.** `SetRequirementsConfig({.abortOnCriticalGaps = true, .riskBasedOrdering = true})`
  runs tests linked to CRITICAL and HIGH requirements first. It also stops the suite with a
  `REQUIREMENT_COVERAGE_PRE` result when a CRITICAL requirement has no test.
- **Reports and queries.**
  - `ExportTraceMatrixHTML()` produces an HTML matrix you can publish as a build artifact.
  - `HasUncoveredCriticalRequirements()` tells whether any CRITICAL requirement lacks a test.
  - `TestRiskScore(testId)` returns the derived risk weight, from 1 to 4.

## Using SpecLab in a CMake project

SpecLab is consumed as a C++ module library. The consuming target needs C++23 and `import std`,
and should be built with Ninja (see [Building SpecLab](#building-speclab)):

```cmake
# Pin a commit rather than a branch; the release tag is kept alongside as documentation.
include(FetchContent)
FetchContent_Declare(speclab
    GIT_REPOSITORY https://github.com/ambroise-leclerc/SpecLab.git
    GIT_TAG        <commit SHA of the release>  # e.g. the commit tagged v0.1.2
)
set(SPECLAB_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(speclab)

add_executable(my_tests tests.cpp)
target_link_libraries(my_tests PRIVATE speclab::speclab)
set_target_properties(my_tests PROPERTIES CXX_STANDARD 23 CXX_MODULE_STD ON)
```

After `cmake --install`, use `find_package(speclab CONFIG REQUIRED)` and the same
`speclab::speclab` target. Either way:
- **Your project must enable `import std` itself.** Set `CMAKE_EXPERIMENTAL_CXX_IMPORT_STD` before
  `project()`, to the value for your CMake series, as SpecLab's root `CMakeLists.txt` does.
- **SpecLab's own build settings stay out of your targets:** warnings, `-Werror`, definitions and
  cache entries.
- **In a sanitizer build, `-fsanitize=…` does reach your targets, on purpose.** Code that imports
  an instrumented module must be instrumented too, or GCC 16 fails with an internal compiler error.

## Building SpecLab

```bash
git clone https://github.com/ambroise-leclerc/SpecLab.git
cd SpecLab
cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Release -DSPECLAB_BUILD_EXAMPLES=ON
cmake --build build --parallel
./build/examples/basic_example
./build/examples/pulse_oximeter_example
```

Add `--toolchain cmake/toolchains/linux-clang21-libcxx.cmake` (Linux Clang) or
`--toolchain cmake/toolchains/macos-arm64-llvm.cmake` (macOS) as needed. Use Ninja. Makefile
generators do not support C++ modules, and Ninja is the generator CI verifies on every platform.
The Visual Studio generator works only with MSVC, through MSVC's own `std` module.

### CMake options

| Option | Default | Effect |
|---|---|---|
| `SPECLAB_BUILD_EXAMPLES` | `ON` | Build `basic_example` and `pulse_oximeter_example` |
| `SPECLAB_BUILD_TESTS` | `ON` | No effect yet: the self-tests are not implemented (see below) |
| `SPECLAB_WARNINGS_AS_ERRORS` | on when top-level | `-Werror` / `/WX` for SpecLab's own sources, clang-tidy and cppcheck |
| `ENABLE_SANITIZER_ADDRESS`, `…_UNDEFINED_BEHAVIOR`, `…_LEAK`, `…_THREAD`, `…_MEMORY` | `OFF` | Sanitizers (GCC/Clang) |
| `ENABLE_COVERAGE` | `OFF` | `--coverage` (GCC/Clang) |
| `ENABLE_CLANG_TIDY`, `ENABLE_CPPCHECK` | `OFF` | Static analysis during the build |
| `ENABLE_CACHE` | `ON` | Use ccache/sccache if found |
| `SPECLAB_PROJECT_VERSION` | release version | Version recorded in the package config |

## Current limitations

- **The `speclab::Requirement(…)`, `Feature(…)` and `IEC62304Process(…)` builders only record
  metadata.** Their `Execute()` runs no test and returns no result. For traceability that actually
  gates, use the registry described above (`RegisterRequirement`, `LinkTestRequirement`,
  `TestSuite`).
- **There are no self-tests yet.** The `tests/` directory is a stub and `ctest` runs nothing. CI's
  verification is building everything and running the two examples on every platform.
  `examples/functional_api_examples.cpp` and `examples/requirements_example.cpp` are not built.
- **No `ASSERT_*` macros.** A `#define` in a module interface is not exported by `import speclab;`,
  so call `speclab::core::Assertions::…` directly.

## Architecture

```
include/speclab/
├── core/
│   ├── TestResult.cppm          # TestResult, TestStatus, result collections
│   ├── Assertions.cppm          # Assertions, AssertionFailure
│   ├── TestCase.cppm            # Class-based test cases
│   ├── TestSuite.cppm           # Suites, parallel execution, coverage gate
│   ├── Requirements.cppm        # Requirement registry and traceability exports
│   ├── RequirementAPI.cppm      # Requirement / Feature / IEC62304Process builders (metadata only)
│   └── FunctionalAPI.cppm       # Test, ParameterizedTest, benchmarks
├── medical/
│   ├── MedicalTestCase.cppm     # RiskLevel, SafetyClass, MedicalTestCase
│   ├── ComplianceValidator.cppm # Compliance validation helpers
│   └── MedicalFunctionalAPI.cppm# MedicalTest, MedicalRequirement
├── reporters/
│   ├── Reporter.cppm            # Reporter interface
│   └── ConsoleReporter.cppm     # Console output
├── runners/
│   └── TestRunner.cppm          # Test execution engine
└── SpecLab.cppm                 # `import speclab;` aggregates every module above
```

The authoritative list is the `FILE_SET cxx_modules` in the root `CMakeLists.txt`. The only
dependency is the C++ standard library, through `import std`.

## Contributing

See [CONTRIBUTING.md](CONTRIBUTING.md) and [AGENTS.md](AGENTS.md). AGENTS.md records the toolchain
facts that are easy to get wrong: the `import std` gates, the compiler floors, what reaches
consumers, and the release procedure.

Code style, applied by hand because the committed `.clang-format` is empty:
- 4-space indentation, no tabs.
- Pointers and references are glued to the type: `int* ptr`, `const std::string& name`.
- `UpperCamelCase` for types, `lowerCamelCase` for functions and variables.
- `nullptr` rather than `NULL`, and `[[maybe_unused]]` rather than casting to `(void)`.
- Module files are named `UpperCamelCase.cppm`.
- Follow the C++ Core Guidelines. `.clang-tidy` is comprehensive: run it with
  `-DENABLE_CLANG_TIDY=ON`, or `clang-tidy -p build`.

Releases follow [CHANGELOG.md](CHANGELOG.md), and release tags are immutable.

## License

This project is available under the [European Union Public Licence 1.2](LICENSE), or under separate
commercial terms. See [LICENSING.md](LICENSING.md).

## Medical device notice

**IMPORTANT**: This framework is designed to assist in medical device software testing but does
not guarantee regulatory compliance by itself. Always consult regulatory experts and follow your
organization's quality management system procedures.

## Support

- **Issues**: [GitHub Issues](https://github.com/ambroise-leclerc/SpecLab/issues)
- **Releases**: [GitHub Releases](https://github.com/ambroise-leclerc/SpecLab/releases)

## Roadmap

- [x] Given/When/Then functional API, parameterized tests, benchmarks
- [x] Requirement registry, coverage gate, CSV/HTML traceability matrix
- [x] Windows, Linux (GCC and Clang) and macOS Apple Silicon in CI
- [ ] Self-tests run by `ctest` on every platform
- [ ] Requirement, Feature and IEC62304Process builders that execute their tests
- [ ] Soft (collecting) assertions, test registration and a `--list-tests` / `--run=` runner (currently implemented in MduX's `SpecLabBridge.hpp`)
- [ ] Visual test reporting dashboard
- [ ] Cryptographic audit trail signing
