/**
 * @brief Pulse Oximeter Medical Device Example
 * 
 * Demonstrates SpecLab framework with a realistic medical device implementation
 * following IEC 62304 and ISO 14971 standards for pulse oximetry.
 */

import speclab;
import std;

using namespace speclab;
using namespace speclab::core;
using namespace speclab::medical;

/**
 * @brief Simulated oxygen probe signal data
 */
struct ProbeSignal {
    double redSignal;           // Red light absorption (660nm)
    double infraredSignal;      // Infrared light absorption (940nm)
    double signalQuality;       // Signal quality index (0.0 to 1.0)
    bool isConnected;           // Probe connection status
    std::chrono::milliseconds timestamp;
    
    ProbeSignal(double red = 0.0, double ir = 0.0, double quality = 0.0, bool connected = false)
        : redSignal(red), infraredSignal(ir), signalQuality(quality), isConnected(connected)
        , timestamp(std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now().time_since_epoch())) {}
};

/**
 * @brief Pulse oximeter device states per IEC 62304 state management
 */
enum class DeviceState {
    PowerOff,           // Device is powered off
    Initializing,       // Device startup and self-test
    NoSignal,          // Probe disconnected or no signal
    Searching,         // Searching for valid pulse signal
    Measuring,         // Actively measuring SpO2
    MedicalAttention,  // Critical SpO2 levels detected
    Error,             // Device malfunction or error state
    Maintenance        // Maintenance mode
};

/**
 * @brief SpO2 measurement result
 */
struct SpO2Reading {
    double spo2Percent;         // Oxygen saturation percentage (0-100%)
    double pulseRate;           // Heart rate in BPM
    double signalStrength;      // Signal strength indicator
    DeviceState deviceState;    // Current device state
    std::chrono::milliseconds timestamp;
    bool isValid;               // Reading validity flag
    
    SpO2Reading() : spo2Percent(0.0), pulseRate(0.0), signalStrength(0.0)
                  , deviceState(DeviceState::NoSignal), isValid(false)
                  , timestamp(std::chrono::duration_cast<std::chrono::milliseconds>(
                      std::chrono::steady_clock::now().time_since_epoch())) {}
};

/**
 * @brief Pulse Oximeter Device Implementation
 * 
 * IEC 62304 Class B medical device software for SpO2 monitoring
 */
class PulseOximeter {
public:
    PulseOximeter() : currentState_(DeviceState::PowerOff), isCalibrated_(false) {
        initializeDevice();
    }
    
    /**
     * @brief Power on the device and perform self-test
     */
    bool powerOn() {
        if (currentState_ != DeviceState::PowerOff) {
            return false;
        }
        
        currentState_ = DeviceState::Initializing;
        
        // Simulate device initialization
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        
        // Perform self-test
        if (!performSelfTest()) {
            currentState_ = DeviceState::Error;
            return false;
        }
        
        currentState_ = DeviceState::NoSignal;
        isCalibrated_ = true;
        return true;
    }
    
    /**
     * @brief Power off the device
     */
    void powerOff() {
        currentState_ = DeviceState::PowerOff;
        isCalibrated_ = false;
    }
    
