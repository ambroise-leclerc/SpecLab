/**
 * @brief Medical device compliance validation utilities - C++23 Module
 */

export module speclab.medical.compliancevalidator;

import std;
import speclab.core.testresult;

export namespace speclab::medical {

    /**
     * @brief Compliance standards enumeration
     */
    enum class ComplianceStandard {
        IEC_62304,      ///< Medical device software lifecycle processes
        ISO_13485,      ///< Quality management systems for medical devices
        ISO_14971,      ///< Risk management for medical devices
        IEC_60601_1,    ///< General requirements for basic safety and essential performance
        FDA_510K,       ///< FDA premarket notification
        CE_MDR,         ///< EU Medical Device Regulation
        Custom          ///< Custom compliance standard
    };
    
    /**
     * @brief Convert compliance standard to string
     */
    constexpr std::string_view toString(ComplianceStandard standard) noexcept {
        switch (standard) {
            case ComplianceStandard::IEC_62304:    return "IEC_62304";
            case ComplianceStandard::ISO_13485:    return "ISO_13485";
            case ComplianceStandard::ISO_14971:    return "ISO_14971";
            case ComplianceStandard::IEC_60601_1:  return "IEC_60601_1";
            case ComplianceStandard::FDA_510K:     return "FDA_510K";
            case ComplianceStandard::CE_MDR:       return "CE_MDR";
            case ComplianceStandard::Custom:       return "CUSTOM";
            default:                               return "UNKNOWN";
        }
    }

    /**
     * @brief Compliance validation result
     */
    struct ComplianceResult {
        bool isCompliant = false;
        ComplianceStandard standard = ComplianceStandard::IEC_62304;
        std::string requirementId;
        std::string description;
        std::vector<std::string> violations;
        std::vector<std::string> warnings;
        std::unordered_map<std::string, std::string> evidence;
        std::chrono::system_clock::time_point validationTime = std::chrono::system_clock::now();
        
        /**
         * @brief Add a compliance violation
         * @param violation Description of the violation
         */
        void addViolation(std::string_view violation) {
            violations.emplace_back(violation);
            isCompliant = false;
        }
        
        /**
         * @brief Add a compliance warning
         * @param warning Description of the warning
         */
        void addWarning(std::string_view warning) {
            warnings.emplace_back(warning);
        }
        
        /**
         * @brief Add evidence for compliance
         * @param key Evidence key/identifier
         * @param value Evidence value/description
         */
        void addEvidence(std::string_view key, std::string_view value) {
            evidence[std::string(key)] = std::string(value);
        }
        
        /**
         * @brief Check if there are any violations
         * @return True if violations exist
         */
        bool hasViolations() const noexcept {
            return !violations.empty();
        }
        
        /**
         * @brief Check if there are any warnings
         * @return True if warnings exist
         */
        bool hasWarnings() const noexcept {
            return !warnings.empty();
        }
        
        /**
         * @brief Generate compliance report
         * @return Formatted compliance report string
         */
        std::string generateReport() const {
            std::ostringstream report;
            report << "=== COMPLIANCE VALIDATION REPORT ===\n";
            report << "Standard: " << toString(standard) << "\n";
            report << "Requirement: " << requirementId << "\n";
            report << "Status: " << (isCompliant ? "COMPLIANT" : "NON-COMPLIANT") << "\n";
            report << "Validation Time: " << std::format("{:%Y-%m-%d %H:%M:%S}", validationTime) << "\n\n";
            
            if (!description.empty()) {
                report << "Description: " << description << "\n\n";
            }
            
            if (hasViolations()) {
                report << "VIOLATIONS:\n";
                for (const auto& violation : violations) {
                    report << "  - " << violation << "\n";
                }
                report << "\n";
            }
            
            if (hasWarnings()) {
                report << "WARNINGS:\n";
                for (const auto& warning : warnings) {
                    report << "  - " << warning << "\n";
                }
                report << "\n";
            }
            
            if (!evidence.empty()) {
                report << "EVIDENCE:\n";
                for (const auto& [key, value] : evidence) {
                    report << "  " << key << ": " << value << "\n";
                }
                report << "\n";
            }
            
            return report.str();
        }
    };

