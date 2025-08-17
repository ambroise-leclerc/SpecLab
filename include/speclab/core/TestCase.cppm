/**
 * @brief Test case base class for medical device testing - C++23 Module
 */

export module speclab.core.testcase;

import std;
import speclab.core.testresult;
import speclab.core.assertions;

export namespace speclab::core {

    /**
     * @brief Test fixture base class providing setup and teardown
     */
    class TestFixture {
    public:
        virtual ~TestFixture() = default;
        
        /**
         * @brief Setup executed before each test
         */
        virtual void setUp() {}
        
        /**
         * @brief Teardown executed after each test
         */
        virtual void tearDown() {}
        
        /**
         * @brief One-time setup for the entire test fixture
         */
        virtual void setUpClass() {}
        
        /**
         * @brief One-time teardown for the entire test fixture
         */
        virtual void tearDownClass() {}
    };

    /**
     * @brief Abstract base class for test cases
     */
    class TestCase {
    public:
        /**
         * @brief Constructor with test identification
         * @param id Unique test identifier
         * @param name Human-readable test name
         * @param description Test description
         */
        explicit TestCase(std::string_view id, 
                         std::string_view name = "",
                         std::string_view description = "")
            : testId_(id)
            , testName_(name.empty() ? std::string(id) : std::string(name))
            , description_(description)
            , enabled_(true)
            , riskLevel_("LOW")
            , complianceStandard_("IEC_62304")
            , requiresAudit_(false) {}
        
        virtual ~TestCase() = default;
        
        /**
         * @brief Execute the test case
         * @return Test result with timing and status
         */
        TestResult execute() {
            auto startTime = std::chrono::high_resolution_clock::now();
            TestResult result(TestStatus::Passed, "Test completed successfully");
            result.testId = testId_;
            result.timestamp = std::chrono::system_clock::now();
            
            try {
                if (!enabled_) {
                    result.status = TestStatus::Skipped;
                    result.message = "Test disabled";
                    return result;
                }
                
                // Setup phase
                setUp();
                
                // Execute test
                run();
                
                // Teardown phase
                tearDown();
                
            } catch (const AssertionFailure& e) {
                result.status = (riskLevel_ == "CRITICAL" || riskLevel_ == "HIGH") 
                              ? TestStatus::Critical : TestStatus::Failed;
                result.message = "Assertion failed";
                result.errorDetails = std::format("{} at {}", e.what(), e.formatLocation());
                result.location = e.location();
                
            } catch (const std::exception& e) {
                result.status = TestStatus::Error;
                result.message = "Exception thrown during test execution";
                result.errorDetails = e.what();
                
            } catch (...) {
                result.status = TestStatus::Error;
                result.message = "Unknown exception thrown during test execution";
                result.errorDetails = "Unknown exception type";
            }
            
            auto endTime = std::chrono::high_resolution_clock::now();
            result.duration = std::chrono::duration_cast<std::chrono::nanoseconds>(endTime - startTime);
            
            // Set medical device information
            result.setMedicalInfo(riskLevel_, complianceStandard_, deviceComponent_, requiresAudit_);
            
            return result;
        }
        
        // Getters
        const std::string& getId() const noexcept { return testId_; }
        const std::string& getName() const noexcept { return testName_; }
        const std::string& getDescription() const noexcept { return description_; }
        bool isEnabled() const noexcept { return enabled_; }
        const std::string& getRiskLevel() const noexcept { return riskLevel_; }
        const std::string& getComplianceStandard() const noexcept { return complianceStandard_; }
        const std::string& getDeviceComponent() const noexcept { return deviceComponent_; }
        bool requiresAudit() const noexcept { return requiresAudit_; }
        
        // Setters
        void setEnabled(bool enabled) { enabled_ = enabled; }
        void setRiskLevel(std::string_view level) { riskLevel_ = level; }
        void setComplianceStandard(std::string_view standard) { complianceStandard_ = standard; }
        void setDeviceComponent(std::string_view component) { deviceComponent_ = component; }
        void setRequiresAudit(bool audit) { requiresAudit_ = audit; }
        
        /**
         * @brief Add custom metadata to the test
         * @param key Metadata key
         * @param value Metadata value
         */
        void addMetadata(std::string_view key, std::string_view value) {
            metadata_[std::string(key)] = std::string(value);
        }
        
        /**
         * @brief Get test metadata
         * @return Const reference to metadata map
         */
        const std::unordered_map<std::string, std::string>& getMetadata() const noexcept {
            return metadata_;
        }
        
    protected:
        /**
         * @brief Pure virtual method implementing the test logic
         */
        virtual void run() = 0;
        
        /**
         * @brief Setup executed before test run
         */
        virtual void setUp() {}
        
        /**
         * @brief Teardown executed after test run
         */
        virtual void tearDown() {}
        
    private:
        std::string testId_;
        std::string testName_;
        std::string description_;
        bool enabled_;
        std::string riskLevel_;
        std::string complianceStandard_;
        std::string deviceComponent_;
        bool requiresAudit_;
        std::unordered_map<std::string, std::string> metadata_;
    };

    /**
     * @brief Function-based test case for simple tests
     */
    class FunctionTestCase : public TestCase {
    public:
        using TestFunction = std::function<void()>;
        
