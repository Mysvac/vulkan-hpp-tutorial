#include <iostream>
#include <print>
#include <stdexcept>

#include <vulkan/vulkan.hpp>
#include <vulkan/vulkan_raii.hpp>
#include <GLFW/glfw3.h>



constexpr uint32_t WIDTH = 800;
constexpr uint32_t HEIGHT = 600;



class HelloTriangleApplication {
public:
    void run() {
        initWindow();
        initVulkan();
        mainLoop();
        cleanup();
    }

private:
    /////////////////////////////////////////////////////////////
    //// class member
    GLFWwindow* m_window{ nullptr };
    /////////////////////////////////////////////////////////////

    void initWindow() {
        glfwInit();

        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
        glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

        m_window = glfwCreateWindow(WIDTH, HEIGHT, "Vulkan", nullptr, nullptr);
    }

    void initVulkan() {

    }

    void mainLoop() {
        while (!glfwWindowShouldClose( m_window )) {
            glfwPollEvents();
        }
    }

    void cleanup() {
        glfwDestroyWindow( m_window );
        glfwTerminate();
    }
};

int main() {
    try {
        HelloTriangleApplication app;
        app.run();
    } catch(const vk::SystemError& err ) {
        // use err.code() to check err type
        std::println( std::cerr, "vk::SystemError - code: {} ",err.code().message());
        std::println( std::cerr, "vk::SystemError - what: {}",err.what());
    } catch (const std::exception& err ) {
        std::println( std::cerr, "std::exception: {}", err.what());
    }
}
