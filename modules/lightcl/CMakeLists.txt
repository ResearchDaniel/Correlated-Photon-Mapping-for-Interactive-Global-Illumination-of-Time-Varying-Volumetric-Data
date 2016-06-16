#--------------------------------------------------------------------
# Inviwo LightCL Module
ivw_module(LightCL)

#--------------------------------------------------------------------
# Add header files
set(HEADER_FILES
    #${CMAKE_CURRENT_SOURCE_DIR}/lightclprocessor.h
    ${CMAKE_CURRENT_SOURCE_DIR}/convexhull2d.h
    ${CMAKE_CURRENT_SOURCE_DIR}/directionallightsamplercl.h
    ${CMAKE_CURRENT_SOURCE_DIR}/lightsample.h
    ${CMAKE_CURRENT_SOURCE_DIR}/lightsamplemeshintersectioncl.h
    ${CMAKE_CURRENT_SOURCE_DIR}/lightsourcesamplercl.h
    ${CMAKE_CURRENT_SOURCE_DIR}/lightsourcescl.h
    ${CMAKE_CURRENT_SOURCE_DIR}/orientedboundingbox2d.h
    ${CMAKE_CURRENT_SOURCE_DIR}/pointplaneprojection.h
    ${CMAKE_CURRENT_SOURCE_DIR}/processors/directionallightsamplerclprocessor.h
    ${CMAKE_CURRENT_SOURCE_DIR}/sample.h
    ${CMAKE_CURRENT_SOURCE_DIR}/samplegenerator2dcl.h
)
ivw_group("Header Files" ${HEADER_FILES})

#--------------------------------------------------------------------
# Add source files
set(SOURCE_FILES
    #${CMAKE_CURRENT_SOURCE_DIR}/lightclprocessor.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/convexhull2d.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/directionallightsamplercl.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/lightsample.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/lightsamplemeshintersectioncl.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/lightsourcesamplercl.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/lightsourcescl.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/orientedboundingbox2d.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/pointplaneprojection.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/processors/directionallightsamplerclprocessor.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/samplegenerator2dcl.cpp
)
ivw_group("Source Files" ${SOURCE_FILES})


#--------------------------------------------------------------------
# Add shaders
set(SHADER_FILES
    ${CMAKE_CURRENT_SOURCE_DIR}/cl/directionallightsampler.cl
    ${CMAKE_CURRENT_SOURCE_DIR}/cl/datastructures/lightsample.cl
    ${CMAKE_CURRENT_SOURCE_DIR}/cl/intersection/lightsamplemeshintersection.cl
)
ivw_group("Shader Files" ${SHADER_FILES})


#--------------------------------------------------------------------
# Add Unittests
#set(TEST_FILES
#    ${CMAKE_CURRENT_SOURCE_DIR}/tests/lightcl-test.cpp
#)
#ivw_add_unittest(${TEST_FILES})

#--------------------------------------------------------------------
# Create module
ivw_create_module(${SOURCE_FILES} ${HEADER_FILES} ${SHADER_FILES})

#--------------------------------------------------------------------
# Add shader directory to pack
ivw_add_to_module_pack(${CMAKE_CURRENT_SOURCE_DIR}/cl)