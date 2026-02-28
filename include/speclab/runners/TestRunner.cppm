/**
 * @brief Test execution engine for medical device testing - C++23 Module
 */

export module speclab.runners.testrunner;

import std;
import speclab.core.testresult;
import speclab.core.testcase;
import speclab.core.testsuite;
import speclab.reporters.reporter;
import speclab.reporters.consolereporter;

export namespace speclab::runners {

    /**
     * @brief Multi-threaded test execution engine with medical device compliance
     * 
     * Provides thread-safe test execution with result aggregation,
     * compliance reporting, and audit trail generation.
     */
    class TestRunner {
    public:
        /**
         * @brief Configuration for test execution
         */
        struct Config {
            std::size_t maxThreads = 0; // Will be set to hardware_concurrency in constructor if 0
            bool parallelExecution = true;
            bool stopOnFirstFailure = false;
            bool stopOnCriticalFailure = true;
            std::chrono::milliseconds testTimeout{30000}; // 30 seconds default
            bool generateAuditTrail = true;
            bool enablePerformanceLogging = true;
        };

        /**
         * @brief Construct test runner with default configuration
         */
        TestRunner() : config_{} {
            // Set default thread count
            if (config_.maxThreads == 0) {
                config_.maxThreads = std::thread::hardware_concurrency();
            }
            setReporter(std::make_unique<reporters::ConsoleReporter>());
        }

        /**
         * @brief Construct test runner with custom configuration
         * @param config Execution configuration
         */
        explicit TestRunner(Config config) 
            : config_(std::move(config)) {
            // Set default thread count if not specified
            if (config_.maxThreads == 0) {
                config_.maxThreads = std::thread::hardware_concurrency();
            }
            setReporter(std::make_unique<reporters::ConsoleReporter>());
        }

        /**
         * @brief Set the reporter for test results
         * @param reporter Reporter instance to use
         */
        void setReporter(std::unique_ptr<reporters::Reporter> reporter) {
            reporter_ = std::move(reporter);
        }

        /**
         * @brief Get the current reporter
         * @return Reference to current reporter
         */
        reporters::Reporter& getReporter() const {
            return *reporter_;
        }

        /**
         * @brief Execute a single test suite
         * @param suite Test suite to execute
         * @return Collection of test results
         */
        core::TestResultCollection executeSuite(core::TestSuite& suite) {
            core::TestResultCollection results;
            
            if (reporter_) {
                reporter_->reportSuiteStart(suite.getName(), suite.getTestCount());
            }

            auto startTime = std::chrono::steady_clock::now();
            
            try {
                if (config_.parallelExecution && suite.getTestCount() > 1) {
                    results = executeParallel(suite);
                } else {
                    results = executeSequential(suite);
                }
            } catch (const std::exception& e) {
                if (reporter_) {
                    reporter_->reportError("SUITE_EXECUTION", e.what());
                }
            }

            auto endTime = std::chrono::steady_clock::now();
            auto duration = endTime - startTime;

            // Add execution metadata
            addExecutionMetadata(results, suite.getName(), duration);

            if (reporter_) {
                reporter_->reportSuiteEnd(suite.getName(), results);
            }

            return results;
        }

        /**
         * @brief Execute multiple test suites
         * @param suites Vector of test suites to execute
         * @return Combined collection of test results
         */
        core::TestResultCollection executeMultiple(std::vector<std::reference_wrapper<core::TestSuite>> suites) {
            core::TestResultCollection combinedResults;

            for (auto& suiteRef : suites) {
                auto results = executeSuite(suiteRef.get());
                
                // Merge results
                for (const auto& result : results.getResults()) {
                    combinedResults.addResult(result);
                }

                // Stop on critical failure if configured
                if (config_.stopOnCriticalFailure && results.hasCriticalFailures()) {
                    if (reporter_) {
                        reporter_->reportError("EXECUTION_STOPPED", "Critical failure detected - stopping execution");
                    }
                    break;
                }
            }

            return combinedResults;
        }

        /**
         * @brief Get execution configuration
         * @return Current configuration
         */
        const Config& getConfig() const noexcept {
            return config_;
        }

