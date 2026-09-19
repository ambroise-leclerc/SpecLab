# Linux toolchain: upstream Clang 21 with libc++, for C++23 modules and `import std`.
#
# Adapted from MduX's cmake/toolchains/linux-clang21-libcxx.cmake, which builds SpecLab with this
# exact setup on every push. Two things make `import std` resolve for Clang, and neither is
# optional:
#   1. the compile selects libc++ (-stdlib=libc++), because libc++ is the standard library that
#      ships a std module Clang can build;
#   2. CMake is pointed at libc++'s own `libc++.modules.json`, which is how it finds that module.
#
# Usage:
#   cmake -B build -G Ninja --toolchain cmake/toolchains/linux-clang21-libcxx.cmake
# Packages (apt.llvm.org): clang-21, libc++-21-dev, libc++abi-21-dev.
if(NOT CMAKE_HOST_SYSTEM_NAME STREQUAL "Linux")
    message(FATAL_ERROR "The linux-clang21-libcxx toolchain is only valid on Linux hosts.")
endif()

set(_speclab_llvm_root "$ENV{SPECLAB_LLVM_ROOT}")
if(NOT _speclab_llvm_root)
    # apt.llvm.org installs versioned roots here; this is the layout CI provisions.
    set(_speclab_llvm_root "/usr/lib/llvm-21")
endif()
if(NOT IS_DIRECTORY "${_speclab_llvm_root}")
    message(FATAL_ERROR
        "No LLVM 21 root at '${_speclab_llvm_root}'. Set SPECLAB_LLVM_ROOT, or install "
        "clang-21, libc++-21-dev and libc++abi-21-dev from apt.llvm.org.")
endif()

set(CMAKE_C_COMPILER "${_speclab_llvm_root}/bin/clang" CACHE FILEPATH "" FORCE)
set(CMAKE_CXX_COMPILER "${_speclab_llvm_root}/bin/clang++" CACHE FILEPATH "" FORCE)
set(CMAKE_AR "${_speclab_llvm_root}/bin/llvm-ar" CACHE FILEPATH "" FORCE)
set(CMAKE_RANLIB "${_speclab_llvm_root}/bin/llvm-ranlib" CACHE FILEPATH "" FORCE)

# CMAKE_CXX_FLAGS_INIT only seeds CMAKE_CXX_FLAGS on the first configure of a build tree. When
# CMAKE_CXX_FLAGS already exists - passed with -D, or kept by a reused build tree - the _INIT value
# is ignored, and a compile without libc++ would reject `import std` even though the manifest
# below is found. So make sure the flag is in the effective value, and refuse a conflicting
# standard library rather than silently mixing two.
set(CMAKE_CXX_FLAGS_INIT "-stdlib=libc++")
if(DEFINED CMAKE_CXX_FLAGS)
    # Every -stdlib= occurrence is checked, not only the first: "-stdlib=libc++ -stdlib=libstdc++"
    # would otherwise pass, and the compiler honours the last one.
    string(REGEX MATCHALL "-stdlib=[^ ]+" _speclab_stdlib_flags "${CMAKE_CXX_FLAGS}")
    if(_speclab_stdlib_flags)
        foreach(_speclab_stdlib_flag IN LISTS _speclab_stdlib_flags)
            if(NOT _speclab_stdlib_flag STREQUAL "-stdlib=libc++")
                message(FATAL_ERROR
                    "CMAKE_CXX_FLAGS contains ${_speclab_stdlib_flag}, but this toolchain needs libc++: "
                    "libc++ is the standard library whose std module `import std` uses here.")
            endif()
        endforeach()
    else()
        string(STRIP "${CMAKE_CXX_FLAGS} -stdlib=libc++" _speclab_cxx_flags)
        set(CMAKE_CXX_FLAGS "${_speclab_cxx_flags}" CACHE STRING "Flags used by the CXX compiler during all build types." FORCE)
    endif()
endif()

# Where Debian/Ubuntu put `libc++.modules.json` is not fixed across LLVM packagings, so search the
# layouts that exist rather than asserting one.
#
# The compilers above are re-FORCED on every configure while the manifest is cached, so record
# which root the cached manifest came from and re-discover when it moves; otherwise a reconfigure
# with a different SPECLAB_LLVM_ROOT would pair one LLVM's clang++ with another's libc++. An
# explicit -DCMAKE_CXX_STDLIB_MODULES_JSON has no recorded root and is left alone.
if(DEFINED CMAKE_CXX_STDLIB_MODULES_JSON AND NOT DEFINED SPECLAB_STDLIB_MODULES_JSON_ROOT)
    # Pinned by the operator.
elseif(NOT DEFINED CMAKE_CXX_STDLIB_MODULES_JSON
       OR NOT SPECLAB_STDLIB_MODULES_JSON_ROOT STREQUAL "${_speclab_llvm_root}")
    file(GLOB_RECURSE _speclab_libcxx_modules_json
        "${_speclab_llvm_root}/lib/*/libc++.modules.json"
        "${_speclab_llvm_root}/lib/libc++.modules.json"
        "${_speclab_llvm_root}/share/libc++/*/libc++.modules.json")
    list(LENGTH _speclab_libcxx_modules_json _speclab_libcxx_modules_json_count)
    if(_speclab_libcxx_modules_json_count EQUAL 0)
        message(FATAL_ERROR
            "No libc++.modules.json under '${_speclab_llvm_root}'. Install libc++-21-dev from "
            "apt.llvm.org; without that manifest `import std` cannot resolve for libc++.")
    endif()

    # GLOB_RECURSE order is unspecified; sort so the same installation always picks the same file.
    list(SORT _speclab_libcxx_modules_json)
    list(GET _speclab_libcxx_modules_json 0 _speclab_libcxx_modules_json_first)
    if(_speclab_libcxx_modules_json_count GREATER 1)
        message(WARNING
            "Multiple libc++.modules.json under '${_speclab_llvm_root}': "
            "${_speclab_libcxx_modules_json}. Using '${_speclab_libcxx_modules_json_first}'. Set "
            "CMAKE_CXX_STDLIB_MODULES_JSON explicitly to choose a different one.")
    endif()
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
        message(FATAL_ERROR "Required Linux Clang toolchain input is missing: ${_speclab_required_tool}")
    endif()
endforeach()
