#--------------------------------------------------------------------
# Inviwo uniformgridcl Module
ivw_module(UniformGridCL)

#--------------------------------------------------------------------
# Add header files
set(HEADER_FILES
	${CMAKE_CURRENT_SOURCE_DIR}/buffermixercl.h
    ${CMAKE_CURRENT_SOURCE_DIR}/minmaxuniformgrid3d.h
    ${CMAKE_CURRENT_SOURCE_DIR}/processors/dynamicvolumedifferenceanalysis.h
    ${CMAKE_CURRENT_SOURCE_DIR}/processors/uniformgrid3dexport.h
    ${CMAKE_CURRENT_SOURCE_DIR}/processors/uniformgrid3dplayerprocessor.h
    ${CMAKE_CURRENT_SOURCE_DIR}/processors/uniformgrid3dsequenceselector.h
    ${CMAKE_CURRENT_SOURCE_DIR}/processors/uniformgrid3dsourceprocessor.h
    ${CMAKE_CURRENT_SOURCE_DIR}/processors/uniformgrid3dvectorsource.h
    ${CMAKE_CURRENT_SOURCE_DIR}/processors/volumeminmaxclprocessor.h
    ${CMAKE_CURRENT_SOURCE_DIR}/processors/volumesequenceplayer.h
    ${CMAKE_CURRENT_SOURCE_DIR}/uniformgrid3d.h
    ${CMAKE_CURRENT_SOURCE_DIR}/uniformgrid3dreader.h
    ${CMAKE_CURRENT_SOURCE_DIR}/uniformgrid3dwriter.h
    ${CMAKE_CURRENT_SOURCE_DIR}/uniformgridclmodule.h
    ${CMAKE_CURRENT_SOURCE_DIR}/uniformgridclmoduledefine.h
)
ivw_group("Header Files" ${HEADER_FILES})

#--------------------------------------------------------------------
# Add source files
set(SOURCE_FILES
	${CMAKE_CURRENT_SOURCE_DIR}/buffermixercl.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/processors/dynamicvolumedifferenceanalysis.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/processors/uniformgrid3dexport.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/processors/uniformgrid3dplayerprocessor.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/processors/uniformgrid3dsequenceselector.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/processors/uniformgrid3dsourceprocessor.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/processors/uniformgrid3dvectorsource.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/processors/volumeminmaxclprocessor.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/processors/volumesequenceplayer.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/uniformgrid3d.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/uniformgrid3dreader.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/uniformgrid3dwriter.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/uniformgridclmodule.cpp
) 
ivw_group("Source Files" ${SOURCE_FILES})


#--------------------------------------------------------------------
# Add shaders
set(CL_FILES
	${CMAKE_CURRENT_SOURCE_DIR}/cl/buffermixer.cl
    ${CMAKE_CURRENT_SOURCE_DIR}/cl/uniformgrid/uniformgrid.cl
    ${CMAKE_CURRENT_SOURCE_DIR}/cl/uniformgrid/volumeminmax.cl
)
source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR}/cl PREFIX "Shader Files" FILES ${CL_FILES})

set(SHADER_FILES
	${CMAKE_CURRENT_SOURCE_DIR}/glsl/volume_mix.frag
)
ivw_group("Shader Files" ${SHADER_FILES})


#--------------------------------------------------------------------
# Add Unittests
#set(TEST_FILES
#    ${CMAKE_CURRENT_SOURCE_DIR}/uniformgridcl/tests/uniformgridcl-test.cpp
#)
#ivw_add_unittest(${TEST_FILES})

#--------------------------------------------------------------------
# Create module
ivw_create_module(${SOURCE_FILES} ${HEADER_FILES} ${SHADER_FILES} ${CL_FILES})

#--------------------------------------------------------------------
# Add shader directory to pack
ivw_add_to_module_pack(${CMAKE_CURRENT_SOURCE_DIR}/cl)
ivw_handle_shader_resources(${CMAKE_CURRENT_SOURCE_DIR}/glsl ${SHADER_FILES})
ivw_add_to_module_pack(${CMAKE_CURRENT_SOURCE_DIR}/workspaces)