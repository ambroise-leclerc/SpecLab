cmake_minimum_required(VERSION 4.0.0)

# CRITICAL: Must be the first thing after cmake_minimum_required for toolchain detection
# Official UUID for CMake 4.0.3 import std support
# NOTE: enabling CMAKE_EXPERIMENTAL_CXX_IMPORT_STD unconditionally can cause CMake to synthesize
# a __CMAKE::CXX23 target that references non-existent 'std.cc' sources on some GCC/libstdc++ setups.
# Guard it: enable explicitly for MSVC where import std handling is stable; for GNU we use the
# manual `build_std_module` custom target defined later in this file.
if(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
    set(CMAKE_EXPERIMENTAL_CXX_IMPORT_STD "d0edc3af-4c50-42ea-a356-e2862fe7a444")
elseif(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    message(STATUS "Skipping global CMAKE_EXPERIMENTAL_CXX_IMPORT_STD for GNU - using manual build_std_module target")
elseif(CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    message(STATUS "Skipping global CMAKE_EXPERIMENTAL_CXX_IMPORT_STD for Clang - using native modules support")
else()
    message(STATUS "Skipping global CMAKE_EXPERIMENTAL_CXX_IMPORT_STD for ${CMAKE_CXX_COMPILER_ID} - using compiler-specific modules support")
endif()
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(cmake/PreventInSourceBuilds.cmake)

project(speclab VERSION 0.1.0 DESCRIPTION "Modern C++23 Modules Testing Framework for Medical Devices" HOMEPAGE_URL "https://github.com/ambroise-leclerc/speclab" LANGUAGES CXX)

set(CMAKE_CXX_EXTENSIONS OFF)

# Debug: Check if import std support was detected
message(STATUS "CMAKE_EXPERIMENTAL_CXX_IMPORT_STD: ${CMAKE_EXPERIMENTAL_CXX_IMPORT_STD}")
message(STATUS "Compiler: ${CMAKE_CXX_COMPILER_ID} ${CMAKE_CXX_COMPILER_VERSION}")
if(TARGET __CMAKE::CXX23)
    message(STATUS "__CMAKE::CXX23 target available")
else()
    if(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC" AND CMAKE_CXX_COMPILER_VERSION VERSION_GREATER_EQUAL "19.40")
        message(STATUS "__CMAKE::CXX23 target NOT available, but MSVC ${CMAKE_CXX_COMPILER_VERSION} should support import std with /experimental:module")
    elseif(CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
        message(STATUS "__CMAKE::CXX23 target NOT available - using Clang native modules support instead")
    elseif(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
        message(STATUS "__CMAKE::CXX23 target NOT available - using manual build_std_module target instead")
    else()
        message(WARNING "__CMAKE::CXX23 target NOT available - import std; will not work")
    endif()
endif()

# Compiler version requirements for C++23 modules support
if(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
    if(CMAKE_CXX_COMPILER_VERSION VERSION_LESS "19.40")
        message(FATAL_ERROR "MSVC 17.14 (Visual Studio 2022 version 17.10) or later is required for C++23 modules support. Current version: ${CMAKE_CXX_COMPILER_VERSION}")
    endif()
elseif(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    if(CMAKE_CXX_COMPILER_VERSION VERSION_LESS "15.0")
        message(FATAL_ERROR "GCC 15 or later is required for C++23 modules support. Current version: ${CMAKE_CXX_COMPILER_VERSION}")
    endif()
elseif(CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    if(CMAKE_CXX_COMPILER_VERSION VERSION_LESS "20.0")
        message(FATAL_ERROR "Clang 20 or later is required for C++23 modules support. Current version: ${CMAKE_CXX_COMPILER_VERSION}")
    endif()
else()
    message(WARNING "Unsupported compiler: ${CMAKE_CXX_COMPILER_ID}. SpecLab requires MSVC 17.14+, GCC 15+, or Clang 20+ for C++23 modules support.")
endif()


# C++23 Modules support
set(CMAKE_CXX_SCAN_FOR_MODULES ON)

# Compiler-specific modules configuration
if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    add_compile_options($<$<COMPILE_LANGUAGE:CXX>:-fmodules>)
elseif(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
    # MSVC-specific modules configuration for C++23 and import std
    add_compile_options($<$<COMPILE_LANGUAGE:CXX>:/experimental:module>)
    add_compile_options($<$<COMPILE_LANGUAGE:CXX>:/std:c++latest>)
elseif(CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    # Clang 20+ modules configuration with stdlib modules
    add_compile_options($<$<COMPILE_LANGUAGE:CXX>:-fmodules>)
    add_compile_options($<$<COMPILE_LANGUAGE:CXX>:-std=c++2c>)
    add_compile_options($<$<COMPILE_LANGUAGE:CXX>:-fbuiltin-module-map>)
    add_compile_options($<$<COMPILE_LANGUAGE:CXX>:-fimplicit-module-maps>)
endif()

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
include(StandardProjectSettings)
include(Cache)
include(CompilerSettings)
include(Linker)
include(CompilerWarnings)
include(Sanitizers)
include(StaticAnalyzers)



# Create module library target
add_library(speclab)
add_library(speclab::speclab ALIAS speclab)
add_library(speclab_options INTERFACE)
add_library(speclab_warnings INTERFACE)

# Add module source files in dependency order
target_sources(speclab
    PUBLIC
    FILE_SET cxx_modules TYPE CXX_MODULES FILES
        include/speclab/core/TestResult.cppm
        include/speclab/core/Assertions.cppm
        include/speclab/core/TestCase.cppm
        include/speclab/core/TestSuite.cppm
        include/speclab/core/RequirementAPI.cppm
        include/speclab/core/FunctionalAPI.cppm
        include/speclab/core/Requirements.cppm
        include/speclab/medical/MedicalTestCase.cppm
        include/speclab/medical/ComplianceValidator.cppm
        include/speclab/medical/MedicalFunctionalAPI.cppm
        include/speclab/reporters/Reporter.cppm
        include/speclab/reporters/ConsoleReporter.cppm
        include/speclab/runners/TestRunner.cppm
        include/speclab/SpecLab.cppm
)
# Configuration application
configure_compiler_settings(speclab_options)
configure_medical_compliance(speclab_options ${PROJECT_VERSION_MAJOR} ${PROJECT_VERSION_MINOR} ${PROJECT_VERSION_PATCH})
configure_linker(speclab_options)
set_project_warnings(speclab_warnings)

enable_sanitizers(speclab_options)

# Set target properties
target_include_directories(speclab PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)

target_compile_features(speclab PUBLIC cxx_std_23)

# Build the std module first for GCC and Clang
if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    # Create a custom target to build the std module
    add_custom_target(build_std_module
        COMMAND ${CMAKE_CXX_COMPILER} -std=c++23 -fmodules-ts -x c++-system-header iostream
        COMMAND ${CMAKE_CXX_COMPILER} -std=c++23 -fmodules-ts -x c++-system-header vector
        COMMAND ${CMAKE_CXX_COMPILER} -std=c++23 -fmodules-ts -x c++-system-header string
        COMMAND ${CMAKE_CXX_COMPILER} -std=c++23 -fmodules-ts -x c++-system-header memory
        COMMAND ${CMAKE_CXX_COMPILER} -std=c++23 -fmodules-ts -x c++-system-header thread
        COMMAND ${CMAKE_CXX_COMPILER} -std=c++23 -fmodules-ts -x c++-system-header mutex
        COMMAND ${CMAKE_CXX_COMPILER} -std=c++23 -fmodules-ts -x c++-system-header atomic
        COMMAND ${CMAKE_CXX_COMPILER} -std=c++23 -fmodules-ts -x c++-system-header chrono
        COMMAND ${CMAKE_CXX_COMPILER} -std=c++23 -fmodules-ts -x c++-system-header source_location
        WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
        COMMENT "Building std module for GCC"
    )
    # Make speclab depend on the std module
    add_dependencies(speclab build_std_module)
elseif(CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    # Create a std module for Clang using libcxx modules
    add_custom_target(build_std_module
        COMMAND ${CMAKE_CXX_COMPILER} -std=c++2c -stdlib=libc++ --precompile -x c++-module /usr/lib/llvm-20/share/libc++/v1/std.cppm -o std.pcm
        WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
        COMMENT "Building std module for Clang"
        VERBATIM
    )
    # Make speclab depend on the std module
    add_dependencies(speclab build_std_module)
    # Add the module to the compile flags
    target_compile_options(speclab PRIVATE -fmodule-file=std=${CMAKE_BINARY_DIR}/std.pcm)
endif()

# Link to the std module if available (only in build context, not export)
if(TARGET __CMAKE::CXX23)
    target_link_libraries(speclab PRIVATE __CMAKE::CXX23)
endif()

# Apply compiler settings
target_link_libraries(speclab PUBLIC speclab_options speclab_warnings)

# Build options
option(SPECLAB_BUILD_EXAMPLES "Build SpecLab examples" ON)
option(SPECLAB_BUILD_TESTS "Build SpecLab tests" ON)

# Add subdirectories
if(SPECLAB_BUILD_EXAMPLES)
    add_subdirectory(examples)
endif()

if(SPECLAB_BUILD_TESTS AND FALSE)  # Temporarily disabled until APIs are fully implemented
    enable_testing()
    add_subdirectory(tests)
endif()

# Installation
include(GNUInstallDirs)

install(TARGETS speclab speclab_options speclab_warnings
    EXPORT speclabTargets
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
    FILE_SET cxx_modules DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/modules
)

install(EXPORT speclabTargets
    FILE speclabTargets.cmake
    NAMESPACE speclab::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/speclab
)

# Create config file
include(CMakePackageConfigHelpers)
write_basic_package_version_file(
    "${CMAKE_CURRENT_BINARY_DIR}/speclabConfigVersion.cmake"
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY AnyNewerVersion
)

configure_package_config_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/cmake/speclabConfig.cmake.in"
    "${CMAKE_CURRENT_BINARY_DIR}/speclabConfig.cmake"
    INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/speclab
)

install(FILES
    "${CMAKE_CURRENT_BINARY_DIR}/speclabConfig.cmake"
    "${CMAKE_CURRENT_BINARY_DIR}/speclabConfigVersion.cmake"
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/speclab
)
