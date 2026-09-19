# Coverage and sanitizers have different requirements for consumers:
#
# - Coverage: `--coverage -O0 -g` is how SpecLab itself is built, so it goes on `options_target`
#   (speclab_options, linked privately). Only the link flag is required downstream, because an
#   instrumented libspeclab.a references __gcov_* symbols. It goes on `library_target`'s exported
#   INTERFACE.
#
# - Sanitizers: -fsanitize=... is required downstream for *both* compiling and linking, so it is
#   PUBLIC on `library_target`. That covers SpecLab's own sources, and consumers in the build tree
#   and through find_package(speclab). The reason is that SpecLab ships C++ named modules. A
#   consumer that does `import speclab;` compiles inline and template code from SpecLab's module
#   interfaces in its own translation unit, as those interfaces were built, with sanitizer checks
#   included. When the importer is compiled without the same -fsanitize=..., GCC 16 fails with an
#   internal compiler error (`in expand_UBSAN_NULL, at internal-fn.cc`). MduX's ASan+UBSan CI job
#   hit this once the flags stopped leaking through a PUBLIC speclab_options. Carrying the flag also
#   keeps a consumer's spec code instrumented in a sanitizer build, which is the point of such a
#   build.
function(enable_sanitizers options_target library_target)

  if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU" OR CMAKE_CXX_COMPILER_ID MATCHES ".*Clang")
    option(ENABLE_COVERAGE "Enable coverage reporting for gcc/clang" OFF)

    if(ENABLE_COVERAGE)
      target_compile_options(${options_target} INTERFACE --coverage -O0 -g)
      target_link_options(${library_target} INTERFACE --coverage)
    endif()

    set(SANITIZERS "")

    option(ENABLE_SANITIZER_ADDRESS "Enable address sanitizer" OFF)
    if(ENABLE_SANITIZER_ADDRESS)
      list(APPEND SANITIZERS "address")
    endif()

    option(ENABLE_SANITIZER_LEAK "Enable leak sanitizer" OFF)
    if(ENABLE_SANITIZER_LEAK)
      list(APPEND SANITIZERS "leak")
    endif()

    option(ENABLE_SANITIZER_UNDEFINED_BEHAVIOR "Enable undefined behavior sanitizer" OFF)
    if(ENABLE_SANITIZER_UNDEFINED_BEHAVIOR)
      list(APPEND SANITIZERS "undefined")
    endif()

    option(ENABLE_SANITIZER_THREAD "Enable thread sanitizer" OFF)
    if(ENABLE_SANITIZER_THREAD)
      if("address" IN_LIST SANITIZERS OR "leak" IN_LIST SANITIZERS)
        message(WARNING "Thread sanitizer does not work with Address and Leak sanitizer enabled")
      else()
        list(APPEND SANITIZERS "thread")
      endif()
    endif()

    option(ENABLE_SANITIZER_MEMORY "Enable memory sanitizer" OFF)
    if(ENABLE_SANITIZER_MEMORY AND CMAKE_CXX_COMPILER_ID MATCHES ".*Clang")
      message(WARNING "Memory sanitizer requires all the code (including libc++) to be MSan-instrumented otherwise it reports false positives")
      if("address" IN_LIST SANITIZERS
         OR "thread" IN_LIST SANITIZERS
         OR "leak" IN_LIST SANITIZERS)
        message(WARNING "Memory sanitizer does not work with Address, Thread and Leak sanitizer enabled")
      else()
        list(APPEND SANITIZERS "memory")
      endif()
    endif()

    list(
      JOIN
      SANITIZERS
      ","
      LIST_OF_SANITIZERS)

  endif()

  if(LIST_OF_SANITIZERS)
    if(NOT
       "${LIST_OF_SANITIZERS}"
       STREQUAL
       "")
      target_compile_options(${library_target} PUBLIC -fsanitize=${LIST_OF_SANITIZERS})
      target_link_options(${library_target} INTERFACE -fsanitize=${LIST_OF_SANITIZERS})
    endif()
  endif()

endfunction()
