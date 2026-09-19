/**
 * @brief Fixture for the runMain contract tests in CMakeLists.txt: one scenario that passes and
 * one that fails, with and without labels. The expected output is checked there, byte for byte.
 */
import std;
import speclab;

namespace {

const speclab::Register passing{"A passing scenario", "unit,fast", [] {
    return speclab::Test("fixture-pass").Then("nothing fails", [] {}).Execute();
}};

const speclab::Register failing{"A failing scenario", "", [] {
    return speclab::Test("fixture-fail")
        .Then("an assertion fails", [] { speclab::core::Assertions::fail("deliberate failure"); })
        .Execute();
}};

}  // namespace

int main(int argc, char** argv) {
    return speclab::runMain(argc, argv, "Fixture");
}