        /**
         * @brief Constructor with test function
         * @param id Test identifier
         * @param testFunc Function containing test logic
         * @param name Test name (optional)
         * @param description Test description (optional)
         */
        explicit FunctionTestCase(std::string_view id,
                                TestFunction testFunc,
                                std::string_view name = "",
                                std::string_view description = "")
            : TestCase(id, name, description), testFunction_(std::move(testFunc)) {}
        
    protected:
        void run() override {
            if (testFunction_) {
                testFunction_();
            }
        }
        
    private:
        TestFunction testFunction_;
    };

    /**
     * @brief Parameterized test case for data-driven testing
     */
    template<typename ParameterType>
    class ParameterizedTestCase : public TestCase {
    public:
        using TestFunction = std::function<void(const ParameterType&)>;
        using ParameterList = std::vector<ParameterType>;
        
        /**
         * @brief Constructor with test function and parameters
         * @param id Base test identifier
         * @param testFunc Function containing test logic
         * @param parameters List of parameters to test
         * @param name Test name (optional)
         */
        explicit ParameterizedTestCase(std::string_view id,
                                     TestFunction testFunc,
                                     ParameterList parameters,
                                     std::string_view name = "")
            : TestCase(id, name)
            , testFunction_(std::move(testFunc))
            , parameters_(std::move(parameters)) {}
        
        /**
         * @brief Execute parameterized test for all parameters
         * @return Collection of results for each parameter
         */
        std::vector<TestResult> executeAll() {
            std::vector<TestResult> results;
            results.reserve(parameters_.size());
            
            for (std::size_t i = 0; i < parameters_.size(); ++i) {
                currentParameter_ = &parameters_[i];
                currentParameterIndex_ = i;
                
                auto result = execute();
                result.testId = std::format("{}[{}]", getId(), i);
                result.addMetadata("parameter_index", std::to_string(i));
                
                results.push_back(std::move(result));
            }
            
            return results;
        }
        
        /**
         * @brief Get current parameter being tested
         * @return Reference to current parameter
         */
        const ParameterType& getCurrentParameter() const {
            if (!currentParameter_) {
                throw std::runtime_error("No current parameter set");
            }
            return *currentParameter_;
        }
        
        /**
         * @brief Get current parameter index
         * @return Current parameter index
         */
        std::size_t getCurrentParameterIndex() const noexcept {
            return currentParameterIndex_;
        }
        
    protected:
        void run() override {
            if (testFunction_ && currentParameter_) {
                testFunction_(*currentParameter_);
            }
        }
        
    private:
        TestFunction testFunction_;
        ParameterList parameters_;
        const ParameterType* currentParameter_ = nullptr;
        std::size_t currentParameterIndex_ = 0;
    };

    /**
     * @brief Benchmark test case for performance testing
     */
    class BenchmarkTestCase : public TestCase {
    public:
        using BenchmarkFunction = std::function<void()>;
        
        /**
         * @brief Constructor with benchmark function
         * @param id Test identifier
         * @param benchFunc Function to benchmark
         * @param iterations Number of iterations (default: 1000)
         * @param name Test name (optional)
         */
        explicit BenchmarkTestCase(std::string_view id,
                                 BenchmarkFunction benchFunc,
                                 std::size_t iterations = 1000,
                                 std::string_view name = "")
            : TestCase(id, name)
            , benchmarkFunction_(std::move(benchFunc))
            , iterations_(iterations) {}
        
        /**
         * @brief Execute benchmark and measure performance
         * @return Test result with detailed performance metrics
         */
        TestResult executeBenchmark() {
            auto result = execute();
            
            // Add performance metadata
            result.addMetadata("iterations", std::to_string(iterations_));
            result.addMetadata("avg_duration_ns", std::to_string(avgDuration_.count()));
            result.addMetadata("min_duration_ns", std::to_string(minDuration_.count()));
            result.addMetadata("max_duration_ns", std::to_string(maxDuration_.count()));
            
            return result;
        }
        
        // Performance metrics getters
        std::chrono::nanoseconds getAverageDuration() const noexcept { return avgDuration_; }
        std::chrono::nanoseconds getMinDuration() const noexcept { return minDuration_; }
        std::chrono::nanoseconds getMaxDuration() const noexcept { return maxDuration_; }
        std::size_t getIterations() const noexcept { return iterations_; }
        
    protected:
        void run() override {
            if (!benchmarkFunction_) return;
            
            std::vector<std::chrono::nanoseconds> durations;
            durations.reserve(iterations_);
            
            for (std::size_t i = 0; i < iterations_; ++i) {
                auto start = std::chrono::high_resolution_clock::now();
                benchmarkFunction_();
                auto end = std::chrono::high_resolution_clock::now();
                
                auto duration = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start);
                durations.push_back(duration);
            }
            
            // Calculate statistics
            auto [minIt, maxIt] = std::minmax_element(durations.begin(), durations.end());
            minDuration_ = *minIt;
            maxDuration_ = *maxIt;
            
            auto totalDuration = std::accumulate(durations.begin(), durations.end(), 
                                               std::chrono::nanoseconds{0});
            avgDuration_ = totalDuration / iterations_;
        }
        
    private:
        BenchmarkFunction benchmarkFunction_;
        std::size_t iterations_;
        std::chrono::nanoseconds avgDuration_{0};
        std::chrono::nanoseconds minDuration_{0};
        std::chrono::nanoseconds maxDuration_{0};
    };

} // namespace speclab::core