    /**
     * @brief IEC 62304 software lifecycle process validator
     */
    class IEC62304Validator {
    public:
        /**
         * @brief Validate software planning process (5.1)
         * @param testResults Collection of test results to validate
         * @return Compliance validation result
         */
        static ComplianceResult validateSoftwarePlanning(const speclab::core::TestResultCollection& testResults) {
            ComplianceResult result;
            result.standard = ComplianceStandard::IEC_62304;
            result.requirementId = "5.1";
            result.description = "Software development planning";
            
            // Check for required planning documentation tests
            bool hasPlanningTests = false;
            bool hasLifecycleModel = false;
            bool hasConfigurationManagement = false;
            
            for (const auto& testResult : testResults.getResults()) {
                const auto& metadata = testResult.metadata;
                
                if (metadata.find("lifecycle_process") != metadata.end()) {
                    const auto& process = metadata.at("lifecycle_process");
                    if (process == "PLANNING") {
                        hasPlanningTests = true;
                        result.addEvidence("planning_test", testResult.testId);
                    }
                }
                
                if (testResult.testId.find("lifecycle_model") != std::string::npos) {
                    hasLifecycleModel = true;
                    result.addEvidence("lifecycle_model_test", testResult.testId);
                }
                
                if (testResult.testId.find("configuration") != std::string::npos) {
                    hasConfigurationManagement = true;
                    result.addEvidence("configuration_test", testResult.testId);
                }
            }
            
            if (!hasPlanningTests) {
                result.addViolation("No software planning process tests found");
            }
            
            if (!hasLifecycleModel) {
                result.addWarning("No lifecycle model validation tests found");
            }
            
            if (!hasConfigurationManagement) {
                result.addViolation("No configuration management tests found");
            }
            
            result.isCompliant = !result.hasViolations();
            return result;
        }
        
        /**
         * @brief Validate software requirements analysis (5.2)
         * @param testResults Collection of test results to validate
         * @return Compliance validation result
         */
        static ComplianceResult validateRequirementsAnalysis(const speclab::core::TestResultCollection& testResults) {
            ComplianceResult result;
            result.standard = ComplianceStandard::IEC_62304;
            result.requirementId = "5.2";
            result.description = "Software requirements analysis";
            
            bool hasRequirementTests = false;
            bool hasTraceability = false;
            bool hasVerification = false;
            std::size_t requirementCoverage = 0;
            
            for (const auto& testResult : testResults.getResults()) {
                const auto& metadata = testResult.metadata;
                
                if (metadata.find("lifecycle_process") != metadata.end() &&
                    metadata.at("lifecycle_process") == "REQUIREMENTS") {
                    hasRequirementTests = true;
                    requirementCoverage++;
                    result.addEvidence("requirement_test", testResult.testId);
                }
                
                if (metadata.find("requirement_id") != metadata.end() &&
                    !metadata.at("requirement_id").empty()) {
                    hasTraceability = true;
                    result.addEvidence("traceability", testResult.testId + " -> " + metadata.at("requirement_id"));
                }
                
                if (metadata.find("verification_required") != metadata.end() &&
                    metadata.at("verification_required") == "true") {
                    hasVerification = true;
                }
            }
            
            if (!hasRequirementTests) {
                result.addViolation("No requirements analysis tests found");
            }
            
            if (!hasTraceability) {
                result.addViolation("No requirement traceability found in tests");
            }
            
            if (!hasVerification) {
                result.addWarning("No verification requirements specified");
            }
            
            if (requirementCoverage < 5) {
                result.addWarning(std::format("Low requirement test coverage: {} tests", requirementCoverage));
            }
            
            result.addEvidence("requirement_coverage", std::to_string(requirementCoverage));
            result.isCompliant = !result.hasViolations();
            return result;
        }
        
