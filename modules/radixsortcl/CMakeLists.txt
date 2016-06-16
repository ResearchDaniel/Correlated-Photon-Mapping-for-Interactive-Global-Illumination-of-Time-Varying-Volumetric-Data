#--------------------------------------------------------------------
# RadixSortCL Module
ivw_module(RadixSortCL)

#--------------------------------------------------------------------
# Add header files
set(HEADER_FILES
    ${CMAKE_CURRENT_SOURCE_DIR}/clogs/clogs.h
    ${CMAKE_CURRENT_SOURCE_DIR}/clogs/core.h
    ${CMAKE_CURRENT_SOURCE_DIR}/clogs/platform.h
    ${CMAKE_CURRENT_SOURCE_DIR}/clogs/radixsort.h
    ${CMAKE_CURRENT_SOURCE_DIR}/clogs/reduce.h
    ${CMAKE_CURRENT_SOURCE_DIR}/clogs/scan.h
    ${CMAKE_CURRENT_SOURCE_DIR}/clogs/tune.h
    ${CMAKE_CURRENT_SOURCE_DIR}/processors/radixsortcl.h
)
ivw_group("Header Files" ${HEADER_FILES})

#--------------------------------------------------------------------
# Add source files
set(SOURCE_FILES
    ${CMAKE_CURRENT_SOURCE_DIR}/processors/radixsortcl.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/cache.cpp  
    ${CMAKE_CURRENT_SOURCE_DIR}/src/cache_types.cpp  
    ${CMAKE_CURRENT_SOURCE_DIR}/src/core.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/parameters.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/radixsort.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/reduce.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/scan.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/sqlite3.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/tune.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/utils.cpp    
)
ivw_group("Source Files" ${SOURCE_FILES})

#--------------------------------------------------------------------
# Add OpenCL files
set(SHADER_FILES
    ${CMAKE_CURRENT_SOURCE_DIR}/kernels/radixsort.cl
	${CMAKE_CURRENT_SOURCE_DIR}/kernels/reduce.cl
	${CMAKE_CURRENT_SOURCE_DIR}/kernels/scan.cl
)
ivw_group("Shader Files" ${SHADER_FILES})

ivw_include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/
)
find_package(Boost 1.55.0 REQUIRED)
if(Boost_FOUND)
ivw_include_directories(
    ${Boost_INCLUDE_DIRS}
)
endif(Boost_FOUND)
#--------------------------------------------------------------------
# Add definition
ivw_add_definition(__CL_ENABLE_EXCEPTIONS)
ivw_add_definition(HAVE_FUNCTIONAL)
ivw_add_definition(HAVE_RANDOM)
if(WIN32)
	ivw_add_definition(_VARIADIC_MAX=10)
	ivw_add_definition(UNICODE)
	ivw_add_definition(CLOGS_FS_WINDOWS)
	add_dependency_libs_to_module(shlwapi.lib)
else()
	ivw_add_definition(CLOGS_FS_WINDOWS)
endif(WIN32)

#--------------------------------------------------------------------
# Create module
ivw_create_module(${SOURCE_FILES} ${HEADER_FILES} ${SHADER_FILES})

#--------------------------------------------------------------------
# Add kernel directory to pack
ivw_add_to_module_pack(${CMAKE_CURRENT_SOURCE_DIR}/kernels)