#include <iostream>
#include <fstream>
#include <print>
#include <set>
#include <stdexcept>
#include <optional>
#include <limits>
#include <algorithm>

#include <vulkan/vulkan.hpp>
#include <vulkan/vulkan_raii.hpp>
#include <GLFW/glfw3.h>



constexpr uint32_t WIDTH = 800;
constexpr uint32_t HEIGHT = 600;

constexpr std::array<const char*,1> REQUIRED_LAYERS {
    "VK_LAYER_KHRONOS_validation"
};

constexpr std::array<const char*,1> DEVICE_EXTENSIONS {
    vk::KHRSwapchainExtensionName
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

struct SwapChainSupportDetails {
    vk::SurfaceCapabilitiesKHR capabilities;
    std::vector<vk::SurfaceFormatKHR>  formats;
    std::vector<vk::PresentModeKHR> presentModes;
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
    vk::raii::SwapchainKHR m_swapChain{ nullptr };
    std::vector<vk::Image> m_swapChainImages;
    vk::Format m_swapChainImageFormat{};
    vk::Extent2D m_swapChainExtent{};
    std::vector<vk::raii::ImageView> m_swapChainImageViews;
    vk::raii::RenderPass m_renderPass{ nullptr };
    std::vector<vk::raii::Framebuffer> m_swapChainFramebuffers;
    vk::raii::PipelineLayout m_pipelineLayout{ nullptr };
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
        createSwapChain();
        createImageViews();
        createRenderPass();
        createFramebuffers();
        createGraphicsPipeline();
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
    static bool checkDeviceExtensionSupport(const vk::raii::PhysicalDevice& physicalDevice) {
        // std::vector<vk::ExtensionProperties>
        const auto availableExtensions = physicalDevice.enumerateDeviceExtensionProperties();
        std::set<std::string> requiredExtensions(DEVICE_EXTENSIONS.begin(), DEVICE_EXTENSIONS.end());
        for (const auto& extension : availableExtensions) {
            requiredExtensions.erase(extension.extensionName);
        }
        return requiredExtensions.empty();
    }
    SwapChainSupportDetails querySwapChainSupport(const vk::raii::PhysicalDevice& physicalDevice) const {
        SwapChainSupportDetails details;
        details.capabilities = physicalDevice.getSurfaceCapabilitiesKHR( m_surface );
        details.formats = physicalDevice.getSurfaceFormatsKHR( m_surface );
        details.presentModes = physicalDevice.getSurfacePresentModesKHR( m_surface );
        return details;
    }
    bool isDeviceSuitable(const vk::raii::PhysicalDevice& physicalDevice) const {
        if (const auto indices = findQueueFamilies(physicalDevice);
            !indices.isComplete()
        ) return false;

        if (const bool extensionsSupported = checkDeviceExtensionSupport(physicalDevice);
            !extensionsSupported
        ) return false;

        if (const auto swapChainSupport = querySwapChainSupport(physicalDevice);
            swapChainSupport.formats.empty() || swapChainSupport.presentModes.empty()
        ) return false;

        return true;
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
        createInfo.setPEnabledExtensionNames( DEVICE_EXTENSIONS );
        m_device = m_physicalDevice.createDevice( createInfo );
        m_graphicsQueue = m_device.getQueue( graphics.value(), 0 );
        m_presentQueue = m_device.getQueue( present.value(), 0 );
    }
    /////////////////////////////////////////////////////////////

    /////////////////////////////////////////////////////////////
    /// swap chain and imageview
    static vk::SurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<vk::SurfaceFormatKHR>& availableFormats) {
        for (const auto& availableFormat : availableFormats) {
            if (availableFormat.format == vk::Format::eB8G8R8A8Srgb &&
                availableFormat.colorSpace == vk::ColorSpaceKHR::eSrgbNonlinear
            ) return availableFormat;
        }
        return availableFormats.at(0);
    }
    static vk::PresentModeKHR chooseSwapPresentMode(const std::vector<vk::PresentModeKHR>& availablePresentModes) {
        for (const auto& availablePresentMode : availablePresentModes) {
            if (availablePresentMode == vk::PresentModeKHR::eMailbox) {
                return availablePresentMode;
            }
        }
        return vk::PresentModeKHR::eFifo;
    }
    vk::Extent2D chooseSwapExtent(const vk::SurfaceCapabilitiesKHR& capabilities) const {
        if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max()) {
            return capabilities.currentExtent;
        }
        int width, height;
        glfwGetFramebufferSize( m_window, &width, &height );
        vk::Extent2D actualExtent (
            static_cast<uint32_t>(width),
            static_cast<uint32_t>(height)
        );

        actualExtent.width = std::clamp(actualExtent.width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width);
        actualExtent.height = std::clamp(actualExtent.height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height);
        return actualExtent;
    }
    void createSwapChain() {
        const auto [capabilities, formats, presentModes]  = querySwapChainSupport( m_physicalDevice );
        const auto surfaceFormat = chooseSwapSurfaceFormat( formats );
        const auto presentMode = chooseSwapPresentMode( presentModes );
        const auto extent = chooseSwapExtent( capabilities );

        uint32_t imageCount = capabilities.minImageCount + 1;
        if (capabilities.maxImageCount > 0 && imageCount > capabilities.maxImageCount) {
            imageCount = capabilities.maxImageCount;
        }
        vk::SwapchainCreateInfoKHR createInfo;
        createInfo.surface = m_surface;
        createInfo.minImageCount = imageCount;
        createInfo.imageFormat = surfaceFormat.format;
        createInfo.imageColorSpace = surfaceFormat.colorSpace;
        createInfo.imageExtent = extent;
        createInfo.imageArrayLayers = 1;
        createInfo.imageUsage = vk::ImageUsageFlagBits::eColorAttachment;

        const auto [graphicsFamily, presentFamily] = findQueueFamilies( m_physicalDevice );
        std::vector<uint32_t> queueFamilyIndices { graphicsFamily.value(), presentFamily.value() };
        if (graphicsFamily != presentFamily) {
            createInfo.imageSharingMode = vk::SharingMode::eConcurrent;
            createInfo.setQueueFamilyIndices( queueFamilyIndices );
        } else {
            createInfo.imageSharingMode = vk::SharingMode::eExclusive;
        }

        createInfo.preTransform = capabilities.currentTransform;
        createInfo.compositeAlpha = vk::CompositeAlphaFlagBitsKHR::eOpaque;
        createInfo.presentMode = presentMode;
        createInfo.clipped = true;
        createInfo.oldSwapchain = nullptr;

        m_swapChain = m_device.createSwapchainKHR( createInfo );
        m_swapChainImages = m_swapChain.getImages();
        m_swapChainImageFormat = surfaceFormat.format;
        m_swapChainExtent = extent;
    }
    void createImageViews() {
        m_swapChainImageViews.reserve( m_swapChainImages.size() );
        vk::ImageViewCreateInfo createInfo;
        createInfo.viewType = vk::ImageViewType::e2D;
        createInfo.format = m_swapChainImageFormat;
        createInfo.subresourceRange.aspectMask = vk::ImageAspectFlagBits::eColor;
        createInfo.subresourceRange.baseMipLevel = 0;
        createInfo.subresourceRange.levelCount = 1;
        createInfo.subresourceRange.baseArrayLayer = 0;
        createInfo.subresourceRange.layerCount = 1;
        for (const auto& image : m_swapChainImages) {
            createInfo.image = image;
            m_swapChainImageViews.emplace_back( m_device.createImageView(createInfo) );
        }
    }
    /////////////////////////////////////////////////////////////

    /////////////////////////////////////////////////////////////
    /// render pass and framebuffer
    void createRenderPass() {
        vk::AttachmentDescription colorAttachment;
        colorAttachment.format = m_swapChainImageFormat;
        colorAttachment.samples = vk::SampleCountFlagBits::e1;
        colorAttachment.loadOp = vk::AttachmentLoadOp::eClear;
        colorAttachment.storeOp = vk::AttachmentStoreOp::eStore;
        colorAttachment.stencilLoadOp = vk::AttachmentLoadOp::eDontCare;
        colorAttachment.stencilStoreOp = vk::AttachmentStoreOp::eDontCare;
        colorAttachment.initialLayout = vk::ImageLayout::eUndefined;
        colorAttachment.finalLayout = vk::ImageLayout::ePresentSrcKHR;

        vk::AttachmentReference colorAttachmentRef;
        colorAttachmentRef.attachment = 0;
        colorAttachmentRef.layout = vk::ImageLayout::eColorAttachmentOptimal;

        vk::SubpassDescription subpass;
        subpass.pipelineBindPoint = vk::PipelineBindPoint::eGraphics;
        subpass.setColorAttachments( colorAttachmentRef );

        vk::RenderPassCreateInfo renderPassInfo;
        renderPassInfo.setAttachments( colorAttachment );
        renderPassInfo.setSubpasses( subpass );

        m_renderPass = m_device.createRenderPass(renderPassInfo);
    }
    void createFramebuffers() {
        m_swapChainFramebuffers.reserve( m_swapChainImageViews.size() );
        vk::FramebufferCreateInfo framebufferInfo;
        framebufferInfo.renderPass = m_renderPass;
        framebufferInfo.width = m_swapChainExtent.width;
        framebufferInfo.height = m_swapChainExtent.height;
        framebufferInfo.layers = 1;
        for (const auto& imageView : m_swapChainImageViews) {
            framebufferInfo.setAttachments( *imageView );
            m_swapChainFramebuffers.emplace_back( m_device.createFramebuffer(framebufferInfo) );
        }
    }
    /////////////////////////////////////////////////////////////

    /////////////////////////////////////////////////////////////
    /// graphics pipeline
    static std::vector<char> readFile(const std::string& filename) {
        std::ifstream file(filename, std::ios::ate | std::ios::binary);
        if (!file.is_open()) throw std::runtime_error("failed to open file!");
        const size_t fileSize = file.tellg();
        std::vector<char> buffer(fileSize);
        file.seekg(0);
        file.read(buffer.data(), fileSize);
        file.close(); // optional
        return buffer;
    }
    vk::raii::ShaderModule createShaderModule(const std::vector<char>& code) const {
        vk::ShaderModuleCreateInfo createInfo;
        createInfo.codeSize = code.size();
        createInfo.pCode = reinterpret_cast<const uint32_t*>(code.data());
        return m_device.createShaderModule(createInfo);
    }
    void createGraphicsPipeline() {
        const auto vertShaderCode = readFile("shaders/graphics.vert.spv");
        const auto fragShaderCode = readFile("shaders/graphics.frag.spv");

        vk::raii::ShaderModule vertShaderModule = createShaderModule(vertShaderCode);
        vk::raii::ShaderModule fragShaderModule = createShaderModule(fragShaderCode);

        vk::PipelineShaderStageCreateInfo vertShaderStageInfo;
        vertShaderStageInfo.stage = vk::ShaderStageFlagBits::eVertex;
        vertShaderStageInfo.module = vertShaderModule;
        vertShaderStageInfo.pName = "main";
        vk::PipelineShaderStageCreateInfo fragShaderStageInfo;
        fragShaderStageInfo.stage = vk::ShaderStageFlagBits::eFragment;
        fragShaderStageInfo.module = fragShaderModule;
        fragShaderStageInfo.pName = "main";
        const auto shaderStages = { vertShaderStageInfo, fragShaderStageInfo };

        const auto dynamicStates = {
            vk::DynamicState::eViewport,
            vk::DynamicState::eScissor
        };
        vk::PipelineDynamicStateCreateInfo dynamicState;
        dynamicState.setDynamicStates( dynamicStates );

        vk::PipelineVertexInputStateCreateInfo vertexInputInfo;
        vk::PipelineInputAssemblyStateCreateInfo inputAssembly;
        inputAssembly.topology = vk::PrimitiveTopology::eTriangleList;

        vk::PipelineViewportStateCreateInfo viewportState;
        viewportState.viewportCount = 1;
        viewportState.scissorCount = 1;

        vk::PipelineRasterizationStateCreateInfo rasterizer;
        rasterizer.polygonMode = vk::PolygonMode::eFill;
        rasterizer.lineWidth = 1.0f;
        rasterizer.cullMode = vk::CullModeFlagBits::eBack;
        rasterizer.frontFace = vk::FrontFace::eClockwise;

        vk::PipelineMultisampleStateCreateInfo multisampling;
        multisampling.rasterizationSamples =  vk::SampleCountFlagBits::e1;
        multisampling.sampleShadingEnable = false;

        vk::PipelineColorBlendAttachmentState colorBlendAttachment;
        colorBlendAttachment.blendEnable = false; // default
        colorBlendAttachment.colorWriteMask = vk::FlagTraits<vk::ColorComponentFlagBits>::allFlags;

        vk::PipelineColorBlendStateCreateInfo colorBlending;
        colorBlending.logicOpEnable = false;
        colorBlending.logicOp = vk::LogicOp::eCopy;
        colorBlending.setAttachments( colorBlendAttachment );

        vk::PipelineLayoutCreateInfo pipelineLayoutInfo;
        m_pipelineLayout = m_device.createPipelineLayout( pipelineLayoutInfo );


    }

    /////////////////////////////////////////////////////////////
};

int main() {
    try {
        HelloTriangleApplication app;
        app.run();
    } catch (const vk::SystemError &err) {
        // use err.code() to check err type
        std::println( std::cerr, "vk::SystemError - code: {} ",err.code().message());
        std::println( std::cerr, "vk::SystemError - what: {}",err.what());
    } catch (const std::exception& err ) {
        std::println( std::cerr, "std::exception: {}", err.what());
    }
}
