/**
 * @brief Requirement traceability and feature organization API - C++23 Module
 */

export module speclab.core.requirementapi;

import std;
import speclab.core.testresult;
import speclab.core.functionalapi;
import speclab.core.requirements;

export namespace speclab {

    /**
     * @brief Requirement builder for traceability and compliance
     */
    class RequirementBuilder {
    public:
        explicit RequirementBuilder(std::string_view requirementId, std::string_view description)
            : requirementId_(requirementId)
            , description_(description)
            , riskLevel_("LOW")
            , complianceStandard_("IEC_62304")
            , enabled_(true) {}
        
        /**
         * @brief Set safety class for IEC 62304 compliance
         */
        RequirementBuilder& SafetyClass(std::string_view safetyClass) {
            // Normalised on the way in, so the registry, the trace matrix and the metadata all
            // carry one spelling. The coverage gate compares against the canonical form.
            safetyClass_ = speclab::core::NormalizeSafetyClass(safetyClass);
            const std::string_view canonical{safetyClass_};

            // Auto-set audit requirements based on safety class
            if (canonical == "CLASS_C") {
                requiresAudit_ = true;
                requiresValidation_ = true;
            } else if (canonical == "CLASS_B") {
                requiresAudit_ = true;
            }
            
            return *this;
        }
        
        /**
         * @brief Set risk level for ISO 14971 compliance
         */
        RequirementBuilder& RiskLevel(std::string_view riskLevel) {
            riskLevel_ = speclab::core::NormalizeRiskLevel(riskLevel);
            const std::string_view canonical{riskLevel_};

            // Auto-escalate based on risk level
            if (canonical == "CRITICAL") {
                requiresValidation_ = true;
                requiresAudit_ = true;
            } else if (canonical == "HIGH") {
                requiresValidation_ = true;
            }
            
            return *this;
        }
        
        /**
         * @brief Set compliance standard
         */
        RequirementBuilder& ComplianceStandard(std::string_view standard) {
            complianceStandard_ = standard;
            return *this;
        }
        
        /**
         * @brief Set risk assessment information
         */
        RequirementBuilder& RiskAssessment(std::string_view riskId, std::string_view description) {
            riskAssessmentId_ = riskId;
            riskAssessmentDescription_ = description;
            return *this;
        }
        
        /**
         * @brief Set hazard ID for risk management
         */
        RequirementBuilder& HazardId(std::string_view hazardId) {
            hazardId_ = hazardId;
            return *this;
        }
        
        /**
         * @brief Set performance requirement threshold
         */
        RequirementBuilder& PerformanceRequirement(std::chrono::nanoseconds threshold) {
            performanceThreshold_ = threshold;
            return *this;
        }
        
        /**
         * @brief Set fault tolerance category
         */
        RequirementBuilder& FaultTolerance(std::string_view category) {
            faultToleranceCategory_ = category;
            return *this;
        }
        
        /**
         * @brief Add resource constraint
         */
        RequirementBuilder& ResourceConstraint(std::string_view resource, std::string_view limit) {
            resourceConstraints_[std::string(resource)] = std::string(limit);
            return *this;
        }
        
        /**
         * @brief Add custom metadata
         */
        RequirementBuilder& Metadata(std::string_view key, std::string_view value) {
            metadata_[std::string(key)] = std::string(value);
            return *this;
        }
        
        /**
         * @brief Associate a test ID with this requirement for traceability
         *
         * For a test that runs elsewhere. The link is registered when Execute() runs, so the
         * traceability matrix and the coverage gate see it, but this builder does not run it.
         */
        RequirementBuilder& AssociateTest(std::string_view testId) {
            tests_.push_back(std::string(testId));
            associatedTests_.push_back(std::string(testId));
            return *this;
        }

        /**
         * @brief Add a test to this requirement, and return its builder
         *
         *     auto requirement = speclab::Requirement("REQ-100", "Boots in under 250 ms")
         *                            .RiskLevel("HIGH");
         *     requirement.Test("T_BootTime")
         *         .When("the device boots", [] { ... })
         *         .Then("it took less than 250 ms", [] { ... });
         *     const auto results = requirement.Execute();
         *
         * Keep the requirement in a variable: the chained calls return the TestBuilder, and
         * Execute() is what runs the tests and attaches the requirement's metadata to their
         * results. The returned reference stays valid when further tests are added (std::deque).
         */
        TestBuilder& Test(std::string_view testId) {
            testBuilders_.emplace_back(testId);
            tests_.push_back(std::string(testId));
            return testBuilders_.back();
        }

        /**
         * @brief Enable/disable this requirement's tests
         */
        RequirementBuilder& SetEnabled(bool enabled) {
            enabled_ = enabled;
            return *this;
        }
        
