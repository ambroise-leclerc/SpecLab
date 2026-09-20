/**
 * @brief Self-tests for the Requirement / Feature / IEC62304Process builders (#25)
 *
 * Their Execute() used to return an empty vector: nothing ran, and a caller counting failures saw
 * none. These scenarios pin the two properties that replaced it - tests attached to a requirement
 * really run, and a requirement with no test reports a non-passing result.
 *
 * Requirement ids are unique per scenario: the registry is a process-wide singleton, and the whole
 * suite runs in one process when the binary is invoked without --run=.
 */
import std;
import speclab;

using speclab::core::Assertions;
using speclab::core::Checks;
using speclab::core::TestStatus;

namespace {

const speclab::Register runsItsTests{"A requirement runs the tests attached to it", "unit", [] {
    auto ran = std::make_shared<int>(0);
    auto requirement = speclab::Requirement("REQ-RUN", "Attached tests run")
                           .RiskLevel("HIGH")
                           .SafetyClass("CLASS_B");
    requirement.Test("T_RUN_1").When("the first test runs", [ran] { ++*ran; }).Then("it passed", [] {});
    requirement.Test("T_RUN_2").When("the second test runs", [ran] { ++*ran; }).Then("it passed", [] {});
    const std::vector<speclab::core::TestResult> results = requirement.Execute();

    return speclab::Test("requirement-runs-tests")
        .Then("both tests ran, and each result carries the requirement", [results, ran] {
            Checks checks;
            checks.expect(*ran == 2, "both steps executed");
            checks.expect(results.size() == 2, "one result per test");
            if (results.size() == 2) {
                checks.expect(results[0].testId == "T_RUN_1", "the first result names its test");
                checks.expect(results[1].testId == "T_RUN_2", "the second result names its test");
                checks.expect(results[0].passed() && results[1].passed(), "both passed");
                checks.expect(results[0].metadata.at("requirement_id") == "REQ-RUN", "requirement_id metadata");
                checks.expect(results[0].metadata.at("risk_level") == "HIGH", "risk_level metadata");
                checks.expect(results[0].metadata.at("safety_class") == "CLASS_B", "safety_class metadata");
                checks.expect(results[0].requirementIds == std::vector<std::string>{"REQ-RUN"},
                              "the result's requirementIds");
            }
            checks.raise();
        })
        .Execute();
}};

const speclab::Register reportsFailures{"A failing test under a requirement is reported as failed", "unit", [] {
    auto requirement = speclab::Requirement("REQ-FAIL", "Failures propagate");
    requirement.Test("T_FAIL").Then("it fails", [] { Assertions::fail("deliberate"); });
    const std::vector<speclab::core::TestResult> results = requirement.Execute();

    return speclab::Test("requirement-reports-failure")
        .Then("the requirement's result is not passing", [results] {
            Checks checks;
            checks.expect(results.size() == 1, "one result");
            if (!results.empty()) {
                checks.expect(results[0].status == TestStatus::Failed, "status is Failed");
                checks.expect(results[0].errorDetails.starts_with("deliberate at RequirementApiTests.cpp:"),
                              results[0].errorDetails);
            }
            checks.raise();
        })
        .Execute();
}};

const speclab::Register noTestIsBlocked{"A requirement with no test reports Blocked, not nothing", "unit", [] {
    const std::vector<speclab::core::TestResult> bare =
        speclab::Requirement("REQ-EMPTY", "No test at all").Execute();
    const std::vector<speclab::core::TestResult> associatedOnly =
        speclab::Requirement("REQ-ASSOCIATED", "Only an associated id")
            .AssociateTest("T_ELSEWHERE")
            .Execute();

    return speclab::Test("requirement-without-test")
        .Then("both report one Blocked result naming the requirement", [bare, associatedOnly] {
            Checks checks;
            checks.expect(bare.size() == 1, "a bare requirement yields one result");
            checks.expect(associatedOnly.size() == 1, "an association-only requirement yields one result");
            if (!bare.empty()) {
                checks.expect(bare[0].status == TestStatus::Blocked, "bare: Blocked");
                checks.expect(!bare[0].passed(), "bare: not passing");
                checks.expect(bare[0].testId == "REQ-EMPTY", "bare: named after the requirement");
            }
            if (!associatedOnly.empty()) {
                checks.expect(associatedOnly[0].status == TestStatus::Blocked, "associated: Blocked");
                checks.expect(associatedOnly[0].message.find("traceability only") != std::string::npos,
                              associatedOnly[0].message);
            }
            checks.raise();
        })
        .Execute();
}};

const speclab::Register disabledIsSkipped{"A disabled requirement is Skipped and runs no test", "unit", [] {
    auto ran = std::make_shared<bool>(false);
    auto requirement = speclab::Requirement("REQ-DISABLED", "Disabled").SetEnabled(false);
    requirement.Test("T_DISABLED").Then("never runs", [ran] { *ran = true; });
    const std::vector<speclab::core::TestResult> results = requirement.Execute();

    return speclab::Test("requirement-disabled")
        .Then("one Skipped result, and the test did not run", [results, ran] {
            Checks checks;
            checks.expect(results.size() == 1, "one result");
            if (!results.empty()) {
                checks.expect(results[0].status == TestStatus::Skipped, "status is Skipped");
            }
            checks.expect(!*ran, "the step did not run");
            checks.raise();
        })
        .Execute();
}};

const speclab::Register traceability{"Executing a requirement registers it and its test links", "unit", [] {
    auto requirement = speclab::Requirement("REQ-TRACE", "Registered on Execute")
                           .RiskLevel("CRITICAL")
                           .SafetyClass("CLASS_C");
    requirement.Test("T_TRACE").Then("it passes", [] {});
    requirement.AssociateTest("T_TRACE_ELSEWHERE");
    requirement.Execute();
    const std::string matrix = speclab::core::ExportTraceMatrixCSV();

    return speclab::Test("requirement-traceability")
        .Then("the matrix lists the requirement with both test ids, and no CRITICAL gap remains",
              [matrix] {
                  Checks checks;
                  checks.expect(matrix.find("REQ-TRACE,CRITICAL,CLASS_C") != std::string::npos, matrix);
                  checks.expect(matrix.find("T_TRACE") != std::string::npos, "the executed test id");
                  checks.expect(matrix.find("T_TRACE_ELSEWHERE") != std::string::npos, "the associated test id");
                  checks.expect(!speclab::core::HasUncoveredCriticalRequirements(),
                                "a CRITICAL requirement with a linked test is covered");
                  checks.raise();
              })
        .Execute();
}};

const speclab::Register featureAggregates{"A feature runs its requirements and adds its own metadata", "unit", [] {
    auto feature = speclab::Feature("Vital signs").Description("Monitoring").Category("clinical");
    feature.Requirement("REQ-FEAT-1", "First").Test("T_FEAT_1").Then("passes", [] {});
    feature.Requirement("REQ-FEAT-2", "Second").Test("T_FEAT_2").Then("fails", [] { Assertions::fail("x"); });
    const std::vector<speclab::core::TestResult> results = feature.Execute();
    const std::vector<speclab::core::TestResult> empty = speclab::Feature("Empty").Execute();

    return speclab::Test("feature-aggregates")
        .Then("both requirements ran, and an empty feature is Blocked", [results, empty] {
            Checks checks;
            checks.expect(results.size() == 2, "one result per requirement's test");
            if (results.size() == 2) {
                checks.expect(results[0].passed(), "the first passed");
                checks.expect(results[1].status == TestStatus::Failed, "the second failed");
                checks.expect(results[0].metadata.at("feature_name") == "Vital signs", "feature_name");
                checks.expect(results[0].metadata.at("feature_category") == "clinical", "feature_category");
            }
            checks.expect(empty.size() == 1 && empty[0].status == TestStatus::Blocked,
                          "an empty feature reports Blocked");
            checks.raise();
        })
        .Execute();
}};

const speclab::Register lifecycleProcess{"An IEC 62304 process runs its requirements", "unit", [] {
    auto process = speclab::IEC62304Process(speclab::IEC62304ProcessBuilder::LifecycleProcess::SystemTesting)
                       .SafetyClass("CLASS_C");
    process.Requirement("REQ-IEC-1", "Verified").Test("T_IEC_1").Then("passes", [] {});
    const std::vector<speclab::core::TestResult> results = process.Execute();
    const std::vector<speclab::core::TestResult> empty =
        speclab::IEC62304Process(speclab::IEC62304ProcessBuilder::LifecycleProcess::SystemTesting).Execute();

    return speclab::Test("iec62304-process")
        .Then("the test ran with process metadata, and an empty process is Blocked", [results, empty] {
            Checks checks;
            checks.expect(results.size() == 1, "one result");
            if (!results.empty()) {
                checks.expect(results[0].passed(), "it passed");
                checks.expect(results[0].metadata.at("iec62304_safety_class") == "CLASS_C",
                              "iec62304_safety_class");
                checks.expect(results[0].metadata.contains("iec62304_process"), "iec62304_process");
            }
            checks.expect(empty.size() == 1 && empty[0].status == TestStatus::Blocked,
                          "an empty process reports Blocked");
            checks.raise();
        })
        .Execute();
}};

}  // namespace
