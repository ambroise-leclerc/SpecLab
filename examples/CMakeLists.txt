# SpecLab Examples CMakeLists.txt

# Functional API Examples
add_executable(functional_api_examples functional_api_examples.cpp)

# Link to SpecLab framework
target_link_libraries(functional_api_examples PRIVATE speclab::speclab)

# Ensure C++23 support
target_compile_features(functional_api_examples PRIVATE cxx_std_23)

# Set executable properties
set_target_properties(functional_api_examples PROPERTIES
    CXX_STANDARD 23
    CXX_STANDARD_REQUIRED ON
    CXX_EXTENSIONS OFF
)

# Add compiler-specific settings
if(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
    target_compile_options(functional_api_examples PRIVATE 
        /experimental:module
        /std:c++latest
    )
elseif(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    target_compile_options(functional_api_examples PRIVATE 
        -fmodules
    )
endif()

# Link to standard library module if available
if(TARGET __CMAKE::CXX23)
    target_link_libraries(functional_api_examples PRIVATE __CMAKE::CXX23)
endif()

# Install examples (optional)
if(SPECLAB_INSTALL_EXAMPLES)
    install(TARGETS functional_api_examples
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}/examples
    )
    
    install(FILES functional_api_examples.cpp
        DESTINATION ${CMAKE_INSTALL_DOCDIR}/examples
    )
endif()

# Add custom target to run examples
add_custom_target(run_examples
    COMMAND functional_api_examples
    DEPENDS functional_api_examples
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    COMMENT "Running SpecLab functional API examples"
    VERBATIM
)

# Requirements Example
add_executable(requirements_example requirements_example.cpp)
set_target_properties(requirements_example PROPERTIES
    CXX_STANDARD 23
)
target_link_libraries(requirements_example PRIVATE speclab)