        /**
         * @brief Register this requirement, run its tests, and annotate their results
         *
         * Never returns an empty vector: a requirement with no test of its own yields one Blocked
         * result naming it. An empty vector would read as "nothing failed" to any caller counting
         * failures, which is the worst way for a test framework to be wrong.
         */
        std::vector<speclab::core::TestResult> Execute() {
            // The requirement is always registered, so the coverage gate sees it. Its test links
            // are rebuilt from what this execution actually ran: a test that did not run must not
            // make the requirement look covered, and a link left by an earlier execution must not
            // survive the requirement being disabled.
            registerForTraceability();

            std::vector<speclab::core::TestResult> results;
            std::vector<std::string> testsThatRan;

            if (!enabled_) {
                results.push_back(syntheticResult(speclab::core::TestStatus::Skipped,
                                                  "Requirement disabled"));
            } else if (testBuilders_.empty()) {
                results.push_back(syntheticResult(
                    speclab::core::TestStatus::Blocked,
                    tests_.empty()
                        ? "No test attached to this requirement"
                        : std::format("No test attached to this requirement; {} test id(s) "
                                      "associated for traceability only",
                                      tests_.size())));
            } else {
                results.reserve(testBuilders_.size());
                for (TestBuilder& test : testBuilders_) {
                    const speclab::core::TestResult result = test.Execute();
                    // Only a test that actually ran covers the requirement.
                    if (result.status != speclab::core::TestStatus::Skipped) {
                        testsThatRan.push_back(test.getTestId());
                    }
                    results.push_back(result);
                }
            }

            relinkTests(testsThatRan);

            // Add requirement metadata to all results
            for (auto& result : results) {
                result.addMetadata("requirement_id", requirementId_);
                result.addMetadata("requirement_description", description_);
                result.addMetadata("safety_class", safetyClass_);
                result.addMetadata("risk_level", riskLevel_);
                result.addMetadata("compliance_standard", complianceStandard_);
                result.addMetadata("requires_audit", requiresAudit_ ? "true" : "false");
                result.addMetadata("requires_validation", requiresValidation_ ? "true" : "false");
                
                if (!riskAssessmentId_.empty()) {
                    result.addMetadata("risk_assessment_id", riskAssessmentId_);
                    result.addMetadata("risk_assessment_description", riskAssessmentDescription_);
                }
                
                if (!hazardId_.empty()) {
                    result.addMetadata("hazard_id", hazardId_);
                }
                
                if (!faultToleranceCategory_.empty()) {
                    result.addMetadata("fault_tolerance_category", faultToleranceCategory_);
                }
                
                if (performanceThreshold_.count() > 0) {
                    result.addMetadata("performance_threshold_ns", std::to_string(performanceThreshold_.count()));
                }
                
                // Add resource constraints
                for (const auto& [resource, limit] : resourceConstraints_) {
                    result.addMetadata("constraint_" + resource, limit);
                }
                
                // Add custom metadata
                for (const auto& [key, value] : metadata_) {
                    result.addMetadata("req_" + key, value);
                }

                // Traceability on the result itself, as TestSuite does for registered links.
                result.requirementIds.push_back(requirementId_);
            }

            return results;
        }

        // Getters for introspection
        const std::string& getRequirementId() const noexcept { return requirementId_; }
        const std::string& getDescription() const noexcept { return description_; }
        const std::string& getSafetyClass() const noexcept { return safetyClass_; }
        const std::string& getRiskLevel() const noexcept { return riskLevel_; }
        const std::vector<std::string>& getTests() const noexcept { return tests_; }
        bool requiresAudit() const noexcept { return requiresAudit_; }
        bool requiresValidation() const noexcept { return requiresValidation_; }
        bool isEnabled() const noexcept { return enabled_; }

    private:
        /// Puts this requirement in the registry, so that ExportTraceMatrixCSV/HTML and the
        /// REQUIREMENT_COVERAGE gate see it.
        void registerForTraceability() const {
            speclab::core::RegisterRequirement({
                .id = requirementId_,
                .description = description_,
                .riskLevel = riskLevel_,
                .safetyClass = safetyClass_,
                .requiresAudit = requiresAudit_,
                .requiresValidation = requiresValidation_,
                .source = complianceStandard_,
            });
        }

        /// Rebuilds this requirement's test links from one execution: the tests it ran and did not
        /// skip, plus the ids associated for traceability, which run elsewhere. Links from an
        /// earlier execution are dropped first, so disabling a requirement - or its tests - takes
        /// its coverage away instead of leaving a stale link behind.
        ///
        /// Only ids of real tests are linked. The synthetic result of a requirement with no test
        /// carries the requirement's own id, and linking that would make the requirement cover
        /// itself - the exact false pass this API is meant to stop producing.
        void relinkTests(const std::vector<std::string>& testsThatRan) const {
            speclab::core::ClearRequirementTestLinks(requirementId_);
            if (!enabled_) {
                return;
            }
            for (const std::string& testId : testsThatRan) {
                speclab::core::LinkTestRequirement(testId, requirementId_);
            }
            for (const std::string& testId : associatedTests_) {
                speclab::core::LinkTestRequirement(testId, requirementId_);
            }
        }

