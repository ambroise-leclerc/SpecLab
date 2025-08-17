/**
 * @brief Medical device specific functional API extensions - C++23 Module
 */

export module speclab.medical.functionalapi;

import std;
import speclab.core.testresult;
import speclab.core.functionalapi;
import speclab.core.requirementapi;
import speclab.core.assertions;
import speclab.medical.medicaltestcase;

export namespace speclab::medical {

    /**
     * @brief Medical test builder with safety-critical features
     */
    class MedicalTestBuilder : public speclab::TestBuilder {
    public:
        explicit MedicalTestBuilder(std::string_view testId, MedicalTestContext context)
            : TestBuilder(testId), medicalContext_(std::move(context)) {}
        
        /**
         * @brief Add medical assertion for safety-critical conditions
         */
        MedicalTestBuilder& MedicalAssertion(std::string_view description, CriticalAssertionFunction func) {
            medicalAssertions_.emplace_back(description, std::move(func));
            return *this;
        }
        
        /**
         * @brief Add safety-critical assertion with hazard tracking
         */
        MedicalTestBuilder& SafetyCriticalAssertion(std::string_view description, 
                                                  CriticalAssertionFunction func,
                                                  std::string_view hazardId = "") {
            safetyCriticalAssertions_.emplace_back(
                std::make_tuple(description, std::move(func), std::string(hazardId))
            );
            return *this;
        }
        
        /**
         * @brief Add performance assertion for real-time medical systems
         */
        MedicalTestBuilder& PerformanceAssertion(std::string_view operationName,
                                                std::chrono::nanoseconds maxAllowed,
                                                bool isCriticalTiming = false) {
            performanceAssertions_.emplace_back(
                std::make_tuple(std::string(operationName), maxAllowed, isCriticalTiming)
            );
            return *this;
        }
        
        /**
         * @brief Add compliance requirement assertion
         */
        MedicalTestBuilder& ComplianceAssertion(std::string_view requirementRef,
                                              CriticalAssertionFunction func,
                                              std::string_view standardSection = "") {
            complianceAssertions_.emplace_back(
                std::make_tuple(std::string(requirementRef), std::move(func), std::string(standardSection))
            );
            return *this;
        }
        
        /**
         * @brief Set traceability matrix validation
         */
        MedicalTestBuilder& TraceabilityMatrix(std::function<void(TraceabilityMatrix&)> validator) {
            traceabilityValidator_ = std::move(validator);
            return *this;
        }
        
        /**
         * @brief Execute medical test with enhanced compliance tracking
         */
        speclab::core::TestResult ExecuteMedical() {
            auto result = Execute(); // Execute base test
            
            try {
                // Execute medical-specific assertions
                executeMedicalAssertions(result);
                
                // Execute safety-critical assertions
                executeSafetyCriticalAssertions(result);
                
                // Execute compliance assertions
                executeComplianceAssertions(result);
                
                // Execute performance assertions
                executePerformanceAssertions(result);
                
                // Execute traceability validation
                executeTraceabilityValidation(result);
                
                // Add medical device metadata
                addMedicalMetadata(result);
                
                // Determine compliance status
                determineComplianceStatus(result);
                
            } catch (const std::exception& e) {
                result.status = speclab::core::TestStatus::Critical;
                result.message = "Medical assertion failure: " + std::string(e.what());
            }
            
            return result;
        }
        
        /**
         * @brief Get medical context
         */
        const MedicalTestContext& getMedicalContext() const noexcept {
            return medicalContext_;
        }
        
    private:
        /**
         * @brief Simple traceability matrix for requirement validation
         */
        class TraceabilityMatrix {
        public:
            void verifyRequirementsCoverage() {
                // Implementation would verify all requirements are covered by tests
                requirementsCovered_ = true;
            }
            
            void verifyTestCompleteness() {
                // Implementation would verify all tests map to requirements
                testsComplete_ = true;
            }
            
            bool isValid() const {
                return requirementsCovered_ && testsComplete_;
            }
            
        private:
            bool requirementsCovered_ = false;
            bool testsComplete_ = false;
        };
        