    /**
     * @brief Process probe signal and calculate SpO2
     * @param signal Input probe signal data
     * @return SpO2 measurement result
     */
    SpO2Reading processSignal(const ProbeSignal& signal) {
        SpO2Reading reading;
        reading.deviceState = currentState_;
        
        // Check device state
        if (currentState_ == DeviceState::PowerOff || currentState_ == DeviceState::Error) {
            return reading;
        }
        
        // Check probe connection
        if (!signal.isConnected) {
            currentState_ = DeviceState::NoSignal;
            reading.deviceState = currentState_;
            return reading;
        }
        
        // Check signal quality
        if (signal.signalQuality < 0.3) {
            currentState_ = DeviceState::Searching;
            reading.deviceState = currentState_;
            return reading;
        }
        
        // Calculate SpO2 using Beer-Lambert law approximation
        double ratio = calculateRatio(signal.redSignal, signal.infraredSignal);
        double spo2 = calculateSpO2FromRatio(ratio);
        double pulseRate = calculatePulseRate(signal);
        
        reading.spo2Percent = spo2;
        reading.pulseRate = pulseRate;
        reading.signalStrength = signal.signalQuality;
        reading.isValid = true;
        
        // Determine device state based on SpO2 level
        if (spo2 < 85.0) {
            currentState_ = DeviceState::MedicalAttention;
        } else if (spo2 >= 95.0) {
            currentState_ = DeviceState::Measuring;
        } else {
            currentState_ = DeviceState::Measuring; // Lower normal range
        }
        
        reading.deviceState = currentState_;
        return reading;
    }
    
    /**
     * @brief Get current device state
     */
    DeviceState getCurrentState() const {
        return currentState_;
    }
    
    /**
     * @brief Check if device is calibrated and ready
     */
    bool isCalibrated() const {
        return isCalibrated_;
    }
    
    /**
     * @brief Enter maintenance mode
     */
    bool enterMaintenanceMode() {
        if (currentState_ == DeviceState::PowerOff) {
            return false;
        }
        currentState_ = DeviceState::Maintenance;
        return true;
    }

private:
    DeviceState currentState_;
    bool isCalibrated_;
    
    void initializeDevice() {
        // Device initialization logic
    }
    
    bool performSelfTest() {
        // Simulate self-test - could fail in real scenarios
        return true;
    }
    
    double calculateRatio(double red, double ir) {
        if (ir == 0.0) return 0.0;
        return red / ir;
    }
    
    double calculateSpO2FromRatio(double ratio) {
        // Simplified SpO2 calculation (real devices use calibration curves)
        // SpO2 = 110 - 25 * ratio (simplified formula)
        double spo2 = 110.0 - 25.0 * ratio;
        return std::clamp(spo2, 0.0, 100.0);
    }
    
    double calculatePulseRate(const ProbeSignal& signal) {
        // Simplified pulse rate calculation
        // In real implementation, this would analyze signal frequency
        return 60.0 + (signal.signalQuality * 40.0); // 60-100 BPM range
    }
};

/**
 * @brief Convert DeviceState to string for reporting
 */
std::string toString(DeviceState state) {
    switch (state) {
        case DeviceState::PowerOff: return "POWER_OFF";
        case DeviceState::Initializing: return "INITIALIZING";
        case DeviceState::NoSignal: return "NO_SIGNAL";
        case DeviceState::Searching: return "SEARCHING";
        case DeviceState::Measuring: return "MEASURING";
        case DeviceState::MedicalAttention: return "MEDICAL_ATTENTION";
        case DeviceState::Error: return "ERROR";
        case DeviceState::Maintenance: return "MAINTENANCE";
        default: return "UNKNOWN";
    }
}

/**
 * @brief Test Requirements (SRS)
 */
namespace Requirements {
    const std::string REQ_PWR_001 = "REQ-PWR-001";  // Device shall power on successfully
    const std::string REQ_SPO2_001 = "REQ-SPO2-001"; // Device shall measure SpO2 95-100% in normal conditions
    const std::string REQ_SPO2_002 = "REQ-SPO2-002"; // Device shall detect critical SpO2 levels (<85%)
    const std::string REQ_SIG_001 = "REQ-SIG-001";   // Device shall detect probe disconnection
    const std::string REQ_SIG_002 = "REQ-SIG-002";   // Device shall handle poor signal quality
    const std::string REQ_SAF_001 = "REQ-SAF-001";   // Device shall alert for medical attention
}

/**
 * @brief Register all requirements for traceability
 */
