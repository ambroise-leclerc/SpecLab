/**
 * @brief Main SpecLab module - Medical Device Testing Framework
 */

export module speclab;

// Export core testing functionality
export import speclab.core.testresult;
export import speclab.core.assertions;
export import speclab.core.testcase;
export import speclab.core.testsuite;

// Export functional BDD API
export import speclab.core.functionalapi;
export import speclab.core.requirementapi;
export import speclab.core.requirements;

// Export medical device specific features
export import speclab.medical.medicaltestcase;
export import speclab.medical.functionalapi;
export import speclab.medical.compliancevalidator;

import std;

/**
 * @brief Main SpecLab namespace containing all framework functionality
 */
export namespace SpecLab {
    
    // Re-export core functional API for convenience
    using speclab::Test;
    using speclab::ParameterizedTest;
    using speclab::Requirement;
    using speclab::Feature;
    using speclab::IEC62304Process;
    
    // Re-export medical device API
    using speclab::medical::MedicalRequirement;
    using speclab::medical::MedicalTest;
    
    // Re-export core types
    using speclab::core::TestResult;
    using speclab::core::TestStatus;
    using speclab::core::TestCase;
    using speclab::core::TestSuite;
    using speclab::core::FunctionTestCase;
    using speclab::core::ParameterizedTestCase;
    using speclab::core::BenchmarkTestCase;
    
    // Re-export medical types
    using speclab::medical::MedicalTestCase;
    using speclab::medical::IEC62304TestCase;
    using speclab::medical::RiskLevel;
    using speclab::medical::SafetyClass;
    using speclab::medical::MedicalTestContext;
    
    // Re-export assertion utilities
    using Assert = speclab::core::Assertions;
    
    /**
     * @brief Framework version information
     */
    constexpr std::string_view VERSION = "1.0.0";
    constexpr std::string_view BUILD_DATE = __DATE__;
    constexpr std::string_view CPP_VERSION = "C++23";
    
    /**
     * @brief Medical device compliance standards supported
     */
    namespace Standards {
        constexpr std::string_view IEC_62304 = "IEC_62304";
        constexpr std::string_view ISO_13485 = "ISO_13485";
        constexpr std::string_view ISO_14971 = "ISO_14971";
        constexpr std::string_view FDA_21_CFR_820 = "FDA_21_CFR_820";
        constexpr std::string_view EU_MDR = "EU_MDR_2017_745";
    }
    
    /**
     * @brief Get framework information
     */
    struct FrameworkInfo {
        std::string_view version = VERSION;
        std::string_view buildDate = BUILD_DATE;
        std::string_view cppVersion = CPP_VERSION;
        bool medicalComplianceEnabled = true;
        std::vector<std::string_view> supportedStandards = {
            Standards::IEC_62304,
            Standards::ISO_13485,
            Standards::ISO_14971,
            Standards::FDA_21_CFR_820,
            Standards::EU_MDR
        };
    };
    
    /**
     * @brief Get framework information
     */
    FrameworkInfo getFrameworkInfo() {
        return FrameworkInfo{};
    }
    
    /**
     * @brief Initialize SpecLab framework
     * @param enableMedicalCompliance Enable medical device compliance features
     * @param logLevel Logging level for framework operations
     */
    void initialize(bool enableMedicalCompliance = true, 
                   std::string_view logLevel = "INFO") {
        // Framework initialization would go here
        // For now, this is a placeholder
        
        if (enableMedicalCompliance) {
            // Initialize medical device compliance features
            // Set up audit trail logging
            // Configure regulatory reporting
        }
    }
    
    /**
     * @brief Quick test execution for single tests
     * @param testName Test identifier
     * @param testFunction Test function to execute
     * @return Test result
     */
    speclab::core::TestResult quickTest(std::string_view testName, 
                                       std::function<void()> testFunction) {
        return Test(testName)
            .Given("Test setup", [](){})
            .When("Test executes", testFunction)
            .Then("Test completes", [](){})
            .Execute();
    }
    
    /**
     * @brief Run a simple assertion test
     * @param testName Test identifier
     * @param condition Condition to assert
     * @param message Assertion message
     * @return Test result
     */
    speclab::core::TestResult assertTest(std::string_view testName,
                                        bool condition,
                                        std::string_view message = "Assertion") {
        return Test(testName)
            .Given("Condition to test", [](){})
            .When("Assertion is evaluated", [](){})
            .Then("Condition must be true", [condition, message]() {
                Assert::IsTrue(condition, std::string(message));
            })
            .Execute();
    }
    
    /**
     * @brief Medical device quick test with compliance
     * @param testName Test identifier
     * @param deviceId Device identifier
     * @param testFunction Test function
     * @param riskLevel Risk level (default: Low)
     * @return Medical test result
     */
    speclab::core::TestResult medicalQuickTest(std::string_view testName,
                                              std::string_view deviceId,
                                              std::function<void()> testFunction,
                                              speclab::medical::RiskLevel riskLevel = speclab::medical::RiskLevel::Low) {
        return MedicalTest(testName, deviceId, "QuickTest", riskLevel)
            .Given("Medical device setup", [](){})
            .When("Test executes", testFunction)
            .Then("Test completes", [](){})
            .ExecuteMedical();
    }

} // namespace SpecLab

/**
 * @brief Global convenience aliases for common usage patterns
 */
export namespace SpecLabShort {
    // Short aliases for quick testing
    using T = SpecLab::Test;
    using R = SpecLab::Requirement;
    using F = SpecLab::Feature;
    using MT = SpecLab::MedicalTest;
    using MR = SpecLab::MedicalRequirement;
    
    // Quick assertion alias
    constexpr auto Assert = SpecLab::Assert;
}