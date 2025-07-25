cmake_minimum_required(VERSION 4.0.0)

find_package(Vulkan REQUIRED)

set(SHADER_DIR ${CMAKE_CURRENT_SOURCE_DIR})
set(STAGE_VERT "-fshader-stage=vert")
set(STAGE_FRAG "-fshader-stage=frag")
set(GRAPHICS_VERT_SHADER ${SHADER_DIR}/graphics.vert.glsl)
set(GRAPHICS_FRAG_SHADER ${SHADER_DIR}/graphics.frag.glsl)
set(GRAPHICS_SPIRV_VERT ${SHADER_DIR}/graphics.vert.spv)
set(GRAPHICS_SPIRV_FRAG ${SHADER_DIR}/graphics.frag.spv)

add_custom_command(
        OUTPUT ${GRAPHICS_SPIRV_VERT}
        COMMAND ${Vulkan_GLSLC_EXECUTABLE} ${STAGE_VERT} ${GRAPHICS_VERT_SHADER} -o ${GRAPHICS_SPIRV_VERT}
        COMMENT "Compiling graphics.vert.glsl to graphics.vert.spv"
        DEPENDS ${GRAPHICS_VERT_SHADER}
)

add_custom_command(
        OUTPUT ${GRAPHICS_SPIRV_FRAG}
        COMMAND ${Vulkan_GLSLC_EXECUTABLE} ${STAGE_FRAG} ${GRAPHICS_FRAG_SHADER} -o ${GRAPHICS_SPIRV_FRAG}
        COMMENT "Compiling graphics.frag.glsl to graphics.frag.spv"
        DEPENDS ${GRAPHICS_FRAG_SHADER}
)


add_custom_target(CompileShaders ALL
        DEPENDS ${GRAPHICS_SPIRV_VERT} ${GRAPHICS_SPIRV_FRAG}
)