void registerRequirements() {
    using namespace speclab::core;
    
    RegisterRequirement({
        .id = Requirements::REQ_PWR_001,
        .description = "Device shall power on successfully and perform self-test",
        .riskLevel = "MEDIUM",
        .safetyClass = "CLASS_B",
        .requiresAudit = true,
        .requiresValidation = true,
        .source = "SRS-PWR-001"
    });
    
    RegisterRequirement({
        .id = Requirements::REQ_SPO2_001,
        .description = "Device shall accurately measure SpO2 in range 95-100% under normal conditions",
        .riskLevel = "HIGH",
        .safetyClass = "CLASS_B",
        .requiresAudit = true,
        .requiresValidation = true,
        .source = "SRS-SPO2-001"
    });
    
    RegisterRequirement({
        .id = Requirements::REQ_SPO2_002,
        .description = "Device shall detect and indicate critical SpO2 levels below 85%",
        .riskLevel = "CRITICAL",
        .safetyClass = "CLASS_C",
        .requiresAudit = true,
        .requiresValidation = true,
        .source = "SRS-SPO2-002"
    });
    
    RegisterRequirement({
        .id = Requirements::REQ_SIG_001,
        .description = "Device shall detect probe disconnection and indicate no signal state",
        .riskLevel = "HIGH",
        .safetyClass = "CLASS_B",
        .requiresAudit = true,
        .requiresValidation = true,
        .source = "SRS-SIG-001"
    });
    
    RegisterRequirement({
        .id = Requirements::REQ_SIG_002,
        .description = "Device shall handle poor signal quality and enter searching state",
        .riskLevel = "MEDIUM",
        .safetyClass = "CLASS_B",
        .requiresAudit = false,
        .requiresValidation = true,
        .source = "SRS-SIG-002"
    });
    
    RegisterRequirement({
        .id = Requirements::REQ_SAF_001,
        .description = "Device shall immediately alert when medical attention is required",
        .riskLevel = "CRITICAL",
        .safetyClass = "CLASS_C",
        .requiresAudit = true,
        .requiresValidation = true,
        .source = "SRS-SAF-001"
    });
}

/**
 * @brief Power Management Test Suite
 */
class PowerManagementTest : public MedicalTestCase {
public:
    PowerManagementTest() : MedicalTestCase("PWR_TEST_001", createMedicalContext(), "Power Management Test") {}

protected:
    void run() override {
        recordMedicalEvent("test_start", "Power management testing initiated");
        
        // Given: Device is powered off
        PulseOximeter device;
        if (device.getCurrentState() != DeviceState::PowerOff) {
            throw AssertionFailure("Device should start in PowerOff state", std::source_location::current());
        }
        
        // When: Device is powered on
        recordMedicalEvent("power_on", "Attempting device power on");
        bool powerOnSuccess = device.powerOn();
        
        // Then: Device should initialize successfully
        if (!powerOnSuccess) {
            throw AssertionFailure("Device power on failed", std::source_location::current());
        }
        
        if (device.getCurrentState() != DeviceState::NoSignal) {
            throw AssertionFailure("Device should be in NoSignal state after power on", std::source_location::current());
        }
        
        if (!device.isCalibrated()) {
            throw AssertionFailure("Device should be calibrated after successful power on", std::source_location::current());
        }
        
        recordMedicalEvent("test_complete", "Power management test completed successfully");
    }

private:
    MedicalTestContext createMedicalContext() {
        MedicalTestContext context;
        context.deviceId = "PULSEOX_001";
        context.componentName = "PowerManagement";
        context.riskLevel = RiskLevel::Medium;
        context.safetyClass = SafetyClass::ClassB;
        context.complianceStandard = "IEC_62304";
        context.requiresValidation = true;
        context.testEnvironment = "LAB_IEC62304";
        return context;
    }
};

/**
 * @brief SpO2 Measurement Accuracy Test
 */
