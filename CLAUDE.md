# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

SpecLab is a modern C++23 modules-based testing framework specifically designed for medical devices. It provides BDD (Behavior-Driven Development) testing capabilities with compliance for IEC 62304, ISO 13485, and ISO 14971 standards through structured test execution, audit trails, and medical device-specific features.

## Build Commands

### Prerequisites
- CMake 4.0+ (required for C++23 `import std` support)
- C++23 compatible compiler:
  - MSVC 17.14+ (Visual Studio 2022 17.10+)
  - GCC 15.0+
  - Clang 20.0+

### Basic Build
```bash
mkdir build && cd build
cmake .. -DSPECLAB_BUILD_EXAMPLES=ON -DSPECLAB_BUILD_TESTS=ON
cmake --build .
```

### Development Commands
- **Configure build**: `cmake .. -DSPECLAB_BUILD_EXAMPLES=ON -DSPECLAB_BUILD_TESTS=ON`
- **Build project**: `cmake --build .`
- **Clean build**: `cmake --build . --target clean`
- **Install**: `cmake --install .`
- **Run tests**: `ctest` or `cmake --build . --target test`

### Code Quality Tools
- **Format code**: Apply `.clang-format` (4 spaces, pointer/reference next to type)
- **Lint code**: Apply `.clang-tidy` (comprehensive C++ Core Guidelines checks)
- **Generate compile commands**: Automatically enabled via `CMAKE_EXPORT_COMPILE_COMMANDS`

## Architecture Overview

### C++23 Module Structure
The framework is built entirely on C++23 modules with zero dependencies except `import std`:

```
speclab/
├── core/
│   ├── TestResult.cppm      # Test execution results and status
│   ├── TestCase.cppm        # Base test case implementations
│   ├── TestSuite.cppm       # Test suite management
│   └── Assertions.cppm      # Assertion framework
├── medical/
│   ├── MedicalTestCase.cppm # Medical device specific test cases
│   └── ComplianceValidator.cppm # Regulatory compliance validation
├── reporters/
│   ├── Reporter.cppm        # Base reporter interface
│   └── ConsoleReporter.cppm # Console output with colors
├── runners/
│   └── TestRunner.cppm      # Test execution engine
├── validators/              # Test validation components
├── utils/                   # Utility components
├── SpecLab.cppm            # Framework interface
└── speclab.cppm            # Main module exports
```

### Module Dependency Chain
1. `TestResult.cppm` (base result types and status)
2. `Assertions.cppm` (assertion framework)
3. `TestCase.cppm` (imports TestResult, Assertions)
4. `MedicalTestCase.cppm` (imports TestCase, TestResult, Assertions)
5. `ComplianceValidator.cppm` (medical compliance validation)
6. `TestSuite.cppm` (imports TestCase)
7. `Reporter.cppm` and `ConsoleReporter.cppm` (reporting interface)
8. `TestRunner.cppm` (imports all core components)
9. `SpecLab.cppm` and `speclab.cppm` (main export modules)

### Key Design Patterns
- **Zero-macro design**: Uses C++23 features instead of preprocessor macros
- **Medical compliance**: Audit trails, risk management, lifecycle testing per IEC 62304
- **BDD support**: Structured test cases with Given-When-Then patterns
- **Safety classification**: IEC 62304 safety classes (A, B, C) with automatic risk escalation
- **Thread-safe execution**: Built on `std::jthread` and atomic operations
- **Performance testing**: Built-in benchmark capabilities with timing analysis

### Core Components

#### TestResult System
Comprehensive test result tracking with medical device specific fields:
- Core: status (Passed, Failed, Error, Skipped, Blocked, Critical), message, duration
- Source: file, line, function (via `std::source_location`)
- Medical: risk level, compliance standard, device component, audit requirements
- Metadata: custom key-value pairs for regulatory tracking

#### TestCase Architecture
- **TestCase**: Base abstract class with setup/teardown and execution framework
- **FunctionTestCase**: Function-based tests for simple scenarios
- **ParameterizedTestCase**: Data-driven testing with parameter lists
- **BenchmarkTestCase**: Performance testing with statistical analysis
- **MedicalTestCase**: Medical device specific test cases with compliance features
- **IEC62304TestCase**: Specialized for IEC 62304 software lifecycle processes

#### Medical Device Compliance Features
- **Risk Classification**: ISO 14971 risk levels (Low, Medium, High, Critical)
- **Safety Classes**: IEC 62304 classes (A, B, C) with automatic audit requirements
- **Compliance Assertions**: `assertSafetyCritical()`, `assertComplianceRequirement()`, `assertMedicalPerformance()`
- **Audit Trail**: Comprehensive event logging with timestamps and context
- **Regulatory Metadata**: Structured data for compliance reporting

