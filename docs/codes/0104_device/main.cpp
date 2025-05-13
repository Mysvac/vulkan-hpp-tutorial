#include <vulkan/vulkan.hpp>
#include <vulkan/vulkan_raii.hpp>
#include <GLFW/glfw3.h>

#include <iostream>
#include <vector>
#include <array>
#include <set>
#include <memory>
#include <optional>
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

    static constexpr std::array<const char*,1> validationLayers {
        "VK_LAYER_KHRONOS_validation"
    };

    #ifdef NDEBUG
        static const bool enableValidationLayers = false;
    #else
        static const bool enableValidationLayers = true;
    #endif
    /////////////////////////////////////////////////////////////////

    /////////////////////////////////////////////////////////////////
    /// class member
    GLFWwindow* m_window{ nullptr };
    vk::raii::Context m_context;
    vk::raii::Instance m_instance{ nullptr };
    vk::raii::DebugUtilsMessengerEXT m_debugMessenger{ nullptr };
    vk::raii::PhysicalDevice m_physicalDevice{ nullptr };
    vk::raii::Device m_device{ nullptr };
    vk::raii::Queue m_graphicsQueue{ nullptr };
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
        setupDebugMessenger();
        pickPhysicalDevice();
        createLogicalDevice();
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
    std::vector<const char*> getRequiredExtensions() {
        uint32_t glfwExtensionCount = 0;
        const char** glfwExtensions;
        glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

        std::vector<const char*> extensions(glfwExtensions, glfwExtensions + glfwExtensionCount);

        if (enableValidationLayers) {
            extensions.emplace_back( VK_EXT_DEBUG_UTILS_EXTENSION_NAME );
        }
        extensions.emplace_back( VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME );

        return extensions;
    }

    void createInstance(){
        if (enableValidationLayers && !checkValidationLayerSupport()) {
            throw std::runtime_error("validation layers requested, but not available!");
        }

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

        std::vector<const char*> requiredExtensions = getRequiredExtensions();
        // special setter
        createInfo.setPEnabledExtensionNames( requiredExtensions );
        createInfo.flags |= vk::InstanceCreateFlagBits::eEnumeratePortabilityKHR;

        // vk::DebugUtilsMessengerCreateInfoEXT
        auto debugMessengerCreateInfo = populateDebugMessengerCreateInfo();
        if (enableValidationLayers) {
            createInfo.setPEnabledLayerNames( validationLayers );
            createInfo.pNext = &debugMessengerCreateInfo;
        }

        m_instance = m_context.createInstance( createInfo );
    }
    /////////////////////////////////////////////////////////////////

    /////////////////////////////////////////////////////////////////
    /// validation layer
    static VKAPI_ATTR VkBool32 VKAPI_CALL debugMessageFunc( 
        vk::DebugUtilsMessageSeverityFlagBitsEXT       messageSeverity,
        vk::DebugUtilsMessageTypeFlagsEXT              messageTypes,
        vk::DebugUtilsMessengerCallbackDataEXT const * pCallbackData,
        void * pUserData ) {

        std::cerr << "validation layer: " << pCallbackData->pMessage << std::endl;

        return false;
    }
    bool checkValidationLayerSupport() {
        auto layers = m_context.enumerateInstanceLayerProperties();
        std::set<std::string> t_requiredLayers( validationLayers.begin(), validationLayers.end() );
        for (const auto& layer : layers) {
            t_requiredLayers.erase( layer.layerName );
        }
        return t_requiredLayers.empty();
    }
    vk::DebugUtilsMessengerCreateInfoEXT populateDebugMessengerCreateInfo() {
        vk::DebugUtilsMessageSeverityFlagsEXT severityFlags( 
            vk::DebugUtilsMessageSeverityFlagBitsEXT::eVerbose |
            vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning |
            vk::DebugUtilsMessageSeverityFlagBitsEXT::eError 
        );
        vk::DebugUtilsMessageTypeFlagsEXT    messageTypeFlags( 
            vk::DebugUtilsMessageTypeFlagBitsEXT::eGeneral |         
            vk::DebugUtilsMessageTypeFlagBitsEXT::ePerformance |
            vk::DebugUtilsMessageTypeFlagBitsEXT::eValidation 
        );
        return vk::DebugUtilsMessengerCreateInfoEXT ( {}, severityFlags, messageTypeFlags, &debugMessageFunc );
    }
    void setupDebugMessenger(){
        if (!enableValidationLayers) return;

        auto createInfo = populateDebugMessengerCreateInfo();
        m_debugMessenger = m_instance.createDebugUtilsMessengerEXT( createInfo );
    }
    /////////////////////////////////////////////////////////////////

    /////////////////////////////////////////////////////////////////
    /// physical device
    bool isDeviceSuitable(const vk::raii::PhysicalDevice& physicalDevice) {
        QueueFamilyIndices indices = findQueueFamilies(physicalDevice);

        return indices.isComplete();
    }
    void pickPhysicalDevice() {
        // std::vector<vk::raii::PhysicalDevice>
        auto physicalDevices = m_instance.enumeratePhysicalDevices();
        if(physicalDevices.empty()){
            throw std::runtime_error("failed to find GPUs with Vulkan support!");
        }

        for (const auto& it : physicalDevices) {
            if (isDeviceSuitable(it)) {
                m_physicalDevice = it;
                break;
            }
        }
        if(m_physicalDevice == nullptr){
            throw std::runtime_error("failed to find a suitable GPU!");
        }
    }
    struct QueueFamilyIndices {
        std::optional<uint32_t> graphicsFamily;

        bool isComplete() {
            return graphicsFamily.has_value();
        }
    };
    QueueFamilyIndices findQueueFamilies(const vk::raii::PhysicalDevice& physicalDevice) {
        QueueFamilyIndices indices;

        // std::vector<vk::QueueFamilyProperties>
        auto queueFamilies = physicalDevice.getQueueFamilyProperties();

        for (int i = 0; const auto& queueFamily : queueFamilies) {
            if (queueFamily.queueFlags & vk::QueueFlagBits::eGraphics) {
                indices.graphicsFamily = i;
            }
            if (indices.isComplete()) {
                break;
            }

            ++i;
        }

        return indices;
    }
    /////////////////////////////////////////////////////////////////
    
    /////////////////////////////////////////////////////////////////
    /// logical device
    void createLogicalDevice() {
        QueueFamilyIndices indices = findQueueFamilies( m_physicalDevice );

        vk::DeviceQueueCreateInfo queueCreateInfo(
            {},                             // flags
            indices.graphicsFamily.value(), // queueFamilyIndex
            1                               // queueCount
        );
        float queuePriority = 1.0f;
        queueCreateInfo.pQueuePriorities = &queuePriority;

        vk::PhysicalDeviceFeatures deviceFeatures;

        vk::DeviceCreateInfo createInfo;
        createInfo.setQueueCreateInfos( queueCreateInfo );
        createInfo.pEnabledFeatures = &deviceFeatures;

        if (enableValidationLayers) {
            createInfo.setPEnabledLayerNames( validationLayers );
        }

        m_device = m_physicalDevice.createDevice( createInfo );
        m_graphicsQueue = m_device.getQueue( indices.graphicsFamily.value(), 0 );
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