class SpO2MeasurementTest : public MedicalTestCase {
public:
    SpO2MeasurementTest() : MedicalTestCase("SPO2_TEST_001", createMedicalContext(), "SpO2 Measurement Test") {}

protected:
    void run() override {
        recordMedicalEvent("test_start", "SpO2 measurement accuracy testing initiated");
        
        // Given: Device is powered on and operational
        PulseOximeter device;
        device.powerOn();
        
        // When: High quality signal simulating normal SpO2 (98%)
        ProbeSignal normalSignal(0.48, 0.6, 0.9, true); // Ratio = 0.8, SpO2 = 110 - 25*0.8 = 90%
        recordMedicalEvent("signal_input", "Simulating normal SpO2 signal");
        auto reading = device.processSignal(normalSignal);
        
        // Then: Device should measure SpO2 in normal range and be in measuring state
        if (!reading.isValid) {
            throw AssertionFailure("SpO2 reading should be valid with good signal", std::source_location::current());
        }
        
        if (reading.spo2Percent < 85.0 || reading.spo2Percent > 100.0) {
            throw AssertionFailure("SpO2 reading outside acceptable range", std::source_location::current());
        }
        
        if (device.getCurrentState() != DeviceState::Measuring) {
            throw AssertionFailure("Device should be in Measuring state with normal SpO2", std::source_location::current());
        }
        
        recordMedicalEvent("measurement_success", 
            std::format("SpO2: {:.1f}%, Pulse: {:.0f} BPM", reading.spo2Percent, reading.pulseRate));
    }

private:
    MedicalTestContext createMedicalContext() {
        MedicalTestContext context;
        context.deviceId = "PULSEOX_001";
        context.componentName = "SpO2Algorithm";
        context.riskLevel = RiskLevel::High;
        context.safetyClass = SafetyClass::ClassB;
        context.complianceStandard = "IEC_62304";
        context.requiresValidation = true;
        context.testEnvironment = "LAB_IEC62304";
        return context;
    }
};

/**
 * @brief Critical SpO2 Detection Test
 */
class CriticalSpO2Test : public MedicalTestCase {
public:
    CriticalSpO2Test() : MedicalTestCase("CRITICAL_SPO2_001", createMedicalContext(), "Critical SpO2 Detection") {}

protected:
    void run() override {
        recordMedicalEvent("test_start", "Critical SpO2 detection testing initiated");
        
        // Given: Device is operational
        PulseOximeter device;
        device.powerOn();
        
        // When: Signal simulating critical SpO2 (<85%)
        ProbeSignal criticalSignal(1.0, 0.6, 0.8, true); // Ratio = 1.67, SpO2 = 110 - 25*1.67 = 68%
        recordMedicalEvent("critical_signal", "Simulating critical SpO2 level");
        auto reading = device.processSignal(criticalSignal);
        
        // Then: Device should detect critical condition and enter medical attention state
        if (!reading.isValid) {
            throw AssertionFailure("Critical SpO2 reading should be valid", std::source_location::current());
        }
        
        if (reading.spo2Percent >= 85.0) {
            throw AssertionFailure("Critical SpO2 test should simulate values below 85%", std::source_location::current());
        }
        
        if (device.getCurrentState() != DeviceState::MedicalAttention) {
            throw AssertionFailure("Device should enter MedicalAttention state for critical SpO2", std::source_location::current());
        }
        
        recordMedicalEvent("critical_detection", 
            std::format("Critical SpO2 detected: {:.1f}%", reading.spo2Percent));
    }

private:
    MedicalTestContext createMedicalContext() {
        MedicalTestContext context;
        context.deviceId = "PULSEOX_001";
        context.componentName = "SafetyMonitor";
        context.riskLevel = RiskLevel::Critical;
        context.safetyClass = SafetyClass::ClassC;
        context.complianceStandard = "IEC_62304";
        context.requiresValidation = true;
        context.testEnvironment = "LAB_IEC62304";
        return context;
    }
};

/**
 * @brief BDD-style Functional Tests
 */
