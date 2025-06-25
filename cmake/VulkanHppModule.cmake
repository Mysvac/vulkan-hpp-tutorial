# VulkanHppModule.cmake
if(NOT TARGET VulkanHppModule)

    find_package(Vulkan REQUIRED)
    message("Vulkan SDK version : ${Vulkan_VERSION}")
    if(${Vulkan_VERSION} VERSION_LESS "1.4.0")
        message(FATAL_ERROR "Vulkan SDK too old (required ≥ 1.4.0)")
    endif()

    add_library( VulkanHppModule )

    target_sources(VulkanHppModule PUBLIC
            FILE_SET CXX_MODULES
            BASE_DIRS ${Vulkan_INCLUDE_DIR}
            FILES ${Vulkan_INCLUDE_DIR}/vulkan/vulkan.cppm
    )
    target_link_libraries( VulkanHppModule PRIVATE Vulkan::Vulkan )
    target_link_libraries( VulkanHppModule PUBLIC Vulkan::Headers )

    target_compile_definitions(VulkanHppModule PUBLIC
            VULKAN_HPP_DISPATCH_LOADER_DYNAMIC=1
    )
endif()
