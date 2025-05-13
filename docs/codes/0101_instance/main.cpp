#include <vulkan/vulkan.hpp>
#include <vulkan/vulkan_raii.hpp>
#include <GLFW/glfw3.h>

#include <iostream>
#include <vector>
#include <memory>
#include <stdexcept>


class HelloTriangleApplication {
public:
    void run() {
        initWindow();
        initVulkan();
        mainLoop();
        cleanup();
    }

private:
    /////////////////////////////////////////////////////////////////
    /// static values
    static const uint32_t WIDTH = 800;
    static const uint32_t HEIGHT = 600;
    /////////////////////////////////////////////////////////////////

    /////////////////////////////////////////////////////////////////
    /// class member
    GLFWwindow* m_window{ nullptr };
    vk::raii::Context m_context;
    vk::raii::Instance m_instance{ nullptr };
    /////////////////////////////////////////////////////////////////
    
    /////////////////////////////////////////////////////////////////
    /// run()
    void initWindow() {
        // initialize glfw lib
        glfwInit();
        
        // Configure GLFW to not use OpenGL
        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
        // Temporarily disable window resizing to simplify operations
        glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
        
        // Create window
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
    /////////////////////////////////////////////////////////////////

    /////////////////////////////////////////////////////////////////
    /// create instance
    void createInstance(){
        vk::ApplicationInfo applicationInfo( 
            "Hello Triangle",   // pApplicationName
            1,                  // applicationVersion
            "No Engine",        // pEngineName
            1,                  // engineVersion
            VK_API_VERSION_1_1  // apiVersion
        );

        vk::InstanceCreateInfo createInfo( 
            {},                 // vk::InstanceCreateFlags
            &applicationInfo    // vk::ApplicationInfo*
        );

        // std::vector<vk::ExtensionProperties>
        auto extensions = m_context.enumerateInstanceExtensionProperties();
        std::cout << "available extensions:\n";

        for (const auto& extension : extensions) {
            std::cout << '\t' << extension.extensionName << std::endl;
        }

        uint32_t glfwExtensionCount = 0;
        const char** glfwExtensions;
        glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);
        
        std::vector<const char*> requiredExtensions( glfwExtensions, glfwExtensions + glfwExtensionCount );
        requiredExtensions.emplace_back(VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME);

        // special setter
        createInfo.setPEnabledExtensionNames( requiredExtensions );
        createInfo.flags |= vk::InstanceCreateFlagBits::eEnumeratePortabilityKHR;

        m_instance = m_context.createInstance( createInfo );
    }
    /////////////////////////////////////////////////////////////////
};

int main() {
    HelloTriangleApplication app;

    try {
        app.run();
    } catch(const vk::SystemError & err ){
        // use err.code() to check err type
        std::cout << "vk::SystemError: " << err.what() << std::endl;
    } catch (const std::exception & err ){
        std::cout << "std::exception: " << err.what() << std::endl;
    }

    return 0;
}
