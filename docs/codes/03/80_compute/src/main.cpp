#include <iostream>
#include <fstream>
#include <print>
#include <set>
#include <stdexcept>
#include <optional>
#include <limits>
#include <algorithm>
#include <chrono>
#include <random>

#include <vulkan/vulkan.hpp>
#include <vulkan/vulkan_raii.hpp>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>


constexpr uint32_t WIDTH = 800;
constexpr uint32_t HEIGHT = 600;

constexpr uint32_t PARTICLE_COUNT = 8192; // 粒子数
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

struct Particle {
    glm::vec2 pos;
    glm::vec2 velocity;
    glm::vec4 color;

    static vk::VertexInputBindingDescription getBindingDescription() {
        vk::VertexInputBindingDescription bindingDescription;
        bindingDescription.binding = 0;
        bindingDescription.stride = sizeof(Particle);
        bindingDescription.inputRate = vk::VertexInputRate::eVertex;

        return bindingDescription;
    }
    static std::array<vk::VertexInputAttributeDescription, 2>  getAttributeDescriptions() {
        std::array<vk::VertexInputAttributeDescription, 2> attributeDescriptions;

        attributeDescriptions[0].binding = 0;
        attributeDescriptions[0].location = 0;
        attributeDescriptions[0].format = vk::Format::eR32G32Sfloat;
        attributeDescriptions[0].offset = offsetof(Particle, pos);

        // velocity 只需要让计算着色器读取，无需传递给图形管线

        attributeDescriptions[1].binding = 0;
        attributeDescriptions[1].location = 1;
        attributeDescriptions[1].format = vk::Format::eR32G32B32A32Sfloat;
        attributeDescriptions[1].offset = offsetof(Particle, color);

        return attributeDescriptions;
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
    vk::raii::Queue m_computeQueue{ nullptr };
    vk::raii::SwapchainKHR m_swapChain{ nullptr };
    std::vector<vk::Image> m_swapChainImages;
    vk::Format m_swapChainImageFormat{};
    vk::Extent2D m_swapChainExtent{};
    std::vector<vk::raii::ImageView> m_swapChainImageViews;
    vk::raii::RenderPass m_renderPass{ nullptr };
    std::vector<vk::raii::Framebuffer> m_swapChainFramebuffers;
    vk::raii::DescriptorSetLayout m_descriptorSetLayout{ nullptr };
    vk::raii::DescriptorPool m_descriptorPool{ nullptr };
    vk::raii::PipelineLayout m_computePipelineLayout{ nullptr };
    vk::raii::Pipeline m_computePipeline{ nullptr };
    vk::raii::PipelineLayout m_pipelineLayout{ nullptr };
    vk::raii::Pipeline m_graphicsPipeline{ nullptr };
    vk::raii::CommandPool m_commandPool{ nullptr };
    std::vector<vk::raii::CommandBuffer> m_commandBuffers;
    std::vector<vk::raii::CommandBuffer> m_computeCommandBuffers;
    std::vector<vk::raii::Semaphore> m_imageAvailableSemaphores;
    std::vector<vk::raii::Semaphore> m_renderFinishedSemaphores;
    std::vector<vk::raii::Fence> m_inFlightFences;
    uint32_t m_currentFrame = 0;
    bool m_framebufferResized = false;
    vk::raii::DeviceMemory m_particleBufferMemory{ nullptr };
    vk::raii::Buffer m_particleBuffer{ nullptr };
    std::vector<vk::raii::DescriptorSet> m_descriptorSets;
    std::vector<vk::raii::Semaphore> m_computeFinishedSemaphores;
    std::vector<vk::raii::Semaphore> m_graphicsFinishedSemaphores;
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
        createDescriptorPool();
        createComputePipeline();
        createGraphicsPipeline();
        createCommandPool();
        createCommandBuffers();
        createComputeCommandBuffers(); // 可以放在图形命令缓冲的下方或上方
        createSyncObjects();
        createParticleBuffer();
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
            if ((queueFamily.queueFlags & vk::QueueFlagBits::eGraphics) &&
                (queueFamily.queueFlags & vk::QueueFlagBits::eCompute)
            ) indices.graphicsFamily = i;

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
        m_computeQueue = m_device.getQueue( graphics.value(), 0 );
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
        const auto bindingDescription = Particle::getBindingDescription();
        const auto attributeDescriptions = Particle::getAttributeDescriptions();
        vertexInputInfo.setVertexBindingDescriptions(bindingDescription);
        vertexInputInfo.setVertexAttributeDescriptions(attributeDescriptions);

        vk::PipelineInputAssemblyStateCreateInfo inputAssembly;
        inputAssembly.topology = vk::PrimitiveTopology::ePointList; // 点列表

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

        const std::array<vk::Buffer,1> vertexBuffers { m_particleBuffer };
        constexpr std::array<vk::DeviceSize,1> offsets { 0 };
        commandBuffer.bindVertexBuffers( 0, vertexBuffers, offsets );

        commandBuffer.draw(PARTICLE_COUNT, 1, 0, 0);

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
            m_computeFinishedSemaphores.emplace_back( m_device, semaphoreInfo );
            m_graphicsFinishedSemaphores.emplace_back( m_device, semaphoreInfo );
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

        m_computeCommandBuffers[m_currentFrame].reset();
        recordComputeCommandBuffer(m_computeCommandBuffers[m_currentFrame]);

        static bool firstTime = true;

        // 设置提交信息，并在任务完成时发送信号量
        vk::SubmitInfo computeSubmitInfo;
        computeSubmitInfo.setCommandBuffers( *m_computeCommandBuffers[m_currentFrame] );
        computeSubmitInfo.setSignalSemaphores( *m_computeFinishedSemaphores[m_currentFrame] );
        if (!firstTime) { // 如果不是第一次，则等待上一次绘制完成
            computeSubmitInfo.setWaitSemaphores( *m_graphicsFinishedSemaphores[ (m_currentFrame+MAX_FRAMES_IN_FLIGHT-1)%MAX_FRAMES_IN_FLIGHT ] );
            // 在计算着色器等待上一次渲染完成
            constexpr vk::PipelineStageFlags pipelineStage = vk::PipelineStageFlagBits::eComputeShader;
            computeSubmitInfo.setWaitDstStageMask( pipelineStage );
        } else {
            firstTime = false;
        }
        m_computeQueue.submit(computeSubmitInfo);


        m_commandBuffers[m_currentFrame].reset();
        recordCommandBuffer(m_commandBuffers[m_currentFrame], imageIndex);

        vk::SubmitInfo submitInfo;
        // 渲染需要等待图像可用和本次计算完成
        const std::array<vk::Semaphore,2> waitSemaphores = {
            m_computeFinishedSemaphores[m_currentFrame],
            m_imageAvailableSemaphores[m_currentFrame]
        };
        submitInfo.setWaitSemaphores( waitSemaphores );
        constexpr std::array<vk::PipelineStageFlags,2> waitStages = {
            vk::PipelineStageFlagBits::eVertexInput,
            vk::PipelineStageFlagBits::eColorAttachmentOutput
        };
        submitInfo.setWaitDstStageMask( waitStages );
        submitInfo.setCommandBuffers( *m_commandBuffers[m_currentFrame] );

        // 设置信号量，表示图形渲染完成。renderFinished 提供给呈现呈现，graphicsFinished提供给计算管线
        const std::array<vk::Semaphore,2> signalSemaphores = {
            m_renderFinishedSemaphores[m_currentFrame],
            m_graphicsFinishedSemaphores[m_currentFrame]
        };
        submitInfo.setSignalSemaphores( signalSemaphores );
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
    /// vertex buffer
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
        vk::CommandBufferAllocateInfo allocInfo;
        allocInfo.level = vk::CommandBufferLevel::ePrimary;
        allocInfo.commandPool = m_commandPool;
        allocInfo.commandBufferCount = 1;

        // std::vector<vk::raii::CommandBuffer>
        auto commandBuffers = m_device.allocateCommandBuffers(allocInfo);
        const vk::raii::CommandBuffer commandBuffer = std::move(commandBuffers.at(0));

        vk::CommandBufferBeginInfo beginInfo;
        beginInfo.flags = vk::CommandBufferUsageFlagBits::eOneTimeSubmit;
        commandBuffer.begin(beginInfo);

        vk::BufferCopy copyRegion;
        copyRegion.srcOffset = 0; // optional
        copyRegion.dstOffset = 0; // optional
        copyRegion.size = size;
        commandBuffer.copyBuffer(srcBuffer, dstBuffer, copyRegion);
        commandBuffer.end();
        vk::SubmitInfo submitInfo;
        submitInfo.setCommandBuffers( *commandBuffer );

        m_graphicsQueue.submit(submitInfo);
        m_graphicsQueue.waitIdle();
    }
    void createParticleBuffer() {
        // 随机生成 PARTICLE_COUNT 个粒子，分布在一个半径为 0.25 的圆内，随机向外的初速度
        std::random_device rd;
        std::default_random_engine rndEngine(rd());
        std::uniform_real_distribution<float> rndDist(0.0f, 1.0f);
        std::vector<Particle> particles(PARTICLE_COUNT);
        for (auto&[pos, velocity, color] : particles) {
            const float r = 0.25f * std::sqrt(rndDist(rndEngine));
            const float theta = rndDist(rndEngine) * 2.0f * 3.141592653f;
            const float x = r * std::cos(theta) * HEIGHT / WIDTH;
            const float y = r * std::sin(theta);
            pos = glm::vec2(x, y);
            velocity = glm::normalize(glm::vec2(x,y)) * 0.00025f;
            color = glm::vec4(rndDist(rndEngine), rndDist(rndEngine), rndDist(rndEngine), 1.0f);
        }


        const vk::DeviceSize bufferSize = sizeof(Particle) * particles.size();

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
        memcpy(data, particles.data(), bufferSize);
        stagingBufferMemory.unmapMemory();

        createBuffer(bufferSize,
            vk::BufferUsageFlagBits::eStorageBuffer | // 添加存储缓冲区标志
            vk::BufferUsageFlagBits::eTransferDst |
            vk::BufferUsageFlagBits::eVertexBuffer,
            vk::MemoryPropertyFlagBits::eDeviceLocal,
            m_particleBuffer,
            m_particleBufferMemory
        );

        copyBuffer(stagingBuffer, m_particleBuffer, bufferSize);
    }
    /////////////////////////////////////////////////////////////

