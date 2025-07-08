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
    /// class member
    GLFWwindow* m_window{ nullptr };
    vk::raii::Context m_context;
    vk::raii::Instance m_instance{ nullptr };
    /////////////////////////////////////////////////////////////

    /////////////////////////////////////////////////////////////
    /// run functions
    void initWindow() {
        glfwInit();

        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
        glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

        m_window = glfwCreateWindow(WIDTH, HEIGHT, "Vulkan", nullptr, nullptr);
    }

    void initVulkan() {
        createInstance();
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
    /////////////////////////////////////////////////////////////

    /////////////////////////////////////////////////////////////
    /// create instance
    void createInstance(){
        vk::ApplicationInfo applicationInfo(
            "Hello Vulkan",     // pApplicationName
            1,                  // applicationVersion
            "No Engine",        // pEngineName
            1,                  // engineVersion
            vk::makeApiVersion(0, 1, 4, 0)  // apiVersion
        );
        vk::InstanceCreateInfo createInfo(
            {},                 // vk::InstanceCreateFlags
            &applicationInfo    // vk::ApplicationInfo*
        );

        uint32_t glfwExtensionCount = 0;
        const char **glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

        std::vector<const char*> requiredExtensions( glfwExtensions, glfwExtensions + glfwExtensionCount );
        requiredExtensions.emplace_back(vk::KHRPortabilityEnumerationExtensionName);

        createInfo.setPEnabledExtensionNames( requiredExtensions );
        createInfo.flags |= vk::InstanceCreateFlagBits::eEnumeratePortabilityKHR;

        // std::vector<vk::ExtensionProperties>
        const auto extensions = m_context.enumerateInstanceExtensionProperties();
        std::cout << "available extensions:\n";
        for (const auto& extension : extensions) {
            std::println("\t{}", std::string_view(extension.extensionName));
        }

        m_instance = m_context.createInstance( createInfo );
    }
    /////////////////////////////////////////////////////////////
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
