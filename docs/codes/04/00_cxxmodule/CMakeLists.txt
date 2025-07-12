cmake_minimum_required(VERSION 4.0.0)

# 请自行设置模块实验性标准
# set(CMAKE_EXPERIMENTAL_CXX_IMPORT_STD "......")

project(HelloCppModule LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_MODULE_STD 1)

include(cmake/VulkanHppModule.cmake)

add_executable(main src/main.cpp)

target_link_libraries(main PRIVATE VulkanHppModule)

