/**
 * @brief Medical device specific test case implementations - C++23 Module
 */

export module speclab.medical.medicaltestcase;

import std;
import speclab.core.testresult;
import speclab.core.testcase;
import speclab.core.assertions;

export namespace speclab::medical {

    /**
     * @brief Risk classification levels per ISO 14971
     */
    enum class RiskLevel : std::uint8_t {
        Low = 0,        ///< Negligible or minor harm
        Medium = 1,     ///< Serious harm
        High = 2,       ///< Death or catastrophic harm
        Critical = 3    ///< Unacceptable risk level
    };
    
    /**
     * @brief Convert risk level to string
     */
    constexpr std::string_view toString(RiskLevel level) noexcept {
        switch (level) {
            case RiskLevel::Low:      return "LOW";
            case RiskLevel::Medium:   return "MEDIUM";
            case RiskLevel::High:     return "HIGH";
            case RiskLevel::Critical: return "CRITICAL";
            default:                  return "UNKNOWN";
        }
    }

    /**
     * @brief Safety class categories per IEC 62304
     */
    enum class SafetyClass : std::uint8_t {
        ClassA = 0,     ///< Non-life-supporting, low risk
        ClassB = 1,     ///< Non-life-supporting, medium risk
        ClassC = 2      ///< Life-supporting or life-sustaining
    };
    
    /**
     * @brief Convert safety class to string
     */
    constexpr std::string_view toString(SafetyClass safetyClass) noexcept {
        switch (safetyClass) {
            case SafetyClass::ClassA: return "CLASS_A";
            case SafetyClass::ClassB: return "CLASS_B";
            case SafetyClass::ClassC: return "CLASS_C";
            default:                  return "UNKNOWN";
        }
    }

    /**
     * @brief Medical device test context for compliance tracking
     */
    struct MedicalTestContext {
        std::string deviceId;
        std::string deviceName;
        std::string softwareVersion;
        std::string componentName;
        RiskLevel riskLevel = RiskLevel::Low;
        SafetyClass safetyClass = SafetyClass::ClassA;
        std::string complianceStandard = "IEC_62304";
        std::string requirementId;
        std::string hazardId;
        bool requiresValidation = false;
        bool requiresVerification = false;
        std::string testEnvironment;
        std::string testOperator;
        std::unordered_map<std::string, std::string> regulatoryMetadata;
    };

    /**
     * @brief Base class for medical device test cases with compliance features
     */
    class MedicalTestCase : public speclab::core::TestCase {
    public:
        /**
         * @brief Constructor with medical device context
         * @param id Test identifier
         * @param context Medical device test context
         * @param name Test name (optional)
         * @param description Test description (optional)
         */
        explicit MedicalTestCase(std::string_view id,
                               MedicalTestContext context,
                               std::string_view name = "",
                               std::string_view description = "")
            : TestCase(id, name, description)
            , medicalContext_(std::move(context)) {
            
            // Configure base test case with medical information
            setRiskLevel(std::string(toString(medicalContext_.riskLevel)));
            setComplianceStandard(medicalContext_.complianceStandard);
            setDeviceComponent(medicalContext_.componentName);
            setRequiresAudit(medicalContext_.requiresValidation || 
                           medicalContext_.safetyClass >= SafetyClass::ClassB);
            
            // Add medical metadata
            addMetadata("device_id", medicalContext_.deviceId);
            addMetadata("device_name", medicalContext_.deviceName);
            addMetadata("software_version", medicalContext_.softwareVersion);
            addMetadata("safety_class", std::string(toString(medicalContext_.safetyClass)));
            addMetadata("requirement_id", medicalContext_.requirementId);
            addMetadata("hazard_id", medicalContext_.hazardId);
            addMetadata("test_environment", medicalContext_.testEnvironment);
            addMetadata("test_operator", medicalContext_.testOperator);
            
            // Add regulatory metadata
            for (const auto& [key, value] : medicalContext_.regulatoryMetadata) {
                addMetadata("regulatory_" + key, value);
            }
        }
        
