/**
 * @brief Comprehensive examples of SpecLab Functional BDD API
 * @file functional_api_examples.cpp
 */

import speclab;
import speclab.core.functionalapi;
import speclab.core.requirementapi;
import speclab.medical.functionalapi;
import speclab.core.assertions;

import std;

// Mock medical device classes for examples
namespace mock {
    class PatientMonitor {
    public:
        void connect() { connected_ = true; }
        bool isConnected() const { return connected_; }
        void receiveVitalSigns(const auto& data) { hasData_ = true; }
        bool isDisplaying() const { return hasData_; }
        std::chrono::milliseconds getResponseTime() const { return std::chrono::milliseconds(1500); }
        
    private:
        bool connected_ = false;
        bool hasData_ = false;
    };
    
    class MedicalDevice {
    public:
        void startOperation() { operating_ = true; }
        bool isOperating() const { return operating_; }
        void emergencyStop() { operating_ = false; safeState_ = true; }
        bool isSafeState() const { return safeState_; }
        
    private:
        bool operating_ = false;
        bool safeState_ = false;
    };
    
    class ECGProcessor {
    public:
        void startStream() { streaming_ = true; }
        void loadTestData(const std::string& pattern) { hasData_ = true; }
        void processSignal() { 
            // Simulate processing time
            std::this_thread::sleep_for(std::chrono::microseconds(50));
            processed_ = true; 
        }
        
    private:
        bool streaming_ = false;
        bool hasData_ = false;
        bool processed_ = false;
    };
    
    struct PatientData {
        int heartRate;
        int systolic;
        int diastolic;
        double temperature;
    };
}

/**
 * @brief Example 1: Simple BDD Test with Requirement Traceability
 */
void example_simple_bdd_test() {
    mock::PatientMonitor monitor;
    mock::PatientData mockData{75, 120, 80, 36.5};
    
    // Simple test with requirement traceability
    auto result = SpecLab::Requirement("SRS-001", "Patient monitor shall display vital signs within 2 seconds")
        .Test("DISPLAY_RESPONSE")
            .Given("Patient monitor is connected", [&]() {
                monitor.connect();
                Assert::IsTrue(monitor.isConnected());
            })
            .When("Vital signs data is received", [&]() {
                monitor.receiveVitalSigns(mockData);
            })
            .Then("Display updates within 2 seconds", [&]() {
                Assert::IsTrue(monitor.isDisplaying());
                Assert::LessThan(monitor.getResponseTime(), std::chrono::seconds(2));
            })
        .Execute();
    
    std::cout << "Simple BDD Test Result: " << (result[0].passed() ? "PASSED" : "FAILED") << std::endl;
}

/**
 * @brief Example 2: Safety-Critical Testing
 */
void example_safety_critical_test() {
    mock::MedicalDevice device;
    
    // Safety-critical test with automatic compliance
    auto results = SpecLab::Requirement("SRS-002", "Emergency stop shall halt all operations")
        .SafetyClass("ClassC")
        .RiskLevel("Critical")
        .Test("EMERGENCY_STOP")
            .Given("Device is operating normally", [&]() {
                device.startOperation();
                Assert::IsTrue(device.isOperating());
            })
            .When("Emergency stop is activated", [&]() {
                device.emergencyStop();
            })
            .Then("All operations halt immediately", [&]() {
                Assert::IsFalse(device.isOperating());
            })
            .CriticalAssertion("System enters safe state", [&]() {
                return device.isSafeState();
            })
        .Execute();
    
    for (const auto& result : results) {
        std::cout << "Safety Critical Test [" << result.testId << "]: " 
                  << (result.passed() ? "PASSED" : "FAILED") << std::endl;
        
        // Print compliance metadata
        auto metadata = result.getMetadata();
        if (metadata.find("compliance_status") != metadata.end()) {
            std::cout << "  Compliance Status: " << metadata.at("compliance_status") << std::endl;
        }
    }
}

/**
 * @brief Example 3: Performance Testing for Medical Devices
 */
void example_performance_test() {
    mock::ECGProcessor ecg;
    std::string standardPattern = "normal_sinus_rhythm";
    
    // Real-time performance requirements
    auto results = SpecLab::Requirement("SRS-003", "ECG processing shall complete within 100ms")
        .PerformanceRequirement(std::chrono::milliseconds(100))
        .Test("ECG_PERFORMANCE")
            .Given("ECG data stream is active", [&]() {
                ecg.startStream();
                ecg.loadTestData(standardPattern);
            })
            .When("Signal processing is performed", [&]() {
                return [&]() { ecg.processSignal(); };
            })
            .Then("Processing completes within time limit", [](auto duration) {
                Assert::LessThan(duration, std::chrono::milliseconds(100));
            })
            .Benchmark(1000)  // Statistical analysis over 1000 iterations
        .Execute();
    
    for (const auto& result : results) {
        std::cout << "Performance Test [" << result.testId << "]: " 
                  << (result.passed() ? "PASSED" : "FAILED") << std::endl;
        
        // Print performance metadata
        auto metadata = result.getMetadata();
        if (metadata.find("avg_duration_ns") != metadata.end()) {
            auto avgNs = std::stoll(metadata.at("avg_duration_ns"));
            auto avgMs = std::chrono::duration<double, std::milli>(std::chrono::nanoseconds(avgNs)).count();
            std::cout << "  Average Duration: " << std::fixed << std::setprecision(3) << avgMs << "ms" << std::endl;
        }
    }
}

