/**
 * @brief Functional BDD API for readable medical device testing - C++23 Module
 */

export module speclab.core.functionalapi;

import std;
import speclab.core.testresult;
import speclab.core.assertions;

export namespace speclab {

    // Forward declarations
    class TestBuilder;
    class RequirementBuilder;
    class FeatureBuilder;
    class ParameterizedTestBuilder;

    /**
     * @brief Step function type for Given/When/Then steps
     */
    using StepFunction = std::function<void()>;
    
    /**
     * @brief Performance test function that returns timing
     */
    using PerformanceFunction = std::function<std::function<void()>()>;
    
    /**
     * @brief Parameterized step function type
     */
    template<typename T>
    using ParameterizedStepFunction = std::function<void(const T&)>;
    
    /**
     * @brief Duration assertion function for performance tests
     */
    using DurationAssertionFunction = std::function<void(std::chrono::nanoseconds)>;
    
    /**
     * @brief Critical assertion function that returns boolean
     */
    using CriticalAssertionFunction = std::function<bool()>;

    /**
     * @brief Test step information for BDD structure
     */
    struct TestStep {
        std::string description;
        StepFunction function;
        std::string stepType; // "Given", "When", "Then", "And"
        
        TestStep(std::string_view desc, StepFunction func, std::string_view type)
            : description(desc), function(std::move(func)), stepType(type) {}
        
        void execute() const {
            if (function) {
                function();
            }
        }
    };

    /**
     * @brief Core test builder for functional BDD API
     */
    class TestBuilder {
    public:
        explicit TestBuilder(std::string_view testId)
            : testId_(testId)
            , enabled_(true)
            , performanceFunction_()
            , benchmarkIterations_(0) {}
        
        /**
         * @brief Add Given step (test setup/preconditions)
         */
        TestBuilder& Given(std::string_view description, StepFunction func) {
            steps_.emplace_back(description, std::move(func), "Given");
            return *this;
        }
        
        /**
         * @brief Add When step (action under test)
         */
        TestBuilder& When(std::string_view description, StepFunction func) {
            steps_.emplace_back(description, std::move(func), "When");
            return *this;
        }
        
        /**
         * @brief Add When step for performance testing
         */
        TestBuilder& When(std::string_view description, PerformanceFunction func) {
            steps_.emplace_back(description, [](){}, "When");
            performanceFunction_ = std::move(func);
            return *this;
        }
        
        /**
         * @brief Add Then step (assertions/verification)
         */
        TestBuilder& Then(std::string_view description, StepFunction func) {
            steps_.emplace_back(description, std::move(func), "Then");
            return *this;
        }
        
        /**
         * @brief Add Then step for performance assertions
         */
        TestBuilder& Then(std::string_view description, DurationAssertionFunction func) {
            performanceThenFunction_ = std::move(func);
            performanceThenDescription_ = description;
            return *this;
        }
        
        /**
         * @brief Add And step (additional assertions/actions)
         */
        TestBuilder& And(std::string_view description, StepFunction func) {
            steps_.emplace_back(description, std::move(func), "And");
            return *this;
        }
        
        /**
         * @brief Add critical assertion for safety-critical systems
         */
        TestBuilder& CriticalAssertion(std::string_view description, CriticalAssertionFunction func) {
            criticalAssertions_.emplace_back(description, std::move(func));
            return *this;
        }
        
        /**
         * @brief Enable benchmark mode with specified iterations
         */
        TestBuilder& Benchmark(std::size_t iterations) {
            benchmarkIterations_ = iterations;
            return *this;
        }
        
        /**
         * @brief Enable/disable the test
         */
        TestBuilder& SetEnabled(bool enabled) {
            enabled_ = enabled;
            return *this;
        }
        
