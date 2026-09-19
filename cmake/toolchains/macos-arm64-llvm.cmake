# macOS toolchain: Apple Silicon, upstream LLVM/Clang 21 with libc++, for C++23 modules and
# `import std`.
#
# Adapted from MduX's cmake/toolchains/macos-arm64-llvm.cmake (MduX ADR-013), which is the macOS
# configuration MduX builds SpecLab with. AppleClang does not provide the `import std` module
# surface this build needs, so upstream LLVM is used instead: select libc++ and point CMake at
# libc++'s own `libc++.modules.json`, as on Linux.
#
# Usage:
#   brew install llvm@21 ninja
#   cmake -B build -G Ninja --toolchain cmake/toolchains/macos-arm64-llvm.cmake
# Override the LLVM installation with SPECLAB_LLVM_ROOT.
#
# Unlike MduX's file, this one does not pin CMake to exactly 4.3.1. SpecLab is a library, its root
# CMakeLists.txt already gates the supported CMake series for `import std`, and CI builds macOS with
# 4.3.1.
if(NOT CMAKE_HOST_SYSTEM_NAME STREQUAL "Darwin")
    message(FATAL_ERROR "The macos-arm64-llvm toolchain is only valid on macOS hosts.")
endif()
if(NOT CMAKE_HOST_SYSTEM_PROCESSOR MATCHES "^(arm64|aarch64)$")
    message(FATAL_ERROR
        "SpecLab's verified macOS target is Apple Silicon (arm64); host architecture is "
        "${CMAKE_HOST_SYSTEM_PROCESSOR}.")
endif()

set(_speclab_llvm_root "$ENV{SPECLAB_LLVM_ROOT}")
if(NOT _speclab_llvm_root)
    find_program(_speclab_brew brew PATHS /opt/homebrew/bin NO_DEFAULT_PATH)
    if(_speclab_brew)
        # `llvm@21` before `llvm`: it is the formula CI installs and it stays on the verified major
        # version, whereas unversioned `llvm` follows whatever Homebrew's current LLVM is.
        foreach(_speclab_brew_formula llvm@21 llvm)
            execute_process(
                COMMAND "${_speclab_brew}" --prefix ${_speclab_brew_formula}
                RESULT_VARIABLE _speclab_brew_result
                OUTPUT_VARIABLE _speclab_brew_prefix
                OUTPUT_STRIP_TRAILING_WHITESPACE
                ERROR_QUIET
            )
            if(_speclab_brew_result EQUAL 0 AND IS_DIRECTORY "${_speclab_brew_prefix}")
                set(_speclab_llvm_root "${_speclab_brew_prefix}")
                break()
            endif()
        endforeach()
    endif()
endif()
if(NOT _speclab_llvm_root OR NOT IS_DIRECTORY "${_speclab_llvm_root}")
    message(FATAL_ERROR
        "No upstream LLVM installation found. Set SPECLAB_LLVM_ROOT, or install the formula CI "
        "uses: brew install llvm@21.")
endif()

set(CMAKE_C_COMPILER "${_speclab_llvm_root}/bin/clang" CACHE FILEPATH "" FORCE)
set(CMAKE_CXX_COMPILER "${_speclab_llvm_root}/bin/clang++" CACHE FILEPATH "" FORCE)
set(CMAKE_AR "${_speclab_llvm_root}/bin/llvm-ar" CACHE FILEPATH "" FORCE)
set(CMAKE_RANLIB "${_speclab_llvm_root}/bin/llvm-ranlib" CACHE FILEPATH "" FORCE)
set(CMAKE_OSX_ARCHITECTURES arm64 CACHE STRING "" FORCE)

