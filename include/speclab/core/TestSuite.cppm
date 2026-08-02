/**
 * @brief Test suite management for medical device testing - C++23 Module
 */

export module speclab.core.testsuite;

import std;
import speclab.core.testresult;
import speclab.core.testcase;
import speclab.core.requirements; // ensure requirements available

export namespace speclab::core {

    /**
     * @brief Test suite containing and managing multiple test cases
     */
    class TestSuite {
    public:
        using TestCasePtr = std::unique_ptr<TestCase>;
        using TestCaseContainer = std::vector<TestCasePtr>;
        
        /**
         * @brief Constructor with suite identification
         * @param name Suite name
         * @param description Suite description
         */
        explicit TestSuite(std::string_view name, std::string_view description = "")
            : name_(name)
            , description_(description)
            , enabled_(true)
            , setupCompleted_(false)
            , teardownCompleted_(false) {}
        
        /**
         * @brief Destructor ensuring proper cleanup
         */
        virtual ~TestSuite() {
            if (setupCompleted_ && !teardownCompleted_) {
                try {
                    tearDownSuite();
                } catch (...) {
                    // Suppress exceptions in destructor
                }
            }
        }
        
        // Non-copyable but movable
        TestSuite(const TestSuite&) = delete;
        TestSuite& operator=(const TestSuite&) = delete;
        TestSuite(TestSuite&&) = default;
        TestSuite& operator=(TestSuite&&) = default;
        
        /**
         * @brief Add a test case to the suite
         * @param testCase Unique pointer to test case
         */
        void addTest(TestCasePtr testCase) {
            if (testCase) {
                testCases_.push_back(std::move(testCase));
            }
        }
        
        /**
         * @brief Add a function-based test case
         * @param id Test identifier
         * @param testFunc Test function
         * @param name Test name (optional)
         * @param description Test description (optional)
         */
        void addTest(std::string_view id, 
                    std::function<void()> testFunc,
                    std::string_view name = "",
                    std::string_view description = "") {
            auto testCase = std::make_unique<FunctionTestCase>(id, std::move(testFunc), name, description);
            addTest(std::move(testCase));
        }
        
        /**
         * @brief Add a parameterized test case
         * @tparam ParameterType Type of test parameters
         * @param id Test identifier
         * @param testFunc Test function taking parameter
         * @param parameters List of parameters
         * @param name Test name (optional)
         */
        template<typename ParameterType>
        void addParameterizedTest(std::string_view id,
                                std::function<void(const ParameterType&)> testFunc,
                                std::vector<ParameterType> parameters,
                                std::string_view name = "") {
            auto testCase = std::make_unique<ParameterizedTestCase<ParameterType>>(
                id, std::move(testFunc), std::move(parameters), name);
            addTest(std::move(testCase));
        }
        
        /**
         * @brief Add a benchmark test case
         * @param id Test identifier
         * @param benchFunc Benchmark function
         * @param iterations Number of iterations
         * @param name Test name (optional)
         */
        void addBenchmark(std::string_view id,
                         std::function<void()> benchFunc,
                         std::size_t iterations = 1000,
                         std::string_view name = "") {
            auto testCase = std::make_unique<BenchmarkTestCase>(id, std::move(benchFunc), iterations, name);
            addTest(std::move(testCase));
        }
        
        /**
         * @brief Execute all test cases in the suite
         * @param parallel Execute tests in parallel (default: false)
         * @return Collection of all test results
         */
        TestResultCollection execute(bool parallel = false) {
            TestResultCollection results;
            try {
                setUpSuite();
                if (parallel && testCases_.size() > 1) {
                    executeParallel(results);
                } else {
                    executeSequential(results);
                }
                tearDownSuite();
            } catch (const std::exception& e) {
                TestResult suiteFailure(TestStatus::Error, std::format("Suite execution failed: {}", e.what()));
                suiteFailure.testId = std::format("{}_SUITE_FAILURE", name_);
                suiteFailure.suiteName = name_;
                results.addResult(std::move(suiteFailure));
            }
            // Phase 2: add coverage validation synthetic result
            speclab::core::AddCoverageValidationResult(results);
            return results;
        }
        
