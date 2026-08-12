# SpecLab - Medical Device Testing Framework

[![C++23](https://img.shields.io/badge/C%2B%2B-23-blue.svg)](https://en.cppreference.com/w/cpp/compiler_support)
[![CMake](https://img.shields.io/badge/CMake-4.0+-green.svg)](https://cmake.org/)
[![License](https://img.shields.io/badge/License-EUPL--1.2-blue.svg)](LICENSE)
[![Medical Device](https://img.shields.io/badge/Medical%20Device-IEC%2062304-red.svg)](https://www.iso.org/standard/64686.html)

⚠️ EXPERIMENTAL PROJECT WARNING

This project is an experimental early evaluation of C++23 modules feasibility for cross-platform development with rich dependencies (Vulkan graphics, medical device compliance frameworks). It represents an attempt to leverage C++23 and emerging C++26 safety evolutions for medical device software development.

Current Status:

C++23 modules support requires cutting-edge toolchains (GCC 16.1+, MSVC 17.14+, Clang 20+)
CMake 4.x+ experimental support for import std;
Cross-platform compatibility still evolving
Medical device compliance framework is conceptual/educational
Exploring modern C++ safety features for medical device reliability
Not recommended for production use. This project serves as a technical proof-of-concept for modern C++ module systems and safety evolutions in complex, regulated software environments.

A modern C++23 modules-based testing framework specifically designed for medical devices. SpecLab provides BDD (Behavior-Driven Development) testing capabilities with built-in compliance for IEC 62304, ISO 13485, and ISO 14971 standards through structured test execution, audit trails, and medical device-specific features.

## Why SpecLab?

Medical device software requires rigorous testing with full traceability, risk management, and regulatory compliance. SpecLab bridges the gap between modern C++ testing practices and medical device regulatory requirements.

### Key Features

- ** Medical Device Focused**: Built-in IEC 62304, ISO 13485, and ISO 14971 compliance
- ** BDD Syntax**: Readable Given-When-Then structure for stakeholders
- ** Requirement Traceability**: Direct linking between requirements and tests
- ** C++23 Modern**: Zero dependencies except `import std`, full modules support
- ** Safety Critical**: Specialized assertions for safety-critical systems
- ** Risk-Based Testing**: Automatic risk classification and audit trails
- ** Zero Macros**: Clean C++23 code without preprocessor magic
- ** Functional API**: Lambda-based tests without class inheritance

## Quick Start

### Functional BDD API (Recommended)

```cpp
import speclab;

// Simple test with requirement traceability
SpecLab::Requirement("SRS-001", "Patient monitor shall display vital signs within 2 seconds")
    .Test("DISPLAY_RESPONSE")
        .Given("Patient monitor is connected", []() {
            monitor.connect();
            Assert::IsTrue(monitor.isConnected());
        })
        .When("Vital signs data is received", []() {
            monitor.receiveVitalSigns(mockPatientData);
        })
        .Then("Display updates within 2 seconds", []() {
            Assert::IsTrue(monitor.isDisplaying());
            Assert::LessThan(monitor.getResponseTime(), std::chrono::seconds(2));
        })
    .Execute();

// Safety-critical testing with automatic compliance
SpecLab::Requirement("SRS-002", "Emergency stop shall halt all operations")
    .SafetyClass(SafetyClass::ClassC)
    .RiskLevel(RiskLevel::Critical)
    .Test("EMERGENCY_STOP")
        .Given("Device is operating normally", []() {
            device.startOperation();
        })
        .When("Emergency stop is activated", []() {
            device.emergencyStop();
        })
        .Then("All operations halt immediately", []() {
            Assert::IsFalse(device.isOperating());
        })
        .CriticalAssertion("System enters safe state", []() {
            return device.isSafeState();
        })
    .Execute();
```

### Performance Testing for Medical Devices

```cpp
// Real-time performance requirements
SpecLab::Requirement("SRS-003", "ECG processing shall complete within 100ms")
    .PerformanceRequirement(std::chrono::milliseconds(100))
    .Test("ECG_PERFORMANCE")
        .Given("ECG data stream is active", []() {
            ecg.startStream();
            ecg.loadTestData(standardPattern);
        })
        .When("Signal processing is performed", []() {
            return [&]() { ecg.processSignal(); };
        })
        .Then("Processing completes within time limit", [](auto duration) {
            Assert::LessThan(duration, std::chrono::milliseconds(100));
        })
        .Benchmark(1000)  // Statistical analysis over 1000 iterations
    .Execute();
```

## Installation

### Prerequisites

- **CMake 4.0+** (required for C++23 `import std` support)
- **C++23 Compatible Compiler**:
  - MSVC 17.14+ (Visual Studio 2022 17.10+)
  - GCC 16.1+ (GCC 15 cannot build this project — see issue #9)
  - Clang 20.0+ (experimental)

### Build Instructions

```bash
# Clone the repository
git clone https://github.com/yourusername/speclab.git
cd speclab

# Configure build
mkdir build && cd build
cmake .. -DSPECLAB_BUILD_EXAMPLES=ON -DSPECLAB_BUILD_TESTS=ON

# Build the framework
cmake --build .

# Run tests
ctest

# Install (optional)
cmake --install .
```

### CMake Integration

```cmake
find_package(SpecLab REQUIRED)

target_link_libraries(your_target PRIVATE SpecLab::SpecLab)
target_compile_features(your_target PRIVATE cxx_std_23)
```

## Documentation

### Basic Usage Patterns

#### Simple Function-Based Tests
```cpp
// Quick functional test
SpecLab::Test("BASIC_001")
    .Given("System is initialized", []() {
        system.initialize();
    })
    .When("User requests status", []() {
        result = system.getStatus();
    })
    .Then("Status is returned successfully", []() {
        Assert::IsNotNull(result);
        Assert::AreEqual(result->code, StatusCode::OK);
    })
    .Execute();
```

#### Data-Driven Testing
```cpp
// Parameterized testing for edge cases
SpecLab::Requirement("SRS-004", "Temperature readings accurate within ±0.1°C")
    .Test("TEMP_ACCURACY")
        .Parameters({
            {36.5, 36.4, 36.6},  // normal
            {35.0, 34.9, 35.1},  // hypothermia
            {38.0, 37.9, 38.1}   // fever
        })
        .Given("Calibrated sensor", [](auto params) {
            sensor.calibrate();
            sensor.setActual(params[0]);
        })
        .When("Temperature measured", [](auto params) {
            measured = sensor.read();
        })
        .Then("Within tolerance", [](auto params) {
            Assert::InRange(measured, params[1], params[2]);
        })
    .Execute();
```

#### Feature Organization
```cpp
// Group related tests by medical device feature
SpecLab::Feature("Patient Monitoring")
    .Description("Core vital signs monitoring functionality")
    .Requirement("SRS-100", "Monitor vital signs continuously")
        .Test("MONITOR_HEARTRATE")
            .Given("Heart rate sensor connected", []() { /* setup */ })
            .When("Patient data collected", []() { /* action */ })
            .Then("Heart rate recorded accurately", []() { /* verify */ })
        .Test("MONITOR_BLOODPRESSURE")
            .Given("BP cuff properly positioned", []() { /* setup */ })
            .When("Measurement cycle initiated", []() { /* action */ })
            .Then("BP values within expected range", []() { /* verify */ })
    .Execute();
```

### Medical Device Compliance

#### IEC 62304 Software Lifecycle
```cpp
// Software lifecycle process validation
SpecLab::IEC62304Process(IEC62304::SystemTesting)
    .Requirement("SRS-008", "Software meets all requirements")
        .Test("LIFECYCLE_VALIDATION")
            .Given("Software build complete", []() {
                build.verify();
            })
            .When("Validation suite executed", []() {
                results = suite.executeValidation();
            })
            .Then("All requirements verified", []() {
                Assert::AreEqual(results.getPassRate(), 100.0);
            })
            .TraceabilityMatrix([](auto& matrix) {
                matrix.verifyRequirementsCoverage();
            })
    .Execute();
```

#### Risk Management (ISO 14971)
```cpp
// Risk-based testing approach
SpecLab::Requirement("SRS-005", "Alarm activates for critical values")
    .RiskAssessment("RISK_001", "False negative could endanger patient")
    .HazardId("HAZ_001")
    .Test("CRITICAL_ALARM")
        .Given("Monitor active", []() {
            monitor.activate();
        })
        .When("Critical threshold exceeded", []() {
            monitor.simulateCritical(HeartRate{200});
        })
        .Then("Alarm triggers within 1 second", []() {
            auto alarmTime = monitor.waitForAlarm(std::chrono::seconds(1));
            Assert::IsTrue(alarmTime.has_value());
        })
        .MedicalAssertion("Patient safety maintained", []() {
            return monitor.isAlarmActive();
        })
    .Execute();
```

### Error Handling and Recovery

#### Fault Tolerance Testing
```cpp
// Medical device fault tolerance
SpecLab::Requirement("SRS-006", "Graceful sensor failure recovery")
    .FaultTolerance("sensor_failure")
    .Test("SENSOR_FAILOVER")
        .Given("All sensors operational", []() {
            system.enableAllSensors();
        })
        .When("Primary sensor fails", []() {
            system.simulateFailure(SensorType::HeartRate, 1);
        })
        .Then("Backup sensor activated", []() {
            Assert::IsTrue(system.isUsingBackup(SensorType::HeartRate));
        })
        .And("Failure logged for maintenance", []() {
            auto logs = system.getMaintenanceLogs();
            Assert::Contains(logs, "SENSOR_FAILURE");
        })
    .Execute();
```

## Architecture

### C++23 Module Structure

```
speclab/
├── core/
│   ├── TestResult.cppm          # Test execution results
│   ├── TestCase.cppm            # Base test implementations
│   ├── TestSuite.cppm           # Test suite management
│   ├── Assertions.cppm          # Assertion framework
│   ├── Requirements.cppm        # Requirement model & registry (traceability)
│   ├── RequirementAPI.cppm      # Builder + coverage/traceability free functions
│   └── FunctionalAPI.cppm      # Functional BDD API
├── medical/
│   ├── MedicalTestCase.cppm     # Medical device test cases
│   ├── ComplianceValidator.cppm # Regulatory compliance
│   └── MedicalFunctionalAPI.cppm# Medical functional API
├── reporters/
│   ├── Reporter.cppm            # Base reporter interface
│   └── ConsoleReporter.cppm     # Console output
├── runners/
│   └── TestRunner.cppm          # Test execution engine
└── SpecLab.cppm                 # Main framework interface
```

### Key Design Principles

- **Zero Dependencies**: Only `import std` required
- **Medical Compliance**: Built-in regulatory standard support
- **Thread Safety**: Safe for medical device environments  
- **Performance**: Real-time system optimizations
- **Traceability**: Full audit trail for compliance
- **Safety First**: Critical failure handling and recovery

## Requirements Traceability

SpecLab includes a lightweight requirement model and registry for building a
traceability matrix between requirements and the tests that cover them. The
API lives in the `speclab.core.requirements` module
(`core/Requirements.cppm` / `core/RequirementAPI.cppm`) and is exported from
`speclab::core`.

### Requirement Model

A `Requirement` has these fields:

- `id`, `description` — canonical identifier and human-readable statement.
- `riskLevel` (LOW | MEDIUM | HIGH | CRITICAL) and `safetyClass` (CLASS_A |
  CLASS_B | CLASS_C).
- `requiresAudit`, `requiresValidation` — drive coverage gating and audit
  trail behaviour.
- `source` — original source reference (e.g. SRS section, Jira key).
- `version` — defaults to `"1.0"`.
- `metadata` — `std::unordered_map<std::string,std::string>` extension point
  for domain specifics (e.g. `{"subsystem","Comms"}`).

### Phase 1 — Registration & Linking

```cpp
import speclab.core.requirements;
using namespace speclab::core;

RegisterRequirement({
    .id = "REQ-100",
    .description = "System boots in <250ms",
    .riskLevel = "HIGH",
    .safetyClass = "CLASS_B",
    .requiresAudit = true,
    .requiresValidation = true,
    .source = "SRS-Boot"
});

TestSuite suite("Boot");
suite.addTest("T_BootTime", [](){ /* measure boot */ });
LinkTestRequirement("T_BootTime", "REQ-100");

auto results = suite.execute();
std::println("{}", ExportTraceMatrixCSV());
```

Best practices:

- Use stable canonical IDs: `REQ-###` or domain prefixes (`ALARM-001`).
- One test may link to multiple requirements if it validates an integrated
  behavior; prefer focused tests when possible.
- Store the original source reference (e.g. Jira key, SRS section) in `source`.

### Phase 2 — Automatic Requirement Coverage Gate

Test execution auto-populates `TestResult.requirementIds` based on links, and
appends a synthetic `REQUIREMENT_COVERAGE` result to every suite execution:

- **Passed** — all HIGH/CRITICAL requirements have at least one linked test.
- **Failed** — uncovered HIGH requirements.
- **Critical** — uncovered CRITICAL requirements.

The `missing_requirements` metadata lists uncovered IDs when failing. No
manual augmentation is needed; register requirements early in program init and
link each test ID to requirement IDs via `LinkTestRequirement`. Inspect the
`REQUIREMENT_COVERAGE` result in CI to enforce a coverage gate.

### Phase 3 — Risk-Based Execution & HTML Reporting

```cpp
import speclab.core.requirements;
using namespace speclab::core;

SetRequirementsConfig({
    .abortOnCriticalGaps = true,
    .riskBasedOrdering = true
});
```

- **Risk-weighted ordering** — tests linked to CRITICAL/HIGH requirements
  execute first.
- **Early abort** — when `abortOnCriticalGaps` is set and any CRITICAL
  requirement has no linked test, the suite returns a synthetic
  `REQUIREMENT_COVERAGE_PRE` result (Critical) instead of running.
- **HTML export** — `ExportTraceMatrixHTML()` produces a styled HTML table
  and summary; write it to disk and publish as a build artifact for audit prep.
- **Predicates** — `HasUncoveredCriticalRequirements()` for quick checks,
  `TestRiskScore(testId)` returns the derived risk weight (1..4).

Keep requirement risk levels accurate; ordering and gating hinge on them. Use
early abort in fast feedback loops; disable it in full nightly runs if you
prefer full execution even with gaps.

## Testing Philosophy

### Test Categories

1. **Unit Tests**: Component-level functionality
2. **Integration Tests**: System interaction validation
3. **Performance Tests**: Real-time requirements verification
4. **Safety Tests**: Critical failure scenario handling
5. **Compliance Tests**: Regulatory standard verification

### Medical Device Testing Patterns

- **Risk-Based Priority**: Critical safety tests first
- **Traceability Matrix**: Requirements ↔ Test mapping
- **Audit Trail**: Complete test execution history
- **Validation & Verification**: Separate V&V processes
- **Performance Budgets**: Real-time constraint validation

## Configuration

### CMake Options

- `SPECLAB_BUILD_EXAMPLES=ON` - Build example projects
- `SPECLAB_BUILD_TESTS=ON` - Build framework tests
- `SPECLAB_ENABLE_COVERAGE=ON` - Enable code coverage
- `SPECLAB_MEDICAL_COMPLIANCE=ON` - Enable compliance features (default)

### Build Variants

- **Debug**: Full debugging info, assertions enabled
- **Release**: Optimized for production medical devices
- **RelWithDebInfo**: Release optimization with debug symbols
- **MinSizeRel**: Minimal size for embedded systems

## Medical Device Standards

SpecLab provides built-in support for:

- **IEC 62304**: Medical device software lifecycle processes
- **ISO 13485**: Quality management systems for medical devices
- **ISO 14971**: Risk management for medical devices
- **EU MDR**: Medical Device Regulation (2017/745)
- **FDA 21 CFR Part 820**: Quality system regulation

## Contributing

We welcome contributions! Please see [CONTRIBUTING.md](CONTRIBUTING.md) for guidelines.

### Code Style

There is no `.clang-format` configuration in effect (the committed file is
empty); apply the following conventions by hand:

- 4-space indentation, no tabs.
- Pointer/reference glued to the type: `int* ptr`, `const std::string& name`.
- `UpperCamelCase` for classes/structs, `lowerCamelCase` for functions/variables.
- Use `nullptr` instead of `NULL`.
- Module files: `UpperCamelCase.cppm`. Source files: `UpperCamelCase.cpp` (examples/tests only).
- Follow the C++ Core Guidelines; `.clang-tidy` is comprehensive.
- Use `[[maybe_unused]]` rather than casting to `(void)` for unused locals where
  the intent is to suppress warnings.

### Development Setup

```bash
# Install pre-commit hooks
pip install pre-commit
pre-commit install

# Run code formatting
find . -name "*.cpp" -o -name "*.cppm" | xargs clang-format -i

# Run static analysis
clang-tidy -p build include/**/*.cppm
```

## License

This project is available under the [European Union Public Licence 1.2](LICENSE), or under separate
commercial terms. See [LICENSING.md](LICENSING.md).

## Medical Device Notice

**IMPORTANT**: This framework is designed to assist in medical device software testing but does not guarantee regulatory compliance by itself. Always consult with regulatory experts and follow your organization's quality management system procedures.

## Support

- **Documentation**: [Wiki](https://github.com/yourusername/speclab/wiki)
- **Issues**: [GitHub Issues](https://github.com/yourusername/speclab/issues)
- **Discussions**: [GitHub Discussions](https://github.com/yourusername/speclab/discussions)
- **Medical Device Consulting**: Contact us for specialized support

## Roadmap

- [x] Core BDD functional API
- [x] IEC 62304 compliance features
- [x] Risk-based testing support
- [ ] Visual test reporting dashboard
- [ ] Integration with medical device simulators
- [ ] Real-time monitoring and alerts
- [ ] Cryptographic audit trail signing
- [ ] Cloud-based compliance reporting

---

**Made with ❤️ for the medical device community**