        /**
         * @brief Execute medical test with enhanced compliance tracking
         * @return Test result with medical device information
         */
        speclab::core::TestResult executeMedical() {
            auto result = execute();
            
            // Enhanced medical device result information
            result.addMetadata("execution_environment", getExecutionEnvironment());
            result.addMetadata("validation_required", 
                             medicalContext_.requiresValidation ? "true" : "false");
            result.addMetadata("verification_required", 
                             medicalContext_.requiresVerification ? "true" : "false");
            
            // Add compliance status
            if (result.critical()) {
                result.addMetadata("compliance_status", "NON_COMPLIANT");
                result.addMetadata("regulatory_action", "IMMEDIATE_REVIEW_REQUIRED");
            } else if (result.failed()) {
                result.addMetadata("compliance_status", "NEEDS_REVIEW");
                result.addMetadata("regulatory_action", "RISK_ASSESSMENT_REQUIRED");
            } else {
                result.addMetadata("compliance_status", "COMPLIANT");
                result.addMetadata("regulatory_action", "NONE");
            }
            
            return result;
        }
        
        // Getters for medical context
        const MedicalTestContext& getMedicalContext() const noexcept {
            return medicalContext_;
        }
        
        RiskLevel getRiskLevelEnum() const noexcept {
            return medicalContext_.riskLevel;
        }
        
        SafetyClass getSafetyClass() const noexcept {
            return medicalContext_.safetyClass;
        }
        
        const std::string& getDeviceId() const noexcept {
            return medicalContext_.deviceId;
        }
        
        const std::string& getRequirementId() const noexcept {
            return medicalContext_.requirementId;
        }
        
        const std::string& getHazardId() const noexcept {
            return medicalContext_.hazardId;
        }
        
    protected:
        /**
         * @brief Validate pre-conditions for medical device testing
         */
        void setUp() override {
            TestCase::setUp();
            
            // Medical device specific setup
            validateTestEnvironment();
            validateDeviceState();
            recordTestInitiation();
        }
        
        /**
         * @brief Clean up and record test completion
         */
        void tearDown() override {
            recordTestCompletion();
            validateDeviceStateAfterTest();
            
            TestCase::tearDown();
        }
        
        /**
         * @brief Assert safety-critical condition with automatic risk escalation
         * @param condition Safety condition
         * @param hazardDescription Description of potential hazard
         * @param mitigationRequired Whether mitigation is required
         */
        void assertSafetyCritical(bool condition, 
                                std::string_view hazardDescription,
                                bool mitigationRequired = true) {
            if (!condition) {
                std::string errorMsg = std::format(
                    "SAFETY CRITICAL FAILURE: {} (Device: {}, Component: {}, Hazard: {})",
                    hazardDescription, medicalContext_.deviceId, 
                    medicalContext_.componentName, medicalContext_.hazardId);
                
                if (mitigationRequired) {
                    errorMsg += " - IMMEDIATE MITIGATION REQUIRED";
                }
                
                speclab::core::Assertions::fail(errorMsg);
            }
        }
        
        /**
         * @brief Assert compliance with specific medical standard requirement
         * @param condition Compliance condition
         * @param requirementRef Specific requirement reference
         * @param standardSection Standard section/clause
         */
        void assertComplianceRequirement(bool condition,
                                       std::string_view requirementRef,
                                       std::string_view standardSection = "") {
            if (!condition) {
                std::string errorMsg = std::format(
                    "COMPLIANCE FAILURE: {} {} (Section: {}) - Device: {}, Requirement: {}",
                    medicalContext_.complianceStandard, requirementRef, standardSection,
                    medicalContext_.deviceId, medicalContext_.requirementId);
                
                speclab::core::Assertions::fail(errorMsg);
            }
        }
        
