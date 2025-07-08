#include <iostream>
#include <print>
#include <set>
#include <stdexcept>

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
    /// create instance and validation layers
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
