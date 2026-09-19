# SpecLab self-tests.
#
# Two executables, both built on speclab::runMain:
#
# - speclab_selftests: the framework's behaviour. Every scenario must pass, and each one is a
#   separate CTest entry, discovered through `--list-tests` when the tests run.
# - speclab_discovery_fixture: one passing and one deliberately failing scenario. It exists to test
#   runMain's command-line contract itself (exit codes and output), which a suite of only passing
#   scenarios cannot exercise.

function(speclab_add_selftest_executable target)
    add_executable(${target} ${ARGN})
    target_link_libraries(${target} PRIVATE speclab speclab_options speclab_warnings)
    target_compile_features(${target} PRIVATE cxx_std_23)
    set_target_properties(${target} PROPERTIES CXX_STANDARD 23 CXX_STANDARD_REQUIRED ON CXX_EXTENSIONS OFF)
    if(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
        target_compile_options(${target} PRIVATE /experimental:module /std:c++latest)
        # C5046 ("Symbol involving type with internal linkage not defined"): MSVC 19.44 (VS 2022)
        # emits it for std::function / std::shared_ptr instantiations from `import std` over types
        # with internal linkage - here, the lambdas in each file's anonymous namespace and the
        # function-local State structs. /WX turns it into an error. VS 18's compiler no longer
        # emits it for the same code, and the link step would fail if a symbol really were
        # missing, so it is disabled for the test executables only, not for libspeclab.
        target_compile_options(${target} PRIVATE /wd5046)
    endif()
endfunction()

speclab_add_selftest_executable(speclab_selftests
    SelfTestMain.cpp
    AssertionTests.cpp
    ChecksTests.cpp
    FunctionalApiTests.cpp
)
speclab_add_selftest_executable(speclab_discovery_fixture DiscoveryFixture.cpp)

# One CTest entry per self-test scenario. The list comes from the binary at test time, so adding a
# scenario needs no CMake change; a script registers each name with `--run=<name>` and its labels.
#
# TEST_INCLUDE_FILES does not evaluate generator expressions, so the file CTest includes has a fixed
# name. With a multi-config generator it forwards to the file generated for the configuration CTest
# runs (-C); with Ninja there is only one configuration and it is that file directly.
set(_speclab_ctest_template "${CMAKE_CURRENT_BINARY_DIR}/speclab_selftests_include.cmake.in")
set(_speclab_ctest_include "${CMAKE_CURRENT_BINARY_DIR}/speclab_selftests_include.cmake")
file(WRITE "${_speclab_ctest_template}"
    "include(\"${CMAKE_CURRENT_SOURCE_DIR}/DiscoverScenarios.cmake\")\n"
    "speclab_discover_scenarios(\"$<TARGET_FILE:speclab_selftests>\" \"selftest.\")\n")
get_property(_speclab_multi_config GLOBAL PROPERTY GENERATOR_IS_MULTI_CONFIG)
if(_speclab_multi_config)
    file(GENERATE OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/speclab_selftests_include-$<CONFIG>.cmake"
         INPUT "${_speclab_ctest_template}")
    file(WRITE "${_speclab_ctest_include}"
        "include(\"${CMAKE_CURRENT_BINARY_DIR}/speclab_selftests_include-\${CTEST_CONFIGURATION_TYPE}.cmake\")\n")
else()
    file(GENERATE OUTPUT "${_speclab_ctest_include}" INPUT "${_speclab_ctest_template}")
endif()
set_property(DIRECTORY APPEND PROPERTY TEST_INCLUDE_FILES "${_speclab_ctest_include}")

# runMain's contract, checked on exit code *and* output by ExpectRun.cmake. In the expected-output
# patterns, \\t and \\n stand for a tab and a newline; ExpectRun.cmake expands them, which keeps
# raw control characters off the test command line.
function(speclab_add_contract_test name exit_code output_regex)
    add_test(NAME contract.${name}
        COMMAND ${CMAKE_COMMAND}
            "-DCOMMAND=$<TARGET_FILE:speclab_discovery_fixture>"
            "-DARGUMENTS=${ARGN}"
            "-DEXPECTED_EXIT=${exit_code}"
            "-DEXPECTED_OUTPUT=${output_regex}"
            -P "${CMAKE_CURRENT_SOURCE_DIR}/ExpectRun.cmake")
    set_tests_properties(contract.${name} PROPERTIES LABELS "contract")
endfunction()

speclab_add_contract_test(list-tests 0 "^A passing scenario\\tunit,fast\\nA failing scenario\\t\\n$" --list-tests)
speclab_add_contract_test(run-passing 0 "^$" "--run=A passing scenario")
speclab_add_contract_test(run-failing 1 "^FAILED: A failing scenario\\n  Assertion failed\\n  deliberate failure at DiscoveryFixture.cpp:[0-9]+:[0-9]+\\n$" "--run=A failing scenario")
speclab_add_contract_test(run-unknown 1 "^Fixture: no scenario named 'no such scenario'\\n$" "--run=no such scenario")
speclab_add_contract_test(run-all 1 "^PASS  A passing scenario\\nFAIL  A failing scenario\\n      Assertion failed\\n      deliberate failure at DiscoveryFixture.cpp:[0-9]+:[0-9]+\\n\\nFixture: 2 scenarios, 1 failed\\n$")