        /**
         * @brief Assert performance requirement for real-time medical systems
         * @param actualDuration Measured performance duration
         * @param maxAllowed Maximum allowed duration
         * @param operationName Name of the operation being tested
         * @param isCriticalTiming Whether timing is safety-critical
         */
        void assertMedicalPerformance(std::chrono::nanoseconds actualDuration,
                                    std::chrono::nanoseconds maxAllowed,
                                    std::string_view operationName,
                                    bool isCriticalTiming = false) {
            if (actualDuration > maxAllowed) {
                auto actualMs = std::chrono::duration<double, std::milli>(actualDuration).count();
                auto maxMs = std::chrono::duration<double, std::milli>(maxAllowed).count();
                
                std::string severity = isCriticalTiming ? "SAFETY CRITICAL" : "PERFORMANCE";
                std::string errorMsg = std::format(
                    "{} TIMING FAILURE: {} took {:.2f}ms, max allowed {:.2f}ms (Device: {}, Safety Class: {})",
                    severity, operationName, actualMs, maxMs, 
                    medicalContext_.deviceId, toString(medicalContext_.safetyClass));
                
                speclab::core::Assertions::fail(errorMsg);
            }
        }
        
        /**
         * @brief Record medical test event for audit trail
         * @param eventType Type of event
         * @param description Event description
         * @param additionalData Additional structured data
         */
        void recordMedicalEvent(std::string_view eventType,
                              std::string_view description,
                              const std::unordered_map<std::string, std::string>& additionalData = {}) {
            // In a real implementation, this would integrate with the audit logging system
            // For now, we add it as metadata
            auto timestamp = std::chrono::system_clock::now();
            auto timeStr = std::format("{:%Y-%m-%d %H:%M:%S}", timestamp);
            
            addMetadata(std::format("event_{}_timestamp", eventType), timeStr);
            addMetadata(std::format("event_{}_description", eventType), std::string(description));
            
            for (const auto& [key, value] : additionalData) {
                addMetadata(std::format("event_{}_{}", eventType, key), value);
            }
        }
        
        /**
         * @brief Default run implementation - override in derived classes
         */
        void run() override {
            // Override in derived classes to implement specific medical test logic
            recordMedicalEvent("medical_test_execution", "Executing medical device test");
        }
        
    protected:
        /**
         * @brief Validate test environment meets medical device requirements
         */
        virtual void validateTestEnvironment() {
            // Override in derived classes for specific environment validation
            if (medicalContext_.testEnvironment.empty()) {
                recordMedicalEvent("validation_warning", 
                    "Test environment not specified for medical device test");
            }
        }
        
        /**
         * @brief Validate device state before test execution
         */
        virtual void validateDeviceState() {
            // Override in derived classes for device-specific state validation
            recordMedicalEvent("device_state_check", 
                "Pre-test device state validation performed");
        }
        
        /**
         * @brief Validate device state after test execution
         */
        virtual void validateDeviceStateAfterTest() {
            // Override in derived classes for post-test state validation
            recordMedicalEvent("device_state_check", 
                "Post-test device state validation performed");
        }
        
        /**
         * @brief Record test initiation for audit trail
         */
        void recordTestInitiation() {
            std::unordered_map<std::string, std::string> initiationData = {
                {"test_id", getId()},
                {"device_id", medicalContext_.deviceId},
                {"operator", medicalContext_.testOperator},
                {"safety_class", std::string(toString(medicalContext_.safetyClass))},
                {"risk_level", std::string(toString(medicalContext_.riskLevel))}
            };
            
            recordMedicalEvent("test_initiation", "Medical device test initiated", initiationData);
        }
        
        /**
         * @brief Record test completion for audit trail
         */
        void recordTestCompletion() {
            recordMedicalEvent("test_completion", "Medical device test completed");
        }
        
        /**
         * @brief Get execution environment information
         * @return Environment description string
         */
        std::string getExecutionEnvironment() const {
            return std::format("Environment: {}, Safety Class: {}, Risk Level: {}",
                             medicalContext_.testEnvironment,
                             toString(medicalContext_.safetyClass),
                             toString(medicalContext_.riskLevel));
        }
        
        MedicalTestContext medicalContext_;
    };

    /**
     * @brief Specialized test case for IEC 62304 software lifecycle compliance
     */
    class IEC62304TestCase : public MedicalTestCase {
    public:
        /**
         * @brief IEC 62304 specific processes
         */
        enum class LifecycleProcess {
            Planning,
            Requirements,
            Architecture,
            Design,
            Implementation,
            Integration,
            SystemTesting,
            RiskManagement,
            Configuration,
            ProblemResolution,
            Maintenance
        };
        
