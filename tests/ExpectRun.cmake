# Runs COMMAND with ARGUMENTS (a ;-list) and fails unless the exit code is EXPECTED_EXIT and the
# combined stdout+stderr matches the regular expression EXPECTED_OUTPUT.
#
# CTest's own WILL_FAIL and PASS_REGULAR_EXPRESSION cannot check both at once: a pass regex makes
# CTest ignore the exit code, so a runner that printed the right text but returned 0 for a failure
# would pass. That is exactly the kind of false pass these contract tests exist to catch.
execute_process(
    COMMAND "${COMMAND}" ${ARGUMENTS}
    RESULT_VARIABLE actual_exit
    OUTPUT_VARIABLE actual_stdout
    ERROR_VARIABLE actual_stderr
)
set(actual_output "${actual_stdout}${actual_stderr}")
# \t and \n in the expected pattern stand for a tab and a newline.
string(REPLACE "\\t" "\t" EXPECTED_OUTPUT "${EXPECTED_OUTPUT}")
string(REPLACE "\\n" "\n" EXPECTED_OUTPUT "${EXPECTED_OUTPUT}")
string(REPLACE "\r\n" "\n" actual_output "${actual_output}")

if(NOT actual_exit STREQUAL EXPECTED_EXIT)
    message(FATAL_ERROR "Expected exit code ${EXPECTED_EXIT}, got '${actual_exit}'. Output:\n${actual_output}")
endif()
if(NOT actual_output MATCHES "${EXPECTED_OUTPUT}")
    message(FATAL_ERROR "Output does not match '${EXPECTED_OUTPUT}'. Output:\n${actual_output}")
endif()
message(STATUS "exit ${actual_exit}, output matches")