        /**
         * @brief Validate software risk management (7.1-7.4)
         * @param testResults Collection of test results to validate
         * @return Compliance validation result
         */
        static ComplianceResult validateRiskManagement(const speclab::core::TestResultCollection& testResults) {
            ComplianceResult result;
            result.standard = ComplianceStandard::IEC_62304;
            result.requirementId = "7.1-7.4";
            result.description = "Software risk management";
            
            bool hasRiskAnalysis = false;
            bool hasCriticalTests = false;
            bool hasRiskMitigation = false;
            std::size_t criticalFailures = testResults.getCriticalCount();
            
            for (const auto& testResult : testResults.getResults()) {
                const auto& metadata = testResult.metadata;
                
                if (metadata.find("lifecycle_process") != metadata.end() &&
                    metadata.at("lifecycle_process") == "RISK_MANAGEMENT") {
                    hasRiskAnalysis = true;
                    result.addEvidence("risk_analysis_test", testResult.testId);
                }
                
                if (testResult.critical()) {
                    hasCriticalTests = true;
                    result.addEvidence("critical_failure", testResult.testId + ": " + testResult.message);
                }
                
                if (metadata.find("hazard_id") != metadata.end() &&
                    !metadata.at("hazard_id").empty()) {
                    hasRiskMitigation = true;
                    result.addEvidence("hazard_tracking", testResult.testId + " -> " + metadata.at("hazard_id"));
                }
            }
            
            if (!hasRiskAnalysis) {
                result.addViolation("No risk management process tests found");
            }
            
            if (criticalFailures > 0) {
                result.addViolation(std::format("{} critical failures detected", criticalFailures));
            }
            
            if (!hasRiskMitigation) {
                result.addWarning("No hazard tracking found in tests");
            }
            
            result.addEvidence("critical_failure_count", std::to_string(criticalFailures));
            result.isCompliant = !result.hasViolations();
            return result;
        }
        
        /**
         * @brief Validate complete IEC 62304 compliance
         * @param testResults Collection of test results to validate
         * @return Combined compliance validation result
         */
        static ComplianceResult validateCompleteCompliance(const speclab::core::TestResultCollection& testResults) {
            ComplianceResult combinedResult;
            combinedResult.standard = ComplianceStandard::IEC_62304;
            combinedResult.requirementId = "COMPLETE";
            combinedResult.description = "Complete IEC 62304 compliance validation";
            
            // Validate all major processes
            auto planningResult = validateSoftwarePlanning(testResults);
            auto requirementsResult = validateRequirementsAnalysis(testResults);
            auto riskResult = validateRiskManagement(testResults);
            
            // Combine results
            for (const auto& violation : planningResult.violations) {
                combinedResult.addViolation("Planning: " + violation);
            }
            for (const auto& violation : requirementsResult.violations) {
                combinedResult.addViolation("Requirements: " + violation);
            }
            for (const auto& violation : riskResult.violations) {
                combinedResult.addViolation("Risk Management: " + violation);
            }
            
            // Combine warnings
            for (const auto& warning : planningResult.warnings) {
                combinedResult.addWarning("Planning: " + warning);
            }
            for (const auto& warning : requirementsResult.warnings) {
                combinedResult.addWarning("Requirements: " + warning);
            }
            for (const auto& warning : riskResult.warnings) {
                combinedResult.addWarning("Risk Management: " + warning);
            }
            
            // Combine evidence
            for (const auto& [key, value] : planningResult.evidence) {
                combinedResult.addEvidence("planning_" + key, value);
            }
            for (const auto& [key, value] : requirementsResult.evidence) {
                combinedResult.addEvidence("requirements_" + key, value);
            }
            for (const auto& [key, value] : riskResult.evidence) {
                combinedResult.addEvidence("risk_" + key, value);
            }
            
            // Overall compliance assessment
            combinedResult.isCompliant = planningResult.isCompliant && 
                                       requirementsResult.isCompliant && 
                                       riskResult.isCompliant;
            
            // Add overall metrics
            combinedResult.addEvidence("total_tests", std::to_string(testResults.getTotalCount()));
            combinedResult.addEvidence("success_rate", std::format("{:.1f}%", testResults.getSuccessRate()));
            combinedResult.addEvidence("critical_failures", std::to_string(testResults.getCriticalCount()));
            
            return combinedResult;
        }
    };

