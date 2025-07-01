cmake_minimum_required(VERSION 3.30)

find_package(Vulkan REQUIRED)

set(SHADER_DIR ${CMAKE_CURRENT_SOURCE_DIR})

set(VERT_SHADER ${SHADER_DIR}/shader.vert)
set(FRAG_SHADER ${SHADER_DIR}/shader.frag)
set(VERT_SECOND ${SHADER_DIR}/second.vert)
set(FRAG_SECOND ${SHADER_DIR}/second.frag)


set(SPIRV_VERT ${SHADER_DIR}/vert.spv)
set(SPIRV_FRAG ${SHADER_DIR}/frag.spv)
set(SPIRV_SECOND_VERT ${SHADER_DIR}/second_vert.spv)
set(SPIRV_SECOND_FRAG ${SHADER_DIR}/second_frag.spv)

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
        OUTPUT ${SPIRV_SECOND_VERT}
        COMMAND ${Vulkan_GLSLC_EXECUTABLE} ${VERT_SECOND} -o ${SPIRV_SECOND_VERT}
        COMMENT "Compiling shader.vert to vert.spv"
        DEPENDS ${VERT_SECOND}
)

add_custom_command(
        OUTPUT ${SPIRV_SECOND_FRAG}
        COMMAND ${Vulkan_GLSLC_EXECUTABLE} ${FRAG_SECOND} -o ${SPIRV_SECOND_FRAG}
        COMMENT "Compiling shader.frag to frag.spv"
        DEPENDS ${FRAG_SECOND}
)

add_custom_target(CompileShaders ALL
        DEPENDS ${SPIRV_VERT} ${SPIRV_FRAG} ${SPIRV_SECOND_VERT} ${SPIRV_SECOND_FRAG}
)