/**
 * @brief Example 4: Data-Driven Parameterized Testing
 */
void example_parameterized_test() {
    // Mock temperature sensor
    class TemperatureSensor {
    public:
        void calibrate() { calibrated_ = true; }
        void setActualTemperature(double temp) { actual_ = temp; }
        double read() const { 
            // Simulate sensor accuracy with small variation
            std::random_device rd;
            std::mt19937 gen(rd());
            std::normal_distribution<> dis(actual_, 0.05); // ±0.05°C accuracy
            return dis(gen);
        }
        
    private:
        bool calibrated_ = false;
        double actual_ = 36.5;
    };
    
    TemperatureSensor sensor;
    double measured = 0.0;
    
    // Test parameters: [actual_temp, min_expected, max_expected]
    std::vector<std::array<double, 3>> tempParams = {
        {36.5, 36.4, 36.6},  // normal range
        {35.0, 34.9, 35.1},  // hypothermia threshold  
        {38.0, 37.9, 38.1}   // fever threshold
    };
    
    auto results = SpecLab::ParameterizedTest("TEMP_ACCURACY", tempParams)
        .Given("Temperature sensor is calibrated", [&](const auto& params) {
            sensor.calibrate();
            sensor.setActualTemperature(params[0]);
        })
        .When("Temperature is measured", [&](const auto& params) {
            measured = sensor.read();
        })
        .Then("Reading is within tolerance", [&](const auto& params) {
            Assert::InRange(measured, params[1], params[2]);
        })
        .Execute();
    
    for (const auto& result : results) {
        std::cout << "Parameterized Test [" << result.testId << "]: " 
                  << (result.passed() ? "PASSED" : "FAILED") << std::endl;
    }
}

/**
 * @brief Example 5: Feature Organization with Multiple Requirements
 */
void example_feature_organization() {
    mock::PatientMonitor monitor;
    mock::PatientData patientData{80, 125, 85, 37.0};
    
    // Group related tests by medical device feature
    auto results = SpecLab::Feature("Patient Monitoring")
        .Description("Core vital signs monitoring functionality")
        .DeviceComponent("PatientMonitor")
        .RiskLevel("Medium")
        
        .Requirement("SRS-100", "Monitor shall track heart rate accurately")
            .SafetyClass("ClassB")
            .Test("MONITOR_HEARTRATE")
                .Given("Heart rate sensor is connected", [&]() {
                    monitor.connect();
                    Assert::IsTrue(monitor.isConnected());
                })
                .When("Patient heart rate data is collected", [&]() {
                    monitor.receiveVitalSigns(patientData);
                })
                .Then("Heart rate is recorded accurately", [&]() {
                    Assert::IsTrue(monitor.isDisplaying());
                })
                
        .Requirement("SRS-101", "Monitor shall detect abnormal vital signs")
            .SafetyClass("ClassC")
            .RiskLevel("High")
            .Test("MONITOR_ABNORMAL_DETECTION")
                .Given("Monitor is actively tracking patient", [&]() {
                    monitor.connect();
                    monitor.receiveVitalSigns(patientData);
                })
                .When("Abnormal vital signs are detected", [&]() {
                    mock::PatientData abnormalData{180, 200, 120, 39.5}; // High values
                    monitor.receiveVitalSigns(abnormalData);
                })
                .Then("Alert is triggered appropriately", [&]() {
                    // Mock alert checking
                    Assert::IsTrue(monitor.isDisplaying());
                })
                .CriticalAssertion("Patient safety is maintained", [&]() {
                    return monitor.isConnected(); // Simplified safety check
                })
                
        .Execute();
    
    std::cout << "Feature Test Results:" << std::endl;
    for (const auto& result : results) {
        std::cout << "  [" << result.testId << "]: " 
                  << (result.passed() ? "PASSED" : "FAILED") << std::endl;
        
        // Show feature context
        auto metadata = result.getMetadata();
        if (metadata.find("feature_name") != metadata.end()) {
            std::cout << "    Feature: " << metadata.at("feature_name") << std::endl;
        }
        if (metadata.find("requirement_id") != metadata.end()) {
            std::cout << "    Requirement: " << metadata.at("requirement_id") << std::endl;
        }
    }
}