    /**
     * @brief ISO 14971 risk management validator
     */
    class ISO14971Validator {
    public:
        /**
         * @brief Validate risk analysis completeness
         * @param testResults Collection of test results to validate
         * @return Compliance validation result
         */
        static ComplianceResult validateRiskAnalysis(const speclab::core::TestResultCollection& testResults) {
            ComplianceResult result;
            result.standard = ComplianceStandard::ISO_14971;
            result.requirementId = "4.3";
            result.description = "Risk analysis and evaluation";
            
            std::unordered_set<std::string> identifiedHazards;
            std::unordered_set<std::string> riskLevels;
            bool hasRiskAssessment = false;
            
            for (const auto& testResult : testResults.getResults()) {
                const auto& metadata = testResult.metadata;
                
                // Check for hazard identification
                if (metadata.find("hazard_id") != metadata.end() &&
                    !metadata.at("hazard_id").empty()) {
                    identifiedHazards.insert(metadata.at("hazard_id"));
                    result.addEvidence("hazard", metadata.at("hazard_id"));
                }
                
                // Check for risk levels
                if (metadata.find("regulatory_risk_level") != metadata.end()) {
                    riskLevels.insert(metadata.at("regulatory_risk_level"));
                }
                
                // Check for risk assessment tests
                if (testResult.testId.find("risk_assessment") != std::string::npos ||
                    testResult.testId.find("hazard_analysis") != std::string::npos) {
                    hasRiskAssessment = true;
                    result.addEvidence("risk_assessment_test", testResult.testId);
                }
            }
            
            if (identifiedHazards.empty()) {
                result.addViolation("No hazards identified in test results");
            } else {
                result.addEvidence("hazard_count", std::to_string(identifiedHazards.size()));
            }
            
            if (riskLevels.empty()) {
                result.addWarning("No risk levels specified in tests");
            }
            
            if (!hasRiskAssessment) {
                result.addViolation("No risk assessment tests found");
            }
            
            result.isCompliant = !result.hasViolations();
            return result;
        }
        
        /**
         * @brief Validate risk control measures
         * @param testResults Collection of test results to validate
         * @return Compliance validation result
         */
        static ComplianceResult validateRiskControl(const speclab::core::TestResultCollection& testResults) {
            ComplianceResult result;
            result.standard = ComplianceStandard::ISO_14971;
            result.requirementId = "4.4";
            result.description = "Risk control measures";
            
            bool hasControlMeasures = false;
            bool hasResidualRiskAssessment = false;
            std::size_t mitigatedRisks = 0;
            
            for (const auto& testResult : testResults.getResults()) {
                const auto& metadata = testResult.metadata;
                
                if (testResult.testId.find("control_measure") != std::string::npos ||
                    testResult.testId.find("mitigation") != std::string::npos) {
                    hasControlMeasures = true;
                    mitigatedRisks++;
                    result.addEvidence("control_measure", testResult.testId);
                }
                
                if (testResult.testId.find("residual_risk") != std::string::npos) {
                    hasResidualRiskAssessment = true;
                    result.addEvidence("residual_risk_test", testResult.testId);
                }
                
                // Check for risk control metadata
                if (metadata.find("control_effectiveness") != metadata.end()) {
                    result.addEvidence("control_effectiveness", testResult.testId + ": " + metadata.at("control_effectiveness"));
                }
            }
            
            if (!hasControlMeasures) {
                result.addViolation("No risk control measures tested");
            }
            
            if (!hasResidualRiskAssessment) {
                result.addWarning("No residual risk assessment tests found");
            }
            
            result.addEvidence("mitigated_risks", std::to_string(mitigatedRisks));
            result.isCompliant = !result.hasViolations();
            return result;
        }
    };

    /**
     * @brief Comprehensive compliance validator for medical devices
     */
    class ComplianceValidator {
    public:
        /**
         * @brief Validate test results against specified standard
         * @param testResults Collection of test results
         * @param standard Compliance standard to validate against
         * @return Compliance validation result
         */
        static ComplianceResult validate(const speclab::core::TestResultCollection& testResults,
                                       ComplianceStandard standard) {
            switch (standard) {
                case ComplianceStandard::IEC_62304:
                    return IEC62304Validator::validateCompleteCompliance(testResults);
                
                case ComplianceStandard::ISO_14971:
                    return validateISO14971(testResults);
                
                case ComplianceStandard::ISO_13485:
                    return validateISO13485(testResults);
                
                default:
                    return createUnsupportedStandardResult(standard);
            }
        }
        