        /// A result that stands for the requirement itself rather than for one of its tests.
        [[nodiscard]] speclab::core::TestResult syntheticResult(speclab::core::TestStatus status,
                                                                std::string_view message) const {
            speclab::core::TestResult result(status, message);
            result.testId = requirementId_;
            return result;
        }

        std::string requirementId_;
        std::string description_;
        std::string safetyClass_;
        std::string riskLevel_;
        std::string complianceStandard_;
        std::string riskAssessmentId_;
        std::string riskAssessmentDescription_;
        std::string hazardId_;
        std::string faultToleranceCategory_;
        std::chrono::nanoseconds performanceThreshold_{0};
        bool requiresAudit_ = false;
        bool requiresValidation_ = false;
        bool enabled_;
        
        std::unordered_map<std::string, std::string> resourceConstraints_;
        std::unordered_map<std::string, std::string> metadata_;
        std::vector<std::string> tests_;  // Test IDs for traceability, run here or elsewhere
        std::vector<std::string> associatedTests_;  // AssociateTest() ids only: they run elsewhere
        // std::deque, not std::vector: Test() hands out references to its elements, and a vector
        // would invalidate them as soon as another test is added.
        std::deque<TestBuilder> testBuilders_;
    };

    /**
     * @brief Feature builder for organizing related requirements and tests
     */
    class FeatureBuilder {
    public:
        explicit FeatureBuilder(std::string_view featureName)
            : featureName_(featureName), enabled_(true) {}
        
        /**
         * @brief Set feature description
         */
        FeatureBuilder& Description(std::string_view description) {
            description_ = description;
            return *this;
        }
        
        /**
         * @brief Add a requirement to this feature
         */
        RequirementBuilder& Requirement(std::string_view requirementId, std::string_view description) {
            requirements_.emplace_back(requirementId, description);
            return requirements_.back();
        }
        
        /**
         * @brief Set feature category
         */
        FeatureBuilder& Category(std::string_view category) {
            category_ = category;
            return *this;
        }
        
        /**
         * @brief Set medical device component
         */
        FeatureBuilder& DeviceComponent(std::string_view component) {
            deviceComponent_ = component;
            return *this;
        }
        
        /**
         * @brief Set overall feature risk level
         */
        FeatureBuilder& RiskLevel(std::string_view riskLevel) {
            featureRiskLevel_ = riskLevel;
            return *this;
        }
        
        /**
         * @brief Add feature metadata
         */
        FeatureBuilder& Metadata(std::string_view key, std::string_view value) {
            metadata_[std::string(key)] = std::string(value);
            return *this;
        }
        
        /**
         * @brief Enable/disable the entire feature
         */
        FeatureBuilder& SetEnabled(bool enabled) {
            enabled_ = enabled;
            return *this;
        }
        
        /**
         * @brief Execute all requirements and tests in this feature
         */
        std::vector<speclab::core::TestResult> Execute() {
            std::vector<speclab::core::TestResult> allResults;
            
            if (!enabled_) {
                // Return skipped result for the feature
                speclab::core::TestResult featureResult(speclab::core::TestStatus::Skipped, "Feature disabled");
                featureResult.testId = featureName_;
                featureResult.addMetadata("feature_name", featureName_);
                featureResult.addMetadata("feature_status", "disabled");
                allResults.push_back(std::move(featureResult));
                return allResults;
            }
            
            if (requirements_.empty()) {
                // Not an empty vector: a feature with no requirement must not look like a feature
                // whose tests all passed.
                speclab::core::TestResult featureResult(speclab::core::TestStatus::Blocked,
                                                        "No requirement attached to this feature");
                featureResult.testId = featureName_;
                featureResult.addMetadata("feature_name", featureName_);
                allResults.push_back(std::move(featureResult));
                return allResults;
            }

            // Execute all requirements
            for (auto& requirement : requirements_) {
                auto requirementResults = requirement.Execute();
                
                // Add feature metadata to all results
                for (auto& result : requirementResults) {
                    result.addMetadata("feature_name", featureName_);
                    result.addMetadata("feature_description", description_);
                    result.addMetadata("feature_category", category_);
                    result.addMetadata("feature_device_component", deviceComponent_);
                    result.addMetadata("feature_risk_level", featureRiskLevel_);
                    
                    // Add custom metadata
                    for (const auto& [key, value] : metadata_) {
                        result.addMetadata("feature_" + key, value);
                    }
                }
                
                // Append to results
                allResults.insert(allResults.end(), 
                                requirementResults.begin(), 
                                requirementResults.end());
            }
            
            return allResults;
        }
        
