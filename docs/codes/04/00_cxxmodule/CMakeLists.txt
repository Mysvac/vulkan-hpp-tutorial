cmake_minimum_required(VERSION 4.0.0)


project(HelloCppModule LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)

include(cmake/VulkanHppModule.cmake)

add_executable(main src/main.cpp)

target_link_libraries(main PRIVATE VulkanHppModule)