        /**
         * @brief Execute the test and return result
         */
        speclab::core::TestResult Execute() {
            using namespace std::chrono;
            
            auto startTime = high_resolution_clock::now();
            speclab::core::TestResult result(speclab::core::TestStatus::Passed, "Test completed successfully");
            result.testId = testId_;
            result.timestamp = system_clock::now();
            
            try {
                if (!enabled_) {
                    result.status = speclab::core::TestStatus::Skipped;
                    result.message = "Test disabled";
                    return result;
                }
                
                if (benchmarkIterations_ > 0 && performanceFunction_) {
                    executeBenchmark(result);
                } else if (performanceFunction_) {
                    executeSinglePerformanceTest(result);
                } else {
                    executeStandardTest(result);
                }
                
                // Execute critical assertions
                for (const auto& [description, assertion] : criticalAssertions_) {
                    if (!assertion()) {
                        result.status = speclab::core::TestStatus::Critical;
                        result.message = "Critical assertion failed: " + description;
                        break;
                    }
                }
                
            } catch (const speclab::core::AssertionFailure& e) {
                result.status = speclab::core::TestStatus::Failed;
                result.message = "Assertion failed";
                result.errorDetails = std::format("{} at {}", e.what(), e.formatLocation());
                result.location = e.location();
                
            } catch (const std::exception& e) {
                result.status = speclab::core::TestStatus::Error;
                result.message = "Exception during test execution";
                result.errorDetails = e.what();
                
            } catch (...) {
                result.status = speclab::core::TestStatus::Error;
                result.message = "Unknown exception during test execution";
                result.errorDetails = "Unknown exception type";
            }
            
            auto endTime = high_resolution_clock::now();
            result.duration = duration_cast<nanoseconds>(endTime - startTime);
            
            return result;
        }
        
        /**
         * @brief Get test ID
         */
        const std::string& getTestId() const noexcept { return testId_; }
        
        /**
         * @brief Get test steps for introspection
         */
        const std::vector<TestStep>& getSteps() const noexcept { return steps_; }
        
    private:
        void executeStandardTest(speclab::core::TestResult& result) {
            for (const auto& step : steps_) {
                step.execute();
            }
        }
        
        void executeSinglePerformanceTest(speclab::core::TestResult& result) {
            // Execute Given steps
            for (const auto& step : steps_) {
                if (step.stepType == "Given") {
                    step.execute();
                }
            }
            
            // Execute performance function
            auto testFunction = performanceFunction_();
            auto start = std::chrono::high_resolution_clock::now();
            testFunction();
            auto end = std::chrono::high_resolution_clock::now();
            auto duration = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start);
            
            // Execute Then steps with performance data
            if (performanceThenFunction_) {
                performanceThenFunction_(duration);
            }
            
            // Execute other steps
            for (const auto& step : steps_) {
                if (step.stepType == "Then" || step.stepType == "And") {
                    step.execute();
                }
            }
            
            result.addMetadata("performance_duration_ns", std::to_string(duration.count()));
        }
        
        void executeBenchmark(speclab::core::TestResult& result) {
            // Execute Given steps once
            for (const auto& step : steps_) {
                if (step.stepType == "Given") {
                    step.execute();
                }
            }
            
            // Run benchmark iterations
            std::vector<std::chrono::nanoseconds> durations;
            durations.reserve(benchmarkIterations_);
            
            auto testFunction = performanceFunction_();
            for (std::size_t i = 0; i < benchmarkIterations_; ++i) {
                auto start = std::chrono::high_resolution_clock::now();
                testFunction();
                auto end = std::chrono::high_resolution_clock::now();
                durations.push_back(std::chrono::duration_cast<std::chrono::nanoseconds>(end - start));
            }
            
            // Calculate statistics
            auto [minIt, maxIt] = std::minmax_element(durations.begin(), durations.end());
            auto minDuration = *minIt;
            auto maxDuration = *maxIt;
            auto totalDuration = std::accumulate(durations.begin(), durations.end(), std::chrono::nanoseconds{0});
            auto avgDuration = totalDuration / benchmarkIterations_;
            
            // Execute Then steps with average duration
            if (performanceThenFunction_) {
                performanceThenFunction_(avgDuration);
            }
            
            // Add benchmark metadata
            result.addMetadata("benchmark_iterations", std::to_string(benchmarkIterations_));
            result.addMetadata("avg_duration_ns", std::to_string(avgDuration.count()));
            result.addMetadata("min_duration_ns", std::to_string(minDuration.count()));
            result.addMetadata("max_duration_ns", std::to_string(maxDuration.count()));
        }
        
        std::string testId_;
        std::vector<TestStep> steps_;
        bool enabled_;
        
        // Performance testing
        PerformanceFunction performanceFunction_;
        DurationAssertionFunction performanceThenFunction_;
        std::string performanceThenDescription_;
        std::size_t benchmarkIterations_;
        
