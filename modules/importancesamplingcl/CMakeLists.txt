#--------------------------------------------------------------------
# Inviwo ImportanceSamplingCL Module
ivw_module(ImportanceSamplingCL)

#--------------------------------------------------------------------
# Add header files
set(HEADER_FILES
    ${CMAKE_CURRENT_SOURCE_DIR}/importancesamplingclmodule.h
    ${CMAKE_CURRENT_SOURCE_DIR}/importanceuniformgrid3d.h
    ${CMAKE_CURRENT_SOURCE_DIR}/minmaxuniformgrid3dimportancecl.h
    ${CMAKE_CURRENT_SOURCE_DIR}/processors/minmaxuniformgrid3dimportanceclprocessor.h
    ${CMAKE_CURRENT_SOURCE_DIR}/processors/uniformsamplegenerator2dprocessorcl.h
    ${CMAKE_CURRENT_SOURCE_DIR}/uniformsamplegenerator2dcl.h
)
ivw_group("Header Files" ${HEADER_FILES})

#--------------------------------------------------------------------
# Add source files
set(SOURCE_FILES
    ${CMAKE_CURRENT_SOURCE_DIR}/importancesamplingclmodule.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/minmaxuniformgrid3dimportancecl.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/processors/minmaxuniformgrid3dimportanceclprocessor.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/processors/uniformsamplegenerator2dprocessorcl.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/uniformsamplegenerator2dcl.cpp
)
ivw_group("Source Files" ${SOURCE_FILES})


#--------------------------------------------------------------------
# Add shaders
set(SHADER_FILES
	${CMAKE_CURRENT_SOURCE_DIR}/cl/datastructures/ray.cl
    ${CMAKE_CURRENT_SOURCE_DIR}/cl/intersection/lightsamplemeshintersection.cl
    ${CMAKE_CURRENT_SOURCE_DIR}/cl/minmaxuniformgrid3dimportance.cl
    ${CMAKE_CURRENT_SOURCE_DIR}/cl/light/light.cl
    ${CMAKE_CURRENT_SOURCE_DIR}/cl/light/lightsampling.cl
    ${CMAKE_CURRENT_SOURCE_DIR}/cl/uniformsamplegenerator2d.cl
)
ivw_group("Shader Files" ${SHADER_FILES})

#--------------------------------------------------------------------
# Add definition
ivw_add_definition(__CL_ENABLE_EXCEPTIONS)

#--------------------------------------------------------------------
# Disable Pre-compiled headers as file is not generated properly on CMake 3.1 ->
ivw_set_pch_disabled_for_module()

#--------------------------------------------------------------------
# Add Unittests
#set(TEST_FILES
#    ${CMAKE_CURRENT_SOURCE_DIR}/tests/importancesamplingcl-test.cpp
#)
#ivw_add_unittest(${TEST_FILES})

#--------------------------------------------------------------------
# Create module
ivw_create_module(${SOURCE_FILES} ${HEADER_FILES} ${SHADER_FILES})

#--------------------------------------------------------------------
# Add shader directory to pack
ivw_add_to_module_pack(${CMAKE_CURRENT_SOURCE_DIR}/cl)