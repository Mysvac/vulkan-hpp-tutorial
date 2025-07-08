#include <iostream>
#include <fstream>
#include <print>
#include <set>
#include <stdexcept>
#include <optional>
#include <limits>
#include <algorithm>
#include <chrono>

#include <vulkan/vulkan.hpp>
#include <vulkan/vulkan_raii.hpp>
#include <GLFW/glfw3.h>
#define GLM_FORCE_RADIANS
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

constexpr uint32_t WIDTH = 800;
constexpr uint32_t HEIGHT = 600;

constexpr int MAX_FRAMES_IN_FLIGHT = 2;

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

struct Vertex {
    glm::vec2 pos;
    glm::vec3 color;
    glm::vec2 texCoord;

    static vk::VertexInputBindingDescription getBindingDescription() {
        vk::VertexInputBindingDescription bindingDescription;
        bindingDescription.binding = 0;
        bindingDescription.stride = sizeof(Vertex);
        bindingDescription.inputRate = vk::VertexInputRate::eVertex;
        return bindingDescription;
    }
    static std::array<vk::VertexInputAttributeDescription, 3>  getAttributeDescriptions() {
        std::array<vk::VertexInputAttributeDescription, 3> attributeDescriptions;
        attributeDescriptions[0].binding = 0;
        attributeDescriptions[0].location = 0;
        attributeDescriptions[0].format = vk::Format::eR32G32Sfloat;
        attributeDescriptions[0].offset = offsetof(Vertex, pos);
        attributeDescriptions[1].binding = 0;
        attributeDescriptions[1].location = 1;
        attributeDescriptions[1].format = vk::Format::eR32G32B32Sfloat;
        attributeDescriptions[1].offset = offsetof(Vertex, color);
        attributeDescriptions[2].binding = 0;
        attributeDescriptions[2].location = 2;
        attributeDescriptions[2].format = vk::Format::eR32G32Sfloat;
        attributeDescriptions[2].offset = offsetof(Vertex, texCoord);

        return attributeDescriptions;
    }
};

const std::vector<Vertex> vertices = {
    {{-0.5f, -0.5f}, {1.0f, 0.0f, 0.0f}, {1.0f, 0.0f}},
    {{0.5f, -0.5f}, {0.0f, 1.0f, 0.0f}, {0.0f, 0.0f}},
    {{0.5f, 0.5f}, {0.0f, 0.0f, 1.0f}, {0.0f, 1.0f}},
    {{-0.5f, 0.5f}, {1.0f, 1.0f, 1.0f}, {1.0f, 1.0f}}
};

const std::vector<uint32_t> indices = {
    0, 1, 2, 2, 3, 0
};

struct UniformBufferObject {
    glm::mat4 model;
    glm::mat4 view;
    glm::mat4 proj;
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
    std::vector<vk::raii::DescriptorSetLayout> m_descriptorSetLayouts;
    vk::raii::PipelineLayout m_pipelineLayout{ nullptr };
    vk::raii::Pipeline m_graphicsPipeline{ nullptr };
    vk::raii::CommandPool m_commandPool{ nullptr };
    std::vector<vk::raii::CommandBuffer> m_commandBuffers;
    std::vector<vk::raii::Semaphore> m_imageAvailableSemaphores;
    std::vector<vk::raii::Semaphore> m_renderFinishedSemaphores;
    std::vector<vk::raii::Fence> m_inFlightFences;
    uint32_t m_currentFrame = 0;
    bool m_framebufferResized = false;
    vk::raii::DeviceMemory m_vertexBufferMemory{ nullptr };
    vk::raii::Buffer m_vertexBuffer{ nullptr };
    vk::raii::DeviceMemory m_indexBufferMemory{ nullptr };
    vk::raii::Buffer m_indexBuffer{ nullptr };
    std::vector<vk::raii::DeviceMemory> m_uniformBuffersMemory;
    std::vector<vk::raii::Buffer> m_uniformBuffers;
    std::vector<void*> m_uniformBuffersMapped;
    vk::raii::DeviceMemory m_textureImageMemory{ nullptr };
    vk::raii::Image m_textureImage{ nullptr };
    vk::raii::ImageView m_textureImageView{ nullptr };
    vk::raii::Sampler m_textureSampler{ nullptr };
    vk::raii::DescriptorPool m_descriptorPool{ nullptr };
    std::vector<vk::raii::DescriptorSet> m_descriptorSets;
    vk::raii::DescriptorSet m_combinedDescriptorSet{ nullptr };
    /////////////////////////////////////////////////////////////

