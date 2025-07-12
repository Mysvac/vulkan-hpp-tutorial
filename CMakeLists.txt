cmake_minimum_required(VERSION 4.0.0)

# 请自行设置模块实验性标准
# set(CMAKE_EXPERIMENTAL_CXX_IMPORT_STD "......")

file(TO_CMAKE_PATH "$ENV{VCPKG_ROOT}" VCPKG_CMAKE_PATH)
set(CMAKE_TOOLCHAIN_FILE "${VCPKG_CMAKE_PATH}/scripts/buildsystems/vcpkg.cmake")

project(HelloCppModule LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_MODULE_STD 1)

include(cmake/VulkanHppModule.cmake)

find_package(glfw3 CONFIG REQUIRED)
find_package(glm CONFIG REQUIRED)
find_package(Stb REQUIRED)
find_package(tinyobjloader CONFIG REQUIRED)

file(GLOB_RECURSE CXX_CPP_FILES "src/*.cpp")
file(GLOB_RECURSE CXX_MODULE_FILES "src/*.cppm" "src/*.ixx")
add_executable(main ${CXX_CPP_FILES})
target_sources(main PRIVATE
    FILE_SET cxx_modules
    TYPE CXX_MODULES
    FILES ${CXX_MODULE_FILES}
)

target_link_libraries(main PRIVATE VulkanHppModule)
target_link_libraries(main PRIVATE glm::glm)
target_link_libraries(main PRIVATE glfw )
target_include_directories(main PRIVATE ${Stb_INCLUDE_DIR})
target_link_libraries(main PRIVATE tinyobjloader::tinyobjloader)

add_subdirectory(shaders)