        /**
         * @brief Execute only tests matching filter
         * @param filter Test ID filter (regex pattern)
         * @param parallel Execute tests in parallel
         * @return Collection of filtered test results
         */
        TestResultCollection executeFiltered(std::string_view filter, bool parallel = false) {
            TestResultCollection results;
            const std::regex filterRegex{std::string(filter)};
            try {
                setUpSuite();
                auto matchingTests = getMatchingTests(filterRegex);
                if (parallel && matchingTests.size() > 1) {
                    executeTestsParallel(matchingTests, results);
                } else {
                    executeTestsSequential(matchingTests, results);
                }
                tearDownSuite();
            } catch (const std::exception& e) {
                TestResult suiteFailure(TestStatus::Error, std::format("Filtered suite execution failed: {}", e.what()));
                suiteFailure.testId = std::format("{}_FILTERED_FAILURE", name_);
                suiteFailure.suiteName = name_;
                results.addResult(std::move(suiteFailure));
            }
            speclab::core::AddCoverageValidationResult(results);
            return results;
        }
        
        // Getters
        const std::string& getName() const noexcept { return name_; }
        const std::string& getDescription() const noexcept { return description_; }
        bool isEnabled() const noexcept { return enabled_; }
        std::size_t getTestCount() const noexcept { return testCases_.size(); }
        
        /**
         * @brief Get test case by ID
         * @param id Test case identifier
         * @return Pointer to test case or nullptr if not found
         */
        TestCase* getTest(std::string_view id) const {
            auto it = std::find_if(testCases_.begin(), testCases_.end(),
                [id](const TestCasePtr& test) {
                    return test->getId() == id;
                });
            return (it != testCases_.end()) ? it->get() : nullptr;
        }
        
        /**
         * @brief Get all test cases
         * @return Vector of shared pointers to test cases
         */
        std::vector<std::shared_ptr<TestCase>> getTests() const {
            std::vector<std::shared_ptr<TestCase>> tests;
            tests.reserve(testCases_.size());
            
            for (const auto& test : testCases_) {
                tests.push_back(std::shared_ptr<TestCase>(test.get(), [](TestCase*){})); // Non-owning shared_ptr
            }
            
            return tests;
        }
        
        /**
         * @brief Get all test case IDs
         * @return Vector of test identifiers
         */
        std::vector<std::string> getTestIds() const {
            std::vector<std::string> ids;
            ids.reserve(testCases_.size());
            
            for (const auto& test : testCases_) {
                ids.push_back(test->getId());
            }
            
            return ids;
        }
        
        /**
         * @brief Check if suite contains test with given ID
         * @param id Test identifier
         * @return True if test exists
         */
        bool hasTest(std::string_view id) const noexcept {
            return getTest(id) != nullptr;
        }
        
        // Setters
        void setEnabled(bool enabled) { enabled_ = enabled; }
        void setDescription(std::string_view description) { description_ = description; }
        
        /**
         * @brief Add suite metadata
         * @param key Metadata key
         * @param value Metadata value
         */
        void addMetadata(std::string_view key, std::string_view value) {
            metadata_[std::string(key)] = std::string(value);
        }
        
        /**
         * @brief Get suite metadata
         * @return Const reference to metadata map
         */
        const std::unordered_map<std::string, std::string>& getMetadata() const noexcept {
            return metadata_;
        }
        
    protected:
        /**
         * @brief Suite-level setup executed before all tests
         */
        virtual void setUpSuite() {
            if (!setupCompleted_) {
                setupCompleted_ = true;
            }
        }
        
        /**
         * @brief Suite-level teardown executed after all tests
         */
        virtual void tearDownSuite() {
            if (setupCompleted_ && !teardownCompleted_) {
                teardownCompleted_ = true;
            }
        }
        
    private:
        /**
         * @brief Execute tests sequentially
         * @param results Result collection to populate
         */
        void executeSequential(TestResultCollection& results) {
            // Phase 3: risk-based ordering if enabled
            auto& reg = RequirementRegistry::instance();
            auto cfg = reg.getConfig();
            if (cfg.riskBasedOrdering) {
                // Build vector of indices & risk score
                std::vector<std::pair<int, TestCase*>> ordered;
                ordered.reserve(testCases_.size());
                for (auto& tc : testCases_) {
                    if (tc && tc->isEnabled()) {
                        int score = TestRiskScore(tc->getId());
                        ordered.emplace_back(-score, tc.get()); // negative for descending
                    }
                }
                std::ranges::sort(ordered, [](auto& a, auto& b){ return a.first < b.first; });
                // Optional early abort pre-run if uncovered critical and abort flag set
                if (cfg.abortOnCriticalGaps && HasUncoveredCriticalRequirements()) {
                    TestResult pre;
                    pre.testId = "REQUIREMENT_COVERAGE_PRE";
                    pre.status = TestStatus::Critical;
                    pre.message = "Uncovered CRITICAL requirements detected before execution";
                    auto missing = GetUncoveredCriticalRequirementIds();
                    if (!missing.empty()) {
                        std::string list; for (std::size_t i=0;i<missing.size();++i){ if(i) list+=","; list+=missing[i]; }
                        pre.addMetadata("missing_critical", list);
                        pre.errorDetails = list;
                    }
                    results.addResult(std::move(pre));
                    return; // skip actual tests
                }
                for (auto& [negScore, testPtr] : ordered) {
                    auto result = testPtr->execute();
                    result.suiteName = name_;
                    speclab::core::AugmentResultWithRequirements(result); // Phase 2 augmentation
                    results.addResult(std::move(result));
                }
                return; // done
            }
            // Fallback original behavior
            for (const auto& test : testCases_) {
                if (test && test->isEnabled()) {
                    auto result = test->execute();
                    result.suiteName = name_;
                    speclab::core::AugmentResultWithRequirements(result); // Phase 2 augmentation
                    results.addResult(std::move(result));
                }
            }
        }
        
