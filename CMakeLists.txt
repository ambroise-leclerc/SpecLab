cmake_minimum_required(VERSION 4.0.0)

# CRITICAL: `import std` is gated behind an experimental UUID that CMake rotates between release
# series. It must be set before project(), because the gate is read during compiler detection.
# Set it unconditionally: it cannot be guarded on CMAKE_CXX_COMPILER_ID, which is populated by
# that same detection inside project(), so before that call any if/elseif on it takes the else
# branch - which is how this once disabled `import std` for every compiler, MSVC included.
#
# Values are copied from Help/dev/experimental.rst of each CMake release. A series not listed
# here is refused rather than guessed: CMake rejects an unknown value ("set to incorrect value"),
# silently stops synthesising the std module, and the build then collapses into hundreds of
# "could not find module 'std'" errors that say nothing about the actual cause.
if(CMAKE_VERSION VERSION_GREATER_EQUAL "4.5")
    message(FATAL_ERROR
        "CMake ${CMAKE_VERSION} has not been qualified for SpecLab's `import std` support. "
        "Use a 4.0-4.4 release, or add this series' CMAKE_EXPERIMENTAL_CXX_IMPORT_STD value "
        "(Help/dev/experimental.rst) to the root CMakeLists.txt.")
elseif(CMAKE_VERSION VERSION_GREATER_EQUAL "4.4")
    set(CMAKE_EXPERIMENTAL_CXX_IMPORT_STD "f35a9ac6-8463-4d38-8eec-5d6008153e7d")
elseif(CMAKE_VERSION VERSION_GREATER_EQUAL "4.3")
    set(CMAKE_EXPERIMENTAL_CXX_IMPORT_STD "451f2fe2-a8a2-47c3-bc32-94786d8fc91b")
else()
    set(CMAKE_EXPERIMENTAL_CXX_IMPORT_STD "d0edc3af-4c50-42ea-a356-e2862fe7a444")
endif()
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

include(cmake/PreventInSourceBuilds.cmake)