#### Test Execution Engine
- **TestRunner**: Multi-threaded test execution with result aggregation
- **TestSuite**: Hierarchical test organization with filtering capabilities
- **TestResultCollection**: Statistical analysis and reporting of test outcomes

### Medical Device Compliance Architecture

#### IEC 62304 Software Lifecycle Support
- **Lifecycle Processes**: Planning, Requirements, Architecture, Design, Implementation, Integration, Testing, Risk Management, Configuration, Problem Resolution, Maintenance
- **Safety Classification**: Automatic safety class determination and validation requirements
- **Traceability**: Requirement-to-test traceability with unique identifiers
- **Audit Requirements**: Automatic audit trail generation for Class B/C software

#### ISO 14971 Risk Management Integration
- **Risk Assessment**: Risk level classification with automatic escalation
- **Hazard Tracking**: Hazard identification and mitigation testing
- **Safety Critical Assertions**: Specialized assertions for safety-critical conditions
- **Performance Requirements**: Real-time performance validation for medical systems

#### Regulatory Reporting
- **Compliance Status**: Automatic compliance status determination
- **Regulatory Actions**: Automated regulatory action recommendations
- **Audit Trail**: Tamper-proof logging with cryptographic signatures (planned)
- **Documentation**: Automated compliance report generation

### CMake Integration Points
- **C++23 modules**: Uses `CMAKE_EXPERIMENTAL_CXX_IMPORT_STD` for `import std`
- **Compiler detection**: Enforces minimum versions for module support (MSVC 17.14+, GCC 15+, Clang 20+)
- **Module scanning**: Enables `CMAKE_CXX_SCAN_FOR_MODULES`
- **Medical compliance**: Defines `MDUX_MEDICAL_DEVICE_COMPLIANCE=1`
- **Build options**: `SPECLAB_BUILD_EXAMPLES`, `SPECLAB_BUILD_TESTS`
- **File set support**: Uses `FILE_SET cxx_modules TYPE CXX_MODULES` for proper module installation

## Development Guidelines

### Coding Standards
- Follow C++ Core Guidelines
- Use `UpperCamelCase` for classes/structs, `lowerCamelCase` for functions/variables
- 4-space indentation, no tabs
- Pointer/reference symbols next to type (`int* ptr`, `const std::string& name`)
- Use `nullptr` instead of `NULL`

### File Naming
- Module files: `UpperCamelCase.cppm`
- Source files: `UpperCamelCase.cpp` (examples/tests only)

### Documentation
- Use Doxygen syntax with `@brief`, `@param`, `@return`
- Compact notation for simple functions: `/** @brief Description */`
- Include usage examples with `@code` blocks for complex APIs

### Medical Device Testing Patterns

#### Test Case Structure
```cpp
class MyMedicalTest : public speclab::medical::MedicalTestCase {
public:
    MyMedicalTest() : MedicalTestCase("TEST_001", createContext()) {}
    
protected:
    void run() override {
        // Given - Setup test conditions
        setupDeviceState();
        
        // When - Execute operation under test
        auto result = executeOperation();
        
        // Then - Verify expected outcomes
        assertSafetyCritical(result.isValid(), "Operation must succeed for patient safety");
        assertMedicalPerformance(result.duration, maxAllowedTime, "critical_operation", true);
    }
    
private:
    MedicalTestContext createContext() {
        MedicalTestContext context;
        context.deviceId = "DEV_001";
        context.componentName = "PatientMonitor";
        context.riskLevel = RiskLevel::High;
        context.safetyClass = SafetyClass::ClassC;
        context.requiresValidation = true;
        return context;
    }
};
```

#### Risk-Based Testing
- **Class A Software**: Basic functional testing sufficient
- **Class B Software**: Enhanced testing with validation requirements
- **Class C Software**: Comprehensive testing with verification and validation, automatic audit trail

#### Performance Testing for Medical Devices
```cpp
auto benchmarkTest = std::make_unique<BenchmarkTestCase>(
    "PERF_001", 
    [&]() { criticalOperation(); }, 
    1000,  // iterations
    "Critical Operation Performance"
);
benchmarkTest->setRiskLevel("HIGH");
benchmarkTest->setRequiresAudit(true);
```

### Test Organization
- Group tests by medical device component or system
- Use risk level to prioritize test execution order
- Implement traceability matrix linking requirements to test cases
- Separate validation tests from verification tests per IEC 62304

### Medical Device Considerations
- All test operations must be traceable for audit compliance
- Critical failures must trigger immediate mitigation procedures
- Performance requirements must account for real-time medical system constraints
- Thread safety is mandatory for medical device environments
- Security by design with future cryptographic audit trail support
- All test results must preserve full context for regulatory review

### Phase 1: Requirements Traceability (New)

SpecLab now includes a lightweight requirement model and registry for building a traceability matrix.

