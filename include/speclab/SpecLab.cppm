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

// Export reporters and runners
export import speclab.reporters.reporter;
export import speclab.reporters.consolereporter;
export import speclab.runners.testrunner;

import std;

/**
 * @brief Main SpecLab namespace containing all framework functionality
 */
export namespace SpecLab {
    
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
    
    // Re-export functional API
    using speclab::Test;
    using speclab::ParameterizedTest;
    using speclab::Requirement;
    using speclab::Feature;
    using speclab::IEC62304Process;
    
    // Re-export medical functional API
    using speclab::medical::MedicalTest;
    using speclab::medical::MedicalRequirement;
    
    // Re-export runners and reporters
    using speclab::runners::TestRunner;
    using speclab::reporters::Reporter;
    using speclab::reporters::ConsoleReporter;
    
    /**
     * @brief Framework version information
     */
    constexpr std::string_view VERSION = "1.0.0";
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
        (void)logLevel; // Suppress unused parameter warning
        // Framework initialization would go here
        // For now, this is a placeholder
        
        if (enableMedicalCompliance) {
            // Initialize medical device compliance features
            // Set up audit trail logging
            // Configure regulatory reporting
        }
    }

} // namespace SpecLab

/**
 * @brief Global convenience aliases for common usage patterns
 */
export namespace SpecLabShort {
    // Framework provides convenient access to core components
    // Import the main SpecLab module to access all functionality
}