        /**
         * @brief Execute tests in parallel
         * @param results Result collection to populate
         */
        void executeParallel(TestResultCollection& results) {
            // TestResultCollection is not internally synchronised - this mutex is what makes
            // the addResult() calls below safe, and the same pattern is repeated in
            // executeTestsParallel() and executeAllParallel(). See the thread-safety contract
            // on TestResultCollection in speclab.core.testresult before changing either side.
            std::mutex resultsMutex;
            std::vector<std::jthread> workers;
            for (const auto& test : testCases_) {
                if (test && test->isEnabled()) {
                    workers.emplace_back([testPtr = test.get(), &results, &resultsMutex, this](std::stop_token) {
                        TestResult result;
                        try {
                            result = testPtr->execute();
                            result.suiteName = name_;
                            speclab::core::AugmentResultWithRequirements(result);
                        } catch (const std::exception& e) {
                            result = TestResult(TestStatus::Error,
                                std::format("Parallel execution error: {}", e.what()));
                            result.suiteName = name_;
                        } catch (...) {
                            result = TestResult(TestStatus::Error,
                                "Parallel execution error: unknown exception");
                            result.suiteName = name_;
                        }
                        std::lock_guard<std::mutex> lock(resultsMutex);
                        results.addResult(std::move(result));
                    });
                }
            }
            for (auto& worker : workers) {
                if (worker.joinable()) worker.join();
            }
        }
        
        /**
         * @brief Get tests matching regex filter
         * @param filterRegex Compiled regex pattern
         * @return Vector of matching test pointers
         */
        std::vector<TestCase*> getMatchingTests(const std::regex& filterRegex) const {
            (void)filterRegex; // Suppress unused parameter warning
            std::vector<TestCase*> matching;
            
            for (const auto& test : testCases_) {
                if (test && test->isEnabled()) {
                    // The regex filter is bypassed and all enabled tests are matched. This was
                    // originally a GCC 15 / `import std` workaround; the GCC 15 floor has been
                    // dropped (see issue #9 and CMakeLists.txt), so restoring
                    // std::regex_match(testId, filterRegex) here is likely safe, but it is a
                    // behaviour change (filtering would actually take effect) and should be
                    // verified on GCC 16 before it is made. Until then, accept every enabled
                    // test so filtering stays a no-op rather than silently dropping tests.
                    std::string testId = test->getId();
                    if (testId.length() > 0) {
                        matching.push_back(test.get());
                    }
                }
            }
            
            return matching;
        }
        
        /**
         * @brief Execute specific tests sequentially
         * @param tests Vector of test pointers to execute
         * @param results Result collection to populate
         */
        void executeTestsSequential(const std::vector<TestCase*>& tests, TestResultCollection& results) {
            for (auto* test : tests) {
                auto result = test->execute();
                result.suiteName = name_;
                speclab::core::AugmentResultWithRequirements(result);
                results.addResult(std::move(result));
            }
        }
        
        /**
         * @brief Execute specific tests in parallel
         * @param tests Vector of test pointers to execute
         * @param results Result collection to populate
         */
        void executeTestsParallel(const std::vector<TestCase*>& tests, TestResultCollection& results) {
            std::mutex resultsMutex;
            std::vector<std::jthread> workers;
            for (auto* test : tests) {
                workers.emplace_back([test, &results, &resultsMutex, this](std::stop_token) {
                    TestResult result;
                    try {
                        result = test->execute();
                        result.suiteName = name_;
                        speclab::core::AugmentResultWithRequirements(result);
                    } catch (const std::exception& e) {
                        result = TestResult(TestStatus::Error,
                            std::format("Parallel execution error: {}", e.what()));
                        result.suiteName = name_;
                    } catch (...) {
                        result = TestResult(TestStatus::Error,
                            "Parallel execution error: unknown exception");
                        result.suiteName = name_;
                    }
                    std::lock_guard<std::mutex> lock(resultsMutex);
                    results.addResult(std::move(result));
                });
            }
            for (auto& worker : workers) {
                if (worker.joinable()) worker.join();
            }
        }
        
