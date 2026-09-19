/**
 * @brief Self-tests for speclab::core::Checks
 */
import std;
import speclab;

using speclab::core::AssertionFailure;
using speclab::core::Assertions;
using speclab::core::Checks;

namespace {

const speclab::Register collectsAll{"Checks reports every failed expectation in one message", "unit", [] {
    return speclab::Test("checks-collect")
        .Then("both failures are listed with file and line, the passing one is not", [] {
            Checks checks;
            checks.expect(true, "the one that holds");
            const auto first = std::source_location::current().line() + 1;
            checks.expect(false, "first failure");
            checks.expect(false, "second failure");
            Assertions::assertTrue(checks.anyFailed(), "anyFailed");

            std::string message;
            try {
                checks.raise();
            } catch (const AssertionFailure& failure) {
                message = failure.what();
            }
            Assertions::assertEqual(
                std::format("2 expectation(s) failed:\n"
                            "    - first failure (ChecksTests.cpp:{})\n"
                            "    - second failure (ChecksTests.cpp:{})",
                            first, first + 1),
                message);
        })
        .Execute();
}};

const speclab::Register silentWhenClean{"Checks does not throw when every expectation holds", "unit", [] {
    return speclab::Test("checks-clean")
        .Then("raise returns and anyFailed is false", [] {
            Checks checks;
            checks.expect(true, "a");
            checks.expect(1 + 1 == 2, "b");
            Assertions::assertFalse(checks.anyFailed(), "anyFailed");
            Assertions::assertNoThrow([&] { checks.raise(); });
        })
        .Execute();
}};

const speclab::Register failsTheTest{"A raised Checks fails the enclosing test", "unit", [] {
    const speclab::core::TestResult inner = speclab::Test("inner-checks")
        .Then("two properties are wrong", [] {
            Checks checks;
            checks.expect(false, "x");
            checks.expect(false, "y");
            checks.raise();
        })
        .Execute();
    return speclab::Test("checks-fail-test")
        .Then("the inner test failed and its details name both properties", [inner] {
            Assertions::assertFalse(inner.passed(), "inner passed");
            Assertions::assertTrue(inner.errorDetails.find("- x (") != std::string::npos, inner.errorDetails);
            Assertions::assertTrue(inner.errorDetails.find("- y (") != std::string::npos, inner.errorDetails);
        })
        .Execute();
}};

}  // namespace
