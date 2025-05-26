#include <vulkan/vulkan.hpp>
#include <vulkan/vulkan_raii.hpp>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>

#include <iostream>
#include <fstream>
#include <vector>
#include <array>
#include <set>
#include <limits>
#include <algorithm>
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
    static constexpr std::array<const char*,1> deviceExtensions {
        VK_KHR_SWAPCHAIN_EXTENSION_NAME
    };

    #ifdef NDEBUG
        static const bool enableValidationLayers = false;
    #else
        static const bool enableValidationLayers = true;
    #endif

    static constexpr int MAX_FRAMES_IN_FLIGHT = 2;
    /////////////////////////////////////////////////////////////////

    /////////////////////////////////////////////////////////////////
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
    vk::raii::Buffer m_vertexBuffer{ nullptr };
    vk::raii::DeviceMemory m_vertexBufferMemory{ nullptr };
    vk::raii::SwapchainKHR m_swapChain{ nullptr };
    std::vector<vk::Image> m_swapChainImages;
    vk::Format m_swapChainImageFormat;
    vk::Extent2D m_swapChainExtent;
    std::vector<vk::raii::ImageView> m_swapChainImageViews;
    vk::raii::RenderPass m_renderPass{ nullptr };
    vk::raii::PipelineLayout m_pipelineLayout{ nullptr };
    vk::raii::Pipeline m_graphicsPipeline{ nullptr };
    std::vector<vk::raii::Framebuffer> m_swapChainFramebuffers;
    vk::raii::CommandPool m_commandPool{ nullptr };
    std::vector<vk::raii::CommandBuffer> m_commandBuffers;
    std::vector<vk::raii::Semaphore> m_imageAvailableSemaphores;
    std::vector<vk::raii::Semaphore> m_renderFinishedSemaphores;
    std::vector<vk::raii::Fence> m_inFlightFences;
    uint32_t currentFrame = 0;
    bool m_framebufferResized = false;
    /////////////////////////////////////////////////////////////////
    
    /////////////////////////////////////////////////////////////////
    /// run()
    void initWindow() {
        // initialize glfw lib
        glfwInit();
        
        // Configure GLFW to not use OpenGL
        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
        
        // Create window
        m_window = glfwCreateWindow(WIDTH, HEIGHT, "Vulkan", nullptr, nullptr);
        glfwSetWindowUserPointer(m_window, this);
        glfwSetFramebufferSizeCallback(m_window, framebufferResizeCallback);
    }

    void initVulkan() {
        createInstance();
        setupDebugMessenger();
        createSurface();
        pickPhysicalDevice();
        createLogicalDevice();
        createSwapChain();
        createImageViews();
        createRenderPass();
        createGraphicsPipeline();
        createFramebuffers();
        createCommandPool();
        createVertexBuffer();
        createCommandBuffers();
        createSyncObjects();
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
    bool checkDeviceExtensionSupport(const vk::raii::PhysicalDevice& physicalDevice) {
        // std::vector<vk::ExtensionProperties>
        auto availableExtensions = physicalDevice.enumerateDeviceExtensionProperties();
        std::set<std::string> requiredExtensions(deviceExtensions.begin(), deviceExtensions.end());
        for (const auto& extension : availableExtensions) {
            requiredExtensions.erase(extension.extensionName);
        }
        return requiredExtensions.empty();
    }
    bool isDeviceSuitable(const vk::raii::PhysicalDevice& physicalDevice) {
        QueueFamilyIndices indices = findQueueFamilies(physicalDevice);

        bool extensionsSupported = checkDeviceExtensionSupport(physicalDevice);

        bool swapChainAdequate = false;
        if (extensionsSupported) {
            SwapChainSupportDetails swapChainSupport = querySwapChainSupport(physicalDevice);
            swapChainAdequate = !swapChainSupport.formats.empty() && !swapChainSupport.presentModes.empty();
        }

        return indices.isComplete() && extensionsSupported && swapChainAdequate;
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
        std::optional<uint32_t> presentFamily;

        bool isComplete() {
            return graphicsFamily.has_value() && presentFamily.has_value();
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
            if(physicalDevice.getSurfaceSupportKHR(i, m_surface)){
                indices.presentFamily = i;
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

        std::vector<vk::DeviceQueueCreateInfo> queueCreateInfos;
        // isDeviceSuitable() ensure queue availability
        std::set<uint32_t> uniqueQueueFamilies = {indices.graphicsFamily.value(), indices.presentFamily.value()};

        float queuePriority = 1.0f;
        for (uint32_t queueFamily : uniqueQueueFamilies) {
            queueCreateInfos.emplace_back( vk::DeviceQueueCreateInfo(
                {},                             // flags
                indices.graphicsFamily.value(), // queueFamilyIndex
                1,                              // queueCount
                &queuePriority    
            ));
        }

        vk::PhysicalDeviceFeatures deviceFeatures;

        vk::DeviceCreateInfo createInfo;
        createInfo.setQueueCreateInfos( queueCreateInfos );
        createInfo.pEnabledFeatures = &deviceFeatures;

        if (enableValidationLayers) {
            createInfo.setPEnabledLayerNames( validationLayers );
        }

        createInfo.setPEnabledExtensionNames( deviceExtensions );

        m_device = m_physicalDevice.createDevice( createInfo );
        m_graphicsQueue = m_device.getQueue( indices.graphicsFamily.value(), 0 );
        m_presentQueue = m_device.getQueue( indices.presentFamily.value(), 0 );
    }
    /////////////////////////////////////////////////////////////////

    /////////////////////////////////////////////////////////////////
    /// surface
    void createSurface() {
        VkSurfaceKHR cSurface;

        if (glfwCreateWindowSurface( *m_instance, m_window, nullptr, &cSurface ) != VK_SUCCESS) {
            throw std::runtime_error("failed to create window surface!");
        }
        m_surface = vk::raii::SurfaceKHR( m_instance, cSurface );
    }
    /////////////////////////////////////////////////////////////////

    /////////////////////////////////////////////////////////////////
    /// swapchain
    struct SwapChainSupportDetails {
        vk::SurfaceCapabilitiesKHR capabilities;
        std::vector<vk::SurfaceFormatKHR>  formats;
        std::vector<vk::PresentModeKHR> presentModes;
    };
    vk::SurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<vk::SurfaceFormatKHR>& availableFormats) {
        for (const auto& availableFormat : availableFormats) {
            if (availableFormat.format == vk::Format::eB8G8R8A8Srgb && 
                availableFormat.colorSpace == vk::ColorSpaceKHR::eSrgbNonlinear) {
                return availableFormat;
            }
        }
        return availableFormats.at(0);
    }
    vk::PresentModeKHR chooseSwapPresentMode(const std::vector<vk::PresentModeKHR>& availablePresentModes) {
        for (const auto& availablePresentMode : availablePresentModes) {
            if (availablePresentMode == vk::PresentModeKHR::eMailbox) {
                return availablePresentMode;
            }
        }
        return vk::PresentModeKHR::eFifo;
    }
    vk::Extent2D chooseSwapExtent(const vk::SurfaceCapabilitiesKHR& capabilities) {
        if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max()) {
            return capabilities.currentExtent;
        } else {
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
    }
    SwapChainSupportDetails querySwapChainSupport(const vk::raii::PhysicalDevice& physicalDevice) {
        SwapChainSupportDetails details;

        details.capabilities = physicalDevice.getSurfaceCapabilitiesKHR( m_surface );
        details.formats = physicalDevice.getSurfaceFormatsKHR( m_surface );
        details.presentModes = physicalDevice.getSurfacePresentModesKHR( m_surface );

        return details;
    }
    void createSwapChain() {
        SwapChainSupportDetails swapChainSupport = querySwapChainSupport( m_physicalDevice );

        vk::SurfaceFormatKHR surfaceFormat = chooseSwapSurfaceFormat( swapChainSupport.formats );
        vk::PresentModeKHR presentMode = chooseSwapPresentMode( swapChainSupport.presentModes );
        vk::Extent2D extent = chooseSwapExtent( swapChainSupport.capabilities );

        uint32_t imageCount = swapChainSupport.capabilities.minImageCount + 1;

        if (swapChainSupport.capabilities.maxImageCount > 0 && 
            imageCount > swapChainSupport.capabilities.maxImageCount) {
            imageCount = swapChainSupport.capabilities.maxImageCount;
        }

        vk::SwapchainCreateInfoKHR createInfo(
            {},                         // flags
            m_surface,                  // vk::Surface
            imageCount,                 // minImageCount
            surfaceFormat.format,       // Format
            surfaceFormat.colorSpace,   // ColorSpaceKHR
            extent,                     // Extent2D
            1,                          // imageArrayLayers
            vk::ImageUsageFlagBits::eColorAttachment    // imageUsage
        );

        QueueFamilyIndices indices = findQueueFamilies( m_physicalDevice );
        std::vector<uint32_t> queueFamilyIndices { indices.graphicsFamily.value(), indices.presentFamily.value() };

        if (indices.graphicsFamily != indices.presentFamily) {
            createInfo.imageSharingMode = vk::SharingMode::eConcurrent;
            createInfo.setQueueFamilyIndices( queueFamilyIndices );
        } else {
            createInfo.imageSharingMode = vk::SharingMode::eExclusive;
        }

        createInfo.preTransform = swapChainSupport.capabilities.currentTransform;
        createInfo.compositeAlpha = vk::CompositeAlphaFlagBitsKHR::eOpaque;
        createInfo.presentMode = presentMode;
        createInfo.clipped = true;
        createInfo.oldSwapchain = nullptr;

        m_swapChain = m_device.createSwapchainKHR( createInfo );
        m_swapChainImages = m_swapChain.getImages();
        m_swapChainImageFormat = surfaceFormat.format;
        m_swapChainExtent = extent;
    }
    /////////////////////////////////////////////////////////////////

    /////////////////////////////////////////////////////////////////
    /// imageview
    void createImageViews() {
        m_swapChainImageViews.reserve( m_swapChainImages.size() );
        for (size_t i = 0; i < m_swapChainImages.size(); i++) {
            vk::ImageViewCreateInfo createInfo(
                {},                     // flags
                m_swapChainImages[i],   // vk::Image
                vk::ImageViewType::e2D, // ImageViewType
                m_swapChainImageFormat  // format
            );
            createInfo.subresourceRange.aspectMask = vk::ImageAspectFlagBits::eColor;
            createInfo.subresourceRange.baseMipLevel = 0;
            createInfo.subresourceRange.levelCount = 1;
            createInfo.subresourceRange.baseArrayLayer = 0;
            createInfo.subresourceRange.layerCount = 1;

            m_swapChainImageViews.emplace_back( m_device.createImageView(createInfo) );
        }
    }
    /////////////////////////////////////////////////////////////////

    /////////////////////////////////////////////////////////////////
    /// render pass
    void createRenderPass() {
        vk::AttachmentDescription colorAttachment(
            {},                                 // flags
            m_swapChainImageFormat,             // format
            vk::SampleCountFlagBits::e1,        // samples
            vk::AttachmentLoadOp::eClear,       // loadOp
            vk::AttachmentStoreOp::eStore,      // storeOp
            vk::AttachmentLoadOp::eDontCare,    // stencilLoadOp 
            vk::AttachmentStoreOp::eDontCare,   // stencilStoreOp 
            vk::ImageLayout::eUndefined,        // initialLayout 
            vk::ImageLayout::ePresentSrcKHR     // finalLayout 
        );

        vk::AttachmentReference colorAttachmentRef(
            0,      // attachment  and   layout 
            vk::ImageLayout::eColorAttachmentOptimal
        );

        vk::SubpassDescription subpass;
        subpass.pipelineBindPoint = vk::PipelineBindPoint::eGraphics;
        subpass.setColorAttachments( colorAttachmentRef );

        vk::RenderPassCreateInfo renderPassInfo;
        renderPassInfo.setAttachments( colorAttachment );
        renderPassInfo.setSubpasses( subpass );

        vk::SubpassDependency dependency;
        dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
        dependency.dstSubpass = 0;

        dependency.srcStageMask = vk::PipelineStageFlagBits::eColorAttachmentOutput;
        dependency.srcAccessMask = {};

        dependency.dstStageMask = vk::PipelineStageFlagBits::eColorAttachmentOutput;
        dependency.dstAccessMask = vk::AccessFlagBits::eColorAttachmentWrite;

        renderPassInfo.setDependencies( dependency );

        
        m_renderPass = m_device.createRenderPass(renderPassInfo);
    }
    /////////////////////////////////////////////////////////////////

    /////////////////////////////////////////////////////////////////
    /// pipeline
    static std::vector<char> readFile(const std::string& filename) {
        std::ifstream file(filename, std::ios::ate | std::ios::binary);

        if (!file.is_open()) {
            throw std::runtime_error("failed to open file!");
        }

        size_t fileSize = (size_t) file.tellg();
        std::vector<char> buffer(fileSize);
        file.seekg(0);
        file.read(buffer.data(), fileSize);
        file.close();

        return buffer;
    }
    vk::raii::ShaderModule createShaderModule(const std::vector<char>& code) {
        vk::ShaderModuleCreateInfo createInfo(
            {},             // flags
            code.size(),    // codeSize
            reinterpret_cast<const uint32_t*>(code.data()) // pCode
        );

        return m_device.createShaderModule( createInfo );
    }
    void createGraphicsPipeline() {
        auto vertShaderCode = readFile("shaders/vert.spv");
        auto fragShaderCode = readFile("shaders/frag.spv");

        vk::raii::ShaderModule vertShaderModule = createShaderModule(vertShaderCode);
        vk::raii::ShaderModule fragShaderModule = createShaderModule(fragShaderCode);

        vk::PipelineShaderStageCreateInfo vertShaderStageInfo(
            {},                                 // flags
            vk::ShaderStageFlagBits::eVertex,   // stage
            vertShaderModule,                   // ShaderModule
            "main"                              // pName
        );

        vk::PipelineShaderStageCreateInfo fragShaderStageInfo(
            {},                                 // flags
            vk::ShaderStageFlagBits::eFragment, // stage
            fragShaderModule,                   // ShaderModule
            "main"                              // pName
        );

        std::vector<vk::PipelineShaderStageCreateInfo> shaderStages{ vertShaderStageInfo, fragShaderStageInfo };
    
        std::vector<vk::DynamicState> dynamicStates = {
            vk::DynamicState::eViewport,
            vk::DynamicState::eScissor
        };

        vk::PipelineDynamicStateCreateInfo dynamicState;
        dynamicState.setDynamicStates( dynamicStates );

        vk::PipelineVertexInputStateCreateInfo vertexInputInfo;

        auto bindingDescription = Vertex::getBindingDescription();
        auto attributeDescriptions = Vertex::getAttributeDescriptions();
        vertexInputInfo.setVertexBindingDescriptions(bindingDescription);
        vertexInputInfo.setVertexAttributeDescriptions(attributeDescriptions);

        vk::PipelineInputAssemblyStateCreateInfo inputAssembly(
            {},                                     // flags
            vk::PrimitiveTopology::eTriangleList,   // topology
            false                                   // primitiveRestartEnable - default false
        );

        vk::Viewport viewport(
            0.0f, 0.0f,                                     // x y
            static_cast<float>(m_swapChainExtent.width),    // width
            static_cast<float>(m_swapChainExtent.height),   // height
            0.0f, 1.0f                                      // minDepth maxDepth
        );

        vk::Rect2D scissor(
            {0, 0},             // offset
            m_swapChainExtent   // Extent2D
        );

        vk::PipelineViewportStateCreateInfo viewportState;
        viewportState.setViewports( viewport );
        viewportState.setScissors( scissor );

        vk::PipelineRasterizationStateCreateInfo rasterizer;
        rasterizer.depthClampEnable = false;
        rasterizer.rasterizerDiscardEnable = false;
        rasterizer.polygonMode = vk::PolygonMode::eFill;
        rasterizer.lineWidth = 1.0f;
        rasterizer.cullMode = vk::CullModeFlagBits::eBack;
        rasterizer.frontFace = vk::FrontFace::eClockwise;
        rasterizer.depthBiasEnable = false;

        vk::PipelineMultisampleStateCreateInfo multisampling(
            {},                             // flags
            vk::SampleCountFlagBits::e1,    //  rasterizationSamples
            false                           // sampleShadingEnable
        );

        vk::PipelineColorBlendAttachmentState colorBlendAttachment(
            false,                      // blendEnable 
            vk::BlendFactor::eOne,      // srcColorBlendFactor - optional
            vk::BlendFactor::eZero,     // dstColorBlendFactor - optional
            vk::BlendOp::eAdd,          // colorBlendOp - optional
            vk::BlendFactor::eOne,      // srcAlphaBlendFactor - optional
            vk::BlendFactor::eZero,     // dstAlphaBlendFactor - optional
            vk::BlendOp::eAdd,          // alphaBlendOp - optional
            vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG |
            vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA
            // colorWriteMask - default is RGBA
        );

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

        pipelineInfo.basePipelineHandle = nullptr; // Optional
        pipelineInfo.basePipelineIndex = -1; // Optional

        m_graphicsPipeline = m_device.createGraphicsPipeline( nullptr, pipelineInfo );
    }
    /////////////////////////////////////////////////////////////////

    /////////////////////////////////////////////////////////////////
    /// frame buffer
    void createFramebuffers() {
        m_swapChainFramebuffers.reserve( m_swapChainImageViews.size() );

        for (size_t i = 0; i < m_swapChainImageViews.size(); i++) {
            vk::FramebufferCreateInfo framebufferInfo;
            framebufferInfo.renderPass = m_renderPass;
            framebufferInfo.setAttachments( *m_swapChainImageViews[i] );
            framebufferInfo.width = m_swapChainExtent.width;
            framebufferInfo.height = m_swapChainExtent.height;
            framebufferInfo.layers = 1;

            m_swapChainFramebuffers.emplace_back( m_device.createFramebuffer(framebufferInfo) );
        }
    }
    /////////////////////////////////////////////////////////////////

    /////////////////////////////////////////////////////////////////
    /// command pool and buffer
    void createCommandPool() {
        QueueFamilyIndices queueFamilyIndices = findQueueFamilies( m_physicalDevice );

        vk::CommandPoolCreateInfo poolInfo(
            vk::CommandPoolCreateFlagBits::eResetCommandBuffer, // flags
            queueFamilyIndices.graphicsFamily.value()
        );

        m_commandPool = m_device.createCommandPool( poolInfo );
    }
    void createCommandBuffers() {
        vk::CommandBufferAllocateInfo allocInfo(
            m_commandPool,                      // command pool
            vk::CommandBufferLevel::ePrimary,   // level
            MAX_FRAMES_IN_FLIGHT                // commandBufferCount
        );
        m_commandBuffers = m_device.allocateCommandBuffers(allocInfo);
    }
    void recordCommandBuffer(const vk::raii::CommandBuffer& commandBuffer, uint32_t imageIndex) {
        vk::CommandBufferBeginInfo beginInfo;
        commandBuffer.begin( beginInfo );

        vk::RenderPassBeginInfo renderPassInfo(
            m_renderPass,                       // renderPass
            m_swapChainFramebuffers[imageIndex] // framebuffer
        );
        renderPassInfo.renderArea.offset = vk::Offset2D{0, 0};
        renderPassInfo.renderArea.extent = m_swapChainExtent;

        vk::ClearValue clearColor(vk::ClearColorValue(0.0f, 0.0f, 0.0f, 1.0f));
        renderPassInfo.setClearValues( clearColor );

        commandBuffer.beginRenderPass(renderPassInfo, vk::SubpassContents::eInline);

        commandBuffer.bindPipeline( vk::PipelineBindPoint::eGraphics, m_graphicsPipeline );

        vk::Viewport viewport(
            0.0f, 0.0f, // x, y
            static_cast<float>(m_swapChainExtent.width),    // width
            static_cast<float>(m_swapChainExtent.height),   // height
            0.0f, 1.0f  // minDepth maxDepth
        );
        commandBuffer.setViewport(0, viewport);


        vk::Rect2D scissor(
            vk::Offset2D{0, 0}, // offset
            m_swapChainExtent   // extent
        );
        commandBuffer.setScissor(0, scissor);

        vk::Buffer vertexBuffers[] = { m_vertexBuffer };
        vk::DeviceSize offsets[] = { 0 };
        commandBuffer.bindVertexBuffers( 0, vertexBuffers, offsets );

        commandBuffer.draw(static_cast<uint32_t>(vertices.size()), 1, 0, 0);

        commandBuffer.endRenderPass();
        commandBuffer.end();
    }
    /////////////////////////////////////////////////////////////////

    /////////////////////////////////////////////////////////////////
    /// render and present
    void createSyncObjects() {
        vk::SemaphoreCreateInfo semaphoreInfo;
        vk::FenceCreateInfo fenceInfo(
            vk::FenceCreateFlagBits::eSignaled  // flags
        );

        for(size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i){
            m_imageAvailableSemaphores.emplace_back( m_device, semaphoreInfo );
            m_renderFinishedSemaphores.emplace_back( m_device, semaphoreInfo );
            m_inFlightFences.emplace_back( m_device , fenceInfo );
        }
    }
    void drawFrame() {
        if( auto res = m_device.waitForFences( *m_inFlightFences[currentFrame], true, UINT64_MAX );
            res != vk::Result::eSuccess){
            throw std::runtime_error{"waitForFences error"};
        }

        uint32_t imageIndex;
        try{
            // std::pair<vk::Result, uint32_t>
            auto [res, idx] = m_swapChain.acquireNextImage(UINT64_MAX, m_imageAvailableSemaphores[currentFrame]);
            imageIndex = idx;
        } catch (const vk::OutOfDateKHRError&){
                recreateSwapChain();
                return;
        } // Do not catch other exceptions

        // Only reset the fence if we are submitting work
        m_device.resetFences( *m_inFlightFences[currentFrame] );

        m_commandBuffers[currentFrame].reset();
        recordCommandBuffer(m_commandBuffers[currentFrame], imageIndex);

        vk::SubmitInfo submitInfo;

        submitInfo.setWaitSemaphores( *m_imageAvailableSemaphores[currentFrame] );
        std::vector<vk::PipelineStageFlags> waitStages { vk::PipelineStageFlagBits::eColorAttachmentOutput };
        submitInfo.setWaitDstStageMask( waitStages );

        submitInfo.setCommandBuffers( *m_commandBuffers[currentFrame] );

        submitInfo.setSignalSemaphores( *m_renderFinishedSemaphores[currentFrame] );

        m_graphicsQueue.submit(submitInfo, m_inFlightFences[currentFrame]);

        vk::PresentInfoKHR presentInfo;
        presentInfo.setWaitSemaphores( *m_renderFinishedSemaphores[currentFrame] );

        presentInfo.setSwapchains( *m_swapChain );
        presentInfo.pImageIndices = &imageIndex;

        try{
            auto res = m_presentQueue.presentKHR(presentInfo);
            if( res == vk::Result::eSuboptimalKHR ) {
                recreateSwapChain();
            }
        } catch (const vk::OutOfDateKHRError&){
            recreateSwapChain();
        }
        if( m_framebufferResized ){
            recreateSwapChain();
        }

        currentFrame = (currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
    }
    /////////////////////////////////////////////////////////////////

    /////////////////////////////////////////////////////////////////
    /// recreate swapchain
    static void framebufferResizeCallback(GLFWwindow* window, int width, int height) {
        auto app = reinterpret_cast<HelloTriangleApplication*>(glfwGetWindowUserPointer(window));
        app->m_framebufferResized = true;
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
        // m_swapChainImages.clear(); // optional
        m_swapChain = nullptr;

        createSwapChain();
        createImageViews();
        createFramebuffers();

        m_framebufferResized = false;
    }
    /////////////////////////////////////////////////////////////////

    /////////////////////////////////////////////////////////////////
    /// Vertex input
    struct Vertex {
        glm::vec2 pos;
        glm::vec3 color;

        static vk::VertexInputBindingDescription getBindingDescription() {
            vk::VertexInputBindingDescription bindingDescription;
            bindingDescription.binding = 0;
            bindingDescription.stride = sizeof(Vertex);
            bindingDescription.inputRate = vk::VertexInputRate::eVertex;

            return bindingDescription;
        }

        static std::array<vk::VertexInputAttributeDescription, 2>  getAttributeDescriptions() {
            std::array<vk::VertexInputAttributeDescription, 2> attributeDescriptions;

            attributeDescriptions[0].binding = 0;
            attributeDescriptions[0].location = 0;
            attributeDescriptions[0].format = vk::Format::eR32G32Sfloat;
            attributeDescriptions[0].offset = offsetof(Vertex, pos);

            attributeDescriptions[1].binding = 0;
            attributeDescriptions[1].location = 1;
            attributeDescriptions[1].format = vk::Format::eR32G32B32Sfloat;
            attributeDescriptions[1].offset = offsetof(Vertex, color);

            return attributeDescriptions;
        }
    };
    inline static const std::vector<Vertex> vertices = {
        {{0.0f, -0.5f}, {1.0f, 1.0f, 1.0f}},
        {{0.5f, 0.5f}, {0.0f, 1.0f, 0.0f}},
        {{-0.5f, 0.5f}, {0.0f, 0.0f, 1.0f}}
    };
    /////////////////////////////////////////////////////////////////

    /////////////////////////////////////////////////////////////////
    /// create vertex buffer
    void createVertexBuffer() {
        vk::BufferCreateInfo bufferInfo;
        bufferInfo.size = sizeof(vertices[0]) * vertices.size();
        bufferInfo.usage = vk::BufferUsageFlagBits::eVertexBuffer;
        bufferInfo.sharingMode = vk::SharingMode::eExclusive;

        m_vertexBuffer = m_device.createBuffer(bufferInfo);

        vk::MemoryRequirements memRequirements = m_vertexBuffer.getMemoryRequirements();

        vk::MemoryAllocateInfo allocInfo;
        allocInfo.allocationSize = memRequirements.size;
        allocInfo.memoryTypeIndex = findMemoryType( memRequirements.memoryTypeBits,
            vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent 
        );

        m_vertexBufferMemory = m_device.allocateMemory( allocInfo );

        m_vertexBuffer.bindMemory(m_vertexBufferMemory, 0);

        void* data;
        data = m_vertexBufferMemory.mapMemory(0, bufferInfo.size);
        memcpy(data, vertices.data(), static_cast<size_t>(bufferInfo.size));
        m_vertexBufferMemory.unmapMemory();
    }
    uint32_t findMemoryType(uint32_t typeFilter, vk::MemoryPropertyFlags properties) {
        // vk::PhysicalDeviceMemoryProperties
        auto memProperties = m_physicalDevice.getMemoryProperties();

        for(uint32_t i = 0; i < memProperties.memoryTypeCount; ++i){
            if( (typeFilter & (1 << i)) &&
                (memProperties.memoryTypes[i].propertyFlags & properties ) == properties ) {
                return i;
            }
        }
        throw std::runtime_error("failed to find suitable memory type!");
        return 0; // optional
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