Key APIs (in `speclab.core.requirements`):
- `RegisterRequirement(const Requirement&)` register a requirement.
- `LinkTestRequirement(testId, requirementId)` link an executed test to a requirement.
- `ExportTraceMatrixCSV()` export a simple CSV matrix.

Requirement fields:
- `id`, `description`, `riskLevel`, `safetyClass`, `requiresAudit`, `requiresValidation`, `source`, `version`, `metadata`.

Example:
```cpp
import speclab.core.requirements;
using namespace speclab::core;
RegisterRequirement({ .id="REQ-100", .description="System boots in <250ms", .riskLevel="HIGH", .safetyClass="CLASS_B", .requiresAudit=true, .requiresValidation=true, .source="SRS-Boot" });
TestSuite suite("Boot");
suite.addTest("T_BootTime", [](){ /* measure boot */ });
LinkTestRequirement("T_BootTime", "REQ-100");
auto results = suite.execute();
std::println("{}", ExportTraceMatrixCSV());
```

Rationale for non-medical developers:
- Treat a requirement as a contract statement your code must satisfy.
- Linking tests to requirements enables gap detection (uncovered requirements) and impact analysis (which tests to re-run when a requirement changes).
- `riskLevel` and `safetyClass` will later drive scheduling and reporting.

Best Practices:
- Use stable canonical IDs: `REQ-###` or domain prefixes (`ALARM-001`).
- One test may link to multiple requirements if it validates an integrated behavior; prefer focused tests when possible.
- Store original source reference (e.g. Jira key, SRS section) in `source`.
- Use `metadata` to add domain specifics (e.g. `{ {"subsystem","Comms"} }`).

Planned Next Phases:
- Automatic enforcement of missing links (CI failure if uncovered high-risk requirement)
- Rich report generation (PDF/HTML) with grouping by risk
- Validation vs Verification separation in dashboards

### Phase 2: Automatic Requirement Augmentation & Coverage Gate

Enhancements:
- Test execution now auto-populates `TestResult.requirementIds` based on links.
- Synthetic result `REQUIREMENT_COVERAGE` appended to every suite execution.
  * Status Passed: all HIGH/CRITICAL requirements have at least one linked test.
  * Failed: uncovered HIGH requirements.
  * Critical: uncovered CRITICAL requirements.
- Metadata `missing_requirements` lists uncovered IDs when failing.

Developer Workflow:
1. Register requirements early in program init.
2. Link each test ID to requirement IDs (`LinkTestRequirement`).
3. Execute suites; no manual augmentation needed.
4. Inspect results to enforce CI gate:
   - Fail build if `REQUIREMENT_COVERAGE` status != PASSED.

Example CI Pseudocode:
```bash
speclab_runner --json results.json
jq -e '.results[] | select(.testId=="REQUIREMENT_COVERAGE" and .status=="PASSED")' results.json >/dev/null
```

Planned (Phase 3):
- Risk-weighted execution prioritization
- HTML/PDF traceability report
- Enforcement: abort run early if critical gaps detected

### Phase 3: Risk-Based Execution & HTML Reporting (New)

Enhancements:
- Risk-weighted test ordering: tests linked to CRITICAL/HIGH requirements execute first.
- Configurable early abort if uncovered CRITICAL requirements detected pre-run.
- HTML Traceability Matrix export (`ExportTraceMatrixHTML`).
- Config API: `SetRequirementsConfig({ .abortOnCriticalGaps=true, .riskBasedOrdering=true })`.

Key APIs:
- `SetRequirementsConfig(const RequirementsConfig&)`
- `TestRiskScore(testId)` returns derived risk weight (1..4)
- `ExportTraceMatrixHTML()` generates styled HTML table + summary
- `HasUncoveredCriticalRequirements()` quick predicate

Execution Flow Changes:
1. (Optional) Pre-execution abort: if `abortOnCriticalGaps` and any CRITICAL requirement has no linked test, suite returns synthetic `REQUIREMENT_COVERAGE_PRE` result (Critical).
2. Ordering: internal suite execution sorts tests by descending risk score.
3. Post: existing Phase 2 coverage synthetic result appended.

Example Configuration:
```cpp
import speclab.core.requirements;
using namespace speclab::core;
SetRequirementsConfig({ .abortOnCriticalGaps = true, .riskBasedOrdering = true });
```

HTML Export Sample Usage:
```cpp
auto html = ExportTraceMatrixHTML();
std::ofstream("traceability.html") << html;
```
Add artifact publishing in CI to review trace matrix.

Developer Guidance:
- Keep requirement risk levels accurate; ordering & gating hinge on them.
- Use early abort in fast feedback loops; disable in full nightly runs if you prefer full execution even with gaps.
- Commit HTML report as build artifact for audit prep.

Next (Phase 4 - Planned):
- PDF generation (wkhtmltopdf or libharu integration)
- Delta impact analysis (selective re-run based on changed requirements)
- Real-time streaming reporter with risk progress metrics