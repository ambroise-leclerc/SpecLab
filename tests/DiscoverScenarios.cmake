# Registers one CTest test per scenario of a speclab::runMain binary, from its `--list-tests`
# output (`name<TAB>labels` per line). Included by CTest at test time, via TEST_INCLUDE_FILES.
function(speclab_discover_scenarios executable prefix)
    execute_process(
        COMMAND "${executable}" --list-tests
        RESULT_VARIABLE list_result
        OUTPUT_VARIABLE list_output
        ERROR_VARIABLE list_error
    )
    if(NOT list_result EQUAL 0)
        # Registered as a failing test rather than silently registering nothing: an empty list would
        # otherwise read as a green run.
        add_test("${prefix}discovery-failed" "${executable}" --list-tests)
        return()
    endif()
    string(REPLACE "\r\n" "\n" list_output "${list_output}")
    string(REPLACE ";" "\;" list_output "${list_output}")
    string(REPLACE "\n" ";" lines "${list_output}")
    set(registered 0)
    foreach(line IN LISTS lines)
        if(line STREQUAL "")
            continue()
        endif()
        string(FIND "${line}" "\t" tab)
        string(SUBSTRING "${line}" 0 ${tab} name)
        math(EXPR label_start "${tab} + 1")
        string(SUBSTRING "${line}" ${label_start} -1 labels)
        add_test("${prefix}${name}" "${executable}" "--run=${name}")
        math(EXPR registered "${registered} + 1")
        if(NOT labels STREQUAL "")
            string(REPLACE "," ";" label_list "${labels}")
            set_tests_properties("${prefix}${name}" PROPERTIES LABELS "${label_list}")
        endif()
    endforeach()
    # A binary that lists no scenario must not leave the suite looking complete: other tests (the
    # contract.* ones) would keep ctest non-empty, so --no-tests=error could not notice. Register a
    # test that fails through the runner's own contract: "--run=" with an empty name is an unknown
    # scenario, exit 1.
    if(registered EQUAL 0)
        add_test("${prefix}no-scenarios-discovered" "${executable}" "--run=")
    endif()
endfunction()
