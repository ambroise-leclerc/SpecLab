/**
 * @brief Base reporter interface for medical device test results - C++23 Module
 */

export module speclab.reporters.reporter;

import std;
import speclab.core.testresult;

export namespace speclab::reporters {

    /**
     * @brief Abstract base class for test result reporting
     * 
     * Provides interface for various output formats including
     * medical device compliance reporting requirements.
     */
    class Reporter {
    public:
        virtual ~Reporter() = default;

        /**
         * @brief Report the start of a test suite
         * @param suiteName Name of the test suite
         * @param testCount Number of tests in the suite
         */
        virtual void reportSuiteStart(std::string_view suiteName, std::size_t testCount) = 0;

        /**
         * @brief Report the end of a test suite
         * @param suiteName Name of the test suite
         * @param results Collection of test results
         */
        virtual void reportSuiteEnd(std::string_view suiteName, const core::TestResultCollection& results) = 0;

        /**
         * @brief Report a single test result
         * @param result The test result to report
         */
        virtual void reportTestResult(const core::TestResult& result) = 0;

        /**
         * @brief Report overall summary of all test executions
         * @param results Complete collection of test results
         */
        virtual void reportSummary(const core::TestResultCollection& results) = 0;

        /**
         * @brief Report a test error or exception
         * @param testId Test identifier
         * @param error Error message
         * @param location Source location where error occurred
         */
        virtual void reportError(std::string_view testId, std::string_view error, 
                               const std::source_location& location = std::source_location::current()) = 0;

        /**
         * @brief Report compliance-specific information
         * @param complianceInfo Regulatory compliance data
         */
        virtual void reportCompliance(const std::unordered_map<std::string, std::string>& complianceInfo) {
            (void)complianceInfo; // Suppress unused parameter warning
            // Default implementation - do nothing
            // Derived classes can override for specific compliance reporting
        }

        /**
         * @brief Report audit trail information for medical device compliance
         * @param auditInfo Audit trail data
         */
        virtual void reportAuditTrail(const std::unordered_map<std::string, std::string>& auditInfo) {
            (void)auditInfo; // Suppress unused parameter warning
            // Default implementation - do nothing
            // Derived classes can override for audit logging
        }

        /**
         * @brief Enable or disable verbose output
         * @param enabled True to enable verbose reporting
         */
        virtual void setVerbose(bool enabled) {
            verbose_ = enabled;
        }

        /**
         * @brief Check if verbose mode is enabled
         * @return True if verbose mode is on
         */
        bool isVerbose() const noexcept {
            return verbose_;
        }

        /**
         * @brief Set the output stream for reporting
         * @param stream Output stream to use
         */
        virtual void setOutputStream(std::ostream& stream) {
            output_ = &stream;
        }

        /**
         * @brief Get the current output stream
         * @return Reference to current output stream
         */
        std::ostream& getOutputStream() const {
            return output_ ? *output_ : std::cout;
        }

    protected:
        bool verbose_ = false;
        std::ostream* output_ = nullptr;

        /**
         * @brief Helper to format duration for display
         * @param duration Duration to format
         * @return Formatted duration string
         */
        std::string formatDuration(std::chrono::nanoseconds duration) const {
            auto ms = std::chrono::duration<double, std::milli>(duration).count();
            if (ms < 1000.0) {
                return std::format("{:.2f}ms", ms);
            }
            auto seconds = ms / 1000.0;
            return std::format("{:.2f}s", seconds);
        }

        /**
         * @brief Helper to format timestamp for display
         * @param timestamp Timestamp to format
         * @return Formatted timestamp string
         */
        std::string formatTimestamp(const std::chrono::system_clock::time_point& timestamp) const {
            auto time_t = std::chrono::system_clock::to_time_t(timestamp);
            return std::format("{:%Y-%m-%d %H:%M:%S}", std::chrono::system_clock::from_time_t(time_t));
        }

        /**
         * @brief Helper to get colored status text
         * @param status Test status
         * @param useColors Whether to use ANSI color codes
         * @return Formatted status string
         */
        std::string formatStatus(core::TestStatus status, bool useColors = false) const {
            std::string statusStr{core::toString(status)};
            
            if (!useColors) {
                return statusStr;
            }

            // ANSI color codes for different statuses
            switch (status) {
                case core::TestStatus::Passed:
                    return std::format("\033[32m{}\033[0m", statusStr);  // Green
                case core::TestStatus::Failed:
                    return std::format("\033[31m{}\033[0m", statusStr);  // Red
                case core::TestStatus::Error:
                    return std::format("\033[35m{}\033[0m", statusStr);  // Magenta
                case core::TestStatus::Skipped:
                    return std::format("\033[33m{}\033[0m", statusStr);  // Yellow
                case core::TestStatus::Blocked:
                    return std::format("\033[36m{}\033[0m", statusStr);  // Cyan
                case core::TestStatus::Critical:
                    return std::format("\033[41m{}\033[0m", statusStr);  // Red background
                default:
                    return statusStr;
            }
        }
    };

    /**
     * @brief Factory function to create appropriate reporter
     * @param type Reporter type ("console", "json", "xml", etc.)
     * @return Unique pointer to reporter instance
     */
    std::unique_ptr<Reporter> createReporter(std::string_view type);

} // namespace speclab::reporters