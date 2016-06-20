#--------------------------------------------------------------------
# Progressive Photon Mapping Module
ivw_module(ProgressivePhotonMapping)

#--------------------------------------------------------------------
# Add header files
set(HEADER_FILES
    ${CMAKE_CURRENT_SOURCE_DIR}/photondata.h
    ${CMAKE_CURRENT_SOURCE_DIR}/photonrecomputationdetector.h
    ${CMAKE_CURRENT_SOURCE_DIR}/photontracercl.h
    ${CMAKE_CURRENT_SOURCE_DIR}/processor/photontolightvolumeprocessorcl.h
    ${CMAKE_CURRENT_SOURCE_DIR}/processor/progressivephotontracercl.h
    ${CMAKE_CURRENT_SOURCE_DIR}/progressivephotonmappingmodule.h
)
ivw_group("Header Files" ${HEADER_FILES})

#--------------------------------------------------------------------
# Add source files
set(SOURCE_FILES
    ${CMAKE_CURRENT_SOURCE_DIR}/photondata.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/photonrecomputationdetector.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/photontracercl.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/processor/photontolightvolumeprocessorcl.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/processor/progressivephotontracercl.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/progressivephotonmappingmodule.cpp
)
ivw_group("Source Files" ${SOURCE_FILES})

#--------------------------------------------------------------------
# Add OpenCL files
set(SHADER_FILES
	${CMAKE_CURRENT_SOURCE_DIR}/cl/densityestimationkernel.cl
    ${CMAKE_CURRENT_SOURCE_DIR}/cl/hashlightsample.cl
	${CMAKE_CURRENT_SOURCE_DIR}/cl/indextobuffer.cl
	${CMAKE_CURRENT_SOURCE_DIR}/cl/photon.cl
	${CMAKE_CURRENT_SOURCE_DIR}/cl/photonrecomputationdetector.cl
    ${CMAKE_CURRENT_SOURCE_DIR}/cl/photonstolightvolume.cl
    ${CMAKE_CURRENT_SOURCE_DIR}/cl/photontracer.cl
    ${CMAKE_CURRENT_SOURCE_DIR}/cl/threshold.cl
)
ivw_group("Shader Files" ${SHADER_FILES})

#--------------------------------------------------------------------
# Add definition
ivw_add_definition(__CL_ENABLE_EXCEPTIONS)

#--------------------------------------------------------------------
# Create module
ivw_create_module(${SOURCE_FILES} ${HEADER_FILES} ${SHADER_FILES})

#--------------------------------------------------------------------
# Add shader and kernel directory to pack
ivw_add_to_module_pack(${CMAKE_CURRENT_SOURCE_DIR}/cl)
ivw_add_to_module_pack(${CMAKE_CURRENT_SOURCE_DIR}/workspaces)


