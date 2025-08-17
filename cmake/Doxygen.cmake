# Doxygen configuration for MddLog

find_package(Doxygen QUIET)

if(DOXYGEN_FOUND)
    option(MDDLOG_BUILD_DOCS "Build MddLog documentation" OFF)
    
    if(MDDLOG_BUILD_DOCS)
        set(DOXYGEN_IN ${CMAKE_CURRENT_SOURCE_DIR}/docs/Doxyfile.in)
        set(DOXYGEN_OUT ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile)
        
        if(EXISTS ${DOXYGEN_IN})
            configure_file(${DOXYGEN_IN} ${DOXYGEN_OUT} @ONLY)
            
            add_custom_target(docs ALL
                COMMAND ${DOXYGEN_EXECUTABLE} ${DOXYGEN_OUT}
                WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
                COMMENT "Generating API documentation with Doxygen"
                VERBATIM
            )
        else()
            message(STATUS "Doxygen configuration file not found, skipping documentation generation")
        endif()
    endif()
else()
    message(STATUS "Doxygen not found, documentation will not be built")
endif()