        /**
         * @brief Constructor for IEC 62304 test case
         * @param id Test identifier
         * @param context Medical device context
         * @param process Lifecycle process being tested
         * @param name Test name (optional)
         */
        explicit IEC62304TestCase(std::string_view id,
                                MedicalTestContext context,
                                LifecycleProcess process,
                                std::string_view name = "")
            : MedicalTestCase(id, std::move(context), name)
            , lifecycleProcess_(process) {
            
            setComplianceStandard("IEC_62304");
            addMetadata("lifecycle_process", toString(lifecycleProcess_));
        }
        
    protected:
        void validateTestEnvironment() override {
            MedicalTestCase::validateTestEnvironment();
            
            // IEC 62304 specific validation
            recordMedicalEvent("iec62304_validation", 
                std::format("Validating {} process compliance", toString(lifecycleProcess_)));
        }
        
        /**
         * @brief Default run implementation - override in derived classes
         */
        void run() override {
            // Override in derived classes to implement specific IEC 62304 test logic
            recordMedicalEvent("iec62304_test_execution", 
                std::format("Executing {} lifecycle process test", toString(lifecycleProcess_)));
        }
        
    private:
        std::string toString(LifecycleProcess process) const {
            switch (process) {
                case LifecycleProcess::Planning:          return "PLANNING";
                case LifecycleProcess::Requirements:      return "REQUIREMENTS";
                case LifecycleProcess::Architecture:      return "ARCHITECTURE";
                case LifecycleProcess::Design:           return "DESIGN";
                case LifecycleProcess::Implementation:   return "IMPLEMENTATION";
                case LifecycleProcess::Integration:      return "INTEGRATION";
                case LifecycleProcess::SystemTesting:    return "SYSTEM_TESTING";
                case LifecycleProcess::RiskManagement:   return "RISK_MANAGEMENT";
                case LifecycleProcess::Configuration:    return "CONFIGURATION";
                case LifecycleProcess::ProblemResolution: return "PROBLEM_RESOLUTION";
                case LifecycleProcess::Maintenance:      return "MAINTENANCE";
                default:                                 return "UNKNOWN";
            }
        }
        
        LifecycleProcess lifecycleProcess_;
    };

    /**
     * @brief Factory for creating medical device test cases
     */
    class MedicalTestFactory {
    public:
        /**
         * @brief Create a basic medical test case
         * @param id Test identifier
         * @param deviceId Device identifier
         * @param componentName Component name
         * @param riskLevel Risk level
         * @param safetyClass Safety class
         * @return Unique pointer to medical test case
         */
        static std::unique_ptr<MedicalTestCase> createMedicalTest(
            std::string_view id,
            std::string_view deviceId,
            std::string_view componentName,
            RiskLevel riskLevel = RiskLevel::Low,
            SafetyClass safetyClass = SafetyClass::ClassA) {
            
            MedicalTestContext context;
            context.deviceId = deviceId;
            context.componentName = componentName;
            context.riskLevel = riskLevel;
            context.safetyClass = safetyClass;
            
            return std::make_unique<MedicalTestCase>(id, std::move(context));
        }
        
        /**
         * @brief Create an IEC 62304 lifecycle test case
         * @param id Test identifier
         * @param deviceId Device identifier
         * @param process Lifecycle process
         * @param safetyClass Safety class
         * @return Unique pointer to IEC 62304 test case
         */
        static std::unique_ptr<IEC62304TestCase> createIEC62304Test(
            std::string_view id,
            std::string_view deviceId,
            IEC62304TestCase::LifecycleProcess process,
            SafetyClass safetyClass = SafetyClass::ClassA) {
            
            MedicalTestContext context;
            context.deviceId = deviceId;
            context.safetyClass = safetyClass;
            context.complianceStandard = "IEC_62304";
            context.requiresValidation = (safetyClass >= SafetyClass::ClassB);
            context.requiresVerification = true;
            
            return std::make_unique<IEC62304TestCase>(id, std::move(context), process);
        }
    };

} // namespace speclab::medical