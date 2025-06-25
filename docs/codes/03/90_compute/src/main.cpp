/**
 * @file main.cpp
 * @brief 计算着色器与 SSBO 章节的基础模板代码
 * @details
 * 内容参考“顶点缓冲”章节的结束代码，主要做以下修改：
 * 1. 将 `Vertex` 结构体替换为 `Particle` 结构体。
 * 2. 将 顶点缓冲vertexBuffer 改名为 粒子缓冲particleBuffer。
 * 3. Particle 结构体包含位置、速度和颜色属性。
 * 4. 修改图形管线布局，输入装配点列表，而非三角形顶点列表
 */
#include <vulkan/vulkan.hpp>
#include <vulkan/vulkan_raii.hpp>
#include <GLFW/glfw3.h>
#define GLM_FORCE_RADIANS
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

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
#include <random>
#include <chrono>

class HelloComputeShader {
public:
    void run() {
        initWindow();
        initVulkan();
        mainLoop();
        cleanup();
    }

private:
    /////////////////////////////////////////////////////////////////
    /// static values 常量区域
    static constexpr uint32_t WIDTH = 800;
    static constexpr uint32_t HEIGHT = 600;

    static constexpr uint32_t PARTICLE_COUNT = 4096;

    inline static const std::vector<const char*> validationLayers {
        "VK_LAYER_KHRONOS_validation"
    };
    inline static const std::vector<const char*> deviceExtensions {
        vk::KHRSwapchainExtensionName
    };

    #ifdef NDEBUG
        static constexpr bool enableValidationLayers = false;
    #else
        static constexpr bool enableValidationLayers = true;
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
    vk::raii::Queue m_computeQueue{ nullptr };
    std::vector<vk::raii::DeviceMemory> m_particleBufferMemory;
    std::vector<vk::raii::Buffer> m_particleBuffers;
    std::vector<vk::raii::DeviceMemory> m_uniformBuffersMemory;
    std::vector<vk::raii::Buffer> m_uniformBuffers;
    std::vector<void*> m_uniformBuffersMapped;
    vk::raii::SwapchainKHR m_swapChain{ nullptr };
    std::vector<vk::Image> m_swapChainImages;
    vk::Format m_swapChainImageFormat;
    vk::Extent2D m_swapChainExtent;
    std::vector<vk::raii::ImageView> m_swapChainImageViews;
    vk::raii::RenderPass m_renderPass{ nullptr };
    vk::raii::DescriptorSetLayout m_descriptorSetLayout{ nullptr };
    vk::raii::DescriptorPool m_descriptorPool{ nullptr };
    std::vector<vk::raii::DescriptorSet> m_descriptorSets;
    vk::raii::PipelineLayout m_pipelineLayout{ nullptr };
    vk::raii::Pipeline m_graphicsPipeline{ nullptr };
    vk::raii::PipelineLayout m_computePipelineLayout{ nullptr };
    vk::raii::Pipeline m_computePipeline{ nullptr };
    std::vector<vk::raii::Framebuffer> m_swapChainFramebuffers;
    vk::raii::CommandPool m_commandPool{ nullptr };
    std::vector<vk::raii::CommandBuffer> m_commandBuffers;
    std::vector<vk::raii::CommandBuffer> m_computeCommandBuffers;
    std::vector<vk::raii::Semaphore> m_imageAvailableSemaphores;
    std::vector<vk::raii::Semaphore> m_renderFinishedSemaphores;
    std::vector<vk::raii::Fence> m_inFlightFences;
    std::vector<vk::raii::Semaphore> m_computeFinishedSemaphores;
    std::vector<vk::raii::Fence> m_computeInFlightFences;
    uint32_t m_currentFrame = 0;
    bool m_framebufferResized = false;
    /////////////////////////////////////////////////////////////////

    /////////////////////////////////////////////////////////////////
    /// run()
    void initWindow() {
        glfwInit();
        
        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

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
        createDescriptorSetLayout();
        createGraphicsPipeline();
        createComputePipeline();
        createFramebuffers();
        createCommandPool();
        createParticleBuffers();
        createUniformBuffers();
        createDescriptorPool();
        createDescriptorSets();
        createCommandBuffers();
        createComputeCommandBuffers();
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
        for (const auto& memory : m_uniformBuffersMemory) {
           memory.unmapMemory();
        }
        glfwDestroyWindow( m_window );
        glfwTerminate();
    }
    /////////////////////////////////////////////////////////////////