        // Getters for introspection
        const std::string& getFeatureName() const noexcept { return featureName_; }
        const std::string& getDescription() const noexcept { return description_; }
        const std::string& getCategory() const noexcept { return category_; }
        const std::deque<RequirementBuilder>& getRequirements() const noexcept { return requirements_; }
        bool isEnabled() const noexcept { return enabled_; }
        
    private:
        std::string featureName_;
        std::string description_;
        std::string category_;
        std::string deviceComponent_;
        std::string featureRiskLevel_;
        bool enabled_;
        
        std::unordered_map<std::string, std::string> metadata_;
        // std::deque for the same reason as RequirementBuilder's tests: Requirement() returns a
        // reference into this container, and a vector would invalidate it on the next insertion.
        std::deque<RequirementBuilder> requirements_;
    };

    /**
     * @brief IEC 62304 lifecycle process builder
     */
    class IEC62304ProcessBuilder {
    public:
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
        
        explicit IEC62304ProcessBuilder(LifecycleProcess process)
            : process_(process), enabled_(true) {
            complianceStandard_ = "IEC_62304";
        }
        
        /**
         * @brief Add a requirement for this lifecycle process
         */
        RequirementBuilder& Requirement(std::string_view requirementId, std::string_view description) {
            RequirementBuilder builder(requirementId, description);
            builder.ComplianceStandard(complianceStandard_);
            builder.Metadata("lifecycle_process", toString(process_));
            
            requirements_.push_back(std::move(builder));
            return requirements_.back();
        }
        
        /**
         * @brief Set device safety class
         */
        IEC62304ProcessBuilder& SafetyClass(std::string_view safetyClass) {
            safetyClass_ = safetyClass;
            return *this;
        }
        
        /**
         * @brief Execute all requirements for this lifecycle process
         */
        std::vector<speclab::core::TestResult> Execute() {
            std::vector<speclab::core::TestResult> allResults;

            if (requirements_.empty()) {
                speclab::core::TestResult processResult(
                    speclab::core::TestStatus::Blocked,
                    "No requirement attached to this lifecycle process");
                processResult.testId = toString(process_);
                processResult.addMetadata("iec62304_process", toString(process_));
                allResults.push_back(std::move(processResult));
                return allResults;
            }

            for (auto& requirement : requirements_) {
                auto results = requirement.Execute();
                
                // Add IEC 62304 specific metadata
                for (auto& result : results) {
                    result.addMetadata("iec62304_process", toString(process_));
                    result.addMetadata("iec62304_safety_class", safetyClass_);
                    result.addMetadata("compliance_standard", complianceStandard_);
                }
                
                allResults.insert(allResults.end(), results.begin(), results.end());
            }
            
            return allResults;
        }
        
        // Getters for introspection
        bool isEnabled() const noexcept { return enabled_; }
        
    private:
        std::string toString(LifecycleProcess process) const {
            switch (process) {
                case LifecycleProcess::Planning: return "PLANNING";
                case LifecycleProcess::Requirements: return "REQUIREMENTS";
                case LifecycleProcess::Architecture: return "ARCHITECTURE";
                case LifecycleProcess::Design: return "DESIGN";
                case LifecycleProcess::Implementation: return "IMPLEMENTATION";
                case LifecycleProcess::Integration: return "INTEGRATION";
                case LifecycleProcess::SystemTesting: return "SYSTEM_TESTING";
                case LifecycleProcess::RiskManagement: return "RISK_MANAGEMENT";
                case LifecycleProcess::Configuration: return "CONFIGURATION";
                case LifecycleProcess::ProblemResolution: return "PROBLEM_RESOLUTION";
                case LifecycleProcess::Maintenance: return "MAINTENANCE";
                default: return "UNKNOWN";
            }
        }
        
        LifecycleProcess process_;
        std::string safetyClass_;
        std::string complianceStandard_;
        bool enabled_;
        
        // std::deque: Requirement() returns a reference into this container (see FeatureBuilder).
        std::deque<RequirementBuilder> requirements_;
    };

    /**
     * @brief Factory function to create a requirement
     */
    RequirementBuilder Requirement(std::string_view requirementId, std::string_view description) {
        return RequirementBuilder(requirementId, description);
    }
    
    /**
     * @brief Factory function to create a feature
     */
    FeatureBuilder Feature(std::string_view featureName) {
        return FeatureBuilder(featureName);
    }
    
    /**
     * @brief Factory function to create an IEC 62304 process
     */
    IEC62304ProcessBuilder IEC62304Process(IEC62304ProcessBuilder::LifecycleProcess process) {
        return IEC62304ProcessBuilder(process);
    }

} // namespace speclab