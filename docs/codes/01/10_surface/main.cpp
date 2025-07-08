#include <iostream>
#include <print>
#include <set>
#include <stdexcept>
#include <optional>

#include <vulkan/vulkan.hpp>
#include <vulkan/vulkan_raii.hpp>
#include <GLFW/glfw3.h>



constexpr uint32_t WIDTH = 800;
constexpr uint32_t HEIGHT = 600;

constexpr std::array<const char*,1> REQUIRED_LAYERS {
    "VK_LAYER_KHRONOS_validation"
};

#ifdef NDEBUG
constexpr bool ENABLE_VALIDATION_LAYER = false;
#else
constexpr bool ENABLE_VALIDATION_LAYER = true;
#endif

struct QueueFamilyIndices {
    std::optional<uint32_t> graphicsFamily;
    std::optional<uint32_t> presentFamily;

    bool isComplete() const {
        return graphicsFamily.has_value() && presentFamily.has_value();
    }
};

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
    vk::raii::DebugUtilsMessengerEXT m_debugMessenger{ nullptr };
    vk::raii::SurfaceKHR m_surface{ nullptr };
    vk::raii::PhysicalDevice m_physicalDevice{ nullptr };
    vk::raii::Device m_device{ nullptr };
    vk::raii::Queue m_graphicsQueue{ nullptr };
    vk::raii::Queue m_presentQueue{ nullptr };
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
        setupDebugMessenger();
        createSurface();
        selectPhysicalDevice();
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
    /////////////////////////////////////////////////////////////

    /////////////////////////////////////////////////////////////
    /// create instance , validation layers and surface
    static VKAPI_ATTR uint32_t VKAPI_CALL debugMessageFunc(
        vk::DebugUtilsMessageSeverityFlagBitsEXT       messageSeverity,
        vk::DebugUtilsMessageTypeFlagsEXT              messageTypes,
        vk::DebugUtilsMessengerCallbackDataEXT const * pCallbackData,
        void * pUserData
    ) {
        std::println(std::cerr, "validation layer: {}",  pCallbackData->pMessage);
        return false;
    }
    static constexpr vk::DebugUtilsMessengerCreateInfoEXT populateDebugMessengerCreateInfo() {
        constexpr vk::DebugUtilsMessageSeverityFlagsEXT severityFlags(
            vk::DebugUtilsMessageSeverityFlagBitsEXT::eVerbose |
            vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning |
            vk::DebugUtilsMessageSeverityFlagBitsEXT::eError
        );
        constexpr vk::DebugUtilsMessageTypeFlagsEXT    messageTypeFlags(
            vk::DebugUtilsMessageTypeFlagBitsEXT::eGeneral |
            vk::DebugUtilsMessageTypeFlagBitsEXT::ePerformance |
            vk::DebugUtilsMessageTypeFlagBitsEXT::eValidation
        );
        return { {}, severityFlags, messageTypeFlags, &debugMessageFunc };
    }
    void setupDebugMessenger() {
        if constexpr (!ENABLE_VALIDATION_LAYER) return;
        constexpr auto createInfo = populateDebugMessengerCreateInfo();
        m_debugMessenger = m_instance.createDebugUtilsMessengerEXT( createInfo );
    }
    static std::vector<const char *> getRequiredExtensions() {
        uint32_t glfwExtensionCount = 0;
        const char **glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);
        std::vector<const char *> extensions(glfwExtensions, glfwExtensions + glfwExtensionCount);
        extensions.emplace_back(vk::KHRPortabilityEnumerationExtensionName);
        if constexpr (ENABLE_VALIDATION_LAYER) {
            extensions.emplace_back( vk::EXTDebugUtilsExtensionName );
        }
        return extensions;
    }
    bool checkValidationLayerSupport() const {
        const auto layers = m_context.enumerateInstanceLayerProperties();
        std::set<std::string> requiredLayers(REQUIRED_LAYERS.begin(), REQUIRED_LAYERS.end());
        for (const auto &layer: layers) {
            requiredLayers.erase(layer.layerName);
        }
        return requiredLayers.empty();
    }

    void createInstance() {
        if constexpr (ENABLE_VALIDATION_LAYER) {
            if (!checkValidationLayerSupport()) throw std::runtime_error(
                "validation layers requested, but not available!");
        }

        constexpr vk::ApplicationInfo applicationInfo(
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

        std::vector<const char*> requiredExtensions = getRequiredExtensions();
        createInfo.setPEnabledExtensionNames( requiredExtensions );
        createInfo.flags |= vk::InstanceCreateFlagBits::eEnumeratePortabilityKHR;

        // std::vector<vk::ExtensionProperties>
        const auto extensions = m_context.enumerateInstanceExtensionProperties();
        std::cout << "available extensions:\n";
        for (const auto& extension : extensions) {
            std::println("\t{}", std::string_view(extension.extensionName));
        }

        constexpr auto debugMessengerCreateInfo = populateDebugMessengerCreateInfo();
        if constexpr (ENABLE_VALIDATION_LAYER) {
            createInfo.setPEnabledLayerNames( REQUIRED_LAYERS );
            createInfo.pNext = &debugMessengerCreateInfo ;
        }

        m_instance = m_context.createInstance( createInfo );
    }

    void createSurface() {
        VkSurfaceKHR cSurface;
        if (glfwCreateWindowSurface( *m_instance, m_window, nullptr, &cSurface ) != VK_SUCCESS) {
            throw std::runtime_error("failed to create window surface!");
        }
        m_surface = vk::raii::SurfaceKHR( m_instance, cSurface );
    }
    /////////////////////////////////////////////////////////////


    /////////////////////////////////////////////////////////////
    /// device and queue
    QueueFamilyIndices findQueueFamilies(const vk::raii::PhysicalDevice& physicalDevice) const {
        QueueFamilyIndices indices;
        // std::vector<vk::QueueFamilyProperties>
        const auto queueFamilies = physicalDevice.getQueueFamilyProperties();
        for (int i = 0; const auto& queueFamily : queueFamilies) {
            if (queueFamily.queueFlags & vk::QueueFlagBits::eGraphics) {
                indices.graphicsFamily = i;
            }
            if(physicalDevice.getSurfaceSupportKHR(i, m_surface)){
                indices.presentFamily = i;
            }
            if (indices.isComplete())  break;

            ++i;
        }
        return indices;
    }
    bool isDeviceSuitable(const vk::raii::PhysicalDevice& physicalDevice) const {
        const auto indices = findQueueFamilies(physicalDevice);

        return indices.isComplete();
    }
    void selectPhysicalDevice() {
        // std::vector<vk::raii::PhysicalDevice>
        const auto physicalDevices = m_instance.enumeratePhysicalDevices();
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
    void createLogicalDevice() {
        std::vector<vk::DeviceQueueCreateInfo> queueCreateInfos;
        const auto [graphics, present] = findQueueFamilies( m_physicalDevice );
        std::set<uint32_t> uniqueQueueFamilies = { graphics.value(), present.value() };
        constexpr float queuePriority = 1.0f;
        for (uint32_t queueFamily : uniqueQueueFamilies) {
            vk::DeviceQueueCreateInfo queueCreateInfo;
            queueCreateInfo.queueFamilyIndex = queueFamily;
            queueCreateInfo.setQueuePriorities( queuePriority );
            queueCreateInfos.emplace_back( queueCreateInfo );
        }

        vk::PhysicalDeviceFeatures deviceFeatures;
        vk::DeviceCreateInfo createInfo;
        createInfo.setQueueCreateInfos( queueCreateInfos );
        createInfo.pEnabledFeatures = &deviceFeatures;
        m_device = m_physicalDevice.createDevice( createInfo );
        m_graphicsQueue = m_device.getQueue( graphics.value(), 0 );
        m_presentQueue = m_device.getQueue( present.value(), 0 );
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