        /**
         * @brief Update execution configuration
         * @param config New configuration
         */
        void setConfig(Config config) {
            config_ = std::move(config);
        }

        /**
         * @brief Get execution statistics
         * @return Statistics about recent executions
         */
        struct ExecutionStats {
            std::size_t totalTestsRun = 0;
            std::size_t totalSuitesRun = 0;
            std::chrono::nanoseconds totalExecutionTime{0};
            std::size_t criticalFailures = 0;
            std::size_t timeouts = 0;
        };

        ExecutionStats getStats() const noexcept {
            return stats_;
        }

        /**
         * @brief Reset execution statistics
         */
        void resetStats() {
            stats_ = {};
        }

    private:
        Config config_;
        std::unique_ptr<reporters::Reporter> reporter_;
        ExecutionStats stats_;
        mutable std::mutex statsMutex_;

        /**
         * @brief Execute tests sequentially
         * @param suite Test suite to execute
         * @return Test results
         */
        core::TestResultCollection executeSequential(core::TestSuite& suite) {
            core::TestResultCollection results;
            
            auto tests = suite.getTests();
            for (auto& test : tests) {
                try {
                    auto result = executeTest(*test);
                    results.addResult(result);
                    
                    if (reporter_) {
                        reporter_->reportTestResult(result);
                    }

                    // Check stopping conditions
                    if (config_.stopOnFirstFailure && result.failed()) {
                        break;
                    }
                    if (config_.stopOnCriticalFailure && result.critical()) {
                        break;
                    }
                    
                } catch (const std::exception& e) {
                    core::TestResult errorResult(core::TestStatus::Error, e.what());
                    errorResult.testId = test->getTestId();
                    results.addResult(errorResult);
                    
                    if (reporter_) {
                        reporter_->reportTestResult(errorResult);
                    }
                }
            }
            
            return results;
        }

        /**
         * @brief Execute tests in parallel
         * @param suite Test suite to execute
         * @return Test results
         */
        core::TestResultCollection executeParallel(core::TestSuite& suite) {
            core::TestResultCollection results;
            std::mutex resultsMutex;
            std::atomic<bool> shouldStop{false};
            
            auto tests = suite.getTests();
            std::vector<std::jthread> workers;
            std::queue<std::shared_ptr<core::TestCase>> testQueue;
            std::mutex queueMutex;
            
            // Fill test queue
            for (auto& test : tests) {
                testQueue.push(test);
            }
            
            // Create worker threads
            auto numThreads = std::min(config_.maxThreads, tests.size());
            for (std::size_t i = 0; i < numThreads; ++i) {
                workers.emplace_back([&, this](std::stop_token stopToken) {
                    while (!stopToken.stop_requested() && !shouldStop.load()) {
                        std::shared_ptr<core::TestCase> test;
                        
                        // Get next test
                        {
                            std::lock_guard<std::mutex> lock(queueMutex);
                            if (testQueue.empty()) {
                                break;
                            }
                            test = testQueue.front();
                            testQueue.pop();
                        }
                        
                        try {
                            auto result = executeTest(*test);
                            
                            // Add result thread-safely
                            {
                                std::lock_guard<std::mutex> lock(resultsMutex);
                                results.addResult(result);
                                
                                if (reporter_) {
                                    reporter_->reportTestResult(result);
                                }
                            }
                            
                            // Check stopping conditions
                            if (config_.stopOnCriticalFailure && result.critical()) {
                                shouldStop.store(true);
                            }
                            if (config_.stopOnFirstFailure && result.failed()) {
                                shouldStop.store(true);
                            }
                            
                        } catch (const std::exception& e) {
                            core::TestResult errorResult(core::TestStatus::Error, e.what());
                            errorResult.testId = test->getTestId();
                            
                            std::lock_guard<std::mutex> lock(resultsMutex);
                            results.addResult(errorResult);
                            
                            if (reporter_) {
                                reporter_->reportTestResult(errorResult);
                            }
                        }
                    }
                });
            }
            
            // Wait for all workers to complete
            for (auto& worker : workers) {
                if (worker.joinable()) {
                    worker.join();
                }
            }
            
            return results;
        }

