# SpecLab Examples CMakeLists.txt

# Basic working example demonstrating core functionality
add_executable(basic_example basic_example.cpp)

# Link to SpecLab framework
target_link_libraries(basic_example PRIVATE speclab)
# SpecLab's warnings and settings no longer reach consumers of speclab (see the root
# CMakeLists.txt); the examples are part of this project, so they opt in explicitly.
target_link_libraries(basic_example PRIVATE speclab_options speclab_warnings)

# Ensure C++23 support
target_compile_features(basic_example PRIVATE cxx_std_23)

# Set executable properties
set_target_properties(basic_example PROPERTIES
    CXX_STANDARD 23
    CXX_STANDARD_REQUIRED ON
    CXX_EXTENSIONS OFF
)

# Add compiler-specific settings
if(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
    target_compile_options(basic_example PRIVATE 
        /experimental:module
        /std:c++latest
    )
endif()
# No GCC or Clang branch: CMake's module scanning adds what GCC needs, and `import std` comes
# from CXX_MODULE_STD, which the root CMakeLists.txt switches on for this directory tree.


# Ensure the example depends on the main library
add_dependencies(basic_example speclab)

# Add custom target to run the example
add_custom_target(run_basic_example
    COMMAND basic_example
    DEPENDS basic_example
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    COMMENT "Running SpecLab basic example"
    VERBATIM
)

# Pulse Oximeter Medical Device Example
add_executable(pulse_oximeter_example pulse_oximeter_example.cpp)

# Link to SpecLab framework
target_link_libraries(pulse_oximeter_example PRIVATE speclab)
# SpecLab's warnings and settings no longer reach consumers of speclab (see the root
# CMakeLists.txt); the examples are part of this project, so they opt in explicitly.
target_link_libraries(pulse_oximeter_example PRIVATE speclab_options speclab_warnings)

# Ensure C++23 support
target_compile_features(pulse_oximeter_example PRIVATE cxx_std_23)

# Set executable properties
set_target_properties(pulse_oximeter_example PROPERTIES
    CXX_STANDARD 23
    CXX_STANDARD_REQUIRED ON
    CXX_EXTENSIONS OFF
)

# Add compiler-specific settings
if(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
    target_compile_options(pulse_oximeter_example PRIVATE 
        /experimental:module
        /std:c++latest
    )
endif()
# No GCC or Clang branch - see basic_example above.


# Ensure the example depends on the main library
add_dependencies(pulse_oximeter_example speclab)

# Add custom target to run the pulse oximeter example
add_custom_target(run_pulse_oximeter
    COMMAND pulse_oximeter_example
    DEPENDS pulse_oximeter_example
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    COMMENT "Running SpecLab Pulse Oximeter medical device example"
    VERBATIM
)

# Install examples (optional)
if(SPECLAB_INSTALL_EXAMPLES)
    install(TARGETS basic_example pulse_oximeter_example
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}/examples
    )
    
    install(FILES basic_example.cpp pulse_oximeter_example.cpp
        DESTINATION ${CMAKE_INSTALL_DOCDIR}/examples
    )
endif()