# A cache entry only when SpecLab is the top-level project (the release workflow sets it with -D).
# As a subproject, a CACHE entry would land in the consumer's cache; a plain variable is scoped
# to this directory, and a consumer can still preset SPECLAB_PROJECT_VERSION. PROJECT_IS_TOP_LEVEL
# is not available yet - it is set by project(), which needs this value.
if(CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
    set(SPECLAB_PROJECT_VERSION "0.1.2" CACHE STRING "SpecLab project version")
elseif(NOT DEFINED SPECLAB_PROJECT_VERSION)
    set(SPECLAB_PROJECT_VERSION "0.1.2")
endif()
project(speclab VERSION ${SPECLAB_PROJECT_VERSION} DESCRIPTION "Modern C++23 Modules Testing Framework for Medical Devices" HOMEPAGE_URL "https://github.com/ambroise-leclerc/speclab" LANGUAGES CXX)

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
    if(CMAKE_CXX_COMPILER_VERSION VERSION_LESS "16.1")
        message(FATAL_ERROR "GCC 16.1 or later is required for C++23 modules support. GCC 15 cannot round-trip libstdc++'s std module through a second-level BMI, so compiling speclab.runners.testrunner (which imports speclab.core.testsuite, which imports std) fails. Current version: ${CMAKE_CXX_COMPILER_VERSION}")
    endif()
elseif(CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    if(CMAKE_CXX_COMPILER_VERSION VERSION_LESS "20.0")
        message(FATAL_ERROR "Clang 20 or later is required for C++23 modules support. Current version: ${CMAKE_CXX_COMPILER_VERSION}")
    endif()
elseif(CMAKE_CXX_COMPILER_ID STREQUAL "AppleClang")
    # Refused outright rather than warned about: AppleClang does not provide the `import std`
    # module surface this build needs, so a warning here only postpones the failure to hundreds of
    # "module 'std' not found" errors that do not name the cause.
    message(FATAL_ERROR
        "AppleClang is not supported: it does not provide the C++23 `import std` module this build "
        "needs. On macOS use upstream LLVM/Clang with libc++: "
        "--toolchain cmake/toolchains/macos-arm64-llvm.cmake (brew install llvm@21).")
else()
    message(WARNING "Unsupported compiler: ${CMAKE_CXX_COMPILER_ID}. SpecLab requires MSVC 17.14+, GCC 16.1+, or Clang 20+ for C++23 modules support.")
endif()

# macOS: the configuration MduX verified in its ADR-013, and that SpecLab's CI builds, is Apple
# Silicon with upstream LLVM/Clang 21.1.8 and libc++. Intel Macs and GCC on macOS have never built
# this tree, so they are refused rather than left to fail somewhere less legible. Another upstream
# Clang is allowed (the floor above still applies) but flagged as unverified.
if(CMAKE_SYSTEM_NAME STREQUAL "Darwin")
    # The macOS target architectures are CMAKE_OSX_ARCHITECTURES when set; CMAKE_SYSTEM_PROCESSOR
    # describes the host and only stands for the target when that list is empty. Checking the
    # processor alone would let an Apple Silicon host build for x86_64, or a universal
    # "arm64;x86_64" binary, past this guard. Every listed architecture must be arm64.
    if(CMAKE_OSX_ARCHITECTURES)
        set(_speclab_macos_archs ${CMAKE_OSX_ARCHITECTURES})
    else()
        set(_speclab_macos_archs ${CMAKE_SYSTEM_PROCESSOR})
    endif()
    foreach(_speclab_macos_arch IN LISTS _speclab_macos_archs)
        if(NOT _speclab_macos_arch MATCHES "^(arm64|aarch64)$")
            message(FATAL_ERROR
                "SpecLab supports macOS on Apple Silicon (arm64) only; the target architectures are "
                "'${_speclab_macos_archs}'. Universal and x86_64 builds are not supported.")
        endif()
    endforeach()
    if(NOT CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
        message(FATAL_ERROR
            "SpecLab on macOS requires upstream LLVM/Clang with libc++ "
            "(--toolchain cmake/toolchains/macos-arm64-llvm.cmake); found ${CMAKE_CXX_COMPILER_ID}.")
    endif()
    if(NOT CMAKE_CXX_COMPILER_VERSION VERSION_EQUAL "21.1.8")
        message(WARNING
            "SpecLab's verified macOS compiler is upstream Clang 21.1.8 (what CI and MduX use); "
            "found ${CMAKE_CXX_COMPILER_VERSION}. This combination is not tested.")
    endif()
endif()

# Check whether this compiler, under this CMake and generator, can provide `import std`.
# Reached only by compilers at or above the floor checked above, so a missing entry here is a
# CMake/UUID/generator problem rather than an out-of-date compiler. There is no manual fallback
# for any compiler - see the CXX_MODULE_STD note on the speclab target below.
#
# CMAKE_CXX_COMPILER_IMPORT_STD is the documented query (the list of standards for which a std
# module is available). The previous check, `if(TARGET __CMAKE::CXX23)`, tested an internal
# target name: CMake 4.3 no longer creates it at configure time, so that check reported a
# working toolchain as broken.
if(23 IN_LIST CMAKE_CXX_COMPILER_IMPORT_STD)
    message(STATUS "import std available for C++23")
else()
    if(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
        message(STATUS "import std NOT advertised by CMake, but MSVC ${CMAKE_CXX_COMPILER_VERSION} should support it with /experimental:module")
    elseif(CMAKE_CXX_COMPILER_ID STREQUAL "Clang" OR CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
        message(FATAL_ERROR "import std is NOT available on ${CMAKE_CXX_COMPILER_ID} ${CMAKE_CXX_COMPILER_VERSION} "
                            "(CMAKE_CXX_COMPILER_IMPORT_STD='${CMAKE_CXX_COMPILER_IMPORT_STD}'), and there is no "
                            "fallback path. Usual causes: CMake rejected CMAKE_EXPERIMENTAL_CXX_IMPORT_STD "
                            "(\"set to incorrect value\"), the generator is not Ninja, or - for Clang - CMake "
                            "could not find libc++.modules.json (see cmake/toolchains/linux-clang21-libcxx.cmake).")
    else()
        message(WARNING "import std NOT available - import std; will not work")
    endif()
endif()


# C++23 Modules support
set(CMAKE_CXX_SCAN_FOR_MODULES ON)

# Every target in this directory tree (the library and the examples) uses `import std;`.
# Directory-scoped: when SpecLab is added with add_subdirectory()/FetchContent/CPM this does not
# reach the consumer's own targets.
if(23 IN_LIST CMAKE_CXX_COMPILER_IMPORT_STD)
    set(CMAKE_CXX_MODULE_STD ON)
endif()

# Compiler-specific modules configuration
if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    # CMake's module scanning already adds -fmodules-ts; no extra flag needed
elseif(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
    # MSVC-specific modules configuration for C++23 and import std
    add_compile_options($<$<COMPILE_LANGUAGE:CXX>:/experimental:module>)
    add_compile_options($<$<COMPILE_LANGUAGE:CXX>:/std:c++latest>)
elseif(CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    # Deliberately empty. Clang uses the std module CMake synthesises from
    # CMAKE_EXPERIMENTAL_CXX_IMPORT_STD (CXX_MODULE_STD, above) and nothing else. That module
    # comes from libc++'s libc++.modules.json, so a Clang build must select libc++; the
    # toolchain file cmake/toolchains/linux-clang21-libcxx.cmake does that and is what CI uses.
    #
    # This block used to add -fmodules -fbuiltin-module-map -fimplicit-module-maps, which
    # enable Clang *header* modules (Clang modules / module maps) - a different feature from
    # the C++20 named modules CMake is driving here. Mixing the two makes Clang build the
    # standard library twice under two module systems. It also added -std=c++2c, overriding
    # CMAKE_CXX_STANDARD 23 for this project only while the std module stayed at C++23.
    #
    # Anything added back here must keep the linux-clang job in .github/workflows/ci.yml green.
endif()

# Included by path, not by name through CMAKE_MODULE_PATH. As a subproject, SpecLab inherits the
# consumer's CMAKE_MODULE_PATH, and an `include(CompilerSettings)` resolves to the first match on
# it - which was the consumer's file whenever it had one of the same name. MduX has seven: SpecLab
# built inside MduX was silently configured by MduX's helpers, which is how MduX's
# configure_medical_compliance() came to stamp MDUX_VERSION_*=0.1.0 onto MduX's own test targets.
set(SPECLAB_CMAKE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
include("${SPECLAB_CMAKE_DIR}/StandardProjectSettings.cmake")
include("${SPECLAB_CMAKE_DIR}/Cache.cmake")
include("${SPECLAB_CMAKE_DIR}/CompilerSettings.cmake")
include("${SPECLAB_CMAKE_DIR}/Linker.cmake")
include("${SPECLAB_CMAKE_DIR}/CompilerWarnings.cmake")  # declares SPECLAB_WARNINGS_AS_ERRORS; keep before StaticAnalyzers
include("${SPECLAB_CMAKE_DIR}/Sanitizers.cmake")
include("${SPECLAB_CMAKE_DIR}/StaticAnalyzers.cmake")



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
        include/speclab/runners/Discovery.cppm
        include/speclab/SpecLab.cppm
)
# Configuration application
configure_compiler_settings(speclab_options)
configure_speclab_version(speclab_options ${PROJECT_VERSION_MAJOR} ${PROJECT_VERSION_MINOR} ${PROJECT_VERSION_PATCH})
configure_linker(speclab_options)
set_project_warnings(speclab_warnings)

enable_sanitizers(speclab_options speclab)

# Set target properties
target_include_directories(speclab PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)

target_compile_features(speclab PUBLIC cxx_std_23)

# `import std` comes from CXX_MODULE_STD, initialised from CMAKE_CXX_MODULE_STD above. CMake then
# builds the std module for this target itself, and records the property - not an internal
# target - in the installed export, so a consumer's CMake rebuilds std for its own compiler.
#
# No compiler gets a manually precompiled std module, and there is no longer a hand-written
# link to CMake's internal __CMAKE::CXX23 target: that name is an implementation detail
# (CMake 4.3 no longer exposes it at configure time), and linking it by hand once dragged it
# into the speclabTargets export set, which fails generation with "requires target
# __cmake_cxx23 that is not in any export set". A hand-rolled Clang std.pcm is not offered
# either: it would be a second definition of module std, and std.cppm's location is
# distro-specific.

# Build settings, warnings and definitions are SpecLab's own, and stay out of consumers.
#
# They used to be linked PUBLIC, so every target that linked speclab::speclab inherited this
# project's warning set with -Werror, /W4 /permissive-, NOMINMAX, and a set of MDUX_* macros
# (a leftover from the project this build was copied from) - including MDUX_VERSION_*=0.1.0,
# which silently overrode the version of MduX, the main consumer, in every one of its test
# translation units.
#
# Linking them PRIVATE keeps compile options and definitions out of consumers, and
# $<BUILD_INTERFACE:...> keeps these helper targets out of the installed export set. What an
# instrumented build does require downstream is not carried here: enable_sanitizers() puts it on
# speclab itself, which is exported. That means -fsanitize=... for both compiling and linking
# (an importer of an instrumented module must be instrumented too), and --coverage for linking
# only.
target_link_libraries(speclab PRIVATE
    $<BUILD_INTERFACE:speclab_options>
    $<BUILD_INTERFACE:speclab_warnings>
)

# Build options
option(SPECLAB_BUILD_EXAMPLES "Build SpecLab examples" ON)
# The self-tests are on by default only when SpecLab is the top-level project, so a consumer that
# adds SpecLab as a subproject does not build and register them unless it asks for them.
option(SPECLAB_BUILD_TESTS "Build SpecLab's self-tests and register them with CTest" ${PROJECT_IS_TOP_LEVEL})

# Add subdirectories
if(SPECLAB_BUILD_EXAMPLES)
    add_subdirectory(examples)
endif()

if(SPECLAB_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()

# Installation
include(GNUInstallDirs)

install(TARGETS speclab
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
