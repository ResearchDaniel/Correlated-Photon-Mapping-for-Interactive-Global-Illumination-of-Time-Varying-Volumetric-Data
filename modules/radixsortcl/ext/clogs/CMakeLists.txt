#--------------------------------------------------------------------
# clogs
project(clogs)

#--------------------------------------------------------------------
# Add header files
set(HEADER_FILES
    ${CMAKE_CURRENT_SOURCE_DIR}/clogs.h
    ${CMAKE_CURRENT_SOURCE_DIR}/core.h
    ${CMAKE_CURRENT_SOURCE_DIR}/platform.h
    ${CMAKE_CURRENT_SOURCE_DIR}/radixsort.h
    ${CMAKE_CURRENT_SOURCE_DIR}/reduce.h
    ${CMAKE_CURRENT_SOURCE_DIR}/scan.h
    ${CMAKE_CURRENT_SOURCE_DIR}/tune.h
)
ivw_group("Header Files" ${HEADER_FILES})

#--------------------------------------------------------------------
# Add source files
set(SOURCE_FILES
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
	# Private header files
	${CMAKE_CURRENT_SOURCE_DIR}/src/cache.h  
    ${CMAKE_CURRENT_SOURCE_DIR}/src/cache_types.h  
    ${CMAKE_CURRENT_SOURCE_DIR}/src/clhpp11.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/parameters.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/radixsort.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/reduce.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/scan.h
	${CMAKE_CURRENT_SOURCE_DIR}/src/sqlite3.h
	${CMAKE_CURRENT_SOURCE_DIR}/src/tr1_functional.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/tr1_random.h
	${CMAKE_CURRENT_SOURCE_DIR}/src/tune.h  
	${CMAKE_CURRENT_SOURCE_DIR}/src/utils.h  
	
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

find_package(Boost 1.55 REQUIRED)

#--------------------------------------------------------------------
# Create lib
add_library(clogs ${SOURCE_FILES} ${HEADER_FILES} ${SHADER_FILES})
add_library(inviwo::clogs ALIAS clogs)

#--------------------------------------------------------------------
# Add definitions
target_compile_definitions(clogs PUBLIC
							HAVE_FUNCTIONAL
							HAVE_RANDOM)


if(BUILD_SHARED_LIBS)
	target_compile_definitions(clogs PRIVATE
							CLOGS_DLL_DO_EXPORT)
else()
	target_compile_definitions(clogs PRIVATE
							CLOGS_DLL_DO_STATIC)
endif()

if(WIN32)
	target_compile_definitions(clogs PUBLIC
								_VARIADIC_MAX=32
								UNICODE
								CLOGS_FS_WINDOWS)
	target_link_libraries(clogs PRIVATE shlwapi.lib)
else()
	target_compile_definitions(clogs PUBLIC
								CLOGS_FS_UNIX)
endif(WIN32)

target_link_libraries(clogs PRIVATE inviwo-module-opencl Boost::boost)

#--------------------------------------------------------------------
# Add includes
target_include_directories(clogs PUBLIC 
    ${CMAKE_CURRENT_SOURCE_DIR}/..
)

#--------------------------------------------------------------------
# Supress warnings
ivw_suppress_compiler_warnings(clogs)

#--------------------------------------------------------------------
# Creates VS folder structure
ivw_folder(clogs ext)
source_group("CMake Files" FILES ${CMAKE_CURRENT_SOURCE_DIR}/CMakeLists.txt)

#--------------------------------------------------------------------
# Add to package
ivw_default_install_targets(clogs)
# Add kernel directory to pack
ivw_add_to_module_pack(${CMAKE_CURRENT_SOURCE_DIR}/kernels)