    /////////////////////////////////////////////////////////////
    /// 获取时间差
    static float getDeltaTime() {
        static auto lastTime = std::chrono::steady_clock::now();
        const auto currentTime = std::chrono::steady_clock::now();
        const float res =  std::chrono::duration<float, std::chrono::milliseconds::period>(currentTime - lastTime).count();
        lastTime = currentTime;
        return res;
    }
    /////////////////////////////////////////////////////////////
    /// 描述符
    void createDescriptorSetLayout() {
        vk::DescriptorSetLayoutBinding layoutBinding; // Storage Buffer 粒子数据
        layoutBinding.binding = 0;
        layoutBinding.descriptorCount = 1;
        layoutBinding.descriptorType = vk::DescriptorType::eStorageBuffer; // 存储缓冲区
        layoutBinding.stageFlags = vk::ShaderStageFlagBits::eCompute; // 计算着色器
        vk::DescriptorSetLayoutCreateInfo layoutInfo;
        layoutInfo.setBindings( layoutBinding );
        m_descriptorSetLayout = m_device.createDescriptorSetLayout( layoutInfo );
    }
    void createDescriptorPool() {
        vk::DescriptorPoolSize poolSize;
        poolSize.type = vk::DescriptorType::eStorageBuffer; // 存储缓冲区
        poolSize.descriptorCount = 1;

        vk::DescriptorPoolCreateInfo poolInfo;
        // raii 封装需要使用 eFreeDescriptorSet 标志位
        poolInfo.flags = vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet;
        poolInfo.maxSets = 1;
        poolInfo.setPoolSizes( poolSize );

        m_descriptorPool = m_device.createDescriptorPool( poolInfo );
    }
    void createDescriptorSets() {
        vk::DescriptorSetAllocateInfo allocInfo;
        allocInfo.setDescriptorPool( m_descriptorPool );
        allocInfo.setSetLayouts( *m_descriptorSetLayout ); // 需要至少一次 * 显式转换

        // 此描述符集数组仅单个元素
        m_descriptorSets = m_device.allocateDescriptorSets( allocInfo );

        vk::DescriptorBufferInfo particleBufferInfo;
        // 绑定上一个缓冲区
        particleBufferInfo.buffer = m_particleBuffer;
        particleBufferInfo.offset = 0;
        particleBufferInfo.range = sizeof(Particle) * PARTICLE_COUNT;

        vk::WriteDescriptorSet descriptorWrite;
        descriptorWrite.dstSet = m_descriptorSets.at(0);
        descriptorWrite.dstBinding = 0;
        descriptorWrite.descriptorType = vk::DescriptorType::eStorageBuffer; // 存储缓冲区类型
        descriptorWrite.setBufferInfo( particleBufferInfo );

        m_device.updateDescriptorSets(descriptorWrite, nullptr);
    }
    /////////////////////////////////////////////////////////////