    /////////////////////////////////////////////////////////////
    /// run functions
    void initWindow() {
        glfwInit();

        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

        m_window = glfwCreateWindow(WIDTH, HEIGHT, "Vulkan", nullptr, nullptr);
        glfwSetWindowUserPointer(m_window, this);
        glfwSetFramebufferSizeCallback(m_window, framebufferResizeCallback);
    }

    static void framebufferResizeCallback(GLFWwindow* window, int width, int height) {
        const auto app = static_cast<HelloTriangleApplication*>(glfwGetWindowUserPointer(window));
        app->m_framebufferResized = true;
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
        createDescriptorSetLayout();
        createGraphicsPipeline();
        createCommandPool();
        createCommandBuffers();
        createSyncObjects();
        createVertexBuffer();
        createIndexBuffer();
        createUniformBuffers();
        createTextureImage();
        createTextureImageView();
        createTextureSampler();
        createDescriptorPool();
        createDescriptorSets();
    }

    void mainLoop() {
        while (!glfwWindowShouldClose( m_window )) {
            glfwPollEvents();
            drawFrame();
        }
        m_device.waitIdle();
    }

    void cleanup() {
        for(const auto& it : m_uniformBuffersMemory){
            it.unmapMemory();
        }

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

        if (const auto supportedFeatures = physicalDevice.getFeatures();
            !supportedFeatures.samplerAnisotropy
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
        deviceFeatures.samplerAnisotropy = true;

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
        for (const auto& image : m_swapChainImages) {
            m_swapChainImageViews.emplace_back( createImageView(image, m_swapChainImageFormat) );
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

        vk::SubpassDependency dependency;
        dependency.srcSubpass = vk::SubpassExternal;
        dependency.dstSubpass = 0;
        dependency.dstStageMask = vk::PipelineStageFlagBits::eColorAttachmentOutput;
        dependency.dstAccessMask = vk::AccessFlagBits::eColorAttachmentWrite;
        dependency.srcStageMask = vk::PipelineStageFlagBits::eColorAttachmentOutput;
        dependency.srcAccessMask = {};

        vk::RenderPassCreateInfo renderPassInfo;
        renderPassInfo.setAttachments( colorAttachment );
        renderPassInfo.setSubpasses( subpass );
        renderPassInfo.setDependencies( dependency );

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
        file.read(buffer.data(), static_cast<std::streamsize>(fileSize));
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
        const auto bindingDescription = Vertex::getBindingDescription();
        const auto attributeDescriptions = Vertex::getAttributeDescriptions();
        vertexInputInfo.setVertexBindingDescriptions(bindingDescription);
        vertexInputInfo.setVertexAttributeDescriptions(attributeDescriptions);

        vk::PipelineInputAssemblyStateCreateInfo inputAssembly;
        inputAssembly.topology = vk::PrimitiveTopology::eTriangleList;

        vk::PipelineViewportStateCreateInfo viewportState;
        viewportState.viewportCount = 1;
        viewportState.scissorCount = 1;

        vk::PipelineRasterizationStateCreateInfo rasterizer;
        rasterizer.polygonMode = vk::PolygonMode::eFill;
        rasterizer.lineWidth = 1.0f;
        rasterizer.cullMode = vk::CullModeFlagBits::eBack;
        rasterizer.frontFace = vk::FrontFace::eCounterClockwise;

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
        // 获取内部句柄的数组
        const std::vector<vk::DescriptorSetLayout> descriptorSetLayouts(m_descriptorSetLayouts.begin(), m_descriptorSetLayouts.end());
        pipelineLayoutInfo.setSetLayouts(descriptorSetLayouts);
        m_pipelineLayout = m_device.createPipelineLayout( pipelineLayoutInfo );

        vk::GraphicsPipelineCreateInfo pipelineInfo;
        pipelineInfo.setStages( shaderStages );
        pipelineInfo.pVertexInputState = &vertexInputInfo;
        pipelineInfo.pInputAssemblyState = &inputAssembly;
        pipelineInfo.pViewportState = &viewportState;
        pipelineInfo.pRasterizationState = &rasterizer;
        pipelineInfo.pMultisampleState = &multisampling;
        pipelineInfo.pDepthStencilState = nullptr; // Optional
        pipelineInfo.pColorBlendState = &colorBlending;
        pipelineInfo.pDynamicState = &dynamicState;
        pipelineInfo.layout = m_pipelineLayout;
        pipelineInfo.renderPass = m_renderPass;
        pipelineInfo.subpass = 0;

        m_graphicsPipeline = m_device.createGraphicsPipeline( nullptr, pipelineInfo );
    }

    /////////////////////////////////////////////////////////////

    /////////////////////////////////////////////////////////////
    /// command
    void createCommandPool() {
        const auto [graphicsFamily, presentFamily] = findQueueFamilies( m_physicalDevice );

        vk::CommandPoolCreateInfo poolInfo;
        poolInfo.flags = vk::CommandPoolCreateFlagBits::eResetCommandBuffer;
        poolInfo.queueFamilyIndex =  graphicsFamily.value();

        m_commandPool = m_device.createCommandPool( poolInfo );
    }
    void createCommandBuffers() {
        vk::CommandBufferAllocateInfo allocInfo;
        allocInfo.commandPool = m_commandPool;
        allocInfo.level = vk::CommandBufferLevel::ePrimary;
        allocInfo.commandBufferCount = MAX_FRAMES_IN_FLIGHT;

        m_commandBuffers = m_device.allocateCommandBuffers(allocInfo);
    }
    void recordCommandBuffer(const vk::raii::CommandBuffer& commandBuffer, uint32_t imageIndex) const {
        constexpr vk::CommandBufferBeginInfo beginInfo;
        commandBuffer.begin( beginInfo );

        vk::RenderPassBeginInfo renderPassInfo;
        renderPassInfo.renderPass = m_renderPass;
        renderPassInfo.framebuffer = m_swapChainFramebuffers[imageIndex];
        renderPassInfo.renderArea.offset = vk::Offset2D{0, 0};
        renderPassInfo.renderArea.extent = m_swapChainExtent;
        constexpr vk::ClearValue clearColor(vk::ClearColorValue(0.0f, 0.0f, 0.0f, 1.0f));
        renderPassInfo.setClearValues( clearColor );

        commandBuffer.beginRenderPass(renderPassInfo, vk::SubpassContents::eInline);

        commandBuffer.bindPipeline( vk::PipelineBindPoint::eGraphics, m_graphicsPipeline );

        const vk::Viewport viewport(
            0.0f, 0.0f, // x, y
            static_cast<float>(m_swapChainExtent.width),    // width
            static_cast<float>(m_swapChainExtent.height),   // height
            0.0f, 1.0f  // minDepth maxDepth
        );
        commandBuffer.setViewport(0, viewport);

        const vk::Rect2D scissor(
            vk::Offset2D{0, 0}, // offset
            m_swapChainExtent   // extent
        );
        commandBuffer.setScissor(0, scissor);

        commandBuffer.bindVertexBuffers( 0, *m_vertexBuffer, vk::DeviceSize{ 0 } );
        commandBuffer.bindIndexBuffer( m_indexBuffer, 0, vk::IndexType::eUint32 );

        const std::array<vk::DescriptorSet,2> descriptorSets{ m_descriptorSets[m_currentFrame], m_combinedDescriptorSet };
        commandBuffer.bindDescriptorSets(
            vk::PipelineBindPoint::eGraphics,
            m_pipelineLayout,
            0,
            descriptorSets,
            nullptr
        );

        commandBuffer.drawIndexed(static_cast<uint32_t>(indices.size()), 1, 0, 0, 0);

        commandBuffer.endRenderPass();
        commandBuffer.end();
    }
    /////////////////////////////////////////////////////////////

    /////////////////////////////////////////////////////////////
    /// draw frame
    void createSyncObjects() {
        constexpr vk::SemaphoreCreateInfo semaphoreInfo;
        constexpr vk::FenceCreateInfo fenceInfo(
            vk::FenceCreateFlagBits::eSignaled  // flags
        );

        for(size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i){
            m_imageAvailableSemaphores.emplace_back( m_device, semaphoreInfo );
            m_renderFinishedSemaphores.emplace_back( m_device,  semaphoreInfo );
            m_inFlightFences.emplace_back( m_device , fenceInfo );
        }
    }
    void drawFrame() {
        if(const auto res = m_device.waitForFences( *m_inFlightFences[m_currentFrame], true, std::numeric_limits<uint64_t>::max() );
            res != vk::Result::eSuccess
        ) throw std::runtime_error{ "waitForFences in drawFrame was failed" };

        uint32_t imageIndex;
        try{
            // std::pair<vk::Result, uint32_t>
            const auto [res, idx] = m_swapChain.acquireNextImage(UINT64_MAX, m_imageAvailableSemaphores[m_currentFrame]);
            imageIndex = idx;
        } catch (const vk::OutOfDateKHRError&){
            recreateSwapChain();
            return;
        }

        m_device.resetFences( *m_inFlightFences[m_currentFrame] );

        updateUniformBuffer(m_currentFrame);

        m_commandBuffers[m_currentFrame].reset();
        recordCommandBuffer(m_commandBuffers[m_currentFrame], imageIndex);

        vk::SubmitInfo submitInfo;
        submitInfo.setWaitSemaphores( *m_imageAvailableSemaphores[m_currentFrame] );
        std::array<vk::PipelineStageFlags,1> waitStages = { vk::PipelineStageFlagBits::eColorAttachmentOutput };
        submitInfo.setWaitDstStageMask( waitStages );
        submitInfo.setCommandBuffers( *m_commandBuffers[m_currentFrame] );

        submitInfo.setSignalSemaphores( *m_renderFinishedSemaphores[m_currentFrame] );
        m_graphicsQueue.submit(submitInfo, m_inFlightFences[m_currentFrame]);

        vk::PresentInfoKHR presentInfo;
        presentInfo.setWaitSemaphores( *m_renderFinishedSemaphores[m_currentFrame] );
        presentInfo.setSwapchains( *m_swapChain );
        presentInfo.pImageIndices = &imageIndex;
        try{
            const auto res = m_presentQueue.presentKHR(presentInfo);
            if( res == vk::Result::eSuboptimalKHR ) {
                recreateSwapChain();
            }
        } catch (const vk::OutOfDateKHRError&){
            recreateSwapChain();
        }

        if( m_framebufferResized ){
            recreateSwapChain();
        }

        m_currentFrame = (m_currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
    }
    void recreateSwapChain() {
        int width = 0, height = 0;
        glfwGetFramebufferSize(m_window, &width, &height);
        while (width == 0 || height == 0) {
            glfwGetFramebufferSize(m_window, &width, &height);
            glfwWaitEvents();
        }

        m_device.waitIdle();

        m_swapChainFramebuffers.clear();
        m_swapChainImageViews.clear();
        m_swapChainImages.clear(); // optional
        m_swapChain = nullptr;

        createSwapChain();
        createImageViews();
        createFramebuffers();

        m_framebufferResized = false;
    }
    /////////////////////////////////////////////////////////////

    /////////////////////////////////////////////////////////////
    /// vertex buffer and index buffer
    void createBuffer(
        const vk::DeviceSize size,
        const vk::BufferUsageFlags usage,
        const vk::MemoryPropertyFlags properties,
        vk::raii::Buffer& buffer,
        vk::raii::DeviceMemory& bufferMemory
    ) {
        vk::BufferCreateInfo bufferInfo;
        bufferInfo.size = size;
        bufferInfo.usage = usage;
        bufferInfo.sharingMode = vk::SharingMode::eExclusive;

        buffer = m_device.createBuffer(bufferInfo);

        const vk::MemoryRequirements memRequirements = buffer.getMemoryRequirements();

        vk::MemoryAllocateInfo allocInfo;
        allocInfo.allocationSize = memRequirements.size;
        allocInfo.memoryTypeIndex = findMemoryType(memRequirements.memoryTypeBits, properties);

        bufferMemory = m_device.allocateMemory( allocInfo );

        buffer.bindMemory(bufferMemory, 0);
    }
    uint32_t findMemoryType(const uint32_t typeFilter,const vk::MemoryPropertyFlags properties) const {
        // vk::PhysicalDeviceMemoryProperties
        const auto memProperties = m_physicalDevice.getMemoryProperties();
        for(uint32_t i = 0; i < memProperties.memoryTypeCount; ++i){
            if ((typeFilter & (1 << i)) &&
                (memProperties.memoryTypes[i].propertyFlags & properties ) == properties
            ) return i;
        }
        throw std::runtime_error("failed to find suitable memory type!");
    }
    void copyBuffer(const vk::raii::Buffer& srcBuffer,const vk::raii::Buffer& dstBuffer,const vk::DeviceSize size) const {
        const vk::raii::CommandBuffer commandBuffer = beginSingleTimeCommands();

        vk::BufferCopy copyRegion;
        copyRegion.size = size;
        commandBuffer.copyBuffer(srcBuffer, dstBuffer, copyRegion);

        endSingleTimeCommands( commandBuffer );
    }
    void createVertexBuffer() {
        const vk::DeviceSize bufferSize = sizeof(Vertex) * vertices.size();

        vk::raii::DeviceMemory stagingBufferMemory{ nullptr };
        vk::raii::Buffer stagingBuffer{ nullptr };
        createBuffer(bufferSize,
            vk::BufferUsageFlagBits::eTransferSrc,
            vk::MemoryPropertyFlagBits::eHostVisible |
            vk::MemoryPropertyFlagBits::eHostCoherent,
            stagingBuffer,
            stagingBufferMemory
        );

        void* data = stagingBufferMemory.mapMemory(0, bufferSize);
        memcpy(data, vertices.data(), static_cast<size_t>(bufferSize));
        stagingBufferMemory.unmapMemory();

        createBuffer(bufferSize,
            vk::BufferUsageFlagBits::eTransferDst |
            vk::BufferUsageFlagBits::eVertexBuffer,
            vk::MemoryPropertyFlagBits::eDeviceLocal,
            m_vertexBuffer,
            m_vertexBufferMemory
        );

        copyBuffer(stagingBuffer, m_vertexBuffer, bufferSize);
    }
    void createIndexBuffer() {
        const vk::DeviceSize bufferSize = sizeof(uint32_t) * indices.size();

        vk::raii::DeviceMemory stagingBufferMemory{ nullptr };
        vk::raii::Buffer stagingBuffer{ nullptr };
        createBuffer(bufferSize,
            vk::BufferUsageFlagBits::eTransferSrc,
            vk::MemoryPropertyFlagBits::eHostVisible |
            vk::MemoryPropertyFlagBits::eHostCoherent,
            stagingBuffer,
            stagingBufferMemory
        );

        void* data = stagingBufferMemory.mapMemory(0, bufferSize);
        memcpy(data, indices.data(), static_cast<size_t>(bufferSize));
        stagingBufferMemory.unmapMemory();

        createBuffer(bufferSize,
            vk::BufferUsageFlagBits::eTransferDst |
            vk::BufferUsageFlagBits::eIndexBuffer,
            vk::MemoryPropertyFlagBits::eDeviceLocal,
            m_indexBuffer,
            m_indexBufferMemory
        );

        copyBuffer(stagingBuffer, m_indexBuffer, bufferSize);
    }
    /////////////////////////////////////////////////////////////

    /////////////////////////////////////////////////////////////
    /// UBO
    void createDescriptorSetLayout() {
        vk::DescriptorSetLayoutBinding uboLayoutBinding;
        uboLayoutBinding.binding = 0;
        uboLayoutBinding.descriptorType = vk::DescriptorType::eUniformBuffer;
        uboLayoutBinding.descriptorCount = 1;
        uboLayoutBinding.stageFlags = vk::ShaderStageFlagBits::eVertex;

        vk::DescriptorSetLayoutCreateInfo uboLayoutInfo;
        uboLayoutInfo.setBindings( uboLayoutBinding );
        m_descriptorSetLayouts.emplace_back( m_device.createDescriptorSetLayout( uboLayoutInfo ) );

        vk::DescriptorSetLayoutBinding samplerLayoutBinding;
        samplerLayoutBinding.binding = 0;
        samplerLayoutBinding.descriptorType = vk::DescriptorType::eCombinedImageSampler;
        samplerLayoutBinding.descriptorCount = 1;
        samplerLayoutBinding.stageFlags = vk::ShaderStageFlagBits::eFragment;
        vk::DescriptorSetLayoutCreateInfo samplerLayoutInfo;

        samplerLayoutInfo.setBindings( samplerLayoutBinding );
        m_descriptorSetLayouts.emplace_back( m_device.createDescriptorSetLayout( samplerLayoutInfo ) );
    }
    void createUniformBuffers() {
        constexpr vk::DeviceSize bufferSize  = sizeof(UniformBufferObject);

        m_uniformBuffers.reserve(MAX_FRAMES_IN_FLIGHT);
        m_uniformBuffersMemory.reserve(MAX_FRAMES_IN_FLIGHT);
        m_uniformBuffersMapped.reserve(MAX_FRAMES_IN_FLIGHT);

        for(size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
            m_uniformBuffers.emplace_back( nullptr );
            m_uniformBuffersMemory.emplace_back( nullptr );
            m_uniformBuffersMapped.emplace_back( nullptr );
            createBuffer(bufferSize,
                vk::BufferUsageFlagBits::eUniformBuffer,
                vk::MemoryPropertyFlagBits::eHostVisible |
                vk::MemoryPropertyFlagBits::eHostCoherent,
                m_uniformBuffers[i],
                m_uniformBuffersMemory[i]
            );

            m_uniformBuffersMapped[i] = m_uniformBuffersMemory[i].mapMemory(0, bufferSize);
        }
    }
    void updateUniformBuffer(const uint32_t currentImage) const {
        static auto startTime = std::chrono::high_resolution_clock::now();
        const auto currentTime = std::chrono::high_resolution_clock::now();
        const float time = std::chrono::duration<float, std::chrono::seconds::period>(currentTime - startTime).count();

        UniformBufferObject ubo{};
        ubo.model = glm::rotate(
            glm::mat4(1.0f),
            time * glm::radians(90.0f),
            glm::vec3(0.0f, 0.0f, 1.0f)
        );
        ubo.view = glm::lookAt(
            glm::vec3(2.0f, 2.0f, 2.0f),
            glm::vec3(0.0f, 0.0f, 0.0f),
            glm::vec3(0.0f, 0.0f, 1.0f)
        );
        ubo.proj = glm::perspective(
            glm::radians(45.0f),
            static_cast<float>(m_swapChainExtent.width) / static_cast<float>(m_swapChainExtent.height),
            0.1f,
            20.0f
        );
        ubo.proj[1][1] *= -1;

        memcpy(m_uniformBuffersMapped[currentImage], &ubo, sizeof(ubo));
    }
    void createDescriptorPool() {
        std::array<vk::DescriptorPoolSize, 2> poolSizes;
        poolSizes[0].type = vk::DescriptorType::eUniformBuffer;
        poolSizes[0].descriptorCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);
        poolSizes[1].type = vk::DescriptorType::eCombinedImageSampler;
        poolSizes[1].descriptorCount = 1;

        vk::DescriptorPoolCreateInfo poolInfo;
        poolInfo.flags = vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet;
        poolInfo.setPoolSizes( poolSizes );
        poolInfo.maxSets = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT) + 1;

        m_descriptorPool = m_device.createDescriptorPool(poolInfo);
    }
    void createDescriptorSets() {
        std::vector<vk::DescriptorSetLayout> layouts(MAX_FRAMES_IN_FLIGHT, m_descriptorSetLayouts[0]);
        vk::DescriptorSetAllocateInfo allocInfo;
        allocInfo.descriptorPool = m_descriptorPool;
        allocInfo.setSetLayouts( layouts );

        m_descriptorSets = m_device.allocateDescriptorSets(allocInfo);

        for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i) {
            vk::DescriptorBufferInfo bufferInfo;
            bufferInfo.buffer = m_uniformBuffers[i];
            bufferInfo.offset = 0;
            bufferInfo.range = sizeof(UniformBufferObject);

            vk::WriteDescriptorSet descriptorWrite;
            descriptorWrite.dstSet = m_descriptorSets[i];
            descriptorWrite.dstBinding = 0;
            descriptorWrite.dstArrayElement = 0;
            descriptorWrite.descriptorType = vk::DescriptorType::eUniformBuffer;
            descriptorWrite.setBufferInfo(bufferInfo);

            m_device.updateDescriptorSets(descriptorWrite, nullptr);
        }

        allocInfo.setSetLayouts(*m_descriptorSetLayouts[1]);
        std::vector<vk::raii::DescriptorSet> sets = m_device.allocateDescriptorSets(allocInfo);
        m_combinedDescriptorSet =  std::move(sets.at(0));

        vk::DescriptorImageInfo imageInfo;
        imageInfo.imageLayout = vk::ImageLayout::eShaderReadOnlyOptimal;
        imageInfo.imageView = m_textureImageView;
        imageInfo.sampler = m_textureSampler;

        vk::WriteDescriptorSet combinedDescriptorWrite;
        combinedDescriptorWrite.dstSet = m_combinedDescriptorSet;
        combinedDescriptorWrite.dstBinding = 0;
        combinedDescriptorWrite.dstArrayElement = 0;
        combinedDescriptorWrite.descriptorType = vk::DescriptorType::eCombinedImageSampler;
        combinedDescriptorWrite.setImageInfo(imageInfo);

        m_device.updateDescriptorSets(combinedDescriptorWrite, nullptr);
    }
    /////////////////////////////////////////////////////////////