        // Critical assertions
        std::vector<std::pair<std::string, CriticalAssertionFunction>> criticalAssertions_;
    };

    /**
     * @brief Parameterized test builder for data-driven testing
     */
    template<typename ParameterType>
    class ParameterizedTestBuilder {
    public:
        using ParameterList = std::vector<ParameterType>;
        
        explicit ParameterizedTestBuilder(std::string_view testId, ParameterList parameters)
            : testId_(testId), parameters_(std::move(parameters)), enabled_(true) {}
        
        /**
         * @brief Add Given step with parameter access
         */
        ParameterizedTestBuilder& Given(std::string_view description, ParameterizedStepFunction<ParameterType> func) {
            givenSteps_.emplace_back(description, std::move(func));
            return *this;
        }
        
        /**
         * @brief Add When step with parameter access
         */
        ParameterizedTestBuilder& When(std::string_view description, ParameterizedStepFunction<ParameterType> func) {
            whenSteps_.emplace_back(description, std::move(func));
            return *this;
        }
        
        /**
         * @brief Add Then step with parameter access
         */
        ParameterizedTestBuilder& Then(std::string_view description, ParameterizedStepFunction<ParameterType> func) {
            thenSteps_.emplace_back(description, std::move(func));
            return *this;
        }
        
        /**
         * @brief Execute parameterized test for all parameters
         */
        std::vector<speclab::core::TestResult> Execute() {
            std::vector<speclab::core::TestResult> results;
            results.reserve(parameters_.size());
            
            for (std::size_t i = 0; i < parameters_.size(); ++i) {
                const auto& param = parameters_[i];
                auto result = executeForParameter(param, i);
                results.push_back(std::move(result));
            }
            
            return results;
        }
        
    private:
        speclab::core::TestResult executeForParameter(const ParameterType& param, std::size_t index) {
            using namespace std::chrono;
            
            auto startTime = high_resolution_clock::now();
            speclab::core::TestResult result(speclab::core::TestStatus::Passed, "Parameterized test completed");
            result.testId = std::format("{}[{}]", testId_, index);
            result.timestamp = system_clock::now();
            result.addMetadata("parameter_index", std::to_string(index));
            
            try {
                if (!enabled_) {
                    result.status = speclab::core::TestStatus::Skipped;
                    result.message = "Test disabled";
                    return result;
                }
                
                // Execute Given steps
                for (const auto& [description, func] : givenSteps_) {
                    func(param);
                }
                
                // Execute When steps
                for (const auto& [description, func] : whenSteps_) {
                    func(param);
                }
                
                // Execute Then steps
                for (const auto& [description, func] : thenSteps_) {
                    func(param);
                }
                
            } catch (const speclab::core::AssertionFailure& e) {
                result.status = speclab::core::TestStatus::Failed;
                result.message = "Assertion failed";
                result.errorDetails = std::format("{} at {}", e.what(), e.formatLocation());
                result.location = e.location();
                
            } catch (const std::exception& e) {
                result.status = speclab::core::TestStatus::Error;
                result.message = "Exception during parameterized test execution";
                result.errorDetails = e.what();
                
            } catch (...) {
                result.status = speclab::core::TestStatus::Error;
                result.message = "Unknown exception during parameterized test execution";
            }
            
            auto endTime = high_resolution_clock::now();
            result.duration = duration_cast<nanoseconds>(endTime - startTime);
            
            return result;
        }
        
        std::string testId_;
        ParameterList parameters_;
        bool enabled_;
        
        std::vector<std::pair<std::string, ParameterizedStepFunction<ParameterType>>> givenSteps_;
        std::vector<std::pair<std::string, ParameterizedStepFunction<ParameterType>>> whenSteps_;
        std::vector<std::pair<std::string, ParameterizedStepFunction<ParameterType>>> thenSteps_;
    };

    /**
     * @brief Factory function to create a simple test
     */
    TestBuilder Test(std::string_view testId) {
        return TestBuilder(testId);
    }
    
    /**
     * @brief Factory function to create a parameterized test
     */
    template<typename ParameterType>
    ParameterizedTestBuilder<ParameterType> ParameterizedTest(
        std::string_view testId, 
        std::vector<ParameterType> parameters) {
        return ParameterizedTestBuilder<ParameterType>(testId, std::move(parameters));
    }

} // namespace speclab