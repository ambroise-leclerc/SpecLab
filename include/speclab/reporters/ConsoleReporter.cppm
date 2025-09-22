/**
 * @brief Console reporter for medical device test results - C++23 Module
 */

export module speclab.reporters.consolereporter;

import std;
import speclab.core.testresult;
import speclab.reporters.reporter;

export namespace speclab::reporters {

    /**
     * @brief Console-based test result reporter with color support
     * 
     * Provides human-readable output for test results with optional
     * ANSI color coding and medical device compliance information.
     */
    class ConsoleReporter : public Reporter {
    public:
        /**
         * @brief Construct console reporter
         * @param useColors Enable ANSI color codes
         * @param output Output stream (default: std::cout)
         */
        explicit ConsoleReporter(bool useColors = true, std::ostream& output = std::cout)
            : useColors_(useColors) {
            setOutputStream(output);
        }

        void reportSuiteStart(std::string_view suiteName, std::size_t testCount) override {
            auto& out = getOutputStream();
            out << std::format("\n{}Running test suite: {}{}\n",
                             useColors_ ? "\033[1m" : "",  // Bold
                             suiteName,
                             useColors_ ? "\033[0m" : ""); // Reset
            out << std::format("Tests to run: {}\n", testCount);
            out << std::string(60, '=') << "\n\n";
            startTime_ = std::chrono::steady_clock::now();
        }

        void reportSuiteEnd(std::string_view suiteName, const core::TestResultCollection& results) override {
            auto endTime = std::chrono::steady_clock::now();
            auto duration = endTime - startTime_;
            
            auto& out = getOutputStream();
            out << "\n" << std::string(60, '=') << "\n";
            out << std::format("{}Test suite completed: {}{}\n",
                             useColors_ ? "\033[1m" : "",
                             suiteName,
                             useColors_ ? "\033[0m" : "");
            
            reportSummary(results);
            
            out << std::format("Total execution time: {}\n", formatDuration(duration));
            
            // Medical device specific summary
            if (hasMedicalTests(results)) {
                reportMedicalSummary(results);
            }
        }

        void reportTestResult(const core::TestResult& result) override {
            auto& out = getOutputStream();
            
            std::string statusStr = formatStatus(result.status, useColors_);
            std::string testLine = std::format("[{}] {} - {} ({})",
                                             statusStr,
                                             result.testId,
                                             result.message,
                                             formatDuration(result.duration));
            
            out << testLine << "\n";
            
            // Show additional details in verbose mode
            if (isVerbose()) {
                if (!result.errorDetails.empty()) {
                    out << std::format("    Error: {}\n", result.errorDetails);
                }
                
                if (!result.riskLevel.empty()) {
                    out << std::format("    Risk Level: {}\n", result.riskLevel);
                }
                
                if (!result.complianceStandard.empty()) {
                    out << std::format("    Compliance: {}\n", result.complianceStandard);
                }
                
                if (!result.requirementIds.empty()) {
                    std::string reqList;
                    for (std::size_t i = 0; i < result.requirementIds.size(); ++i) {
                        if (i > 0) reqList += ", ";
                        reqList += result.requirementIds[i];
                    }
                    out << std::format("    Requirements: {}\n", reqList);
                }
                
                out << std::format("    Location: {}:{}\n", 
                                 result.location.file_name(), 
                                 result.location.line());
                out << "\n";
            }
        }

        void reportSummary(const core::TestResultCollection& results) override {
            auto& out = getOutputStream();
            
            auto total = results.getTotalCount();
            auto passed = results.getPassedCount();
            auto failed = results.getFailedCount();
            auto critical = results.getCriticalCount();
            auto successRate = results.getSuccessRate();
            
            out << "\n" << std::string(40, '-') << "\n";
            out << "TEST SUMMARY\n";
            out << std::string(40, '-') << "\n";
            
            out << std::format("Total tests:     {}\n", total);
            out << std::format("Passed:          {}{}{}\n",
                             useColors_ ? "\033[32m" : "",
                             passed,
                             useColors_ ? "\033[0m" : "");
            
            if (failed > 0) {
                out << std::format("Failed:          {}{}{}\n",
                                 useColors_ ? "\033[31m" : "",
                                 failed,
                                 useColors_ ? "\033[0m" : "");
            }
            
            if (critical > 0) {
                out << std::format("Critical:        {}{}{}\n",
                                 useColors_ ? "\033[41m" : "",
                                 critical,
                                 useColors_ ? "\033[0m" : "");
            }
            
            auto skipped = total - passed - failed;
            if (skipped > 0) {
                out << std::format("Skipped:         {}\n", skipped);
            }
            
            out << std::format("Success rate:    {:.1f}%\n", successRate);
            
            // Overall result
            std::string overallResult;
            if (critical > 0) {
                overallResult = useColors_ ? "\033[41mCRITICAL FAILURES\033[0m" : "CRITICAL FAILURES";
            } else if (failed > 0) {
                overallResult = useColors_ ? "\033[31mFAILED\033[0m" : "FAILED";
            } else {
                overallResult = useColors_ ? "\033[32mPASSED\033[0m" : "PASSED";
            }
            
            out << std::format("\nOverall result:  {}\n", overallResult);
        }

