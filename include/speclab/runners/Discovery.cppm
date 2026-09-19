/**
 * @brief Test registration and a runner that follows a two-part discovery contract - C++23 Module
 *
 * A test binary built with speclab::runMain implements this contract:
 *
 * - `--list-tests` prints one `name<TAB>labels` line per registered scenario, and exits 0.
 * - `--run=<name>` runs exactly that scenario. It exits 0 if it passed and 1 otherwise, with the
 *   failure on stderr. A name that matches no scenario is an error (exit 1), not a silent pass,
 *   so that a stale generated test list cannot report success for a test that no longer exists.
 * - With neither option, every scenario runs, with one PASS/FAIL line each and a summary. The exit
 *   code is 0 only if all of them passed.
 *
 * A build system can therefore register one test per scenario (CTest, for example) and attach the
 * labels to it, so a failure names the scenario and not only the binary. The contract and the
 * output format are exactly those of MduX's tests/framework/SpecLabBridge.hpp, which this module
 * replaces.
 *
 * Registration is an object, not a macro:
 *
 *     const speclab::Register addition{"Two numbers add up", "unit", [] {
 *         return speclab::Test("ADD-001")
 *             .When("2 and 3 are added", [] { ... })
 *             .Then("the sum is 5", [] { ... })
 *             .Execute();
 *     }};
 *
 *     int main(int argc, char** argv) { return speclab::runMain(argc, argv, "My Suite"); }
 */

export module speclab.runners.discovery;

import std;
import speclab.core.testresult;

export namespace speclab {

    /// One registered scenario: its name, its comma-separated labels, and how to run it.
    struct Scenario {
        std::string name;
        std::string labels;
        std::function<speclab::core::TestResult()> run;
    };

    /**
     * @brief Every scenario registered in this program, in registration order
     *
     * A function-local static, so that registration from namespace-scope objects in several
     * translation units cannot depend on the initialisation order of a namespace-scope container.
     * Not `inline`: attached to this named module, the function has a single definition in
     * libspeclab, so there is exactly one registry per program.
     */
    std::vector<Scenario>& registry() {
        static std::vector<Scenario> scenarios;
        return scenarios;
    }

    /// Registers one scenario. Construct one per scenario at namespace scope; the object itself is
    /// never used again.
    struct Register {
        Register(std::string name, std::string labels, std::function<speclab::core::TestResult()> run) {
            registry().push_back(
                Scenario{.name = std::move(name), .labels = std::move(labels), .run = std::move(run)});
        }
    };

    /**
     * @brief Implements `--list-tests` and `--run=<name>`; with neither, runs every scenario
     * @return 0 when every selected scenario passed, 1 otherwise (including an unknown name)
     */
    int runMain(int argc, char** argv, std::string_view suiteName) {
        const std::span<char*> args{argv, static_cast<std::size_t>(argc)};

        for (std::size_t i = 1; i < args.size(); ++i) {
            const std::string_view argument{args[i]};

            if (argument == "--list-tests") {
                for (const Scenario& scenario : registry()) {
                    std::println(std::cout, "{}\t{}", scenario.name, scenario.labels);
                }
                return 0;
            }

            if (argument.starts_with("--run=")) {
                const std::string_view wanted = argument.substr(std::string_view{"--run="}.size());
                const auto found = std::ranges::find_if(
                    registry(), [wanted](const Scenario& s) { return s.name == wanted; });
                if (found == registry().end()) {
                    std::println(std::cerr, "{}: no scenario named '{}'", suiteName, wanted);
                    return 1;
                }
                const speclab::core::TestResult result = found->run();
                if (!result.passed()) {
                    std::println(std::cerr, "FAILED: {}\n  {}", found->name, result.message);
                    if (!result.errorDetails.empty()) {
                        std::println(std::cerr, "  {}", result.errorDetails);
                    }
                    return 1;
                }
                return 0;
            }
        }

        std::size_t failed = 0;
        for (const Scenario& scenario : registry()) {
            const speclab::core::TestResult result = scenario.run();
            if (result.passed()) {
                std::println(std::cout, "PASS  {}", scenario.name);
            } else {
                std::println(std::cout, "FAIL  {}\n      {}", scenario.name, result.message);
                if (!result.errorDetails.empty()) {
                    std::println(std::cout, "      {}", result.errorDetails);
                }
                ++failed;
            }
        }
        std::println(std::cout, "\n{}: {} scenarios, {} failed", suiteName, registry().size(), failed);
        return failed == 0 ? 0 : 1;
    }

} // namespace speclab