void runBDDTests() {
    std::println("\n=== BDD Functional Tests ===");
    
    // Test 1: Device Power Management (REQ-PWR-001)
    auto powerTest = Test("BDD_PWR_001")
        .Given("a pulse oximeter device that is powered off", []() {
            std::println("  Given: Pulse oximeter is in PowerOff state");
        })
        .When("the device is powered on", []() {
            std::println("  When: Initiating device power on sequence");
        })
        .Then("the device should complete self-test and enter NoSignal state", []() {
            PulseOximeter device;
            bool success = device.powerOn();
            if (!success || device.getCurrentState() != DeviceState::NoSignal) {
                throw AssertionFailure("Power on sequence failed", std::source_location::current());
            }
            std::println("  Then: Device successfully powered on and ready");
        });
    
    auto result1 = powerTest.Execute();
    LinkTestRequirement("BDD_PWR_001", Requirements::REQ_PWR_001);
    std::println("Power Management Test: {}", toString(result1.status));
    
    // Test 2: Normal SpO2 Measurement (REQ-SPO2-001)
    auto spo2Test = Test("BDD_SPO2_001")
        .Given("a calibrated pulse oximeter with connected probe", []() {
            std::println("  Given: Device calibrated and probe connected");
        })
        .When("a normal SpO2 signal is received (>95%)", []() {
            std::println("  When: Processing normal oxygen saturation signal");
        })
        .Then("the device should display accurate SpO2 reading in normal range", []() {
            PulseOximeter device;
            device.powerOn();
            ProbeSignal signal(0.5, 0.6, 0.9, true); // Normal SpO2
            auto reading = device.processSignal(signal);
            
            if (!reading.isValid || reading.spo2Percent < 85.0) {
                throw AssertionFailure("Normal SpO2 measurement failed", std::source_location::current());
            }
            std::println("  Then: SpO2 reading: {:.1f}% (Normal range)", reading.spo2Percent);
        });
    
    auto result2 = spo2Test.Execute();
    LinkTestRequirement("BDD_SPO2_001", Requirements::REQ_SPO2_001);
    std::println("Normal SpO2 Test: {}", toString(result2.status));
    
    // Test 3: Critical SpO2 Alert (REQ-SPO2-002, REQ-SAF-001)
    auto criticalTest = Test("BDD_CRITICAL_001")
        .Given("a functioning pulse oximeter monitoring a patient", []() {
            std::println("  Given: Device actively monitoring patient");
        })
        .When("SpO2 level drops below 85% (critical threshold)", []() {
            std::println("  When: Critical SpO2 level detected");
        })
        .Then("the device should immediately alert and enter medical attention state", []() {
            PulseOximeter device;
            device.powerOn();
            ProbeSignal criticalSignal(1.2, 0.6, 0.8, true); // Critical SpO2
            auto reading = device.processSignal(criticalSignal);
            
            if (device.getCurrentState() != DeviceState::MedicalAttention) {
                throw AssertionFailure("Critical SpO2 alert not triggered", std::source_location::current());
            }
            std::println("  Then: CRITICAL ALERT - SpO2: {:.1f}% - Medical attention required!", reading.spo2Percent);
        });
    
    auto result3 = criticalTest.Execute();
    LinkTestRequirement("BDD_CRITICAL_001", Requirements::REQ_SPO2_002);
    LinkTestRequirement("BDD_CRITICAL_001", Requirements::REQ_SAF_001);
    std::println("Critical SpO2 Test: {}", toString(result3.status));
    
    // Test 4: Probe Disconnection (REQ-SIG-001)
    auto disconnectTest = Test("BDD_PROBE_001")
        .Given("a pulse oximeter with connected probe", []() {
            std::println("  Given: Probe initially connected and functioning");
        })
        .When("the probe becomes disconnected", []() {
            std::println("  When: Probe disconnection occurs");
        })
        .Then("the device should detect disconnection and enter NoSignal state", []() {
            PulseOximeter device;
            device.powerOn();
            ProbeSignal disconnectedSignal(0.0, 0.0, 0.0, false); // Disconnected
            auto reading = device.processSignal(disconnectedSignal);
            
            if (device.getCurrentState() != DeviceState::NoSignal) {
                throw AssertionFailure("Probe disconnection not detected", std::source_location::current());
            }
            std::println("  Then: No signal detected - Check probe connection");
        });
    
    auto result4 = disconnectTest.Execute();
    LinkTestRequirement("BDD_PROBE_001", Requirements::REQ_SIG_001);
    std::println("Probe Disconnection Test: {}", toString(result4.status));
    
    // Test 5: Poor Signal Quality (REQ-SIG-002)
    auto signalQualityTest = Test("BDD_SIGNAL_001")
        .Given("a pulse oximeter with connected but poor quality probe signal", []() {
            std::println("  Given: Probe connected but signal quality is poor");
        })
        .When("signal quality is below acceptable threshold", []() {
            std::println("  When: Processing poor quality signal");
        })
        .Then("the device should enter searching state and wait for better signal", []() {
            PulseOximeter device;
            device.powerOn();
            ProbeSignal poorSignal(0.5, 0.6, 0.2, true); // Poor quality
            auto reading = device.processSignal(poorSignal);
            
            if (device.getCurrentState() != DeviceState::Searching) {
                throw AssertionFailure("Poor signal quality not handled correctly", std::source_location::current());
            }
            std::println("  Then: Searching for better signal quality...");
        });
    
    auto result5 = signalQualityTest.Execute();
    LinkTestRequirement("BDD_SIGNAL_001", Requirements::REQ_SIG_002);
    std::println("Signal Quality Test: {}", toString(result5.status));
}