    /////////////////////////////////////////////////////////////
    /// texture
    void createImage(
        const uint32_t width,
        const uint32_t height,
        const vk::Format format,
        const vk::ImageTiling tiling,
        const vk::ImageUsageFlags usage,
        const vk::MemoryPropertyFlags properties,
        vk::raii::Image& image,
        vk::raii::DeviceMemory& imageMemory
    ) const {
        vk::ImageCreateInfo imageInfo;
        imageInfo.imageType = vk::ImageType::e2D;
        imageInfo.extent.width = width;
        imageInfo.extent.height = height;
        imageInfo.extent.depth = 1;
        imageInfo.mipLevels = 1;
        imageInfo.arrayLayers = 1;
        imageInfo.format = format;
        imageInfo.tiling = tiling;
        imageInfo.initialLayout = vk::ImageLayout::eUndefined;
        imageInfo.usage = usage;
        imageInfo.samples = vk::SampleCountFlagBits::e1;
        imageInfo.sharingMode = vk::SharingMode::eExclusive;

        image = m_device.createImage(imageInfo);

        const vk::MemoryRequirements memRequirements = image.getMemoryRequirements();
        vk::MemoryAllocateInfo allocInfo;
        allocInfo.allocationSize = memRequirements.size;
        allocInfo.memoryTypeIndex = findMemoryType(memRequirements.memoryTypeBits, properties);

        imageMemory = m_device.allocateMemory(allocInfo);

        image.bindMemory(imageMemory, 0);
    }
    vk::raii::CommandBuffer beginSingleTimeCommands() const {
        vk::CommandBufferAllocateInfo allocInfo;
        allocInfo.level = vk::CommandBufferLevel::ePrimary;
        allocInfo.commandPool = m_commandPool;
        allocInfo.commandBufferCount = 1;

        auto commandBuffers = m_device.allocateCommandBuffers(allocInfo);
        vk::raii::CommandBuffer commandBuffer = std::move(commandBuffers.at(0));

        vk::CommandBufferBeginInfo beginInfo;
        beginInfo.flags = vk::CommandBufferUsageFlagBits::eOneTimeSubmit;

        commandBuffer.begin(beginInfo);

        return commandBuffer;
    }
    void endSingleTimeCommands(const vk::raii::CommandBuffer& commandBuffer) const {
        commandBuffer.end();

        vk::SubmitInfo submitInfo;
        submitInfo.setCommandBuffers( *commandBuffer );

        m_graphicsQueue.submit(submitInfo);
        m_graphicsQueue.waitIdle();
    }
    void transitionImageLayout(
        const vk::raii::Image& image,
        const vk::Format format,
        const vk::ImageLayout oldLayout,
        const vk::ImageLayout newLayout
    ) const {
        const vk::raii::CommandBuffer commandBuffer = beginSingleTimeCommands();
        vk::ImageMemoryBarrier barrier;
        barrier.oldLayout = oldLayout;
        barrier.newLayout = newLayout;
        barrier.srcQueueFamilyIndex = vk::QueueFamilyIgnored;
        barrier.dstQueueFamilyIndex = vk::QueueFamilyIgnored;

        barrier.image = image;
        barrier.subresourceRange.aspectMask = vk::ImageAspectFlagBits::eColor;
        barrier.subresourceRange.baseMipLevel = 0;
        barrier.subresourceRange.levelCount = 1;
        barrier.subresourceRange.baseArrayLayer = 0;
        barrier.subresourceRange.layerCount = 1;

        vk::PipelineStageFlagBits sourceStage;
        vk::PipelineStageFlagBits destinationStage;

        if (oldLayout == vk::ImageLayout::eUndefined &&
            newLayout == vk::ImageLayout::eTransferDstOptimal
        ) {
            barrier.srcAccessMask = {};
            barrier.dstAccessMask = vk::AccessFlagBits::eTransferWrite;
            sourceStage = vk::PipelineStageFlagBits::eTopOfPipe;
            destinationStage = vk::PipelineStageFlagBits::eTransfer;
        } else if(
            oldLayout == vk::ImageLayout::eTransferDstOptimal &&
            newLayout == vk::ImageLayout::eShaderReadOnlyOptimal
        ) {
            barrier.srcAccessMask = vk::AccessFlagBits::eTransferWrite;
            barrier.dstAccessMask = vk::AccessFlagBits::eShaderRead;

            sourceStage = vk::PipelineStageFlagBits::eTransfer;
            destinationStage = vk::PipelineStageFlagBits::eFragmentShader;
        } else {
            throw std::invalid_argument("unsupported layout transition!");
        }

        commandBuffer.pipelineBarrier(
            sourceStage,        // srcStageMask
            destinationStage,   // dstStageMask
            {},         // dependencyFlags
            nullptr,    // memoryBarriers
            nullptr,    // bufferMemoryBarriers
            barrier     // imageMemoryBarriers
        );

        endSingleTimeCommands( commandBuffer );
    }
    void copyBufferToImage(
        const vk::raii::Buffer& buffer,
        const vk::raii::Image& image,
        const uint32_t width,
        const uint32_t height
    ) const {
        const vk::raii::CommandBuffer commandBuffer = beginSingleTimeCommands();

        vk::BufferImageCopy region;
        region.bufferOffset = 0;
        region.bufferRowLength = 0;
        region.bufferImageHeight = 0;
        region.imageSubresource.aspectMask = vk::ImageAspectFlagBits::eColor;
        region.imageSubresource.mipLevel = 0;
        region.imageSubresource.baseArrayLayer = 0;
        region.imageSubresource.layerCount = 1;
        region.imageOffset = vk::Offset3D{0, 0, 0};
        region.imageExtent = vk::Extent3D{width, height, 1};

        commandBuffer.copyBufferToImage(
            buffer,
            image,
            vk::ImageLayout::eTransferDstOptimal,
            region
        );

        endSingleTimeCommands( commandBuffer );
    }
    void createTextureImage() {
        int texWidth, texHeight, texChannels;
        stbi_uc* pixels = stbi_load("textures/rust_cpp.png", &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);
        if (!pixels) throw std::runtime_error("failed to load texture image!");
        const vk::DeviceSize imageSize = texWidth * texHeight * 4;

        vk::raii::DeviceMemory stagingBufferMemory{ nullptr };
        vk::raii::Buffer stagingBuffer{ nullptr };

        createBuffer(
            imageSize,
            vk::BufferUsageFlagBits::eTransferSrc,
            vk::MemoryPropertyFlagBits::eHostVisible |
            vk::MemoryPropertyFlagBits::eHostCoherent,
            stagingBuffer,
            stagingBufferMemory
        );

        void* data = stagingBufferMemory.mapMemory(0, imageSize);
        memcpy(data, pixels, static_cast<size_t>(imageSize));
        stagingBufferMemory.unmapMemory();

        stbi_image_free(pixels);

        createImage(
            texWidth,
            texHeight,
            vk::Format::eR8G8B8A8Srgb,
            vk::ImageTiling::eOptimal,
            vk::ImageUsageFlagBits::eTransferDst |
            vk::ImageUsageFlagBits::eSampled,
            vk::MemoryPropertyFlagBits::eDeviceLocal,
            m_textureImage,
            m_textureImageMemory
        );

        transitionImageLayout(
            m_textureImage,
            vk::Format::eR8G8B8A8Srgb,
            vk::ImageLayout::eUndefined,
            vk::ImageLayout::eTransferDstOptimal
        );

        copyBufferToImage(
            stagingBuffer,
            m_textureImage,
            static_cast<uint32_t>(texWidth),
            static_cast<uint32_t>(texHeight)
        );

        transitionImageLayout(
            m_textureImage,
            vk::Format::eR8G8B8A8Srgb,
            vk::ImageLayout::eTransferDstOptimal,
            vk::ImageLayout::eShaderReadOnlyOptimal
        );
    }
    vk::raii::ImageView createImageView(const vk::Image image,const vk::Format format) const {
        vk::ImageViewCreateInfo viewInfo;
        viewInfo.image = image;
        viewInfo.viewType = vk::ImageViewType::e2D;
        viewInfo.format = format;
        viewInfo.subresourceRange.aspectMask = vk::ImageAspectFlagBits::eColor;
        viewInfo.subresourceRange.baseMipLevel = 0;
        viewInfo.subresourceRange.levelCount = 1;
        viewInfo.subresourceRange.baseArrayLayer = 0;
        viewInfo.subresourceRange.layerCount = 1;

        return m_device.createImageView(viewInfo);
    }
    void createTextureImageView() {
        m_textureImageView = createImageView(m_textureImage, vk::Format::eR8G8B8A8Srgb);
    }
    void createTextureSampler() {
        vk::SamplerCreateInfo samplerInfo;
        samplerInfo.magFilter = vk::Filter::eLinear;
        samplerInfo.minFilter = vk::Filter::eLinear;
        samplerInfo.addressModeU = vk::SamplerAddressMode::eRepeat;
        samplerInfo.addressModeV = vk::SamplerAddressMode::eRepeat;
        samplerInfo.addressModeW = vk::SamplerAddressMode::eRepeat;
        samplerInfo.anisotropyEnable = true;
        const auto properties = m_physicalDevice.getProperties();
        samplerInfo.maxAnisotropy = properties.limits.maxSamplerAnisotropy;
        samplerInfo.borderColor = vk::BorderColor::eIntOpaqueBlack;
        samplerInfo.unnormalizedCoordinates = false;
        samplerInfo.compareEnable = false;
        samplerInfo.compareOp = vk::CompareOp::eAlways;
        samplerInfo.mipmapMode = vk::SamplerMipmapMode::eLinear;
        samplerInfo.mipLodBias = 0.0f;
        samplerInfo.minLod = 0.0f;
        samplerInfo.maxLod = 0.0f;

        m_textureSampler = m_device.createSampler(samplerInfo);
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
