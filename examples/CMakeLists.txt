# SpecLab Examples CMakeLists.txt

# Basic working example demonstrating core functionality
add_executable(basic_example basic_example.cpp)

# Link to SpecLab framework
target_link_libraries(basic_example PRIVATE speclab)

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
elseif(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    target_compile_options(basic_example PRIVATE 
        -fmodules
    )
elseif(CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    target_compile_options(basic_example PRIVATE 
        -fmodules
        -std=c++2c
        -fbuiltin-module-map
        -fimplicit-module-maps
        -fmodule-file=std=${CMAKE_BINARY_DIR}/std.pcm
    )
    target_link_options(basic_example PRIVATE 
        -stdlib=libc++
    )
endif()

# Link to standard library module if available
if(TARGET __CMAKE::CXX23)
    target_link_libraries(basic_example PRIVATE __CMAKE::CXX23)
endif()

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
elseif(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    target_compile_options(pulse_oximeter_example PRIVATE 
        -fmodules
    )
elseif(CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    target_compile_options(pulse_oximeter_example PRIVATE 
        -fmodules
        -std=c++2c
        -fbuiltin-module-map
        -fimplicit-module-maps
        -fmodule-file=std=${CMAKE_BINARY_DIR}/std.pcm
    )
    target_link_options(pulse_oximeter_example PRIVATE 
        -stdlib=libc++
    )
endif()

# Link to standard library module if available
if(TARGET __CMAKE::CXX23)
    target_link_libraries(pulse_oximeter_example PRIVATE __CMAKE::CXX23)
endif()

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