        void executeMedicalAssertions(speclab::core::TestResult& result) {
            for (const auto& [description, assertion] : medicalAssertions_) {
                if (!assertion()) {
                    result.status = (medicalContext_.riskLevel >= RiskLevel::High) 
                                  ? speclab::core::TestStatus::Critical 
                                  : speclab::core::TestStatus::Failed;
                    result.message = "Medical assertion failed: " + description;
                    result.addMetadata("failed_medical_assertion", description);
                    break;
                }
            }
        }
        
        void executeSafetyCriticalAssertions(speclab::core::TestResult& result) {
            for (const auto& [description, assertion, hazardId] : safetyCriticalAssertions_) {
                if (!assertion()) {
                    result.status = speclab::core::TestStatus::Critical;
                    result.message = "SAFETY CRITICAL FAILURE: " + description;
                    result.addMetadata("safety_critical_failure", description);
                    if (!hazardId.empty()) {
                        result.addMetadata("associated_hazard", hazardId);
                    }
                    // Record critical event for audit trail
                    recordCriticalEvent(description, hazardId);
                    break;
                }
            }
        }
        
        void executeComplianceAssertions(speclab::core::TestResult& result) {
            for (const auto& [requirementRef, assertion, section] : complianceAssertions_) {
                if (!assertion()) {
                    result.status = speclab::core::TestStatus::Failed;
                    result.message = std::format("COMPLIANCE FAILURE: {} {}", 
                                                medicalContext_.complianceStandard, requirementRef);
                    result.addMetadata("compliance_failure", requirementRef);
                    if (!section.empty()) {
                        result.addMetadata("failed_section", section);
                    }
                    break;
                }
            }
        }
        
        void executePerformanceAssertions(speclab::core::TestResult& result) {
            // Performance assertions would be integrated with timing measurements
            // This is a simplified implementation
            for (const auto& [operationName, maxAllowed, isCritical] : performanceAssertions_) {
                // In practice, this would measure actual performance
                auto actualDuration = result.duration; // Simplified
                
                if (actualDuration > maxAllowed) {
                    auto actualMs = std::chrono::duration<double, std::milli>(actualDuration).count();
                    auto maxMs = std::chrono::duration<double, std::milli>(maxAllowed).count();
                    
                    std::string severity = isCritical ? "SAFETY CRITICAL" : "PERFORMANCE";
                    result.status = isCritical ? speclab::core::TestStatus::Critical 
                                              : speclab::core::TestStatus::Failed;
                    result.message = std::format("{} TIMING FAILURE: {} took {:.2f}ms, max allowed {:.2f}ms",
                                               severity, operationName, actualMs, maxMs);
                    
                    result.addMetadata("performance_failure", operationName);
                    result.addMetadata("actual_duration_ms", std::to_string(actualMs));
                    result.addMetadata("max_allowed_ms", std::to_string(maxMs));
                    
                    if (isCritical) {
                        recordCriticalEvent("Performance timing violation", operationName);
                    }
                    break;
                }
            }
        }
        
        void executeTraceabilityValidation(speclab::core::TestResult& result) {
            if (traceabilityValidator_) {
                TraceabilityMatrix matrix;
                traceabilityValidator_(matrix);
                
                if (!matrix.isValid()) {
                    result.status = speclab::core::TestStatus::Failed;
                    result.message = "Traceability matrix validation failed";
                    result.addMetadata("traceability_failure", "true");
                }
            }
        }
        
        void addMedicalMetadata(speclab::core::TestResult& result) {
            result.addMetadata("device_id", medicalContext_.deviceId);
            result.addMetadata("device_name", medicalContext_.deviceName);
            result.addMetadata("software_version", medicalContext_.softwareVersion);
            result.addMetadata("component_name", medicalContext_.componentName);
            result.addMetadata("safety_class", std::string(toString(medicalContext_.safetyClass)));
            result.addMetadata("risk_level", std::string(toString(medicalContext_.riskLevel)));
            result.addMetadata("compliance_standard", medicalContext_.complianceStandard);
            result.addMetadata("requirement_id", medicalContext_.requirementId);
            result.addMetadata("hazard_id", medicalContext_.hazardId);
            result.addMetadata("test_environment", medicalContext_.testEnvironment);
            result.addMetadata("test_operator", medicalContext_.testOperator);
            result.addMetadata("requires_validation", medicalContext_.requiresValidation ? "true" : "false");
            result.addMetadata("requires_verification", medicalContext_.requiresVerification ? "true" : "false");
            
            // Add regulatory metadata
            for (const auto& [key, value] : medicalContext_.regulatoryMetadata) {
                result.addMetadata("regulatory_" + key, value);
            }
        }
        
