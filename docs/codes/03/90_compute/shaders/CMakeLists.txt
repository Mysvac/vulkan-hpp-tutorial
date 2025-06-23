cmake_minimum_required(VERSION 3.30)

find_package(Vulkan REQUIRED)

set(SHADER_DIR ${CMAKE_CURRENT_SOURCE_DIR})
set(VERT_SHADER ${SHADER_DIR}/shader.vert)
set(FRAG_SHADER ${SHADER_DIR}/shader.frag)
set(COMP_SHADER ${SHADER_DIR}/shader.comp)
set(SPIRV_VERT ${SHADER_DIR}/vert.spv)
set(SPIRV_FRAG ${SHADER_DIR}/frag.spv)
set(SPIRV_COMP ${SHADER_DIR}/comp.spv)

add_custom_command(
    OUTPUT ${SPIRV_VERT}
    COMMAND ${Vulkan_GLSLC_EXECUTABLE} ${VERT_SHADER} -o ${SPIRV_VERT}
    COMMENT "Compiling shader.vert to vert.spv"
    DEPENDS ${VERT_SHADER}
)

add_custom_command(
    OUTPUT ${SPIRV_FRAG}
    COMMAND ${Vulkan_GLSLC_EXECUTABLE} ${FRAG_SHADER} -o ${SPIRV_FRAG}
    COMMENT "Compiling shader.frag to frag.spv"
    DEPENDS ${FRAG_SHADER}
)

add_custom_command(
    OUTPUT ${SPIRV_COMP}
    COMMAND ${Vulkan_GLSLC_EXECUTABLE} ${COMP_SHADER} -o ${SPIRV_COMP}
    COMMENT "Compiling shader.comp to comp.spv"
    DEPENDS ${COMP_SHADER}
)

add_custom_target(CompileShaders ALL
    DEPENDS ${SPIRV_VERT} ${SPIRV_FRAG} ${SPIRV_COMP}
)
