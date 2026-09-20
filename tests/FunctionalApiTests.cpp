/**
 * @brief Self-tests for speclab::Test, speclab::Test<State> and speclab::ParameterizedTest
 */
import std;
import speclab;

using speclab::core::Assertions;
using speclab::core::TestStatus;

namespace {

// At namespace scope, not inside the scenario's lambda: MSVC 19.44 (VS 2022) cannot instantiate
// speclab::Test<State> for a function-local State - it crashes (C1001) or leaves symbols undefined
// at link time (the C5046 warning). VS 18, GCC and Clang accept either.
struct AdditionState {
    int operand{0};
    int sum{0};
};

const speclab::Register stepOrder{"Given, When and Then run once each, in order", "unit", [] {
    auto order = std::make_shared<std::vector<std::string>>();
    const speclab::core::TestResult inner = speclab::Test("inner-order")
        .Given("g", [order] { order->push_back("Given"); })
        .When("w", [order] { order->push_back("When"); })
        .Then("t", [order] { order->push_back("Then"); })
        .And("a", [order] { order->push_back("And"); })
        .Execute();
    return speclab::Test("step-order")
        .Then("the steps ran in declaration order", [inner, order] {
            Assertions::assertTrue(inner.passed(), "inner passed");
            Assertions::assertTrue(*order == std::vector<std::string>{"Given", "When", "Then", "And"},
                                   "Given, When, Then, And");
        })
        .Execute();
}};

const speclab::Register statefulShared{"Test<State> hands every step the same state", "unit", [] {
    auto test = speclab::Test<AdditionState>("stateful-shared");
    test.Given("an operand of 2", [](AdditionState& s) { s.operand = 2; })
        .When("3 is added", [](AdditionState& s) { s.sum = s.operand + 3; })
        .Then("the sum is 5", [](const AdditionState& s) { Assertions::assertEqual(5, s.sum); });
    const speclab::core::TestResult inner = test.Execute();
    const int sumAfterwards = test.state().sum;
    return speclab::Test("stateful-shared-checked")
        .Then("the inner test passed and the state outlives Execute", [inner, sumAfterwards] {
            Assertions::assertTrue(inner.passed(), inner.errorDetails);
            Assertions::assertEqual(5, sumAfterwards);
        })
        .Execute();
}};

const speclab::Register statefulInitial{"Test<State> starts from the given initial state", "unit", [] {
    return speclab::Test<std::vector<int>>("stateful-initial", std::vector<int>{1, 2})
        .When("a value is appended", [](std::vector<int>& values) { values.push_back(3); })
        .Then("the state holds the initial values and the new one", [](const std::vector<int>& values) {
            Assertions::assertTrue(values == std::vector<int>{1, 2, 3}, "{1, 2, 3}");
        })
        .Execute();
}};

const speclab::Register statefulMixed{"Test<State> accepts steps that ignore the state", "unit", [] {
    auto ran = std::make_shared<bool>(false);
    return speclab::Test<int>("stateful-mixed")
        .Given("a step with no state parameter", [ran] { *ran = true; })
        .Then("it ran", [ran](int&) { Assertions::assertTrue(*ran, "void() step ran"); })
        .Execute();
}};

const speclab::Register statefulFailure{"A failing Test<State> step fails the test", "unit", [] {
    const speclab::core::TestResult inner = speclab::Test<int>("stateful-failure")
        .Then("the state is wrong", [](const int& value) { Assertions::assertEqual(1, value); })
        .Execute();
    return speclab::Test("stateful-failure-checked")
        .Then("the inner test is Failed", [inner] {
            Assertions::assertTrue(inner.status == TestStatus::Failed, "status is Failed");
        })
        .Execute();
}};

const speclab::Register disabledSkipped{"A disabled test is Skipped and runs no step", "unit", [] {
    auto ran = std::make_shared<bool>(false);
    speclab::core::TestResult inner = speclab::Test("inner-disabled")
        .Then("never runs", [ran] { *ran = true; })
        .SetEnabled(false)
        .Execute();
    return speclab::Test("disabled-skipped")
        .Then("the inner test is Skipped", [inner, ran] {
            Assertions::assertTrue(inner.status == TestStatus::Skipped, "status is Skipped");
            Assertions::assertFalse(*ran, "the step ran");
        })
        .Execute();
}};

const speclab::Register parameterized{"ParameterizedTest yields one result per parameter", "unit", [] {
    const std::vector<speclab::core::TestResult> inner =
        speclab::ParameterizedTest<int>("inner-params", {2, 3, 4})
            .Then("the parameter is even", [](const int& value) { Assertions::assertEqual(0, value % 2); })
            .Execute();
    return speclab::Test("parameterized-results")
        .Then("three results, named by index, only the odd one failed", [inner] {
            Assertions::assertEqual(3, inner.size());
            Assertions::assertEqual(std::string{"inner-params[1]"}, inner[1].testId);
            Assertions::assertTrue(inner[0].passed() && !inner[1].passed() && inner[2].passed(),
                                   "pass, fail, pass");
        })
        .Execute();
}};

/// An RAII-like handle: deleting the copy constructor suppresses the move constructor too.
struct Handle {
    Handle() = default;
    Handle(const Handle&) = delete;
    Handle& operator=(const Handle&) = delete;
    int touched{0};
};

/// Declared here rather than inside the scenario: MSVC 19.44 cannot instantiate `Test<State>`
/// for a function-local type.
struct ImmovableState {
    Handle handle;
};

const speclab::Register immovableState{"Test<State> accepts a state that is neither copyable nor movable",
                                       "unit", [] {
    // `Test<State>(id)` used to materialise and move a default-constructed value (issue #32); it
    // now builds the shared state in place, so this compiles at all - which is half the assertion.
    // The steps prove the state is shared, which is the other half.
    const speclab::core::TestResult inner = speclab::Test<ImmovableState>("inner-immovable")
        .Given("a state that cannot be moved", [](ImmovableState& state) { state.handle.touched = 7; })
        .Then("the step sees what the previous step wrote",
              [](ImmovableState& state) { Assertions::assertEqual(7, state.handle.touched); })
        .Execute();
    return speclab::Test("immovable-state")
        .Then("the inner test passed", [inner] {
            Assertions::assertTrue(inner.passed(), "the immovable-state test passed");
        })
        .Execute();
}};

}  // namespace