    /////////////////////////////////////////////////////////////
    /// 计算管线
    void createComputePipeline() {

        // 推送常量配置
        vk::PushConstantRange pushConstantRange;
        pushConstantRange.stageFlags = vk::ShaderStageFlagBits::eCompute; // 计算着色器阶段
        pushConstantRange.offset = 0; // 偏移量
        pushConstantRange.size = sizeof(float); // 大小为 float

        // 创建管线布局
        vk::PipelineLayoutCreateInfo pipelineLayoutInfo;
        pipelineLayoutInfo.setPushConstantRanges( pushConstantRange ); // 设置推送常量范围
        pipelineLayoutInfo.setSetLayouts( *m_descriptorSetLayout ); // 设置描述符布局
        m_computePipelineLayout = m_device.createPipelineLayout(pipelineLayoutInfo);

        // 读取并创建着色器模块
        const auto computeShaderCode = readFile("shaders/compute.comp.spv");
        vk::raii::ShaderModule computeShaderModule = createShaderModule(computeShaderCode);

        // 设置计算着色器阶段信息
        vk::PipelineShaderStageCreateInfo computeShaderStageInfo;
        computeShaderStageInfo.stage = vk::ShaderStageFlagBits::eCompute;
        computeShaderStageInfo.module = computeShaderModule;
        computeShaderStageInfo.pName = "main";

        // 创建计算管线
        vk::ComputePipelineCreateInfo computePipelineInfo;
        computePipelineInfo.stage = computeShaderStageInfo;
        computePipelineInfo.layout = m_computePipelineLayout;
        m_computePipeline = m_device.createComputePipeline(nullptr, computePipelineInfo);
    }
    /////////////////////////////////////////////////////////////