        /**
         * @brief Validate against multiple standards
         * @param testResults Collection of test results
         * @param standards Vector of standards to validate against
         * @return Vector of compliance results
         */
        static std::vector<ComplianceResult> validateMultiple(
            const speclab::core::TestResultCollection& testResults,
            const std::vector<ComplianceStandard>& standards) {
            
            std::vector<ComplianceResult> results;
            results.reserve(standards.size());
            
            for (const auto& standard : standards) {
                results.push_back(validate(testResults, standard));
            }
            
            return results;
        }
        
        /**
         * @brief Generate comprehensive compliance report
         * @param testResults Collection of test results
         * @param standards Standards to validate against
         * @return Formatted compliance report
         */
        static std::string generateComplianceReport(
            const speclab::core::TestResultCollection& testResults,
            const std::vector<ComplianceStandard>& standards) {
            
            std::ostringstream report;
            report << "=== MEDICAL DEVICE COMPLIANCE REPORT ===\n\n";
            
            auto validationResults = validateMultiple(testResults, standards);
            
            bool overallCompliant = true;
            for (const auto& result : validationResults) {
                report << result.generateReport() << "\n";
                if (!result.isCompliant) {
                    overallCompliant = false;
                }
            }
            
            report << "=== OVERALL COMPLIANCE STATUS ===\n";
            report << "Status: " << (overallCompliant ? "COMPLIANT" : "NON-COMPLIANT") << "\n";
            report << "Total Tests: " << testResults.getTotalCount() << "\n";
            report << "Success Rate: " << std::format("{:.1f}%", testResults.getSuccessRate()) << "\n";
            report << "Critical Failures: " << testResults.getCriticalCount() << "\n";
            
            return report.str();
        }
        
    private:
        static ComplianceResult validateISO14971(const speclab::core::TestResultCollection& testResults) {
            auto riskAnalysisResult = ISO14971Validator::validateRiskAnalysis(testResults);
            auto riskControlResult = ISO14971Validator::validateRiskControl(testResults);
            
            ComplianceResult combinedResult;
            combinedResult.standard = ComplianceStandard::ISO_14971;
            combinedResult.requirementId = "COMPLETE";
            combinedResult.description = "Complete ISO 14971 risk management validation";
            
            // Combine violations and warnings
            for (const auto& violation : riskAnalysisResult.violations) {
                combinedResult.addViolation("Risk Analysis: " + violation);
            }
            for (const auto& violation : riskControlResult.violations) {
                combinedResult.addViolation("Risk Control: " + violation);
            }
            
            for (const auto& warning : riskAnalysisResult.warnings) {
                combinedResult.addWarning("Risk Analysis: " + warning);
            }
            for (const auto& warning : riskControlResult.warnings) {
                combinedResult.addWarning("Risk Control: " + warning);
            }
            
            // Combine evidence
            for (const auto& [key, value] : riskAnalysisResult.evidence) {
                combinedResult.addEvidence("analysis_" + key, value);
            }
            for (const auto& [key, value] : riskControlResult.evidence) {
                combinedResult.addEvidence("control_" + key, value);
            }
            
            combinedResult.isCompliant = riskAnalysisResult.isCompliant && riskControlResult.isCompliant;
            return combinedResult;
        }
        
        static ComplianceResult validateISO13485(const speclab::core::TestResultCollection& testResults) {
            ComplianceResult result;
            result.standard = ComplianceStandard::ISO_13485;
            result.requirementId = "BASIC";
            result.description = "Basic ISO 13485 quality management validation";
            
            // Basic quality metrics validation
            double successRate = testResults.getSuccessRate();
            std::size_t totalTests = testResults.getTotalCount();
            
            if (successRate < 95.0) {
                result.addViolation(std::format("Success rate {:.1f}% below required 95%", successRate));
            }
            
            if (totalTests < 10) {
                result.addWarning("Low test coverage for quality validation");
            }
            
            result.addEvidence("success_rate", std::format("{:.1f}%", successRate));
            result.addEvidence("total_tests", std::to_string(totalTests));
            
            result.isCompliant = !result.hasViolations();
            return result;
        }
        
        static ComplianceResult createUnsupportedStandardResult(ComplianceStandard standard) {
            ComplianceResult result;
            result.standard = standard;
            result.requirementId = "UNSUPPORTED";
            result.description = "Unsupported compliance standard";
            result.addViolation(std::format("Validation for {} not implemented", toString(standard)));
            return result;
        }
    };

} // namespace speclab::medical