/**
 * @brief Basic SpecLab example demonstrating core functionality
 */

import speclab;
import std;

using namespace speclab;
using namespace speclab::core;
using namespace speclab::medical;

/**
 * @brief Simple test case for demonstration
 */
class BasicTestCase : public TestCase {
public:
    BasicTestCase() : TestCase("BASIC_001", "Basic Test", "Demonstrates basic functionality") {}

protected:
    void run() override {
        // Simple test logic
        int result = 2 + 2;
        if (result != 4) {
            throw AssertionFailure("Math test failed", std::source_location::current());
        }
    }
};

/**
 * @brief Medical device test case example
 */
class MedicalDeviceTest : public MedicalTestCase {
public:
    MedicalDeviceTest() : MedicalTestCase("MED_001", createMedicalContext(), "Medical Device Test") {}

protected:
    void run() override {
        // Simulate a medical device operation
        recordMedicalEvent("device_startup", "Device initialization started");
        
        // Simulate some medical device logic
        bool deviceReady = true;
        if (!deviceReady) {
            throw AssertionFailure("Device not ready", std::source_location::current());
        }
        
        recordMedicalEvent("device_ready", "Device is ready for operation");
    }

private:
    MedicalTestContext createMedicalContext() {
        MedicalTestContext context;
        context.deviceId = "DEV_001";
        context.componentName = "HeartRateMonitor";
        context.riskLevel = RiskLevel::Medium;
        context.safetyClass = SafetyClass::ClassB;
        context.complianceStandard = "IEC_62304";
        context.requiresValidation = true;
        context.testEnvironment = "LAB_ENV_001";
        return context;
    }
};

/**
 * @brief Functional API example using BDD style
 */
void functionalApiExample() {
    std::println("=== Functional API Example ===");
    
    // Create a BDD-style test
    auto test = Test("FUNC_001")
        .Given("a calculator is available", []() {
            // Setup code
            std::println("  Given: Calculator initialized");
        })
        .When("I add 2 and 3", []() {
            // Action code
            std::println("  When: Performing addition 2 + 3");
        })
        .Then("the result should be 5", []() {
            // Assertion code
            int result = 2 + 3;
            if (result != 5) {
                throw AssertionFailure("Addition failed", std::source_location::current());
            }
            std::println("  Then: Result is correct (5)");
        });
    
    // Execute the test
    auto result = test.Execute();
    std::println("Test Status: {}", toString(result.status));
    std::println("Duration: {}ns", result.duration.count());
}

/**
 * @brief Test suite example
 */
void testSuiteExample() {
    std::println("\n=== Test Suite Example ===");
    
    // Create test suite
    TestSuite suite("BasicSuite", "Basic functionality tests");
    
    // Add tests to suite
    suite.addTest(std::make_unique<BasicTestCase>());
    suite.addTest(std::make_unique<MedicalDeviceTest>());
    
    // Execute all tests
    auto results = suite.execute();
    
    std::println("Suite executed {} tests", results.getTotalCount());
    std::println("Passed: {}", results.getPassedCount());
    std::println("Failed: {}", results.getFailedCount());
    std::println("Success Rate: {:.1f}%", results.getSuccessRate());
}

/**
 * @brief Test runner example with console reporter
 */
void testRunnerExample() {
    std::println("\n=== Test Runner Example ===");
    
    // Create test runner with console reporter
    auto runner = runners::createTestRunner("medical");
    
    // Create test suite
    TestSuite suite("RunnerSuite", "Test runner demonstration");
    suite.addTest(std::make_unique<BasicTestCase>());
    suite.addTest(std::make_unique<MedicalDeviceTest>());
    
    // Execute with runner
    auto results = runner->executeSuite(suite);
    
    std::println("Runner completed execution");
}

/**
 * @brief Requirements example
 */
void requirementsExample() {
    std::println("\n=== Requirements Example ===");
    
    using namespace speclab::core;
    
    // Register a requirement
    RegisterRequirement({
        .id = "REQ-001",
        .description = "System must perform basic arithmetic",
        .riskLevel = "MEDIUM",
        .safetyClass = "CLASS_B",
        .requiresAudit = true,
        .requiresValidation = true,
        .source = "SRS-Math",
        .metadata = {}
    });
    
    // Link test to requirement (this would normally be done during test execution)
    LinkTestRequirement("BASIC_001", "REQ-001");
    
    std::println("Requirement REQ-001 registered and linked to BASIC_001");
    
    // Export traceability matrix
    auto csvMatrix = ExportTraceMatrixCSV();
    std::println("Traceability Matrix CSV generated ({} chars)", csvMatrix.length());
}

/**
 * @brief Medical compliance demonstration
 */
void medicalComplianceExample() {
    std::println("\n=== Medical Compliance Example ===");
    
    // Create medical test context
    MedicalTestContext context;
    context.deviceId = "PUMP_001";
    context.componentName = "InfusionPump";
    context.riskLevel = RiskLevel::High;
    context.safetyClass = SafetyClass::ClassC;
    context.complianceStandard = "IEC_62304";
    context.requiresValidation = true;
    
    // Create IEC 62304 test case
    auto iecTest = std::make_unique<IEC62304TestCase>(
        "IEC_001", 
        context, 
        IEC62304TestCase::LifecycleProcess::SystemTesting,
        "System Testing Process"
    );
    
    // Execute test
    auto result = iecTest->execute();
    
    std::println("IEC 62304 Test Status: {}", toString(result.status));
    std::println("Risk Level: {}", result.riskLevel);
    std::println("Compliance Standard: {}", result.complianceStandard);
    std::println("Requires Audit: {}", result.requiresAudit ? "Yes" : "No");
}

/**
 * @brief Framework information display
 */
void showFrameworkInfo() {
    std::println("=== SpecLab Framework Information ===");
    
    auto info = SpecLab::getFrameworkInfo();
    std::println("Version: {}", info.version);
    std::println("C++ Version: {}", info.cppVersion);
    std::println("Medical Compliance: {}", info.medicalComplianceEnabled ? "Enabled" : "Disabled");
    
    std::println("Supported Standards:");
    for (const auto& standard : info.supportedStandards) {
        std::println("  - {}", standard);
    }
}

/**
 * @brief Main demonstration function
 */
int main() {
    try {
        // Initialize SpecLab
        SpecLab::initialize(true, "INFO");
        
        // Show framework information
        showFrameworkInfo();
        
        // Run examples
        functionalApiExample();
        testSuiteExample();
        testRunnerExample();
        requirementsExample();
        medicalComplianceExample();
        
        std::println("\n=== All Examples Completed Successfully ===");
        return 0;
        
    } catch (const std::exception& e) {
        std::println("Error: {}", e.what());
        return 1;
    } catch (...) {
        std::println("Unknown error occurred");
        return 1;
    }
}