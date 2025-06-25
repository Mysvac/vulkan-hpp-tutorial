cmake_minimum_required(VERSION 4.0.0)

#################################################################################
#### Set up the vcpkg toolchain, which must be done before the project() function.
# You can also set it up by default through the CMakePresets.json file
# requires the VCPKG_ROOT environment variable
# The TO_CMAKE_PATH here is used to convert back slashes to forward slashes
file(TO_CMAKE_PATH "$ENV{VCPKG_ROOT}" VCPKG_CMAKE_PATH)
set(CMAKE_TOOLCHAIN_FILE "${VCPKG_CMAKE_PATH}/scripts/buildsystems/vcpkg.cmake")
#################################################################################

project(HelloVulkan LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)


include(cmake/VulkanHppModule.cmake)

find_package(glfw3 CONFIG REQUIRED)
find_package(glm CONFIG REQUIRED)
find_package(Stb REQUIRED)
find_package(tinyobjloader CONFIG REQUIRED)

file(GLOB_RECURSE MODULE_SOURCES src/*.cppm)

# Add executable program targets
add_executable( main src/main.cpp )
target_sources( main PRIVATE
        FILE_SET CXX_MODULES
        FILES ${MODULE_SOURCES}
)

target_link_libraries(main PRIVATE VulkanHppModule )
target_link_libraries(main PRIVATE glm::glm)
target_link_libraries(main PRIVATE glfw )
target_include_directories(main PRIVATE ${Stb_INCLUDE_DIR})
target_link_libraries(main PRIVATE tinyobjloader::tinyobjloader)

add_subdirectory(shaders)