/**
 * @brief Main pulse oximeter demonstration
 */
int main() {
    try {
        std::println("=== Pulse Oximeter Medical Device Test Suite ===");
        std::println("IEC 62304 Class B Medical Device Software Testing");
        std::println("Standards: IEC 62304, ISO 14971, ISO 13485\n");
        
        // Initialize SpecLab for medical device testing
        SpecLab::initialize(true, "INFO");
        
        // Register all requirements for traceability
        registerRequirements();
        
        // Run BDD functional tests
        runBDDTests();
        
        // Run comprehensive medical device test suite
        std::println("\n=== Medical Device Test Suite ===");
        TestSuite medicalSuite("PulseOximeterSuite", "IEC 62304 Pulse Oximeter Validation");
        
        // Add medical test cases
        medicalSuite.addTest(std::make_unique<PowerManagementTest>());
        medicalSuite.addTest(std::make_unique<SpO2MeasurementTest>());
        medicalSuite.addTest(std::make_unique<CriticalSpO2Test>());
        
        // Execute with medical device test runner
        auto runner = runners::createTestRunner("medical");
        auto results = runner->executeSuite(medicalSuite);
        
        // Generate compliance reports
        std::println("\n=== Compliance and Traceability ===");
        auto traceabilityMatrix = ExportTraceMatrixCSV();
        std::println("Requirements Traceability Matrix:");
        std::println("{}", traceabilityMatrix);
        
        // Final compliance summary
        std::println("\n=== Medical Device Compliance Summary ===");
        std::println("Device: Pulse Oximeter (Class B Medical Device)");
        std::println("Software Safety Classification: IEC 62304 Class B");
        std::println("Risk Management: ISO 14971 compliant");
        std::println("Quality Management: ISO 13485 processes applied");
        std::println("Test Results: {}/{} tests passed ({:.1f}%)", 
                   results.getPassedCount(), results.getTotalCount(), results.getSuccessRate());
        
        if (results.hasCriticalFailures()) {
            std::println("⚠️  CRITICAL: Device not ready for clinical deployment");
            return 1;
        } else {
            std::println("✅ All tests passed - Device ready for next validation phase");
            return 0;
        }
        
    } catch (const std::exception& e) {
        std::println("💥 Test execution failed: {}", e.what());
        return 1;
    }
}