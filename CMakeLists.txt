cmake_minimum_required(VERSION 4.0.0)

# CRITICAL: Must be the first thing after cmake_minimum_required for toolchain detection
# Official UUID for CMake 4.0.3 import std support
# Set unconditionally, before project(). It cannot be guarded on CMAKE_CXX_COMPILER_ID here:
# that variable is populated by compiler detection inside project(), so before that call it is
# empty and any if/elseif on it takes the else branch - which is how this ended up disabling
# `import std` for every compiler, MSVC included.
set(CMAKE_EXPERIMENTAL_CXX_IMPORT_STD "d0edc3af-4c50-42ea-a356-e2862fe7a444")
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(cmake/PreventInSourceBuilds.cmake)

project(speclab VERSION 0.1.0 DESCRIPTION "Modern C++23 Modules Testing Framework for Medical Devices" HOMEPAGE_URL "https://github.com/ambroise-leclerc/speclab" LANGUAGES CXX)

set(CMAKE_CXX_EXTENSIONS OFF)

message(STATUS "CMAKE_EXPERIMENTAL_CXX_IMPORT_STD: ${CMAKE_EXPERIMENTAL_CXX_IMPORT_STD}")
message(STATUS "Compiler: ${CMAKE_CXX_COMPILER_ID} ${CMAKE_CXX_COMPILER_VERSION}")

# Compiler version requirements for C++23 modules support.
#
# This block must stay AHEAD of the __CMAKE::CXX23 diagnostic below. A compiler under the
# floor will not get a synthesised std module either, so running the diagnostic first would
# report a missing __CMAKE::CXX23 (and send the reader off checking UUIDs) when the real
# answer is simply that the compiler is too old. Floor first, then the target check.
if(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
    if(CMAKE_CXX_COMPILER_VERSION VERSION_LESS "19.40")
        message(FATAL_ERROR "MSVC 17.14 (Visual Studio 2022 version 17.10) or later is required for C++23 modules support. Current version: ${CMAKE_CXX_COMPILER_VERSION}")
    endif()
elseif(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    if(CMAKE_CXX_COMPILER_VERSION VERSION_LESS "16.2")
        message(FATAL_ERROR "GCC 16.2 or later is required for C++23 modules support. GCC 15 cannot round-trip libstdc++'s std module through a second-level BMI, so compiling speclab.runners.testrunner (which imports speclab.core.testsuite, which imports std) fails. Current version: ${CMAKE_CXX_COMPILER_VERSION}")
    endif()
elseif(CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    if(CMAKE_CXX_COMPILER_VERSION VERSION_LESS "20.0")
        message(FATAL_ERROR "Clang 20 or later is required for C++23 modules support. Current version: ${CMAKE_CXX_COMPILER_VERSION}")
    endif()
else()
    message(WARNING "Unsupported compiler: ${CMAKE_CXX_COMPILER_ID}. SpecLab requires MSVC 17.14+, GCC 16.2+, or Clang 20+ for C++23 modules support.")
endif()

# Check whether CMake actually synthesised the managed std module. Reached only by compilers
# at or above the floor checked above, so an absent target here is a CMake/UUID problem
# rather than an out-of-date compiler. There is no manual fallback for any compiler - see the
# comment above the __CMAKE::CXX23 link below for why none is offered.
if(TARGET __CMAKE::CXX23)
    message(STATUS "__CMAKE::CXX23 target available")
else()
    if(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
        message(STATUS "__CMAKE::CXX23 target NOT available, but MSVC ${CMAKE_CXX_COMPILER_VERSION} should support import std with /experimental:module")
    elseif(CMAKE_CXX_COMPILER_ID STREQUAL "Clang" OR CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
        message(FATAL_ERROR "__CMAKE::CXX23 target NOT available on ${CMAKE_CXX_COMPILER_ID} ${CMAKE_CXX_COMPILER_VERSION}, "
                            "and there is no fallback path for `import std`. This usually means CMake rejected "
                            "CMAKE_EXPERIMENTAL_CXX_IMPORT_STD (\"set to incorrect value\"): the UUID above is tied "
                            "to a specific CMake release. Use CMake 4.1.1, the version CI pins.")
    else()
        message(WARNING "__CMAKE::CXX23 target NOT available - import std; will not work")
    endif()
endif()


# C++23 Modules support
set(CMAKE_CXX_SCAN_FOR_MODULES ON)

# Compiler-specific modules configuration
if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    # CMake's module scanning already adds -fmodules-ts; no extra flag needed
elseif(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
    # MSVC-specific modules configuration for C++23 and import std
    add_compile_options($<$<COMPILE_LANGUAGE:CXX>:/experimental:module>)
    add_compile_options($<$<COMPILE_LANGUAGE:CXX>:/std:c++latest>)
elseif(CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    # Deliberately empty. Clang uses the std module CMake synthesises from
    # CMAKE_EXPERIMENTAL_CXX_IMPORT_STD (__CMAKE::CXX23, linked below) and nothing else.
    #
    # This block used to add -fmodules -fbuiltin-module-map -fimplicit-module-maps, which
    # enable Clang *header* modules (Clang modules / module maps) - a different feature from
    # the C++20 named modules CMake is driving here. Mixing the two makes Clang build the
    # standard library twice under two module systems. It also added -std=c++2c, overriding
    # CMAKE_CXX_STANDARD 23 for this project only while __CMAKE::CXX23 stayed at C++23.
    #
    # There is no Clang job in CI (see .github/workflows/ci.yml), so anything added back here
    # is unverified by construction. Re-enable that job before reintroducing Clang flags.
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

# No compiler gets a manually precompiled std module here. With
# CMAKE_EXPERIMENTAL_CXX_IMPORT_STD set unconditionally above, CMake synthesises
# __CMAKE::CXX23 for every supported compiler and links it below. The previous GCC
# `build_std_module` target precompiled *header units* (import <vector>;), which is a
# different feature from `import std;` and never provided the fallback it claimed; it
# was dead code (guarded by NOT TARGET __CMAKE::CXX23, which is always false here) and
# is gone. A manually precompiled Clang std.pcm path is deliberately not added either:
# it would be a second definition of module std alongside __CMAKE::CXX23, and the
# std.cppm path is distro-specific (/usr/lib/llvm-20/share/libc++/v1/std.cppm ships in
# Debian's libc++-20-dev but not in silkeh/clang:20 or most source-built LLVMs).

# Link to the std module if available (only in build context, not export).
#
# The $<BUILD_INTERFACE:...> wrapper is what makes that comment true. speclab is a static
# library, so even a PRIVATE dependency is recorded in INTERFACE_LINK_LIBRARIES (consumers must
# link it transitively) - which drags CMake's synthesised std target into the speclabTargets
# export set, and the generate step then fails with:
#     CMake Error in CMakeLists.txt:
#       requires target "__cmake_cxx23" that is not in any export set.
# That target is internal to CMake and cannot be exported. BUILD_INTERFACE keeps it in the
# build and drops it from the installed config.
#
# This only bites where __CMAKE::CXX23 actually exists. It is absent under the Visual Studio
# generator (MSVC supplies its own std module via /experimental:module), which is why the bug
# stayed hidden until CI moved the Windows jobs to Ninja.
if(TARGET __CMAKE::CXX23)
    target_link_libraries(speclab PRIVATE $<BUILD_INTERFACE:__CMAKE::CXX23>)
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