    /////////////////////////////////////////////////////////////
    /// 命令录制
    void createComputeCommandBuffers() {
        vk::CommandBufferAllocateInfo allocInfo;
        allocInfo.commandPool = m_commandPool;
        allocInfo.level = vk::CommandBufferLevel::ePrimary;
        allocInfo.commandBufferCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);

        m_computeCommandBuffers = m_device.allocateCommandBuffers(allocInfo);
    }
    void recordComputeCommandBuffer(const vk::raii::CommandBuffer& commandBuffer) {
        constexpr vk::CommandBufferBeginInfo beginInfo;
        commandBuffer.begin(beginInfo);

        // 绑定计算管线
        commandBuffer.bindPipeline(vk::PipelineBindPoint::eCompute, m_computePipeline);

        // 绑定描述符集
        commandBuffer.bindDescriptorSets(
            vk::PipelineBindPoint::eCompute,
            m_computePipelineLayout,
            0,
            *m_descriptorSets.at(0),
            nullptr
        );

        const float deltaTime = getDeltaTime(); // 获取时间差
        commandBuffer.pushConstants<float>(
            m_computePipelineLayout,
            vk::ShaderStageFlagBits::eCompute, // 计算着色器阶段
            0, // 偏移量
            deltaTime
        );

        // 调用计算着色器
        commandBuffer.dispatch((PARTICLE_COUNT + 255) / 256, 1, 1);
        commandBuffer.end();
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
