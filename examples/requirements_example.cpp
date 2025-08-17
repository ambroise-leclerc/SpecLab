#include <speclab/SpecLab.cppm>
import speclab;
import speclab.core.requirements;
import speclab.core.testcase;
import speclab.core.testsuite;

using namespace speclab::core;

int main() {
    // Register requirements
    RegisterRequirement({ .id = "REQ-001", .description = "System shall initialize within 200ms", .riskLevel = "HIGH", .safetyClass = "CLASS_B", .requiresAudit = true, .requiresValidation = true, .source = "SRS-Init" });
    RegisterRequirement({ .id = "REQ-002", .description = "Provide heartbeat signal", .riskLevel = "MEDIUM", .safetyClass = "CLASS_B", .source = "SRS-Comm" });

    // Simple test suite linking tests to requirements
    TestSuite suite("InitSuite");
    suite.addTest("T_InitTime", [](){ /* simulate check */ });
    suite.addTest("T_Heartbeat", [](){ /* simulate check */ });

    // Link tests to requirements (traceability)
    LinkTestRequirement("T_InitTime", "REQ-001");
    LinkTestRequirement("T_Heartbeat", "REQ-002");

    auto results = suite.execute();

    // Export trace matrix
    auto csv = ExportTraceMatrixCSV();
    std::println("TRACE MATRIX:\n{}", csv);

    // Simple exit status
    return results.getFailedCount() == 0 ? 0 : 1;
}
