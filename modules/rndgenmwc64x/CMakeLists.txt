#--------------------------------------------------------------------
# MWC64X Random Generator Module
ivw_module(RndGenMWC64X)

#--------------------------------------------------------------------
# Add header files
set(HEADER_FILES
    ${CMAKE_CURRENT_SOURCE_DIR}/mwc64xrandomnumbergenerator.h
    ${CMAKE_CURRENT_SOURCE_DIR}/mwc64xseedgenerator.h
    ${CMAKE_CURRENT_SOURCE_DIR}/processors/randomnumbergenerator2dcl.h 
    ${CMAKE_CURRENT_SOURCE_DIR}/processors/randomnumbergeneratorcl.h 
    ${CMAKE_CURRENT_SOURCE_DIR}/rndgenmwc64xmodule.h 
)
ivw_group("Header Files" ${HEADER_FILES})

#--------------------------------------------------------------------
# Add source files
set(SOURCE_FILES
    ${CMAKE_CURRENT_SOURCE_DIR}/mwc64xrandomnumbergenerator.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/mwc64xseedgenerator.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/processors/randomnumbergenerator2dcl.cpp 
    ${CMAKE_CURRENT_SOURCE_DIR}/processors/randomnumbergeneratorcl.cpp 
    ${CMAKE_CURRENT_SOURCE_DIR}/rndgenmwc64xmodule.cpp
)
ivw_group("Source Files" ${SOURCE_FILES})

#--------------------------------------------------------------------
# Add OpenCL files
set(SHADER_FILES
    ${CMAKE_CURRENT_SOURCE_DIR}/cl/random.cl
    ${CMAKE_CURRENT_SOURCE_DIR}/cl/skip_mwc.cl
    ${CMAKE_CURRENT_SOURCE_DIR}/cl/randomnumbergenerator.cl
    ${CMAKE_CURRENT_SOURCE_DIR}/cl/randstategen.cl
)
ivw_group("Shader Files" ${SHADER_FILES})

#--------------------------------------------------------------------
# Create module
ivw_create_module(${SOURCE_FILES} ${HEADER_FILES} ${SHADER_FILES})

#--------------------------------------------------------------------
# Add shader directory to pack
ivw_add_to_module_pack(${CMAKE_CURRENT_SOURCE_DIR}/cl)