    /////////////////////////////////////////////////////////////////
    /// instance creation 创建 Vulkan 实例 未修改
    std::vector<const char*> getRequiredExtensions() {
        uint32_t glfwExtensionCount = 0;
        const char** glfwExtensions;
        glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

        std::vector<const char*> extensions(glfwExtensions, glfwExtensions + glfwExtensionCount);

        if (enableValidationLayers) {
            extensions.emplace_back( vk::EXTDebugUtilsExtensionName );
        }
        extensions.emplace_back( vk::KHRPortabilityEnumerationExtensionName );

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
            vk::makeApiVersion(0, 1, 4, 0)  // apiVersion
        );
        
        vk::InstanceCreateInfo createInfo( 
            {},                 // vk::InstanceCreateFlags
            &applicationInfo    // vk::ApplicationInfo*
        );

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

        auto extensions = m_context.enumerateInstanceExtensionProperties();
        std::cout << "available extensions:\n";

        for (const auto& extension : extensions) {
            std::cout << '\t' << extension.extensionName << std::endl;
        }

        m_instance = m_context.createInstance( createInfo );
    }
    /////////////////////////////////////////////////////////////////

    /////////////////////////////////////////////////////////////////
    /// validation layer 创建 验证层 未修改
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
    /// physical device 创建 物理设备 未修改
    struct QueueFamilyIndices {
        std::optional<uint32_t> graphicsFamily;
        std::optional<uint32_t> presentFamily;

        bool isComplete() {
            return graphicsFamily.has_value() && presentFamily.has_value();
        }
    };
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
    QueueFamilyIndices findQueueFamilies(const vk::raii::PhysicalDevice& physicalDevice) {
        QueueFamilyIndices indices;

        // std::vector<vk::QueueFamilyProperties>
        auto queueFamilies = physicalDevice.getQueueFamilyProperties();
        for (int i = 0; const auto& queueFamily : queueFamilies) {
            if (
                (queueFamily.queueFlags & vk::QueueFlagBits::eGraphics) &&
                (queueFamily.queueFlags & vk::QueueFlagBits::eCompute)
            ) {
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
    /// logical device 创建 逻辑设备 未修改
    void createLogicalDevice() {
        QueueFamilyIndices indices = findQueueFamilies( m_physicalDevice );

        std::vector<vk::DeviceQueueCreateInfo> queueCreateInfos;
        // isDeviceSuitable() ensure queue availability
        std::set<uint32_t> uniqueQueueFamilies = {indices.graphicsFamily.value(), indices.presentFamily.value()};

        float queuePriority = 1.0f;
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

        createInfo.setPEnabledExtensionNames( deviceExtensions );

        if (enableValidationLayers) {
            createInfo.setPEnabledLayerNames( validationLayers );
        }

        m_device = m_physicalDevice.createDevice( createInfo );
        m_graphicsQueue = m_device.getQueue( indices.graphicsFamily.value(), 0 );
        m_presentQueue = m_device.getQueue( indices.presentFamily.value(), 0 );
        m_computeQueue = m_device.getQueue( indices.graphicsFamily.value(), 0 );
    }
    /////////////////////////////////////////////////////////////////

    /////////////////////////////////////////////////////////////////
    /// surface 创建 窗口表面 未修改
    void createSurface() {
        VkSurfaceKHR cSurface;

        if (glfwCreateWindowSurface( *m_instance, m_window, nullptr, &cSurface ) != VK_SUCCESS) {
            throw std::runtime_error("failed to create window surface!");
        }
        m_surface = vk::raii::SurfaceKHR( m_instance, cSurface );
    }
    /////////////////////////////////////////////////////////////////

    /////////////////////////////////////////////////////////////////
    /// swapchain 创建 交换链 未修改
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

        vk::SwapchainCreateInfoKHR createInfo;
        createInfo.surface = m_surface;
        createInfo.minImageCount = imageCount;
        createInfo.imageFormat = surfaceFormat.format;
        createInfo.imageColorSpace = surfaceFormat.colorSpace;
        createInfo.imageExtent = extent;
        createInfo.imageArrayLayers = 1;
        createInfo.imageUsage = vk::ImageUsageFlagBits::eColorAttachment;

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
    /// imageview 创建 图像视图 未修改
    void createImageViews() {
        m_swapChainImageViews.reserve( m_swapChainImages.size() );
        for (size_t i = 0; i < m_swapChainImages.size(); i++) {
            vk::ImageViewCreateInfo createInfo;
            createInfo.image = m_swapChainImages[i];
            createInfo.viewType = vk::ImageViewType::e2D;
            createInfo.format = m_swapChainImageFormat;
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
    /// render pass 创建 渲染通道 未修改
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

        vk::SubpassDependency dependency;
        dependency.srcSubpass = vk::SubpassExternal;
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
    /// pipeline 读取着色器模块 未修改
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
        vk::ShaderModuleCreateInfo createInfo;
        createInfo.codeSize = code.size();
        createInfo.pCode = reinterpret_cast<const uint32_t*>(code.data());
        return m_device.createShaderModule(createInfo);
    }
    /**
     * @brief 创建图形管线 已调整
     * @details 
     * 输入装配输入点列表而非三角形顶点列表
     * 其余内容未调整
     */
    void createGraphicsPipeline() {
        auto vertShaderCode = readFile("shaders/vert.spv");
        auto fragShaderCode = readFile("shaders/frag.spv");

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

        std::vector<vk::PipelineShaderStageCreateInfo> shaderStages{ vertShaderStageInfo, fragShaderStageInfo };

        std::vector<vk::DynamicState> dynamicStates = {
            vk::DynamicState::eViewport,
            vk::DynamicState::eScissor
        };

        vk::PipelineDynamicStateCreateInfo dynamicState;
        dynamicState.setDynamicStates( dynamicStates );

        vk::PipelineVertexInputStateCreateInfo vertexInputInfo;

        auto bindingDescription = Particle::getBindingDescription();
        auto attributeDescriptions = Particle::getAttributeDescriptions();

        vertexInputInfo.setVertexBindingDescriptions(bindingDescription);
        vertexInputInfo.setVertexAttributeDescriptions(attributeDescriptions);

        vk::PipelineInputAssemblyStateCreateInfo inputAssembly;
        // 使用 ePointList 作为输入装配的拓扑类型
        // 这将允许我们使用点列表而不是三角形列表
        inputAssembly.topology = vk::PrimitiveTopology::ePointList; // changed !!!!
        inputAssembly.primitiveRestartEnable = false; // default

        vk::PipelineViewportStateCreateInfo viewportState;
        viewportState.viewportCount = 1;
        viewportState.scissorCount = 1;

        vk::PipelineRasterizationStateCreateInfo rasterizer;
        rasterizer.depthClampEnable = false;
        rasterizer.rasterizerDiscardEnable = false;
        rasterizer.polygonMode = vk::PolygonMode::eFill;
        rasterizer.lineWidth = 1.0f;
        rasterizer.cullMode = vk::CullModeFlagBits::eBack;
        rasterizer.frontFace = vk::FrontFace::eClockwise;
        rasterizer.depthBiasEnable = false;

        vk::PipelineMultisampleStateCreateInfo multisampling;
        multisampling.rasterizationSamples =  vk::SampleCountFlagBits::e1;
        multisampling.sampleShadingEnable = false;  // default

        vk::PipelineColorBlendAttachmentState colorBlendAttachment;
        colorBlendAttachment.blendEnable = false; // default
        // colorBlendAttachment.colorWriteMask = vk::FlagTraits<vk::ColorComponentFlagBits>::allFlags;
        colorBlendAttachment.colorWriteMask = (
            vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG | 
            vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA
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

        vk::CommandPoolCreateInfo poolInfo;
        poolInfo.flags = vk::CommandPoolCreateFlagBits::eResetCommandBuffer;
        poolInfo.queueFamilyIndex =  queueFamilyIndices.graphicsFamily.value();

        m_commandPool = m_device.createCommandPool( poolInfo );
    }
    void createCommandBuffers() {
        vk::CommandBufferAllocateInfo allocInfo;
        allocInfo.commandPool = m_commandPool;
        allocInfo.level = vk::CommandBufferLevel::ePrimary;
        allocInfo.commandBufferCount = MAX_FRAMES_IN_FLIGHT;

        m_commandBuffers = m_device.allocateCommandBuffers(allocInfo);
    }
    void recordCommandBuffer(const vk::raii::CommandBuffer& commandBuffer, uint32_t imageIndex) {
        vk::CommandBufferBeginInfo beginInfo;
        commandBuffer.begin( beginInfo );

        vk::RenderPassBeginInfo renderPassInfo;
        renderPassInfo.renderPass = m_renderPass;
        renderPassInfo.framebuffer = m_swapChainFramebuffers[imageIndex];

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

        std::array<vk::Buffer,1> vertexBuffers { m_particleBuffers[m_currentFrame] };
        std::array<vk::DeviceSize,1> offsets { 0 };
        commandBuffer.bindVertexBuffers( 0, vertexBuffers, offsets );

        commandBuffer.draw(PARTICLE_COUNT, 1, 0, 0);

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
            m_renderFinishedSemaphores.emplace_back( m_device,  semaphoreInfo );
            m_inFlightFences.emplace_back( m_device , fenceInfo );
            m_computeFinishedSemaphores.emplace_back( m_device,  semaphoreInfo );
            m_computeInFlightFences.emplace_back( m_device, fenceInfo );
        }
    }
    void drawFrame() {
        // 等待上一次的计算任务完成
        if( auto res = m_device.waitForFences( *m_computeInFlightFences[m_currentFrame], true, UINT64_MAX );
            res != vk::Result::eSuccess ){
            throw std::runtime_error{ "compute in drawFrame was failed" };
        }
        m_device.resetFences( *m_computeInFlightFences[m_currentFrame] );
        
        // 更新 uniform buffer， 内涵时间间隔
        updateUniformBuffer(m_currentFrame);

        // 录制计算命令缓冲区
        m_computeCommandBuffers[m_currentFrame].reset();
        recordComputeCommandBuffer(m_computeCommandBuffers[m_currentFrame]);


        // 设置提交信息，并在任务完成时发送信号量
        vk::SubmitInfo computeSubmitInfo;
        computeSubmitInfo.setCommandBuffers( *m_computeCommandBuffers[m_currentFrame] );
        computeSubmitInfo.setSignalSemaphores( *m_computeFinishedSemaphores[m_currentFrame] );

        // 提交任务并在完成时发信围栏
        m_computeQueue.submit(computeSubmitInfo, m_computeInFlightFences[m_currentFrame]);


        // 等待上一次的渲染任务完成
        if( auto res = m_device.waitForFences( *m_inFlightFences[m_currentFrame], true, UINT64_MAX );
            res != vk::Result::eSuccess ){
            throw std::runtime_error{ "waitForFences in drawFrame was failed" };
        }

        // 获取下一章图像
        uint32_t imageIndex;
        try{
            // std::pair<vk::Result, uint32_t>
            auto [res, idx] = m_swapChain.acquireNextImage(UINT64_MAX, m_imageAvailableSemaphores[m_currentFrame]);
            imageIndex = idx;
        } catch (const vk::OutOfDateKHRError&){
                recreateSwapChain();
                return;
        } // Do not catch other exceptions

        // 重置
        m_device.resetFences( *m_inFlightFences[m_currentFrame] );


        m_commandBuffers[m_currentFrame].reset();
        recordCommandBuffer(m_commandBuffers[m_currentFrame], imageIndex);


        vk::SubmitInfo submitInfo;

        // 现在渲染操作要同时等待 图像就绪 和 计算任务完成
        std::array<vk::Semaphore, 2> waitSemaphores = { *m_computeFinishedSemaphores[m_currentFrame], *m_imageAvailableSemaphores[m_currentFrame] };
        submitInfo.setWaitSemaphores( waitSemaphores );
        // 顶点着色器阶段等待计算任务完成 ， 颜色附件输出阶段 会等待 交换链图像可用
        std::array<vk::PipelineStageFlags,2> waitStages = { vk::PipelineStageFlagBits::eVertexInput, vk::PipelineStageFlagBits::eColorAttachmentOutput };
        submitInfo.setWaitDstStageMask( waitStages );

        submitInfo.setCommandBuffers( *m_commandBuffers[m_currentFrame] );
        submitInfo.setSignalSemaphores( *m_renderFinishedSemaphores[m_currentFrame] );

        // 提交渲染命令
        m_graphicsQueue.submit(submitInfo, m_inFlightFences[m_currentFrame]);

        // 提交呈现命令，呈现命令要等待渲染命令完成
        vk::PresentInfoKHR presentInfo;
        presentInfo.setWaitSemaphores( *m_renderFinishedSemaphores[m_currentFrame] );
        presentInfo.setSwapchains( *m_swapChain );
        presentInfo.pImageIndices = &imageIndex;

        try{
            auto res = m_presentQueue.presentKHR(presentInfo);
            if( res == vk::Result::eSuboptimalKHR ) {
                recreateSwapChain();
            }
        } catch (const vk::OutOfDateKHRError&){
            recreateSwapChain();
        } // Do not catch other exceptions

        if( m_framebufferResized ){
            recreateSwapChain();
        }

        m_currentFrame = (m_currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
    }
    /////////////////////////////////////////////////////////////////

    /////////////////////////////////////////////////////////////////
    /// recreate swapchain
    static void framebufferResizeCallback(GLFWwindow* window, int width, int height) {
        auto app = reinterpret_cast<HelloComputeShader*>(glfwGetWindowUserPointer(window));
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
        m_swapChainImages.clear(); // optional
        m_swapChain = nullptr;

        createSwapChain();
        createImageViews();
        createFramebuffers();

        m_framebufferResized = false;
    }
    /////////////////////////////////////////////////////////////////

    /////////////////////////////////////////////////////////////////
    /// Vertex input
    /**
     * @brief 粒子结构体
     * @details
     * 该结构体定义了粒子的属性，包括位置、速度和颜色。
     * 在二维平面绘制，因此位置和速度使用 `glm::vec2` 类型。
     * 颜色使用 `glm::vec4` 类型，包含红、绿、蓝和透明度分量。
     * 速度属性不需要输入到图形关系，因此只有两个属性描述
     */
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

            attributeDescriptions[1].binding = 0;
            attributeDescriptions[1].location = 1;
            attributeDescriptions[1].format = vk::Format::eR32G32B32A32Sfloat;
            attributeDescriptions[1].offset = offsetof(Particle, color);

            return attributeDescriptions;
        }
    };
    /**
     * @brief 粒子数据
     * @details
     * 参考“顶点缓冲”章节，依然是三个点
     * 但暂时填充了速度参数
     */
    /////////////////////////////////////////////////////////////////

    /////////////////////////////////////////////////////////////////
    /// create vertex buffer
    // 未修改
    void createBuffer(
        vk::DeviceSize size, 
        vk::BufferUsageFlags usage, 
        vk::MemoryPropertyFlags properties, 
        vk::raii::Buffer& buffer, 
        vk::raii::DeviceMemory& bufferMemory
    ) {

        vk::BufferCreateInfo bufferInfo;
        bufferInfo.size = size;
        bufferInfo.usage = usage;
        bufferInfo.sharingMode = vk::SharingMode::eExclusive;

        buffer = m_device.createBuffer(bufferInfo);

        vk::MemoryRequirements memRequirements = buffer.getMemoryRequirements();

        vk::MemoryAllocateInfo allocInfo;
        allocInfo.allocationSize = memRequirements.size;
        allocInfo.memoryTypeIndex = findMemoryType(memRequirements.memoryTypeBits, properties);

        bufferMemory = m_device.allocateMemory( allocInfo );

        buffer.bindMemory(bufferMemory, 0);
    }
    // 函数改名，将顶点缓冲改名为粒子缓冲，其余内容未调整
    void createParticleBuffers() {
        std::default_random_engine rndEngine(static_cast<unsigned>(time(nullptr)));
        std::uniform_real_distribution<float> rndDist(0.0f, 1.0f);
        std::vector<Particle> particles(PARTICLE_COUNT);

        for (auto& particle : particles) {
            float r = 0.25f * std::sqrt(rndDist(rndEngine));
            float theta = rndDist(rndEngine) * 2.0f * 3.141592653f;
            float x = r * std::cos(theta) * HEIGHT / WIDTH;
            float y = r * std::sin(theta);
            particle.pos = glm::vec2(x, y);
            particle.velocity = glm::normalize(glm::vec2(x,y)) * 0.00025f;
            particle.color = glm::vec4(rndDist(rndEngine), rndDist(rndEngine), rndDist(rndEngine), 1.0f);
        }
        vk::DeviceSize bufferSize = sizeof(Particle) * PARTICLE_COUNT;

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
        memcpy(data, particles.data(), static_cast<size_t>(bufferSize));
        stagingBufferMemory.unmapMemory();

        for(int i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i) {
            m_particleBuffers.emplace_back(nullptr);
            m_particleBufferMemory.emplace_back(nullptr);
            createBuffer(bufferSize, 
                vk::BufferUsageFlagBits::eStorageBuffer |
                vk::BufferUsageFlagBits::eTransferDst |
                vk::BufferUsageFlagBits::eVertexBuffer, 
                vk::MemoryPropertyFlagBits::eDeviceLocal,
                m_particleBuffers[i], 
                m_particleBufferMemory[i]
            );
            copyBuffer(stagingBuffer, m_particleBuffers[i], bufferSize);
        }

    }
    // 未调整
    void copyBuffer(vk::raii::Buffer& srcBuffer, vk::raii::Buffer& dstBuffer, vk::DeviceSize size) {
        vk::CommandBufferAllocateInfo allocInfo;
        allocInfo.level = vk::CommandBufferLevel::ePrimary;
        allocInfo.commandPool = m_commandPool;
        allocInfo.commandBufferCount = 1;

        // std::vector<vk::raii::CommandBuffer>
        auto commandBuffers = m_device.allocateCommandBuffers(allocInfo);
        vk::raii::CommandBuffer commandBuffer = std::move(commandBuffers.at(0));

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
    // 未调整
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

    /////////////////////////////////////////////////////////////////
    /// 存储时差信息的 UBO
    struct UniformBufferObject {
        float deltaTime = 1.0f;
    };
    void createUniformBuffers() {
        vk::DeviceSize bufferSize = sizeof(UniformBufferObject);

        for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
            m_uniformBuffers.emplace_back(nullptr);
            m_uniformBuffersMemory.emplace_back(nullptr);
            m_uniformBuffersMapped.emplace_back(nullptr);
            createBuffer(
                bufferSize,
                vk::BufferUsageFlagBits::eUniformBuffer,
                vk::MemoryPropertyFlagBits::eHostVisible | 
                vk::MemoryPropertyFlagBits::eHostCoherent,
                m_uniformBuffers[i],
                m_uniformBuffersMemory[i]
            );
            m_uniformBuffersMapped[i] = m_uniformBuffersMemory[i].mapMemory(0, bufferSize);
        }
    }
    void updateUniformBuffer(uint32_t currentImage) {
        static auto lastTime = std::chrono::steady_clock::now();
        auto currentTime = std::chrono::steady_clock::now();
        float deltaTime = std::chrono::duration<float, std::chrono::milliseconds::period>(currentTime - lastTime).count();
        lastTime = currentTime;

        UniformBufferObject ubo{ deltaTime };
        memcpy(m_uniformBuffersMapped[currentImage], &ubo, sizeof(ubo));
    }
    /////////////////////////////////////////////////////////////////

    /////////////////////////////////////////////////////////////////
    /// 描述符
    void createDescriptorSetLayout() {
        std::array<vk::DescriptorSetLayoutBinding, 3> layoutBindings;
        // 绑定0：Uniform Buffer 传输时间差
        layoutBindings[0].binding = 0;
        layoutBindings[0].descriptorCount = 1;
        layoutBindings[0].descriptorType = vk::DescriptorType::eUniformBuffer;
        layoutBindings[0].stageFlags = vk::ShaderStageFlagBits::eCompute;
        // 绑定1：Storage Buffer 粒子数据 ，1号位用于访问上一组粒子数据
        layoutBindings[1].binding = 1;
        layoutBindings[1].descriptorCount = 1;
        layoutBindings[1].descriptorType = vk::DescriptorType::eStorageBuffer;
        layoutBindings[1].stageFlags = vk::ShaderStageFlagBits::eCompute;
        // 绑定2：Storage Buffer 粒子数据 ，2号位用于访问当前组粒子数据
        layoutBindings[2] = layoutBindings[1];
        layoutBindings[2].binding = 2;

        vk::DescriptorSetLayoutCreateInfo layoutInfo;
        layoutInfo.setBindings( layoutBindings );

        m_descriptorSetLayout = m_device.createDescriptorSetLayout( layoutInfo );
    }
    void createDescriptorPool() {
        std::array<vk::DescriptorPoolSize, 2> poolSizes;
        poolSizes[0].type = vk::DescriptorType::eUniformBuffer;
        poolSizes[0].descriptorCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);
        poolSizes[1].type = vk::DescriptorType::eStorageBuffer;
        poolSizes[1].descriptorCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT) * 2;

        vk::DescriptorPoolCreateInfo poolInfo;
        poolInfo.flags = vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet;
        poolInfo.maxSets = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);
        poolInfo.setPoolSizes( poolSizes );

        m_descriptorPool = m_device.createDescriptorPool( poolInfo );
    }
    void createDescriptorSets() {
        std::vector<vk::DescriptorSetLayout> layouts(MAX_FRAMES_IN_FLIGHT, *m_descriptorSetLayout);
        vk::DescriptorSetAllocateInfo allocInfo;
        allocInfo.setDescriptorPool( m_descriptorPool );
        allocInfo.setSetLayouts( layouts );

        m_descriptorSets = m_device.allocateDescriptorSets( allocInfo );

        for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
            vk::DescriptorBufferInfo uniformBufferInfo;
            uniformBufferInfo.buffer = *m_uniformBuffers[i];
            uniformBufferInfo.offset = 0;
            uniformBufferInfo.range = sizeof(UniformBufferObject);

            vk::DescriptorBufferInfo particleBufferInfo1;
            particleBufferInfo1.buffer = *m_particleBuffers[(i + MAX_FRAMES_IN_FLIGHT - 1) % MAX_FRAMES_IN_FLIGHT];
            particleBufferInfo1.offset = 0;
            particleBufferInfo1.range = sizeof(Particle) * PARTICLE_COUNT;

            vk::DescriptorBufferInfo particleBufferInfo2;
            particleBufferInfo2.buffer = *m_particleBuffers[i];
            particleBufferInfo2.offset = 0;
            particleBufferInfo2.range = sizeof(Particle) * PARTICLE_COUNT;

            std::array<vk::WriteDescriptorSet, 3> descriptorWrites;
            descriptorWrites[0].dstSet = m_descriptorSets[i];
            descriptorWrites[0].dstBinding = 0;
            descriptorWrites[0].descriptorType = vk::DescriptorType::eUniformBuffer;
            descriptorWrites[0].setBufferInfo( uniformBufferInfo );

            descriptorWrites[1].dstSet = m_descriptorSets[i];
            descriptorWrites[1].dstBinding = 1;
            descriptorWrites[1].descriptorType = vk::DescriptorType::eStorageBuffer;
            descriptorWrites[1].setBufferInfo( particleBufferInfo1 );

            descriptorWrites[2].dstSet = m_descriptorSets[i];
            descriptorWrites[2].dstBinding = 2;
            descriptorWrites[2].descriptorType = vk::DescriptorType::eStorageBuffer;
            descriptorWrites[2].setBufferInfo( particleBufferInfo2 );

            m_device.updateDescriptorSets(descriptorWrites, nullptr);
        }
    }
    /////////////////////////////////////////////////////////////////

    /////////////////////////////////////////////////////////////////
    /// 计算管线
    void createComputePipeline() {
        // 创建管线布局
        vk::PipelineLayoutCreateInfo pipelineLayoutInfo;
        pipelineLayoutInfo.setSetLayouts( *m_descriptorSetLayout ); // 添加描述符
        m_computePipelineLayout = m_device.createPipelineLayout(pipelineLayoutInfo);

        // 读取并创建着色器模块
        auto computeShaderCode = readFile("shaders/comp.spv");
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
    /////////////////////////////////////////////////////////////////

    /////////////////////////////////////////////////////////////////
    /// 命令缓冲与命令录制
    void createComputeCommandBuffers() {
        vk::CommandBufferAllocateInfo allocInfo;
        allocInfo.commandPool = m_commandPool;
        allocInfo.level = vk::CommandBufferLevel::ePrimary;
        allocInfo.commandBufferCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);
        
        m_computeCommandBuffers = m_device.allocateCommandBuffers(allocInfo);
    }
    void recordComputeCommandBuffer(const vk::raii::CommandBuffer& commandBuffer) {
        vk::CommandBufferBeginInfo beginInfo;
        commandBuffer.begin(beginInfo);

        // 绑定计算管线
        commandBuffer.bindPipeline(vk::PipelineBindPoint::eCompute, m_computePipeline);

        // 绑定描述符集
        commandBuffer.bindDescriptorSets(
            vk::PipelineBindPoint::eCompute,
            m_computePipelineLayout,
            0,
            *m_descriptorSets[m_currentFrame],
            nullptr
        );

        // 调用计算着色器
        commandBuffer.dispatch((PARTICLE_COUNT + 255) / 256, 1, 1);

        commandBuffer.end();
    }
    /////////////////////////////////////////////////////////////////
};

int main() {
    HelloComputeShader app;

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
