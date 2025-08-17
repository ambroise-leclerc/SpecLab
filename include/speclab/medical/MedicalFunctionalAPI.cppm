/**
 * @brief Medical device functional API for enhanced compliance testing - C++23 Module
 */

export module speclab.medical.functionalapi;

import std;
import speclab.core.testresult;
import speclab.core.functionalapi;
import speclab.medical.medicaltestcase;

export namespace speclab::medical {

    /**
     * @brief Enhanced test builder for medical device testing with compliance features
     */
    class MedicalTestBuilder : public speclab::TestBuilder {
    public:
        explicit MedicalTestBuilder(std::string_view testId, MedicalTestContext context)
            : TestBuilder(testId), medicalContext_(std::move(context)) {}

        /**
         * @brief Set safety class for IEC 62304 compliance
         */
        MedicalTestBuilder& SafetyClass(SafetyClass safetyClass) {
            medicalContext_.safetyClass = safetyClass;
            return *this;
        }

        /**
         * @brief Set risk level for ISO 14971 compliance
         */
        MedicalTestBuilder& RiskLevel(RiskLevel riskLevel) {
            medicalContext_.riskLevel = riskLevel;
            return *this;
        }

        /**
         * @brief Add safety critical assertion
         */
        MedicalTestBuilder& SafetyCriticalAssertion(std::string_view description, std::function<bool()> assertion) {
            safetyCriticalAssertions_.emplace_back(description, std::move(assertion));
            return *this;
        }

        /**
         * @brief Execute medical test with enhanced compliance tracking
         */
        speclab::core::TestResult ExecuteMedical() {
            auto result = Execute();
            
            // Add medical device metadata
            result.setMedicalInfo(
                medicalContext_.riskLevel == RiskLevel::Critical ? "CRITICAL" :
                medicalContext_.riskLevel == RiskLevel::High ? "HIGH" :
                medicalContext_.riskLevel == RiskLevel::Medium ? "MEDIUM" : "LOW",
                medicalContext_.complianceStandard,
                medicalContext_.componentName,
                medicalContext_.requiresValidation
            );
            
            // Execute safety critical assertions
            for (const auto& [description, assertion] : safetyCriticalAssertions_) {
                try {
                    if (!assertion()) {
                        result.status = speclab::core::TestStatus::Critical;
                        result.message = "Safety critical assertion failed: " + description;
                        break;
                    }
                } catch (const std::exception& e) {
                    result.status = speclab::core::TestStatus::Critical;
                    result.message = "Safety critical assertion exception: " + description;
                    result.errorDetails = e.what();
                    break;
                }
            }
            
            return result;
        }

    private:

        MedicalTestContext medicalContext_;
        std::vector<std::pair<std::string, std::function<bool()>>> safetyCriticalAssertions_;
    };

    /**
     * @brief Factory function to create a medical test
     */
    MedicalTestBuilder MedicalTest(std::string_view testId, 
                                   std::string_view deviceId,
                                   std::string_view componentName,
                                   RiskLevel riskLevel = RiskLevel::Low) {
        MedicalTestContext context;
        context.deviceId = std::string(deviceId);
        context.componentName = std::string(componentName);
        context.riskLevel = riskLevel;
        context.complianceStandard = "IEC_62304";
        context.requiresValidation = (riskLevel == RiskLevel::High || riskLevel == RiskLevel::Critical);
        
        return MedicalTestBuilder(testId, std::move(context));
    }

    /**
     * @brief Medical requirement builder for traceability
     */
    class MedicalRequirementBuilder {
    public:
        explicit MedicalRequirementBuilder(std::string_view requirementId, 
                                         std::string_view description,
                                         MedicalTestContext defaultContext)
            : requirementId_(requirementId)
            , description_(description)
            , defaultContext_(std::move(defaultContext)) {}

        /**
         * @brief Create a medical test for this requirement
         */
        MedicalTestBuilder Test(std::string_view testId) {
            return MedicalTestBuilder(testId, defaultContext_);
        }

        /**
         * @brief Set safety class for all tests in this requirement
         */
        MedicalRequirementBuilder& SafetyClass(SafetyClass safetyClass) {
            defaultContext_.safetyClass = safetyClass;
            return *this;
        }

        /**
         * @brief Set risk level for all tests in this requirement
         */
        MedicalRequirementBuilder& RiskLevel(RiskLevel riskLevel) {
            defaultContext_.riskLevel = riskLevel;
            return *this;
        }

    private:
        std::string requirementId_;
        std::string description_;
        MedicalTestContext defaultContext_;
    };

    /**
     * @brief Factory function to create a medical requirement
     */
    MedicalRequirementBuilder MedicalRequirement(std::string_view requirementId,
                                                std::string_view description,
                                                std::string_view deviceId) {
        MedicalTestContext context;
        context.deviceId = std::string(deviceId);
        context.complianceStandard = "IEC_62304";
        context.riskLevel = RiskLevel::Medium;
        
        return MedicalRequirementBuilder(requirementId, description, std::move(context));
    }

} // namespace speclab::medical