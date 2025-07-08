// main.cpp
#include <iostream>
#include <string_view>
#include <print>

#include <vulkan/vulkan.hpp>
#include <vulkan/vulkan_raii.hpp>

#include <GLFW/glfw3.h>

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/vec4.hpp>
#include <glm/mat4x4.hpp>

int main() {
    // vulkan test
    const vk::raii::Context context;
    const auto extensions = context.enumerateInstanceExtensionProperties();
    std::cout << "vulkan available extensions:" << std::endl;
    for (const auto& extension : extensions) {
        std::println("{}", std::string_view( extension.extensionName ));
    }

    vk::raii::Semaphore semaphore( nullptr );

    // glm test
    constexpr glm::mat4 matrix(1.0f);
    constexpr glm::vec4 vec(1.0f, 2.0f, 3.0f, 4.0f);
    constexpr glm::vec4 test = matrix * vec;
    std::println("{} {} {} {}", test.x, test.y, test.z, test.w);

    // glfw test
    glfwInit();
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    GLFWwindow* window = glfwCreateWindow(800, 600, "Vulkan window", nullptr, nullptr);
    while(!glfwWindowShouldClose(window)) {
        glfwPollEvents();
    }
    glfwDestroyWindow(window);
    glfwTerminate();
}