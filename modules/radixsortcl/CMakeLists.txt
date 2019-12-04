add_subdirectory(ext/clogs)

#--------------------------------------------------------------------
# RadixSortCL Module
ivw_module(RadixSortCL)

#--------------------------------------------------------------------
# Add header files
set(HEADER_FILES
    ${CMAKE_CURRENT_SOURCE_DIR}/processors/radixsortcl.h
)
ivw_group("Header Files" ${HEADER_FILES})

#--------------------------------------------------------------------
# Add source files
set(SOURCE_FILES
    ${CMAKE_CURRENT_SOURCE_DIR}/processors/radixsortcl.cpp 
)
ivw_group("Source Files" ${SOURCE_FILES})


ivw_group("Shader Files" ${SHADER_FILES})

#--------------------------------------------------------------------
# Create module
ivw_create_module(${SOURCE_FILES} ${HEADER_FILES} ${SHADER_FILES})


target_link_libraries(inviwo-module-radixsortcl PUBLIC inviwo::clogs)