        void determineComplianceStatus(speclab::core::TestResult& result) {
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
        }
        
        void recordCriticalEvent(const std::string& description, const std::string& context) {
            // In a real implementation, this would integrate with audit logging
            auto timestamp = std::chrono::system_clock::now();
            auto timeStr = std::format("{:%Y-%m-%d %H:%M:%S}", timestamp);
            
            // For now, we'll store in a simple log format
            criticalEvents_.push_back({
                timeStr,
                description,
                context,
                medicalContext_.deviceId,
                getTestId()
            });
        }
        
        struct CriticalEvent {
            std::string timestamp;
            std::string description;
            std::string context;
            std::string deviceId;
            std::string testId;
        };
        
        MedicalTestContext medicalContext_;
        
        // Medical-specific assertions
        std::vector<std::pair<std::string, CriticalAssertionFunction>> medicalAssertions_;
        std::vector<std::tuple<std::string, CriticalAssertionFunction, std::string>> safetyCriticalAssertions_;
        std::vector<std::tuple<std::string, std::chrono::nanoseconds, bool>> performanceAssertions_;
        std::vector<std::tuple<std::string, CriticalAssertionFunction, std::string>> complianceAssertions_;
        
        // Traceability validation
        std::function<void(TraceabilityMatrix&)> traceabilityValidator_;
        
        // Audit trail
        std::vector<CriticalEvent> criticalEvents_;
    };

    /**
     * @brief Enhanced requirement builder with medical device features
     */
    class MedicalRequirementBuilder : public speclab::RequirementBuilder {
    public:
        explicit MedicalRequirementBuilder(std::string_view requirementId, 
                                         std::string_view description,
                                         MedicalTestContext defaultContext)
            : RequirementBuilder(requirementId, description)
            , defaultContext_(std::move(defaultContext)) {}
        
        /**
         * @brief Create a medical test for this requirement
         */
        MedicalTestBuilder MedicalTest(std::string_view testId) {
            // Create context with requirement information
            MedicalTestContext context = defaultContext_;
            context.requirementId = getRequirementId();
            
            return MedicalTestBuilder(testId, std::move(context));
        }
        
        /**
         * @brief Create a medical test with custom context
         */
        MedicalTestBuilder MedicalTest(std::string_view testId, MedicalTestContext context) {
            // Ensure requirement ID is set
            context.requirementId = getRequirementId();
            
            return MedicalTestBuilder(testId, std::move(context));
        }
        
    private:
        MedicalTestContext defaultContext_;
    };

    /**
     * @brief Factory functions for medical device testing
     */
    
    /**
     * @brief Create a medical requirement with device context
     */
    MedicalRequirementBuilder MedicalRequirement(std::string_view requirementId, 
                                                std::string_view description,
                                                std::string_view deviceId,
                                                std::string_view componentName,
                                                RiskLevel riskLevel = RiskLevel::Low,
                                                SafetyClass safetyClass = SafetyClass::ClassA) {
        MedicalTestContext context;
        context.deviceId = deviceId;
        context.componentName = componentName;
        context.riskLevel = riskLevel;
        context.safetyClass = safetyClass;
        context.requiresValidation = (safetyClass >= SafetyClass::ClassB);
        context.requiresVerification = (safetyClass == SafetyClass::ClassC);
        
        return MedicalRequirementBuilder(requirementId, description, std::move(context));
    }
    
    /**
     * @brief Create a medical test with device context
     */
    MedicalTestBuilder MedicalTest(std::string_view testId,
                                  std::string_view deviceId,
                                  std::string_view componentName,
                                  RiskLevel riskLevel = RiskLevel::Low,
                                  SafetyClass safetyClass = SafetyClass::ClassA) {
        MedicalTestContext context;
        context.deviceId = deviceId;
        context.componentName = componentName;
        context.riskLevel = riskLevel;
        context.safetyClass = safetyClass;
        context.requiresValidation = (safetyClass >= SafetyClass::ClassB);
        context.requiresVerification = (safetyClass == SafetyClass::ClassC);
        
        return MedicalTestBuilder(testId, std::move(context));
    }

} // namespace speclab::medical