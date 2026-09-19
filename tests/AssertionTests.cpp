/**
 * @brief Self-tests for speclab::core::Assertions and AssertionFailure
 */
import std;
import speclab;

using speclab::core::AssertionFailure;
using speclab::core::Assertions;
using speclab::core::TestStatus;

namespace {

/// Runs `callable` and returns the AssertionFailure it threw, failing the scenario if it threw none.
template<typename Callable>
AssertionFailure expectFailure(Callable&& callable) {
    try {
        std::forward<Callable>(callable)();
    } catch (const AssertionFailure& failure) {
        return failure;
    }
    Assertions::fail("expected an AssertionFailure, none was thrown");
}

const speclab::Register mixedIntegers{"assertEqual compares an int literal with a size_t", "unit", [] {
    return speclab::Test("assert-equal-mixed-integers")
        .Then("3 equals a vector's size of 3", [] {
            const std::vector<int> values{1, 2, 3};
            Assertions::assertEqual(3, values.size());
            Assertions::assertNotEqual(4, values.size());
        })
        .Execute();
}};

const speclab::Register signedness{"assertEqual does not equate -1 with SIZE_MAX", "unit", [] {
    return speclab::Test("assert-equal-signedness")
        .Then("a negative int never equals a large unsigned value", [] {
            const std::size_t largest = std::numeric_limits<std::size_t>::max();
            const AssertionFailure failure = expectFailure([&] { Assertions::assertEqual(-1, largest); });
            Assertions::assertTrue(std::string_view{failure.what()}.starts_with("assertEqual failed"),
                                   "a failure is reported");
        })
        .Execute();
}};

const speclab::Register formattedFailure{"assertEqual names both values when it fails", "unit", [] {
    return speclab::Test("assert-equal-message")
        .Then("the message quotes the expected and the actual value", [] {
            const AssertionFailure failure =
                expectFailure([] { Assertions::assertEqual(5, 6L, "sum"); });
            Assertions::assertEqual(std::string{"assertEqual failed: expected '5', got '6'. sum"},
                                    std::string{failure.what()});
        })
        .Execute();
}};

const speclab::Register stringComparison{"assertEqual compares a string literal with std::string", "unit", [] {
    return speclab::Test("assert-equal-strings")
        .Then("equal text compares equal across types", [] {
            const std::string text = "speclab";
            Assertions::assertEqual("speclab", text);
            Assertions::assertNotEqual("other", text);
        })
        .Execute();
}};

const speclab::Register requirePasses{"require does nothing when its condition holds", "unit", [] {
    return speclab::Test("require-passes")
        .Then("no exception is thrown", [] { Assertions::require(true, "never formatted {}", 42); })
        .Execute();
}};

const speclab::Register requireFails{"require formats its message and records the caller's line", "unit", [] {
    return speclab::Test("require-fails")
        .Then("the failure carries the formatted text and the require line", [] {
            std::uint_least32_t line = 0;
            const AssertionFailure failure = expectFailure([&] {
                line = std::source_location::current().line(); Assertions::require(false, "expected {} diagnostic, got {}", 1, 3);
            });
            Assertions::assertEqual(std::string{"expected 1 diagnostic, got 3"}, std::string{failure.what()});
            Assertions::assertEqual(line, failure.location().line(), "require's own line");
        })
        .Execute();
}};

const speclab::Register fileNameOnly{"formatLocation reports the file name, not its path", "unit", [] {
    return speclab::Test("format-location-basename")
        .Then("the location starts with this file's name and holds no directory", [] {
            const AssertionFailure failure("probe");
            const std::string location = failure.formatLocation();
            Assertions::assertTrue(location.starts_with("AssertionTests.cpp:"), location);
            Assertions::assertTrue(location.find_first_of("/\\") == std::string::npos, location);
            Assertions::assertEqual(std::string_view{"b.cpp"}, AssertionFailure::fileName("C:\\a\\b.cpp"));
            Assertions::assertEqual(std::string_view{"b.cpp"}, AssertionFailure::fileName("/a/b.cpp"));
            Assertions::assertEqual(std::string_view{"b.cpp"}, AssertionFailure::fileName("b.cpp"));
        })
        .Execute();
}};

const speclab::Register failedThen{"A failed Then makes the test Failed, located by file name", "unit", [] {
    const speclab::core::TestResult inner = speclab::Test("inner-failure")
        .Then("it fails", [] { Assertions::assertTrue(false, "deliberate"); })
        .Execute();
    return speclab::Test("failed-then-reported")
        .Then("the inner test is Failed and its details name this file", [inner] {
            Assertions::assertTrue(inner.status == TestStatus::Failed, "status is Failed");
            Assertions::assertEqual(std::string{"Assertion failed"}, inner.message);
            Assertions::assertTrue(inner.errorDetails.find("assertTrue failed: deliberate at AssertionTests.cpp:") == 0,
                                   inner.errorDetails);
        })
        .Execute();
}};

const speclab::Register stdException{"A std::exception in a step makes the test Error, not Failed", "unit", [] {
    const speclab::core::TestResult inner = speclab::Test("inner-error")
        .When("a step throws a standard exception", [] { throw std::runtime_error("boom"); })
        .Execute();
    return speclab::Test("std-exception-reported")
        .Then("the inner test is Error and keeps the exception text", [inner] {
            Assertions::assertTrue(inner.status == TestStatus::Error, "status is Error");
            Assertions::assertEqual(std::string{"boom"}, inner.errorDetails);
        })
        .Execute();
}};

}  // namespace
