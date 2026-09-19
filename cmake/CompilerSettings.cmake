# CompilerSettings.cmake
# Handle compiler-specific configuration and generator optimizations

# Configure Ninja generator for optimal builds
if(CMAKE_GENERATOR STREQUAL "Ninja")
  # Enable colored output for Ninja
  set(CMAKE_COLOR_MAKEFILE ON)
  set(CMAKE_COLOR_DIAGNOSTICS ON)
  # Use response files for long command lines to handle very long command lines
  set(CMAKE_NINJA_FORCE_RESPONSE_FILE ON)
  message(STATUS "Using Ninja build system with optimizations")
endif()

# MSVC specific compiler settings
function(configure_msvc_settings target_name)
  if(MSVC)
    target_compile_options(${target_name} INTERFACE 
      /EHsc  # Enable exception handling
      /W4    # Warning level 4
      /wd4530  # Disable C4530 warning about exception handling
    )
    message(STATUS "Applied MSVC-specific compiler settings to ${target_name}")
  endif()
endfunction()

# Function to apply compiler-specific settings and definitions to a target
function(configure_compiler_settings target_name)
  # Apply MSVC settings if applicable
  configure_msvc_settings(${target_name})
  
  # Medical device specific compile options
  target_compile_options(${target_name} INTERFACE
    $<$<CXX_COMPILER_ID:MSVC>:/permissive->
    $<$<CXX_COMPILER_ID:GNU>:-fconcepts-diagnostics-depth=2>
  )

  # Platform-specific definitions
  target_compile_definitions(${target_name} INTERFACE
    $<$<PLATFORM_ID:Windows>:SPECLAB_PLATFORM_WINDOWS;WIN32_LEAN_AND_MEAN;NOMINMAX>
    $<$<PLATFORM_ID:Linux>:SPECLAB_PLATFORM_LINUX>
  )
endfunction()

# SpecLab version and compliance definitions. Applied to SpecLab's own translation units only
# (speclab_options is linked privately); SPECLAB_ prefixed so they cannot collide with a
# consumer's macros.
function(configure_speclab_version target_name version_major version_minor version_patch)
  target_compile_definitions(${target_name} INTERFACE
    SPECLAB_VERSION_MAJOR=${version_major}
    SPECLAB_VERSION_MINOR=${version_minor}
    SPECLAB_VERSION_PATCH=${version_patch}
    SPECLAB_MEDICAL_DEVICE_COMPLIANCE=1
  )
endfunction()