        std::string name_;
        std::string description_;
        bool enabled_;
        bool setupCompleted_;
        bool teardownCompleted_;
        TestCaseContainer testCases_;
        std::unordered_map<std::string, std::string> metadata_;
    };

    /**
     * @brief Test suite registry for managing multiple suites
     */
    class TestSuiteRegistry {
    public:
        using SuitePtr = std::unique_ptr<TestSuite>;
        using SuiteContainer = std::unordered_map<std::string, SuitePtr>;
        
        /**
         * @brief Get singleton instance
         * @return Reference to global registry
         */
        static TestSuiteRegistry& getInstance() {
            static TestSuiteRegistry instance;
            return instance;
        }
        
        /**
         * @brief Register a test suite
         * @param suite Unique pointer to test suite
         * @return True if successfully registered
         */
        bool registerSuite(SuitePtr suite) {
            if (!suite) return false;
            
            const auto& name = suite->getName();
            if (suites_.find(name) != suites_.end()) {
                return false; // Suite already exists
            }
            
            suites_[name] = std::move(suite);
            return true;
        }
        
        /**
         * @brief Get test suite by name
         * @param name Suite name
         * @return Pointer to suite or nullptr if not found
         */
        TestSuite* getSuite(std::string_view name) const {
            auto it = suites_.find(std::string(name));
            return (it != suites_.end()) ? it->second.get() : nullptr;
        }
        
        /**
         * @brief Get all registered suite names
         * @return Vector of suite names
         */
        std::vector<std::string> getSuiteNames() const {
            std::vector<std::string> names;
            names.reserve(suites_.size());
            
            for (const auto& [name, suite] : suites_) {
                names.push_back(name);
            }
            
            return names;
        }
        
        /**
         * @brief Execute all registered suites
         * @param parallel Execute suites in parallel
         * @return Combined results from all suites
         */
        TestResultCollection executeAll(bool parallel = false) {
            TestResultCollection allResults;
            
            if (parallel && suites_.size() > 1) {
                executeAllParallel(allResults);
            } else {
                executeAllSequential(allResults);
            }
            
            return allResults;
        }
        
        /**
         * @brief Clear all registered suites
         */
        void clear() {
            suites_.clear();
        }
        
        /**
         * @brief Get total number of registered suites
         * @return Suite count
         */
        std::size_t getSuiteCount() const noexcept {
            return suites_.size();
        }
        
    private:
        TestSuiteRegistry() = default;
        
        void executeAllSequential(TestResultCollection& allResults) {
            for (const auto& [name, suite] : suites_) {
                if (suite && suite->isEnabled()) {
                    auto suiteResults = suite->execute();
                    
                    // Merge results
                    for (const auto& result : suiteResults.getResults()) {
                        allResults.addResult(result);
                    }
                }
            }
        }
        
        void executeAllParallel(TestResultCollection& allResults) {
            std::mutex resultsMutex;
            std::vector<std::jthread> workers;
            
            for (const auto& [name, suite] : suites_) {
                if (suite && suite->isEnabled()) {
                    workers.emplace_back([suitePtr = suite.get(), &allResults, &resultsMutex](std::stop_token) {
                        try {
                            auto suiteResults = suitePtr->execute();
                            std::lock_guard<std::mutex> lock(resultsMutex);
                            for (const auto& result : suiteResults.getResults()) {
                                allResults.addResult(result);
                            }
                        } catch (const std::exception& e) {
                            TestResult errorResult(TestStatus::Error, 
                                std::format("Suite execution error: {}", e.what()));
                            std::lock_guard<std::mutex> lock(resultsMutex);
                            allResults.addResult(std::move(errorResult));
                        } catch (...) {
                            TestResult errorResult(TestStatus::Error,
                                "Suite execution error: unknown exception");
                            std::lock_guard<std::mutex> lock(resultsMutex);
                            allResults.addResult(std::move(errorResult));
                        }
                    });
                }
            }
            
            for (auto& worker : workers) {
                if (worker.joinable()) worker.join();
            }
        }
        
        SuiteContainer suites_;
    };

} // namespace speclab::core