        /**
         * @brief Execute a single test with timeout handling
         * @param test Test case to execute
         * @return Test result
         */
        core::TestResult executeTest(core::TestCase& test) {
            auto startTime = std::chrono::steady_clock::now();
            
            // Execute test with timeout
            std::promise<core::TestResult> resultPromise;
            auto resultFuture = resultPromise.get_future();
            
            std::jthread testThread([&](std::stop_token stopToken) {
                (void)stopToken; // Suppress unused parameter warning
                try {
                    auto result = test.execute();
                    result.testId = test.getTestId();
                    resultPromise.set_value(result);
                } catch (const std::exception& e) {
                    core::TestResult errorResult(core::TestStatus::Error, e.what());
                    errorResult.testId = test.getTestId();
                    resultPromise.set_value(errorResult);
                } catch (...) {
                    core::TestResult errorResult(core::TestStatus::Error, "Unknown exception");
                    errorResult.testId = test.getTestId();
                    resultPromise.set_value(errorResult);
                }
            });
            
            // Wait for completion or timeout
            auto status = resultFuture.wait_for(config_.testTimeout);
            
            if (status == std::future_status::timeout) {
                testThread.request_stop();
                if (testThread.joinable()) {
                    testThread.join();
                }
                
                std::lock_guard<std::mutex> lock(statsMutex_);
                stats_.timeouts++;
                
                core::TestResult timeoutResult(core::TestStatus::Error, "Test timeout");
                timeoutResult.testId = test.getTestId();
                timeoutResult.duration = config_.testTimeout;
                return timeoutResult;
            }
            
            if (testThread.joinable()) {
                testThread.join();
            }
            
            auto result = resultFuture.get();
            auto endTime = std::chrono::steady_clock::now();
            result.duration = std::chrono::duration_cast<std::chrono::nanoseconds>(endTime - startTime);
            
            // Update statistics
            {
                std::lock_guard<std::mutex> lock(statsMutex_);
                stats_.totalTestsRun++;
                stats_.totalExecutionTime += result.duration;
                if (result.critical()) {
                    stats_.criticalFailures++;
                }
            }
            
            return result;
        }

        /**
         * @brief Add execution metadata to results
         * @param results Results to augment
         * @param suiteName Name of test suite
         * @param duration Total execution duration
         */
        void addExecutionMetadata(core::TestResultCollection& results, 
                                std::string_view suiteName,
                                std::chrono::nanoseconds duration) {
            
            // Update overall statistics
            {
                std::lock_guard<std::mutex> lock(statsMutex_);
                stats_.totalSuitesRun++;
            }
            
            // Report compliance information if enabled
            if (config_.generateAuditTrail && reporter_) {
                std::unordered_map<std::string, std::string> auditInfo;
                auditInfo["execution_time"] = std::to_string(std::chrono::duration_cast<std::chrono::milliseconds>(duration).count()) + "ms";
                auditInfo["test_count"] = std::to_string(results.getTotalCount());
                auditInfo["parallel_execution"] = config_.parallelExecution ? "true" : "false";
                auditInfo["max_threads"] = std::to_string(config_.maxThreads);
                auditInfo["suite_name"] = std::string(suiteName);
                
                reporter_->reportAuditTrail(auditInfo);
            }
        }
    };

    /**
     * @brief Factory function to create test runner with common configurations
     * @param preset Configuration preset ("default", "medical", "performance", "debug")
     * @return Configured test runner
     */
    std::unique_ptr<TestRunner> createTestRunner(std::string_view preset = "default") {
        TestRunner::Config config;
        
        if (preset == "medical") {
            config.stopOnCriticalFailure = true;
            config.generateAuditTrail = true;
            config.enablePerformanceLogging = true;
            config.testTimeout = std::chrono::seconds(60); // More time for medical tests
        } else if (preset == "performance") {
            config.parallelExecution = true;
            config.maxThreads = std::thread::hardware_concurrency();
            config.stopOnFirstFailure = false;
            config.enablePerformanceLogging = true;
        } else if (preset == "debug") {
            config.parallelExecution = false;
            config.stopOnFirstFailure = true;
            config.testTimeout = std::chrono::minutes(5); // Long timeout for debugging
        }
        
        return std::make_unique<TestRunner>(std::move(config));
    }

} // namespace speclab::runners