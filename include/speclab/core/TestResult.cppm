/**
 * @brief Test result types and status for medical device testing - C++23 Module
 */

export module speclab.core.testresult;

import std;

export namespace speclab::core {

    /**
     * @brief Enumeration of test execution results
     * 
     * Ordered by severity for medical device compliance.
     * Includes IEC 62304 specific result types.
     */
    enum class TestStatus : std::uint8_t {
        Passed = 0,         ///< Test passed successfully
        Failed = 1,         ///< Test failed - functionality issue
        Error = 2,          ///< Test error - execution issue
        Skipped = 3,        ///< Test skipped - not applicable
        Blocked = 4,        ///< Test blocked - dependency issue
        Critical = 5        ///< Critical failure - safety/compliance issue
    };

    /**
     * @brief Convert test status to string representation
     * @param status The test status to convert
     * @return String representation of the status
     */
    constexpr std::string_view toString(TestStatus status) noexcept {
        switch (status) {
            case TestStatus::Passed:    return "PASSED";
            case TestStatus::Failed:    return "FAILED";
            case TestStatus::Error:     return "ERROR";
            case TestStatus::Skipped:   return "SKIPPED";
            case TestStatus::Blocked:   return "BLOCKED";
            case TestStatus::Critical:  return "CRITICAL";
            default:                    return "UNKNOWN";
        }
    }

    /**
     * @brief Convert string to test status
     * @param str String representation
     * @return Corresponding TestStatus or nullopt if invalid
     */
    constexpr std::optional<TestStatus> fromString(std::string_view str) noexcept {
        if (str == "PASSED")    return TestStatus::Passed;
        if (str == "FAILED")    return TestStatus::Failed;
        if (str == "ERROR")     return TestStatus::Error;
        if (str == "SKIPPED")   return TestStatus::Skipped;
        if (str == "BLOCKED")   return TestStatus::Blocked;
        if (str == "CRITICAL")  return TestStatus::Critical;
        return std::nullopt;
    }

    /**
     * @brief Test execution result with timing and metadata
     */
    struct TestResult {
        TestStatus status = TestStatus::Passed;
        std::string message;
        std::string errorDetails;
        std::chrono::nanoseconds duration{0};
        std::source_location location = std::source_location::current();
        std::string testId;
        std::string suiteName;
        std::unordered_map<std::string, std::string> metadata;
        std::chrono::system_clock::time_point timestamp = std::chrono::system_clock::now();
        std::string riskLevel;          ///< Risk classification per ISO 14971
        std::string complianceStandard; ///< Applicable standard (IEC 62304, etc.)
        std::string deviceComponent;    ///< Component under test
        bool requiresAudit = false;     ///< Requires audit trail logging
        std::vector<std::string> requirementIds; ///< Covered requirement IDs (Phase1 traceability)

        TestResult(TestStatus stat = TestStatus::Passed,
                            std::string_view msg = "",
                            std::source_location loc = std::source_location::current())
            : status(stat), message(msg), location(loc) {}

        constexpr bool passed() const noexcept { return status == TestStatus::Passed; }
        constexpr bool failed() const noexcept { return status != TestStatus::Passed && status != TestStatus::Skipped; }
        constexpr bool critical() const noexcept { return status == TestStatus::Critical; }

        void addMetadata(std::string_view key, std::string_view value) {
            metadata[std::string(key)] = std::string(value);
        }

        void setMedicalInfo(std::string_view risk, std::string_view standard, std::string_view component, bool audit = false) {
            riskLevel = risk;
            complianceStandard = standard;
            deviceComponent = component;
            requiresAudit = audit;
        }

        void addRequirementId(std::string_view id) {
            requirementIds.emplace_back(id);
            if (!metadata.contains("requirements")) metadata["requirements"] = std::string(id);
            else metadata["requirements"] += "," + std::string(id);
        }

        double getDurationMs() const noexcept {
            return std::chrono::duration<double, std::milli>(duration).count();
        }

        std::string format() const {
            return std::format("[{}] {} - {} ({:.2f}ms)", toString(status), testId, message, getDurationMs());
        }
    };

    /**
     * @brief Collection of test results with statistics
     *
     * @note Thread-safety contract: this class is **not** internally synchronised. Callers
     *       serialise access themselves - every parallel execution path in
     *       speclab.core.testsuite (executeParallel, executeTestsParallel, executeAllParallel)
     *       and in speclab.runners.testrunner calls addResult() while holding its own local
     *       resultsMutex. Do not add a mutex here without removing those external locks: if an
     *       internal mutex is ever the *same* object a caller already holds, the runner
     *       deadlocks rather than merely double-locking.
     */
    class TestResultCollection {
    public:
        /**
         * @brief Add a test result
         * @param result Result to add
         * @note Not thread-safe; the caller must serialise concurrent calls. See the
         *       class-level thread-safety contract above.
         */
        void addResult(TestResult result) {
            results_.push_back(std::move(result));
        }
        
        /**
         * @brief Get all results
         * @return Const reference to results vector
         */
        const std::vector<TestResult>& getResults() const noexcept {
            return results_;
        }
        
        /**
         * @brief Get total number of tests
         * @return Total count
         */
        std::size_t getTotalCount() const noexcept {
            return results_.size();
        }
        
        /**
         * @brief Get number of passed tests
         * @return Passed count
         */
        std::size_t getPassedCount() const noexcept {
            return static_cast<std::size_t>(std::count_if(results_.begin(), results_.end(),
                               [](const TestResult& r) { return r.passed(); }));
        }
        
        /**
         * @brief Get number of failed tests
         * @return Failed count
         */
        std::size_t getFailedCount() const noexcept {
            return static_cast<std::size_t>(std::count_if(results_.begin(), results_.end(),
                               [](const TestResult& r) { return r.failed(); }));
        }
        
        /**
         * @brief Get number of critical failures
         * @return Critical failure count
         */
        std::size_t getCriticalCount() const noexcept {
            return static_cast<std::size_t>(std::count_if(results_.begin(), results_.end(),
                               [](const TestResult& r) { return r.critical(); }));
        }
        
        /**
         * @brief Get success rate as percentage
         * @return Success rate (0.0 to 100.0)
         */
        double getSuccessRate() const noexcept {
            if (results_.empty()) return 100.0;
            return (static_cast<double>(getPassedCount()) / static_cast<double>(results_.size())) * 100.0;
        }
        
        /**
         * @brief Get total execution time
         * @return Sum of all test durations
         */
        std::chrono::nanoseconds getTotalDuration() const noexcept {
            return std::accumulate(results_.begin(), results_.end(), 
                                 std::chrono::nanoseconds{0},
                                 [](auto sum, const TestResult& r) {
                                     return sum + r.duration;
                                 });
        }
        
        /**
         * @brief Clear all results
         */
        void clear() {
            results_.clear();
        }
        
        /**
         * @brief Check if any critical failures occurred
         * @return True if any critical failures present
         */
        bool hasCriticalFailures() const noexcept {
            return getCriticalCount() > 0;
        }
        
    private:
        std::vector<TestResult> results_;
    };

} // namespace speclab::core