execute_process(
    COMMAND xcrun --sdk macosx --show-sdk-path
    RESULT_VARIABLE _speclab_sdk_result
    OUTPUT_VARIABLE _speclab_macos_sdk
    OUTPUT_STRIP_TRAILING_WHITESPACE
)
if(NOT _speclab_sdk_result EQUAL 0 OR NOT IS_DIRECTORY "${_speclab_macos_sdk}")
    message(FATAL_ERROR "xcrun could not locate the active macOS SDK (install the Xcode command line tools).")
endif()
set(CMAKE_OSX_SYSROOT "${_speclab_macos_sdk}" CACHE PATH "" FORCE)

# Same libc++ selection as the Linux toolchain: CMAKE_CXX_FLAGS_INIT only seeds the first configure
# of a build tree, so also make sure an existing CMAKE_CXX_FLAGS selects libc++, and refuse a
# conflicting standard library.
set(CMAKE_CXX_FLAGS_INIT "-stdlib=libc++")
if(DEFINED CMAKE_CXX_FLAGS)
    if(CMAKE_CXX_FLAGS MATCHES "-stdlib=([^ ]+)")
        if(NOT CMAKE_MATCH_1 STREQUAL "libc++")
            message(FATAL_ERROR
                "CMAKE_CXX_FLAGS selects -stdlib=${CMAKE_MATCH_1}, but this toolchain needs libc++: "
                "libc++ is the standard library whose std module `import std` uses here.")
        endif()
    else()
        string(STRIP "${CMAKE_CXX_FLAGS} -stdlib=libc++" _speclab_cxx_flags)
        set(CMAKE_CXX_FLAGS "${_speclab_cxx_flags}" CACHE STRING "Flags used by the CXX compiler during all build types." FORCE)
    endif()
endif()

# Homebrew's LLVM puts the manifest in lib/c++/; search the other known layouts too rather than
# asserting one, and re-discover when the LLVM root changes (see the Linux toolchain for why).
if(DEFINED CMAKE_CXX_STDLIB_MODULES_JSON AND NOT DEFINED SPECLAB_STDLIB_MODULES_JSON_ROOT)
    # Pinned by the operator.
elseif(NOT DEFINED CMAKE_CXX_STDLIB_MODULES_JSON
       OR NOT SPECLAB_STDLIB_MODULES_JSON_ROOT STREQUAL "${_speclab_llvm_root}")
    file(GLOB _speclab_libcxx_modules_json
        "${_speclab_llvm_root}/lib/c++/libc++.modules.json"
        "${_speclab_llvm_root}/lib/libc++.modules.json"
        "${_speclab_llvm_root}/lib/*/libc++.modules.json")
    list(SORT _speclab_libcxx_modules_json)
    list(LENGTH _speclab_libcxx_modules_json _speclab_libcxx_modules_json_count)
    if(_speclab_libcxx_modules_json_count EQUAL 0)
        message(FATAL_ERROR
            "No libc++.modules.json under '${_speclab_llvm_root}'. Without that manifest `import std` "
            "cannot resolve for libc++; use Homebrew's llvm@21 or set CMAKE_CXX_STDLIB_MODULES_JSON.")
    endif()
    list(GET _speclab_libcxx_modules_json 0 _speclab_libcxx_modules_json_first)
    set(CMAKE_CXX_STDLIB_MODULES_JSON "${_speclab_libcxx_modules_json_first}"
        CACHE FILEPATH "" FORCE)
    set(SPECLAB_STDLIB_MODULES_JSON_ROOT "${_speclab_llvm_root}"
        CACHE INTERNAL "LLVM root the cached libc++ module manifest was discovered under")
endif()

foreach(_speclab_required_tool
        "${CMAKE_CXX_COMPILER}"
        "${CMAKE_AR}"
        "${CMAKE_RANLIB}"
        "${CMAKE_CXX_STDLIB_MODULES_JSON}")
    if(NOT EXISTS "${_speclab_required_tool}")
        message(FATAL_ERROR "Required macOS toolchain input is missing: ${_speclab_required_tool}")
    endif()
endforeach()
