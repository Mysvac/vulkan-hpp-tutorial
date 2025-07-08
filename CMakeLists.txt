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

set(CMAKE_CXX_STANDARD 23)

#################################################################################
#### Search for Vulkan package.
# CMake provides FindVulkan support, but requires the VULKAN_SDK environment variable
# Setting by default during Vulkan SDK installation, such as E: \ Vulkan \ 1.4.309.0
find_package(Vulkan REQUIRED)
#################################################################################

#################################################################################
#### add_executable
# Import third-party libraries through vcpkg
find_package(glfw3 CONFIG REQUIRED)
find_package(glm CONFIG REQUIRED)
find_package(Stb REQUIRED)
find_package(tinyobjloader CONFIG REQUIRED)

# Add executable program targets
add_executable(${PROJECT_NAME} src/main.cpp)

target_link_libraries(${PROJECT_NAME} PRIVATE Vulkan::Vulkan )
target_link_libraries(${PROJECT_NAME} PRIVATE glm::glm )
target_link_libraries(${PROJECT_NAME} PRIVATE glfw )
target_include_directories(${PROJECT_NAME} PRIVATE ${Stb_INCLUDE_DIR})
target_link_libraries(${PROJECT_NAME} PRIVATE tinyobjloader::tinyobjloader)
#################################################################################

#################################################################################
#### shader compilation
add_subdirectory(shaders)
#################################################################################