/**
 * @brief Example 6: Medical Device Specific API
 */
void example_medical_device_api() {
    using namespace speclab::medical;
    
    mock::MedicalDevice device;
    
    // Using medical-specific API with enhanced compliance
    auto result = MedicalRequirement("SRS-004", "Device shall enter safe state on failure",
                                    "DEV_001", "SafetyController", 
                                    RiskLevel::Critical, SafetyClass::ClassC)
        .MedicalTest("SAFE_STATE_TEST")
            .Given("Device is operating normally", [&]() {
                device.startOperation();
                Assert::IsTrue(device.isOperating());
            })
            .When("Critical failure is simulated", [&]() {
                device.emergencyStop(); // Simulate failure response
            })
            .Then("Device enters safe state", [&]() {
                Assert::IsFalse(device.isOperating());
            })
            .SafetyCriticalAssertion("Safe state is maintained", [&]() {
                return device.isSafeState();
            }, "HAZ_001")
            .MedicalAssertion("All safety systems operational", [&]() {
                return device.isSafeState();
            })
            .ComplianceAssertion("IEC_62304_5.1.1", [&]() {
                return device.isSafeState();
            }, "Safety Architecture")
        .ExecuteMedical(); // Enhanced medical execution
    
    std::cout << "Medical Device Test [" << result.testId << "]: " 
              << (result.passed() ? "PASSED" : "FAILED") << std::endl;
    
    // Print enhanced medical metadata
    auto metadata = result.getMetadata();
    if (metadata.find("device_id") != metadata.end()) {
        std::cout << "  Device ID: " << metadata.at("device_id") << std::endl;
    }
    if (metadata.find("safety_class") != metadata.end()) {
        std::cout << "  Safety Class: " << metadata.at("safety_class") << std::endl;
    }
    if (metadata.find("compliance_status") != metadata.end()) {
        std::cout << "  Compliance Status: " << metadata.at("compliance_status") << std::endl;
    }
}

/**
 * @brief Example 7: IEC 62304 Lifecycle Process Testing
 */
void example_iec62304_process() {
    mock::MedicalDevice device;
    
    // IEC 62304 software lifecycle process validation
    auto results = SpecLab::IEC62304Process(SpecLab::IEC62304ProcessBuilder::LifecycleProcess::SystemTesting)
        .SafetyClass("ClassC")
        .Requirement("SRS-008", "Software shall meet all specified requirements")
            .Test("LIFECYCLE_VALIDATION")
                .Given("Software build is complete", [&]() {
                    // Mock build verification
                    Assert::IsTrue(true); // Placeholder
                })
                .When("Validation test suite is executed", [&]() {
                    // Mock validation execution
                    device.startOperation();
                })
                .Then("All requirements are verified", [&]() {
                    Assert::IsTrue(device.isOperating());
                })
                .TraceabilityMatrix([](auto& matrix) {
                    matrix.verifyRequirementsCoverage();
                    matrix.verifyTestCompleteness();
                })
        .Execute();
    
    for (const auto& result : results) {
        std::cout << "IEC 62304 Process Test [" << result.testId << "]: " 
                  << (result.passed() ? "PASSED" : "FAILED") << std::endl;
        
        auto metadata = result.getMetadata();
        if (metadata.find("iec62304_process") != metadata.end()) {
            std::cout << "  Process: " << metadata.at("iec62304_process") << std::endl;
        }
    }
}

/**
 * @brief Main function to run all examples
 */
int main() {
    std::cout << "=== SpecLab Functional BDD API Examples ===\n" << std::endl;
    
    try {
        std::cout << "1. Simple BDD Test with Requirement Traceability" << std::endl;
        example_simple_bdd_test();
        std::cout << std::endl;
        
        std::cout << "2. Safety-Critical Testing" << std::endl;
        example_safety_critical_test();
        std::cout << std::endl;
        
        std::cout << "3. Performance Testing for Medical Devices" << std::endl;
        example_performance_test();
        std::cout << std::endl;
        
        std::cout << "4. Data-Driven Parameterized Testing" << std::endl;
        example_parameterized_test();
        std::cout << std::endl;
        
        std::cout << "5. Feature Organization with Multiple Requirements" << std::endl;
        example_feature_organization();
        std::cout << std::endl;
        
        std::cout << "6. Medical Device Specific API" << std::endl;
        example_medical_device_api();
        std::cout << std::endl;
        
        std::cout << "7. IEC 62304 Lifecycle Process Testing" << std::endl;
        example_iec62304_process();
        std::cout << std::endl;
        
        std::cout << "=== All Examples Completed Successfully ===" << std::endl;
        
    } catch (const std::exception& e) {
        std::cerr << "Example execution failed: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
}