        void reportError(std::string_view testId, std::string_view error, 
                        const std::source_location& location) override {
            auto& out = getOutputStream();
            out << std::format("{}ERROR{} in test {}: {}\n",
                             useColors_ ? "\033[31m" : "",
                             useColors_ ? "\033[0m" : "",
                             testId, error);
            
            if (isVerbose()) {
                out << std::format("    at {}:{}\n", location.file_name(), location.line());
            }
        }

        void reportCompliance(const std::unordered_map<std::string, std::string>& complianceInfo) override {
            if (complianceInfo.empty()) return;
            
            auto& out = getOutputStream();
            out << "\n" << std::string(40, '-') << "\n";
            out << "COMPLIANCE INFORMATION\n";
            out << std::string(40, '-') << "\n";
            
            for (const auto& [key, value] : complianceInfo) {
                out << std::format("{}: {}\n", key, value);
            }
        }

        void reportAuditTrail(const std::unordered_map<std::string, std::string>& auditInfo) override {
            if (auditInfo.empty() || !isVerbose()) return;
            
            auto& out = getOutputStream();
            out << "\n" << std::string(40, '-') << "\n";
            out << "AUDIT TRAIL\n";
            out << std::string(40, '-') << "\n";
            
            for (const auto& [key, value] : auditInfo) {
                out << std::format("{}: {}\n", key, value);
            }
        }

        /**
         * @brief Enable or disable color output
         * @param enabled True to enable colors
         */
        void setUseColors(bool enabled) {
            useColors_ = enabled;
        }

        /**
         * @brief Check if colors are enabled
         * @return True if colors are enabled
         */
        bool getUseColors() const noexcept {
            return useColors_;
        }

    private:
        bool useColors_ = true;
        std::chrono::steady_clock::time_point startTime_;

        /**
         * @brief Check if any tests have medical device information
         * @param results Test results to check
         * @return True if medical tests are present
         */
        bool hasMedicalTests(const core::TestResultCollection& results) const {
            const auto& testResults = results.getResults();
            return std::any_of(testResults.begin(), testResults.end(),
                             [](const core::TestResult& r) {
                                 return !r.riskLevel.empty() || 
                                        !r.complianceStandard.empty() ||
                                        r.requiresAudit;
                             });
        }

        /**
         * @brief Report medical device specific summary
         * @param results Test results to summarize
         */
        void reportMedicalSummary(const core::TestResultCollection& results) {
            auto& out = getOutputStream();
            
            out << "\n" << std::string(40, '-') << "\n";
            out << "MEDICAL DEVICE SUMMARY\n";
            out << std::string(40, '-') << "\n";
            
            // Count by risk level
            std::unordered_map<std::string, std::size_t> riskCounts;
            std::unordered_map<std::string, std::size_t> complianceCounts;
            std::size_t auditRequired = 0;
            
            for (const auto& result : results.getResults()) {
                if (!result.riskLevel.empty()) {
                    riskCounts[result.riskLevel]++;
                }
                if (!result.complianceStandard.empty()) {
                    complianceCounts[result.complianceStandard]++;
                }
                if (result.requiresAudit) {
                    auditRequired++;
                }
            }
            
            if (!riskCounts.empty()) {
                out << "Risk Level Distribution:\n";
                for (const auto& [risk, count] : riskCounts) {
                    out << std::format("  {}: {}\n", risk, count);
                }
                out << "\n";
            }
            
            if (!complianceCounts.empty()) {
                out << "Compliance Standards:\n";
                for (const auto& [standard, count] : complianceCounts) {
                    out << std::format("  {}: {}\n", standard, count);
                }
                out << "\n";
            }
            
            if (auditRequired > 0) {
                out << std::format("Tests requiring audit: {}\n", auditRequired);
            }
            
            // Critical safety check
            if (results.hasCriticalFailures()) {
                out << std::format("\n{}WARNING: Critical safety failures detected!{}\n",
                                 useColors_ ? "\033[41m" : "",
                                 useColors_ ? "\033[0m" : "");
                out << "Review required before medical device deployment.\n";
            }
        }
    };

} // namespace speclab::reporters