#include <vulkan/vulkan.hpp>
#include <vulkan/vulkan_raii.hpp>
#include <GLFW/glfw3.h>
#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/hash.hpp>
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>
#define TINYOBJLOADER_IMPLEMENTATION
#include <tiny_obj_loader.h>

#include <chrono>
#include <iostream>
#include <fstream>
#include <vector>
#include <array>
#include <set>
#include <unordered_map>
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
    /// forwoard declare
    struct Vertex;
    /////////////////////////////////////////////////////////////////

    /////////////////////////////////////////////////////////////////
    /// static values
    static constexpr uint32_t WIDTH = 800;
    static constexpr uint32_t HEIGHT = 600;

    inline static const std::string MODEL_PATH = "models/viking_room.obj";
    inline static const std::string TEXTURE_PATH = "textures/viking_room.png";

    inline static const std::vector<const char*> validationLayers {
        "VK_LAYER_KHRONOS_validation"
    };
    inline static const std::vector<const char*> deviceExtensions {
        VK_KHR_SWAPCHAIN_EXTENSION_NAME
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
    std::vector<Vertex> m_vertices;
    std::vector<uint32_t> m_indices;
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
    vk::raii::SwapchainKHR m_swapChain{ nullptr };
    std::vector<vk::Image> m_swapChainImages;
    vk::Format m_swapChainImageFormat;
    vk::Extent2D m_swapChainExtent;
    std::vector<vk::raii::ImageView> m_swapChainImageViews;
    vk::raii::DeviceMemory m_depthImageMemory{ nullptr };
    vk::raii::Image m_depthImage{ nullptr };
    vk::raii::ImageView m_depthImageView{ nullptr };
    vk::raii::RenderPass m_renderPass{ nullptr };
    vk::raii::DescriptorSetLayout m_descriptorSetLayout{ nullptr };
    vk::raii::DescriptorPool m_descriptorPool{ nullptr };
    std::vector<vk::raii::DescriptorSet> m_descriptorSets;
    vk::raii::PipelineLayout m_pipelineLayout{ nullptr };
    vk::raii::Pipeline m_graphicsPipeline{ nullptr };
    std::vector<vk::raii::Framebuffer> m_swapChainFramebuffers;
    vk::raii::CommandPool m_commandPool{ nullptr };
    std::vector<vk::raii::CommandBuffer> m_commandBuffers;
    std::vector<vk::raii::Semaphore> m_imageAvailableSemaphores;
    std::vector<vk::raii::Semaphore> m_renderFinishedSemaphores;
    std::vector<vk::raii::Fence> m_inFlightFences;
    uint32_t m_currentFrame = 0;
    bool m_framebufferResized = false;
    glm::vec3 m_cameraPos{ 2.0f, 2.0f, 2.0f };
    glm::vec3 m_cameraUp{ 0.0f, 1.0f, 0.0f };
    float m_pitch = -35.0f;
    float m_yaw = -135.0f;
    float m_cameraMoveSpeed = 1.0f;
    float m_cameraRotateSpeed = 25.0f;
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
        createCommandPool();
        createDepthResources();
        createFramebuffers();
        createTextureImage();
        createTextureImageView();
        createTextureSampler();
        loadModel();
        createVertexBuffer();
        createIndexBuffer();
        createUniformBuffers();
        createDescriptorPool();
        createDescriptorSets();
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
        for(const auto& it : m_uniformBuffersMemory){
            it.unmapMemory();
        }

        glfwDestroyWindow( m_window );
        glfwTerminate();
    }
    /////////////////////////////////////////////////////////////////

    /////////////////////////////////////////////////////////////////
    /// instance creation
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

        auto supportedFeatures = physicalDevice.getFeatures();

        return indices.isComplete() && extensionsSupported && swapChainAdequate && supportedFeatures.samplerAnisotropy;
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

        createInfo.setPEnabledExtensionNames( deviceExtensions );

        if (enableValidationLayers) {
            createInfo.setPEnabledLayerNames( validationLayers );
        }

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
    /// imageview
    void createImageViews() {
        m_swapChainImageViews.reserve( m_swapChainImages.size() );
        for (size_t i = 0; i < m_swapChainImages.size(); ++i) {
            m_swapChainImageViews.emplace_back( 
                createImageView(m_swapChainImages[i], m_swapChainImageFormat, vk::ImageAspectFlagBits::eColor) 
            );
        }
    }
    /////////////////////////////////////////////////////////////////

    /////////////////////////////////////////////////////////////////
    /// render pass
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

        vk::AttachmentDescription depthAttachment;
        depthAttachment.format = findDepthFormat();
        depthAttachment.samples = vk::SampleCountFlagBits::e1;
        depthAttachment.loadOp = vk::AttachmentLoadOp::eClear;
        depthAttachment.storeOp = vk::AttachmentStoreOp::eDontCare;
        depthAttachment.stencilLoadOp = vk::AttachmentLoadOp::eDontCare;
        depthAttachment.stencilStoreOp = vk::AttachmentStoreOp::eDontCare;
        depthAttachment.initialLayout = vk::ImageLayout::eUndefined;
        depthAttachment.finalLayout = vk::ImageLayout::eDepthStencilAttachmentOptimal;

        vk::AttachmentReference depthAttachmentRef;
        depthAttachmentRef.attachment = 1;
        depthAttachmentRef.layout = vk::ImageLayout::eDepthStencilAttachmentOptimal;

        vk::SubpassDescription subpass;
        subpass.pipelineBindPoint = vk::PipelineBindPoint::eGraphics;
        subpass.setColorAttachments( colorAttachmentRef );
        subpass.pDepthStencilAttachment = &depthAttachmentRef;

        auto attachments = { colorAttachment, depthAttachment };
        vk::RenderPassCreateInfo renderPassInfo;
        renderPassInfo.setAttachments( attachments );
        renderPassInfo.setSubpasses( subpass );

        vk::SubpassDependency dependency;
        dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
        dependency.dstSubpass = 0;
        dependency.srcStageMask = vk::PipelineStageFlagBits::eColorAttachmentOutput | vk::PipelineStageFlagBits::eEarlyFragmentTests;
        dependency.srcAccessMask = {};
        dependency.dstStageMask = vk::PipelineStageFlagBits::eColorAttachmentOutput | vk::PipelineStageFlagBits::eEarlyFragmentTests;
        dependency.dstAccessMask = vk::AccessFlagBits::eColorAttachmentWrite | vk::AccessFlagBits::eDepthStencilAttachmentWrite;

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
        vk::ShaderModuleCreateInfo createInfo;
        createInfo.codeSize = code.size();
        createInfo.pCode = reinterpret_cast<const uint32_t*>(code.data());
        return m_device.createShaderModule(createInfo);
    }
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

        auto bindingDescription = Vertex::getBindingDescription();
        auto attributeDescriptions = Vertex::getAttributeDescriptions();

        vertexInputInfo.setVertexBindingDescriptions(bindingDescription);
        vertexInputInfo.setVertexAttributeDescriptions(attributeDescriptions);

        vk::PipelineInputAssemblyStateCreateInfo inputAssembly;
        inputAssembly.topology = vk::PrimitiveTopology::eTriangleList;
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
        rasterizer.frontFace = vk::FrontFace::eCounterClockwise;
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

        vk::PipelineDepthStencilStateCreateInfo depthStencil;
        depthStencil.depthTestEnable = true;
        depthStencil.depthWriteEnable = true;
        depthStencil.depthCompareOp = vk::CompareOp::eLess;
        depthStencil.depthBoundsTestEnable = false; // Optional
        depthStencil.stencilTestEnable = false; // Optional

        vk::PipelineLayoutCreateInfo pipelineLayoutInfo;
        pipelineLayoutInfo.setSetLayouts(*m_descriptorSetLayout);
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
        pipelineInfo.pDepthStencilState = &depthStencil;

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
            std::array<vk::ImageView, 2> attachments { m_swapChainImageViews[i], m_depthImageView };
            framebufferInfo.setAttachments( attachments );
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

        std::array<vk::ClearValue, 2> clearValues;
        clearValues[0].color = vk::ClearColorValue{0.0f, 0.0f, 0.0f, 1.0f};
        clearValues[1].depthStencil = vk::ClearDepthStencilValue{1.0f, 0};

        renderPassInfo.setClearValues( clearValues );

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

        // std::array<vk::Buffer,1> vertexBuffers { m_vertexBuffer };
        // std::array<vk::DeviceSize,1> offsets { 0 };
        commandBuffer.bindVertexBuffers( 0, *m_vertexBuffer, vk::DeviceSize{0} );
        commandBuffer.bindIndexBuffer( m_indexBuffer, 0, vk::IndexType::eUint32 );

        commandBuffer.bindDescriptorSets(
            vk::PipelineBindPoint::eGraphics, 
            m_pipelineLayout,
            0,
            *m_descriptorSets[m_currentFrame],
            nullptr
        );

        commandBuffer.drawIndexed(static_cast<uint32_t>(m_indices.size()), 1, 0, 0, 0);

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
        }
    }
    void drawFrame() {
        if( auto res = m_device.waitForFences( *m_inFlightFences[m_currentFrame], true, UINT64_MAX );
            res != vk::Result::eSuccess ){
            throw std::runtime_error{ "waitForFences in drawFrame was failed" };
        }

        uint32_t imageIndex;
        try{
            // std::pair<vk::Result, uint32_t>
            auto [res, idx] = m_swapChain.acquireNextImage(UINT64_MAX, m_imageAvailableSemaphores[m_currentFrame]);
            imageIndex = idx;
        } catch (const vk::OutOfDateKHRError&){
                recreateSwapChain();
                return;
        } // Do not catch other exceptions

        // Only reset the fence if we are submitting work
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

        m_depthImageView = nullptr;
        m_depthImage = nullptr;
        m_depthImageMemory = nullptr;

        m_swapChainImageViews.clear();
        m_swapChainImages.clear(); // optional
        m_swapChain = nullptr;


        createSwapChain();
        createImageViews();
        createDepthResources();
        createFramebuffers();

        m_framebufferResized = false;
    }
    /////////////////////////////////////////////////////////////////

    /////////////////////////////////////////////////////////////////
    /// Vertex input
    struct Vertex {
        glm::vec3 pos;
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
            attributeDescriptions[0].format = vk::Format::eR32G32B32Sfloat;
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
    /////////////////////////////////////////////////////////////////

    /////////////////////////////////////////////////////////////////
    /// create vertex buffer
    void createBuffer(
        vk::DeviceSize size, 
        vk::BufferUsageFlags usage, 
        vk::MemoryPropertyFlags properties, 
        vk::raii::Buffer& buffer, 
        vk::raii::DeviceMemory& bufferMemory) {

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
    void createVertexBuffer() {
        vk::DeviceSize bufferSize = sizeof(m_vertices[0]) * m_vertices.size();

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
        memcpy(data, m_vertices.data(), static_cast<size_t>(bufferSize));
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
        vk::DeviceSize bufferSize = sizeof(m_indices[0]) * m_indices.size();

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
        memcpy(data, m_indices.data(), static_cast<size_t>(bufferSize));
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
    void copyBuffer(vk::raii::Buffer& srcBuffer, vk::raii::Buffer& dstBuffer, vk::DeviceSize size) {
        vk::raii::CommandBuffer commandBuffer = beginSingleTimeCommands();

        vk::BufferCopy copyRegion;
        copyRegion.size = size;
        commandBuffer.copyBuffer(srcBuffer, dstBuffer, copyRegion);

        endSingleTimeCommands( std::move(commandBuffer) );
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

    /////////////////////////////////////////////////////////////////
    /// descriptor layout and buffer
    struct alignas(16) UniformBufferObject {
        glm::mat4 model;
        glm::mat4 view;
        glm::mat4 proj;
    };
    void createDescriptorSetLayout() {
        vk::DescriptorSetLayoutBinding uboLayoutBinding;
        uboLayoutBinding.binding = 0;
        uboLayoutBinding.descriptorType = vk::DescriptorType::eUniformBuffer;
        uboLayoutBinding.descriptorCount = 1;
        uboLayoutBinding.stageFlags = vk::ShaderStageFlagBits::eVertex;

        vk::DescriptorSetLayoutBinding samplerLayoutBinding;
        samplerLayoutBinding.binding = 1;
        samplerLayoutBinding.descriptorType = vk::DescriptorType::eCombinedImageSampler;
        samplerLayoutBinding.descriptorCount = 1;
        samplerLayoutBinding.stageFlags = vk::ShaderStageFlagBits::eFragment;

        std::array<vk::DescriptorSetLayoutBinding, 2> bindings{ uboLayoutBinding, samplerLayoutBinding };
        vk::DescriptorSetLayoutCreateInfo layoutInfo;
        layoutInfo.setBindings( bindings );

        m_descriptorSetLayout = m_device.createDescriptorSetLayout( layoutInfo );
    }
    void createUniformBuffers() {
        vk::DeviceSize bufferSize  = sizeof(UniformBufferObject);

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
    void updateUniformBuffer(uint32_t currentImage) {
        updateCamera();

        glm::vec3 front;
        front.x = std::cos(glm::radians(m_yaw)) * std::cos(glm::radians(m_pitch));
        front.y = std::sin(glm::radians(m_pitch));
        front.z = std::sin(glm::radians(m_yaw)) * std::cos(glm::radians(m_pitch));
        front = glm::normalize(front);

        UniformBufferObject ubo{};
        ubo.model = glm::rotate(
            glm::mat4(1.0f), 
            glm::radians(-90.0f), 
            glm::vec3(1.0f, 0.0f, 0.0f)
        );
        ubo.model *= glm::rotate(
            glm::mat4(1.0f), 
            glm::radians(-90.0f), 
            glm::vec3(0.0f, 0.0f, 1.0f)
        );
        ubo.view = glm::lookAt(
            m_cameraPos, 
            m_cameraPos + front, 
            m_cameraUp
        );
        ubo.proj = glm::perspective(
            glm::radians(45.0f), 
            static_cast<float>(m_swapChainExtent.width) / m_swapChainExtent.height, 
            0.1f, 
            10.0f
        );

        ubo.proj[1][1] *= -1;

        memcpy(m_uniformBuffersMapped[currentImage], &ubo, sizeof(ubo));
    }
    /////////////////////////////////////////////////////////////////

    /////////////////////////////////////////////////////////////////
    /// descriptor pool and sets
    void createDescriptorPool() {
        std::array<vk::DescriptorPoolSize, 2> poolSizes;
        poolSizes[0].type = vk::DescriptorType::eUniformBuffer;
        poolSizes[0].descriptorCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);
        poolSizes[1].type = vk::DescriptorType::eCombinedImageSampler;
        poolSizes[1].descriptorCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);

        vk::DescriptorPoolCreateInfo poolInfo;
        poolInfo.flags = vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet;
        poolInfo.setPoolSizes( poolSizes );
        poolInfo.maxSets = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);

        m_descriptorPool = m_device.createDescriptorPool(poolInfo);
    }
    void createDescriptorSets() {
        std::vector<vk::DescriptorSetLayout> layouts(MAX_FRAMES_IN_FLIGHT, *m_descriptorSetLayout); 
        vk::DescriptorSetAllocateInfo allocInfo;
        allocInfo.descriptorPool = m_descriptorPool;
        allocInfo.setSetLayouts( layouts );

        m_descriptorSets = m_device.allocateDescriptorSets(allocInfo);

        for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i) {
            vk::DescriptorBufferInfo bufferInfo;
            bufferInfo.buffer = m_uniformBuffers[i];
            bufferInfo.offset = 0;
            bufferInfo.range = sizeof(UniformBufferObject);

            vk::DescriptorImageInfo imageInfo;
            imageInfo.imageLayout = vk::ImageLayout::eShaderReadOnlyOptimal;
            imageInfo.imageView = m_textureImageView;
            imageInfo.sampler = m_textureSampler;

            std::array<vk::WriteDescriptorSet, 2> descriptorWrites;
            descriptorWrites[0].dstSet = m_descriptorSets[i];
            descriptorWrites[0].dstBinding = 0;
            descriptorWrites[0].dstArrayElement = 0;
            descriptorWrites[0].descriptorType = vk::DescriptorType::eUniformBuffer;
            descriptorWrites[0].setBufferInfo(bufferInfo);
            descriptorWrites[1].dstSet = m_descriptorSets[i];
            descriptorWrites[1].dstBinding = 1;
            descriptorWrites[1].dstArrayElement = 0;
            descriptorWrites[1].descriptorType = vk::DescriptorType::eCombinedImageSampler;
            descriptorWrites[1].setImageInfo(imageInfo);

            m_device.updateDescriptorSets(descriptorWrites, nullptr);
        }
    }
    /////////////////////////////////////////////////////////////////

    /////////////////////////////////////////////////////////////////
    /// texture
    vk::raii::CommandBuffer beginSingleTimeCommands() {
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
    void endSingleTimeCommands(vk::raii::CommandBuffer commandBuffer) {
        commandBuffer.end();

        vk::SubmitInfo submitInfo;
        submitInfo.setCommandBuffers( *commandBuffer );

        m_graphicsQueue.submit(submitInfo);
        m_graphicsQueue.waitIdle();
    }
    void createImage(
        uint32_t width,
        uint32_t height,
        vk::Format format,
        vk::ImageTiling tilling,
        vk::ImageUsageFlags usage,
        vk::MemoryPropertyFlags properties,
        vk::raii::Image& image,
        vk::raii::DeviceMemory& imageMemory
    ) {
        vk::ImageCreateInfo imageInfo;
        imageInfo.imageType = vk::ImageType::e2D;
        imageInfo.extent.width = width;
        imageInfo.extent.height = height;
        imageInfo.extent.depth = 1;
        imageInfo.mipLevels = 1;
        imageInfo.arrayLayers = 1;
        imageInfo.format = format;
        imageInfo.tiling = tilling;
        imageInfo.initialLayout = vk::ImageLayout::eUndefined;
        imageInfo.usage = usage;
        imageInfo.samples = vk::SampleCountFlagBits::e1;
        imageInfo.sharingMode = vk::SharingMode::eExclusive;

        image = m_device.createImage(imageInfo);

        vk::MemoryRequirements memRequirements = image.getMemoryRequirements();
        vk::MemoryAllocateInfo allocInfo;
        allocInfo.allocationSize = memRequirements.size;
        allocInfo.memoryTypeIndex = findMemoryType(memRequirements.memoryTypeBits, properties);

        imageMemory = m_device.allocateMemory(allocInfo);

        image.bindMemory(imageMemory, 0);
    }
    void transitionImageLayout(
        vk::raii::Image& image,
        vk::Format format,
        vk::ImageLayout oldLayout,
        vk::ImageLayout newLayout
    ) {
        vk::raii::CommandBuffer commandBuffer = beginSingleTimeCommands();

        vk::ImageMemoryBarrier barrier;
        barrier.oldLayout = oldLayout;
        barrier.newLayout = newLayout;
        barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.image = image;
        // barrier.subresourceRange.aspectMask = vk::ImageAspectFlagBits::eColor;
        barrier.subresourceRange.baseMipLevel = 0;
        barrier.subresourceRange.levelCount = 1;
        barrier.subresourceRange.baseArrayLayer = 0;
        barrier.subresourceRange.layerCount = 1;

        if( newLayout == vk::ImageLayout::eDepthStencilAttachmentOptimal ) {
            barrier.subresourceRange.aspectMask = vk::ImageAspectFlagBits::eDepth;
            if( hasStencilComponent(format) ){
                barrier.subresourceRange.aspectMask |= vk::ImageAspectFlagBits::eStencil;
            }
        } else {
            barrier.subresourceRange.aspectMask = vk::ImageAspectFlagBits::eColor;
        }

        vk::PipelineStageFlagBits sourceStage;
        vk::PipelineStageFlagBits destinationStage;

        if( oldLayout == vk::ImageLayout::eUndefined &&
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
        } else if (
            oldLayout == vk::ImageLayout::eUndefined &&
            newLayout == vk::ImageLayout::eDepthStencilAttachmentOptimal
        ) {
            barrier.srcAccessMask = {};
            barrier.dstAccessMask = vk::AccessFlagBits::eDepthStencilAttachmentRead | vk::AccessFlagBits::eDepthStencilAttachmentWrite;
            sourceStage = vk::PipelineStageFlagBits::eTopOfPipe;
            destinationStage = vk::PipelineStageFlagBits::eEarlyFragmentTests;
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

        endSingleTimeCommands( std::move(commandBuffer) );
    }
    void copyBufferToImage(
        vk::raii::Buffer& buffer, 
        vk::raii::Image& image,
        uint32_t width,
        uint32_t height
    ) {
        vk::raii::CommandBuffer commandBuffer = beginSingleTimeCommands();

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

        endSingleTimeCommands( std::move(commandBuffer) );
    }
    void createTextureImage() {
        int texWidth, texHeight, texChannels;
        stbi_uc* pixels = stbi_load(TEXTURE_PATH.c_str(), &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);
        if (!pixels) {
            throw std::runtime_error("failed to load texture image!");
        }
        vk::DeviceSize imageSize = texWidth * texHeight * 4;

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
    /////////////////////////////////////////////////////////////////

    /////////////////////////////////////////////////////////////////
    /// image view and sampler
    vk::raii::ImageView createImageView(vk::Image image, vk::Format format, vk::ImageAspectFlags aspectFlags) {
        vk::ImageViewCreateInfo viewInfo;
        viewInfo.image = image;
        viewInfo.viewType = vk::ImageViewType::e2D;
        viewInfo.format = format;
        viewInfo.subresourceRange.aspectMask = aspectFlags;
        viewInfo.subresourceRange.baseMipLevel = 0;
        viewInfo.subresourceRange.levelCount = 1;
        viewInfo.subresourceRange.baseArrayLayer = 0;
        viewInfo.subresourceRange.layerCount = 1;

        return m_device.createImageView(viewInfo);
    }
    void createTextureImageView() {
        m_textureImageView = createImageView(m_textureImage, vk::Format::eR8G8B8A8Srgb, vk::ImageAspectFlagBits::eColor);
    }
    void createTextureSampler() {
        vk::SamplerCreateInfo samplerInfo;
        samplerInfo.magFilter = vk::Filter::eLinear;
        samplerInfo.minFilter = vk::Filter::eLinear;

        samplerInfo.addressModeU = vk::SamplerAddressMode::eRepeat;
        samplerInfo.addressModeV = vk::SamplerAddressMode::eRepeat;
        samplerInfo.addressModeW = vk::SamplerAddressMode::eRepeat;

        samplerInfo.anisotropyEnable = true;
        auto properties = m_physicalDevice.getProperties();
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
    /////////////////////////////////////////////////////////////////

    /////////////////////////////////////////////////////////////////
    /// depth buffer
    vk::Format findSupportedFormat(
        const std::vector<vk::Format>& candidates,
        vk::ImageTiling tiling,
        vk::FormatFeatureFlags features
    ) {
        for(vk::Format format : candidates) {
            // vk::FormatProperties
            auto props = m_physicalDevice.getFormatProperties(format);

            switch (tiling){
            case vk::ImageTiling::eLinear:
                if(props.linearTilingFeatures & features) return format;
                break;
            case vk::ImageTiling::eOptimal:
                if(props.optimalTilingFeatures & features) return format;
                break;
            default: 
                break;
            }
        }
        throw std::runtime_error("failed to find supported format!");
    }
    vk::Format findDepthFormat() {
        return findSupportedFormat(
            { vk::Format::eD32Sfloat, vk::Format::eD32SfloatS8Uint, vk::Format::eD24UnormS8Uint },
            vk::ImageTiling::eOptimal,
            vk::FormatFeatureFlagBits::eDepthStencilAttachment
        );
    }
    bool hasStencilComponent(vk::Format format) {
        return format == vk::Format::eD32SfloatS8Uint || format == vk::Format::eD24UnormS8Uint;
    }
    void createDepthResources() {
        vk::Format depthFormat = findDepthFormat();

        createImage(
            m_swapChainExtent.width,
            m_swapChainExtent.height,
            depthFormat,
            vk::ImageTiling::eOptimal,
            vk::ImageUsageFlagBits::eDepthStencilAttachment,
            vk::MemoryPropertyFlagBits::eDeviceLocal,
            m_depthImage,
            m_depthImageMemory
        );

        m_depthImageView = createImageView(m_depthImage, depthFormat, vk::ImageAspectFlagBits::eDepth);

        // transitionImageLayout(
        //     m_depthImage,
        //     depthFormat,
        //     vk::ImageLayout::eUndefined,
        //     vk::ImageLayout::eDepthStencilAttachmentOptimal
        // );
    }
    /////////////////////////////////////////////////////////////////

    /////////////////////////////////////////////////////////////////
    /// load model
    void loadModel() {
        tinyobj::attrib_t attrib;
        std::vector<tinyobj::shape_t> shapes;
        std::vector<tinyobj::material_t> materials;
        std::string warn, err;

        if (!tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, MODEL_PATH.c_str())) {
            throw std::runtime_error(warn + err);
        }
        
        std::unordered_map<
            Vertex, 
            uint32_t,
            decltype( [](const Vertex& vertex) -> size_t {
                return (((std::hash<glm::vec3>()(vertex.pos) << 1) ^ std::hash<glm::vec3>()(vertex.color) ) >> 1) ^
                    (std::hash<glm::vec2>()(vertex.texCoord) << 1);
            } ),
            decltype( [](const Vertex& vtx_1, const Vertex& vtx_2){
                return vtx_1.pos == vtx_2.pos && vtx_1.color == vtx_2.color && vtx_1.texCoord == vtx_2.texCoord;
            } )
        > uniqueVertices;

        for (const auto& shape : shapes) {
            for (const auto& index : shape.mesh.indices) {
                Vertex vertex;

                vertex.pos = {
                    attrib.vertices[3 * index.vertex_index + 0],
                    attrib.vertices[3 * index.vertex_index + 1],
                    attrib.vertices[3 * index.vertex_index + 2]
                };

                vertex.texCoord = {
                    attrib.texcoords[2 * index.texcoord_index + 0],
                    1.0f - attrib.texcoords[2 * index.texcoord_index + 1]
                };

                vertex.color = {1.0f, 1.0f, 1.0f};

                if (!uniqueVertices.contains(vertex)) {
                    uniqueVertices[vertex] = static_cast<uint32_t>(m_vertices.size());
                    m_vertices.push_back(vertex);
                }
                m_indices.push_back(uniqueVertices[vertex]);

                // m_vertices.push_back(vertex);
                // m_indices.push_back(m_indices.size());
            }
        }
    }
    /////////////////////////////////////////////////////////////////

    /////////////////////////////////////////////////////////////////
    /// move camera
    void updateCamera() {
        static auto startTime = std::chrono::high_resolution_clock::now();
        auto currentTime = std::chrono::high_resolution_clock::now();
        float time = std::chrono::duration<float, std::chrono::seconds::period>(currentTime - startTime).count();
        startTime = currentTime;

        glm::vec3 front;
        front.x = std::cos(glm::radians(m_yaw)) * std::cos(glm::radians(m_pitch));
        front.y = 0.0f;
        front.z = std::sin(glm::radians(m_yaw)) * std::cos(glm::radians(m_pitch));
        front = glm::normalize(front);

        if (glfwGetKey(m_window, GLFW_KEY_W) == GLFW_PRESS)
            m_cameraPos += front * m_cameraMoveSpeed * time;
        if (glfwGetKey(m_window, GLFW_KEY_S) == GLFW_PRESS)
            m_cameraPos -= front * m_cameraMoveSpeed * time;
        if (glfwGetKey(m_window, GLFW_KEY_A) == GLFW_PRESS)
            m_cameraPos -= glm::normalize(glm::cross(front, m_cameraUp)) * m_cameraMoveSpeed * time;
        if (glfwGetKey(m_window, GLFW_KEY_D) == GLFW_PRESS)
            m_cameraPos += glm::normalize(glm::cross(front, m_cameraUp)) * m_cameraMoveSpeed * time;
        if (glfwGetKey(m_window, GLFW_KEY_SPACE) == GLFW_PRESS)
            m_cameraPos += m_cameraUp * m_cameraMoveSpeed * time;
        if (glfwGetKey(m_window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS)
            m_cameraPos -= m_cameraUp *m_cameraMoveSpeed * time;

        if (glfwGetKey(m_window, GLFW_KEY_UP) == GLFW_PRESS)
            m_pitch += m_cameraRotateSpeed * time;
        if (glfwGetKey(m_window, GLFW_KEY_DOWN) == GLFW_PRESS)
            m_pitch -= m_cameraRotateSpeed * time;
        if (glfwGetKey(m_window, GLFW_KEY_LEFT) == GLFW_PRESS)
            m_yaw   -= m_cameraRotateSpeed * time;
        if (glfwGetKey(m_window, GLFW_KEY_RIGHT) == GLFW_PRESS)
            m_yaw   += m_cameraRotateSpeed * time;

        m_yaw = std::fmodf(m_yaw + 180.0f, 360.0f);
        if (m_yaw < 0.0f) m_yaw += 360.0f;
        m_yaw -= 180.0f;

        if (m_pitch > 89.0f) m_pitch = 89.0f;
        if (m_pitch < -89.0f) m_pitch = -89.0f;
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
