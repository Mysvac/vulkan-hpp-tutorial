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
#include <random>
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
    struct InstanceData;
    /////////////////////////////////////////////////////////////////

    /////////////////////////////////////////////////////////////////
    /// static values
    static constexpr uint32_t WIDTH = 800;
    static constexpr uint32_t HEIGHT = 600;

    inline static const std::string MODEL_PATH = "models/viking_room.obj";
    inline static const std::string TEXTURE_PATH = "textures/viking_room.png";
    inline static const std::string BUNNY_PATH = "models/bunny.obj";
    inline static const std::string CRATE_OBJ_PATH = "models/crate.obj";
    inline static const std::string CRATE_TEXTURE_PATH = "textures/crate.jpg";

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
    static constexpr int BUNNY_NUMBER = 5;
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
    std::vector<InstanceData> m_instanceDatas;
    std::vector<glm::mat4> m_dynamicUboMatrices;
    std::vector<uint32_t> m_indices;
    std::vector<uint32_t> m_firstIndices;
    vk::raii::DeviceMemory m_vertexBufferMemory{ nullptr };
    vk::raii::Buffer m_vertexBuffer{ nullptr };
    vk::raii::DeviceMemory m_instanceBufferMemory{ nullptr };
    vk::raii::Buffer m_instanceBuffer{ nullptr };
    vk::raii::DeviceMemory m_indexBufferMemory{ nullptr };
    vk::raii::Buffer m_indexBuffer{ nullptr };
    std::vector<vk::raii::DeviceMemory> m_uniformBuffersMemory;
    std::vector<vk::raii::Buffer> m_uniformBuffers;
    std::vector<void*> m_uniformBuffersMapped;
    std::vector<vk::raii::DeviceMemory> m_dynamicUniformBuffersMemory;
    std::vector<vk::raii::Buffer> m_dynamicUniformBuffers;
    std::vector<void*> m_dynamicUniformBuffersMapped;
    std::vector<vk::raii::DeviceMemory> m_textureImageMemory;
    std::vector<vk::raii::Image> m_textureImages;
    std::vector<vk::raii::ImageView> m_textureImageViews;
    vk::raii::Sampler m_textureSampler{ nullptr };
    uint32_t m_mipLevels;
    vk::raii::SwapchainKHR m_swapChain{ nullptr };
    std::vector<vk::Image> m_swapChainImages;
    vk::Format m_swapChainImageFormat;
    vk::Extent2D m_swapChainExtent;
    std::vector<vk::raii::ImageView> m_swapChainImageViews;
    vk::SampleCountFlagBits m_msaaSamples = vk::SampleCountFlagBits::e1;
    vk::raii::DeviceMemory m_colorImageMemory{ nullptr };
    vk::raii::Image m_colorImage{ nullptr };
    vk::raii::ImageView m_colorImageView{ nullptr };
    vk::raii::DeviceMemory m_depthImageMemory{ nullptr };
    vk::raii::Image m_depthImage{ nullptr };
    vk::raii::ImageView m_depthImageView{ nullptr };
    vk::raii::RenderPass m_renderPass{ nullptr };
    vk::raii::DescriptorSetLayout m_descriptorSetLayout{ nullptr };
    vk::raii::DescriptorPool m_descriptorPool{ nullptr };
    std::vector<vk::raii::DescriptorSet> m_descriptorSets;
    vk::raii::PipelineLayout m_pipelineLayout{ nullptr };
    vk::raii::PipelineCache m_pipelineCache{ nullptr };
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
        createPipelineCache();
        createGraphicsPipeline();
        savePipelineCache();
        createCommandPool();
        createColorResources();
        createDepthResources();
        createFramebuffers();
        createTextureImage(TEXTURE_PATH);
        createTextureImage(CRATE_TEXTURE_PATH);
        createTextureImageView();
        createTextureSampler();
        loadModel(MODEL_PATH);
        loadModel(BUNNY_PATH);
        loadModel(CRATE_OBJ_PATH);
        initInstanceDatas();
        initDynamicUboMatrices();
        createVertexBuffer();
        createInstanceBuffer();
        createIndexBuffer();
        createUniformBuffers();
        createDynamicUniformBuffers();
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
        for(const auto& it : m_dynamicUniformBuffersMemory){
            it.unmapMemory();
        }

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
                m_msaaSamples = getMaxUsableSampleCount();
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
                createImageView(
                    m_swapChainImages[i], 
                    m_swapChainImageFormat, 
                    vk::ImageAspectFlagBits::eColor,
                    1
                ) 
            );
        }
    }
    /////////////////////////////////////////////////////////////////

    /////////////////////////////////////////////////////////////////
    /// render pass
    void createRenderPass() {
        vk::AttachmentDescription colorAttachment;
        colorAttachment.format = m_swapChainImageFormat;
        colorAttachment.samples = m_msaaSamples;
        colorAttachment.loadOp = vk::AttachmentLoadOp::eClear;
        colorAttachment.storeOp = vk::AttachmentStoreOp::eStore;
        colorAttachment.stencilLoadOp = vk::AttachmentLoadOp::eDontCare;
        colorAttachment.stencilStoreOp = vk::AttachmentStoreOp::eDontCare;
        colorAttachment.initialLayout = vk::ImageLayout::eUndefined;
        colorAttachment.finalLayout = vk::ImageLayout::eColorAttachmentOptimal;

        vk::AttachmentReference colorAttachmentRef;
        colorAttachmentRef.attachment = 0;
        colorAttachmentRef.layout = vk::ImageLayout::eColorAttachmentOptimal;

        vk::AttachmentDescription depthAttachment;
        depthAttachment.format = findDepthFormat();
        depthAttachment.samples = m_msaaSamples;
        depthAttachment.loadOp = vk::AttachmentLoadOp::eClear;
        depthAttachment.storeOp = vk::AttachmentStoreOp::eDontCare;
        depthAttachment.stencilLoadOp = vk::AttachmentLoadOp::eDontCare;
        depthAttachment.stencilStoreOp = vk::AttachmentStoreOp::eDontCare;
        depthAttachment.initialLayout = vk::ImageLayout::eUndefined;
        depthAttachment.finalLayout = vk::ImageLayout::eDepthStencilAttachmentOptimal;

        vk::AttachmentReference depthAttachmentRef;
        depthAttachmentRef.attachment = 1;
        depthAttachmentRef.layout = vk::ImageLayout::eDepthStencilAttachmentOptimal;
        
        vk::AttachmentDescription colorAttachmentResolve;
        colorAttachmentResolve.format = m_swapChainImageFormat;
        colorAttachmentResolve.samples = vk::SampleCountFlagBits::e1;
        colorAttachmentResolve.loadOp = vk::AttachmentLoadOp::eDontCare;
        colorAttachmentResolve.storeOp = vk::AttachmentStoreOp::eStore;
        colorAttachmentResolve.stencilLoadOp = vk::AttachmentLoadOp::eDontCare;
        colorAttachmentResolve.stencilStoreOp = vk::AttachmentStoreOp::eDontCare;
        colorAttachmentResolve.initialLayout = vk::ImageLayout::eUndefined;
        colorAttachmentResolve.finalLayout = vk::ImageLayout::ePresentSrcKHR;

        vk::AttachmentReference colorAttachmentResolveRef;
        colorAttachmentResolveRef.attachment = 2;
        colorAttachmentResolveRef.layout = vk::ImageLayout::eColorAttachmentOptimal;


        vk::SubpassDescription subpass;
        subpass.pipelineBindPoint = vk::PipelineBindPoint::eGraphics;
        subpass.colorAttachmentCount = 1;
        subpass.pColorAttachments = &colorAttachmentRef;
        subpass.pDepthStencilAttachment = &depthAttachmentRef;
        subpass.pResolveAttachments = &colorAttachmentResolveRef;

        auto attachments = { colorAttachment, depthAttachment, colorAttachmentResolve };
        vk::RenderPassCreateInfo renderPassInfo;
        renderPassInfo.setAttachments( attachments );
        renderPassInfo.setSubpasses( subpass );

        vk::SubpassDependency dependency;
        dependency.srcSubpass = vk::SubpassExternal;
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

        auto vertexBindingDescription = Vertex::getBindingDescription();
        auto vertexAttributeDescriptions = Vertex::getAttributeDescriptions();
        auto instanceBindingDescription = InstanceData::getBindingDescription();
        auto instanceAttributeDescriptions = InstanceData::getAttributeDescriptions();

        std::vector<vk::VertexInputBindingDescription> bindingDescriptions = { 
            vertexBindingDescription, 
            instanceBindingDescription 
        };
        std::vector<vk::VertexInputAttributeDescription> attributeDescriptions;
        attributeDescriptions.insert(
            attributeDescriptions.end(),
            vertexAttributeDescriptions.begin(),
            vertexAttributeDescriptions.end()
        );
        attributeDescriptions.insert(
            attributeDescriptions.end(),
            instanceAttributeDescriptions.begin(),
            instanceAttributeDescriptions.end()
        );

        vertexInputInfo.setVertexBindingDescriptions(bindingDescriptions);
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
        multisampling.rasterizationSamples =  m_msaaSamples;
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

        vk::PushConstantRange pushConstantRange;
        pushConstantRange.stageFlags = vk::ShaderStageFlagBits::eFragment;
        pushConstantRange.offset = 0;
        pushConstantRange.size = sizeof(int32_t);

        vk::PipelineLayoutCreateInfo pipelineLayoutInfo;
        pipelineLayoutInfo.setPushConstantRanges( pushConstantRange );
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

        m_graphicsPipeline = m_device.createGraphicsPipeline( m_pipelineCache, pipelineInfo );
    }
    /////////////////////////////////////////////////////////////////

    /////////////////////////////////////////////////////////////////
    /// frame buffer
    void createFramebuffers() {
        m_swapChainFramebuffers.reserve( m_swapChainImageViews.size() );
        for (size_t i = 0; i < m_swapChainImageViews.size(); i++) {
            vk::FramebufferCreateInfo framebufferInfo;
            framebufferInfo.renderPass = m_renderPass;
            std::array<vk::ImageView, 3> attachments { 
                m_colorImageView, 
                m_depthImageView,
                m_swapChainImageViews[i]
            };
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

        vk::Buffer vertexBuffers[] = { *m_vertexBuffer, *m_instanceBuffer };
        vk::DeviceSize offsets[] = { 0, 0 };
        commandBuffer.bindVertexBuffers( 0, vertexBuffers, offsets );
        commandBuffer.bindIndexBuffer( m_indexBuffer, 0, vk::IndexType::eUint32 );

        uint32_t dynamicOffset = 0;
        int32_t enableTexture = 0;
        commandBuffer.bindDescriptorSets(
            vk::PipelineBindPoint::eGraphics, 
            m_pipelineLayout,
            0,
            *m_descriptorSets[m_currentFrame],
            dynamicOffset
        );
        commandBuffer.pushConstants<uint32_t>(
            m_pipelineLayout,
            vk::ShaderStageFlagBits::eFragment,
            0,              // offset
            enableTexture   // value
        );
        commandBuffer.drawIndexed( // draw the room
            m_firstIndices[1],
            1,
            0,
            0,
            0
        );

        dynamicOffset = sizeof(glm::mat4);
        enableTexture = -1;
        commandBuffer.bindDescriptorSets(
            vk::PipelineBindPoint::eGraphics, 
            m_pipelineLayout,
            0,
            *m_descriptorSets[m_currentFrame],
            dynamicOffset
        );
        commandBuffer.pushConstants<uint32_t>(
            m_pipelineLayout,
            vk::ShaderStageFlagBits::eFragment,
            0,              // offset
            enableTexture   // value
        );
        commandBuffer.drawIndexed( // draw the bunny
            m_firstIndices[2] - m_firstIndices[1],
            BUNNY_NUMBER,
            m_firstIndices[1],
            0, 
            1
        );

        dynamicOffset = 2 * sizeof(glm::mat4);
        enableTexture = 1;
        commandBuffer.bindDescriptorSets(
            vk::PipelineBindPoint::eGraphics, 
            m_pipelineLayout,
            0,
            *m_descriptorSets[m_currentFrame],
            dynamicOffset
        );
        commandBuffer.pushConstants<uint32_t>( // optional
            m_pipelineLayout,
            vk::ShaderStageFlagBits::eFragment,
            0,
            enableTexture
        );
        commandBuffer.drawIndexed( // draw the crate
            static_cast<uint32_t>(m_indices.size() - m_firstIndices[2]),
            1,
            m_firstIndices[2],
            0,
            BUNNY_NUMBER + 1
        );

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
        updateDynamicUniformBuffer(m_currentFrame);

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

        m_colorImageView = nullptr;
        m_colorImage = nullptr;
        m_colorImageMemory = nullptr;

        m_swapChainImageViews.clear();
        m_swapChainImages.clear(); // optional
        m_swapChain = nullptr;


        createSwapChain();
        createImageViews();
        createColorResources();
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
    struct UniformBufferObject {
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
        // samplerLayoutBinding.descriptorType = vk::DescriptorType::eCombinedImageSampler;
        samplerLayoutBinding.descriptorType = vk::DescriptorType::eSampler;
        samplerLayoutBinding.descriptorCount = 1;
        samplerLayoutBinding.stageFlags = vk::ShaderStageFlagBits::eFragment;

        vk::DescriptorSetLayoutBinding dynamicUboLayoutBinding;
        dynamicUboLayoutBinding.binding = 2;
        dynamicUboLayoutBinding.descriptorType = vk::DescriptorType::eUniformBufferDynamic;
        dynamicUboLayoutBinding.descriptorCount = 1;
        dynamicUboLayoutBinding.stageFlags = vk::ShaderStageFlagBits::eVertex;

        vk::DescriptorSetLayoutBinding textureLayoutBinding;
        textureLayoutBinding.binding = 3;
        textureLayoutBinding.descriptorType = vk::DescriptorType::eSampledImage;
        textureLayoutBinding.descriptorCount = 2;
        textureLayoutBinding.stageFlags = vk::ShaderStageFlagBits::eFragment;

        auto bindings = { 
            uboLayoutBinding, 
            samplerLayoutBinding, 
            dynamicUboLayoutBinding,
            textureLayoutBinding
        };
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
        std::array<vk::DescriptorPoolSize, 4> poolSizes;

        poolSizes[0].type = vk::DescriptorType::eUniformBuffer;
        poolSizes[0].descriptorCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);

        poolSizes[1].type = vk::DescriptorType::eSampler;
        poolSizes[1].descriptorCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);

        poolSizes[2].type = vk::DescriptorType::eUniformBufferDynamic;
        poolSizes[2].descriptorCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);

        poolSizes[3].type = vk::DescriptorType::eSampledImage;
        poolSizes[3].descriptorCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT * 2);

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

            vk::DescriptorImageInfo samplerInfo;
            samplerInfo.sampler = m_textureSampler;

            vk::DescriptorBufferInfo dynamicBufferInfo;
            dynamicBufferInfo.buffer = m_dynamicUniformBuffers[i];
            dynamicBufferInfo.offset = 0;
            dynamicBufferInfo.range = sizeof(glm::mat4);

            std::array<vk::DescriptorImageInfo, 2> textureInfos;
            for (size_t index = 0; auto& info : textureInfos) {
                info.imageLayout = vk::ImageLayout::eShaderReadOnlyOptimal;
                info.imageView = m_textureImageViews[index];
                ++index;
            }

            std::array<vk::WriteDescriptorSet, 4> descriptorWrites;
            descriptorWrites[0].dstSet = m_descriptorSets[i];
            descriptorWrites[0].dstBinding = 0;
            descriptorWrites[0].dstArrayElement = 0;
            descriptorWrites[0].descriptorType = vk::DescriptorType::eUniformBuffer;
            descriptorWrites[0].setBufferInfo(bufferInfo);

            descriptorWrites[1].dstSet = m_descriptorSets[i];
            descriptorWrites[1].dstBinding = 1;
            descriptorWrites[1].dstArrayElement = 0;
            descriptorWrites[1].descriptorType = vk::DescriptorType::eSampler;
            descriptorWrites[1].setImageInfo(samplerInfo);

            descriptorWrites[2].dstSet = m_descriptorSets[i];
            descriptorWrites[2].dstBinding = 2;
            descriptorWrites[2].dstArrayElement = 0;
            descriptorWrites[2].descriptorType = vk::DescriptorType::eUniformBufferDynamic;
            descriptorWrites[2].setBufferInfo(dynamicBufferInfo);

            descriptorWrites[3].dstSet = m_descriptorSets[i];
            descriptorWrites[3].dstBinding = 3;
            descriptorWrites[3].dstArrayElement = 0;
            descriptorWrites[3].descriptorType = vk::DescriptorType::eSampledImage;
            descriptorWrites[3].setImageInfo(textureInfos);

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
        uint32_t mipLevels,
        vk::SampleCountFlagBits numSamples,
        vk::Format format,
        vk::ImageTiling tiling,
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
        imageInfo.mipLevels = mipLevels;
        imageInfo.arrayLayers = 1;
        imageInfo.format = format;
        imageInfo.tiling = tiling;
        imageInfo.initialLayout = vk::ImageLayout::eUndefined;
        imageInfo.usage = usage;
        imageInfo.samples = numSamples;
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
        vk::ImageLayout newLayout,
        uint32_t mipLevels
    ) {
        vk::raii::CommandBuffer commandBuffer = beginSingleTimeCommands();

        vk::ImageMemoryBarrier barrier;
        barrier.oldLayout = oldLayout;
        barrier.newLayout = newLayout;
        barrier.srcQueueFamilyIndex = vk::QueueFamilyIgnored;
        barrier.dstQueueFamilyIndex = vk::QueueFamilyIgnored;
        barrier.image = image;
        // barrier.subresourceRange.aspectMask = vk::ImageAspectFlagBits::eColor;
        barrier.subresourceRange.baseMipLevel = 0;
        barrier.subresourceRange.levelCount = mipLevels;
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
    void createTextureImage(const std::string& texturePath) {
        int texWidth, texHeight, texChannels;
        stbi_uc* pixels = stbi_load(texturePath.c_str(), &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);
        if (!pixels) {
            throw std::runtime_error("failed to load texture image!");
        }
        vk::DeviceSize imageSize = texWidth * texHeight * 4;
        m_mipLevels = static_cast<uint32_t>(std::floor(std::log2(std::max(texWidth, texHeight)))) + 1;

        vk::raii::DeviceMemory stagingBufferMemory{ nullptr };
        vk::raii::Buffer stagingBuffer{ nullptr };
        vk::raii::DeviceMemory tmpTextureBufferMemory{ nullptr };
        vk::raii::Image tmpTextureBuffer{ nullptr };

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
            m_mipLevels,
            vk::SampleCountFlagBits::e1,
            vk::Format::eR8G8B8A8Srgb,
            vk::ImageTiling::eOptimal,
            vk::ImageUsageFlagBits::eTransferSrc |
            vk::ImageUsageFlagBits::eTransferDst | 
            vk::ImageUsageFlagBits::eSampled,
            vk::MemoryPropertyFlagBits::eDeviceLocal,
            tmpTextureBuffer,
            tmpTextureBufferMemory
        );

        transitionImageLayout(
            tmpTextureBuffer,
            vk::Format::eR8G8B8A8Srgb,
            vk::ImageLayout::eUndefined,
            vk::ImageLayout::eTransferDstOptimal,
            m_mipLevels
        );

        copyBufferToImage(
            stagingBuffer,
            tmpTextureBuffer,
            static_cast<uint32_t>(texWidth),
            static_cast<uint32_t>(texHeight)
        );

        generateMipmaps(
            tmpTextureBuffer,
            vk::Format::eR8G8B8A8Srgb,
            texWidth,
            texHeight,
            m_mipLevels
        );

        m_textureImages.emplace_back( std::move(tmpTextureBuffer) );
        m_textureImageMemory.emplace_back( std::move(tmpTextureBufferMemory) );
    }
    /////////////////////////////////////////////////////////////////

    /////////////////////////////////////////////////////////////////
    /// image view and sampler
    vk::raii::ImageView createImageView(
        vk::Image image, 
        vk::Format format, 
        vk::ImageAspectFlags aspectFlags,
        uint32_t mipLevels
    ) {
        vk::ImageViewCreateInfo viewInfo;
        viewInfo.image = image;
        viewInfo.viewType = vk::ImageViewType::e2D;
        viewInfo.format = format;
        viewInfo.subresourceRange.aspectMask = aspectFlags;
        viewInfo.subresourceRange.baseMipLevel = 0;
        viewInfo.subresourceRange.levelCount = mipLevels;
        viewInfo.subresourceRange.baseArrayLayer = 0;
        viewInfo.subresourceRange.layerCount = 1;

        return m_device.createImageView(viewInfo);
    }
    void createTextureImageView() {
        for (vk::raii::Image& image : m_textureImages) {
            m_textureImageViews.emplace_back(
                createImageView(
                    *image, 
                    vk::Format::eR8G8B8A8Srgb, 
                    vk::ImageAspectFlagBits::eColor,
                    m_mipLevels
                )
            );
        }
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
        samplerInfo.maxLod = static_cast<float>(m_mipLevels);

        // samplerInfo.minLod = static_cast<float>(m_mipLevels / 2);

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
            1,
            m_msaaSamples,
            depthFormat,
            vk::ImageTiling::eOptimal,
            vk::ImageUsageFlagBits::eDepthStencilAttachment,
            vk::MemoryPropertyFlagBits::eDeviceLocal,
            m_depthImage,
            m_depthImageMemory
        );

        m_depthImageView = createImageView(
            m_depthImage, 
            depthFormat, 
            vk::ImageAspectFlagBits::eDepth,
            1
        );

        // transitionImageLayout(
        //     m_depthImage,
        //     depthFormat,
        //     vk::ImageLayout::eUndefined,
        //     vk::ImageLayout::eDepthStencilAttachmentOptimal,
        //     1
        // );
    }
    /////////////////////////////////////////////////////////////////

    /////////////////////////////////////////////////////////////////
    /// load model
    void loadModel(const std::string& model_path) {
        tinyobj::attrib_t attrib;
        std::vector<tinyobj::shape_t> shapes;
        std::vector<tinyobj::material_t> materials;
        std::string warn, err;

        if (!tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, model_path.c_str())) {
            throw std::runtime_error(warn + err);
        }

        m_firstIndices.push_back(m_indices.size());
        
        static std::unordered_map<
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

                if (!attrib.texcoords.empty() && index.texcoord_index >= 0) {
                    vertex.texCoord = {
                        attrib.texcoords[2 * index.texcoord_index],
                        1.0f - attrib.texcoords[2 * index.texcoord_index + 1]
                    };
                } else {
                    vertex.texCoord = {0.0f, 0.0f};
                }

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

    /////////////////////////////////////////////////////////////////
    /// mipmaps
    void generateMipmaps(
        vk::raii::Image& image, 
        vk::Format imageFormat,
        int32_t texWidth, 
        int32_t texHeight, 
        uint32_t mipLevels
    ) {
        // vk::FormatProperties
        auto formatProperties = m_physicalDevice.getFormatProperties(imageFormat);

        if(!(formatProperties.optimalTilingFeatures & vk::FormatFeatureFlagBits::eSampledImageFilterLinear)){
            throw std::runtime_error("texture image format does not support linear blitting!");
        }


        auto commandBuffer = beginSingleTimeCommands();

        vk::ImageMemoryBarrier barrier;
        barrier.image = image;
        barrier.srcQueueFamilyIndex = vk::QueueFamilyIgnored;
        barrier.dstQueueFamilyIndex = vk::QueueFamilyIgnored;
        barrier.subresourceRange.aspectMask = vk::ImageAspectFlagBits::eColor;
        barrier.subresourceRange.baseArrayLayer = 0;
        barrier.subresourceRange.layerCount = 1;
        barrier.subresourceRange.levelCount = 1;

        int32_t mipWidth = texWidth;
        int32_t mipHeight = texHeight;

        for (uint32_t i = 1; i < mipLevels; ++i) {
            barrier.subresourceRange.baseMipLevel = i - 1;
            barrier.oldLayout = vk::ImageLayout::eTransferDstOptimal;
            barrier.newLayout = vk::ImageLayout::eTransferSrcOptimal;
            barrier.srcAccessMask = vk::AccessFlagBits::eTransferWrite;
            barrier.dstAccessMask = vk::AccessFlagBits::eTransferRead;
            commandBuffer.pipelineBarrier(
                vk::PipelineStageFlagBits::eTransfer,
                vk::PipelineStageFlagBits::eTransfer,
                {},
                nullptr,
                nullptr,
                barrier
            );
            
            vk::ImageBlit blit;
            blit.srcOffsets[0] = vk::Offset3D{ 0, 0, 0 };
            blit.srcOffsets[1] = vk::Offset3D{ mipWidth, mipHeight, 1 };
            blit.srcSubresource.aspectMask = vk::ImageAspectFlagBits::eColor;
            blit.srcSubresource.mipLevel = i - 1;
            blit.srcSubresource.baseArrayLayer = 0;
            blit.srcSubresource.layerCount = 1;
            blit.dstOffsets[0] = vk::Offset3D{ 0, 0, 0 };
            blit.dstOffsets[1] = vk::Offset3D{ 
                mipWidth > 1 ? mipWidth / 2 : 1, 
                mipHeight > 1 ? mipHeight / 2 : 1, 
                1 
            };
            blit.dstSubresource.aspectMask = vk::ImageAspectFlagBits::eColor;
            blit.dstSubresource.mipLevel = i;
            blit.dstSubresource.baseArrayLayer = 0;
            blit.dstSubresource.layerCount = 1;
            commandBuffer.blitImage(
                image, vk::ImageLayout::eTransferSrcOptimal,
                image, vk::ImageLayout::eTransferDstOptimal,
                blit,
                vk::Filter::eLinear
            );

            barrier.oldLayout = vk::ImageLayout::eTransferSrcOptimal;
            barrier.newLayout = vk::ImageLayout::eShaderReadOnlyOptimal;
            barrier.srcAccessMask = vk::AccessFlagBits::eTransferRead;
            barrier.dstAccessMask = vk::AccessFlagBits::eShaderRead;
            commandBuffer.pipelineBarrier(
                vk::PipelineStageFlagBits::eTransfer,
                vk::PipelineStageFlagBits::eFragmentShader,
                {},
                nullptr,
                nullptr,
                barrier
            );

            if (mipWidth > 1) mipWidth /= 2;
            if (mipHeight > 1) mipHeight /= 2;
        }

        barrier.subresourceRange.baseMipLevel = mipLevels - 1;
        barrier.oldLayout = vk::ImageLayout::eTransferDstOptimal;
        barrier.newLayout = vk::ImageLayout::eShaderReadOnlyOptimal;
        barrier.srcAccessMask = vk::AccessFlagBits::eTransferRead;
        barrier.dstAccessMask = vk::AccessFlagBits::eShaderRead;
        commandBuffer.pipelineBarrier(
            vk::PipelineStageFlagBits::eTransfer,
            vk::PipelineStageFlagBits::eFragmentShader,
            {},
            nullptr,
            nullptr,
            barrier
        );

        if (mipWidth > 1) mipWidth /= 2;
        if (mipHeight > 1) mipHeight /= 2;

        endSingleTimeCommands( std::move(commandBuffer) );
    }
    /////////////////////////////////////////////////////////////////

    /////////////////////////////////////////////////////////////////
    /// multi sample
    vk::SampleCountFlagBits getMaxUsableSampleCount() {
        // vk::PhysicalDeviceProperties
        auto properties = m_physicalDevice.getProperties();

        vk::SampleCountFlags counts = (
            properties.limits.framebufferColorSampleCounts & 
            properties.limits.framebufferDepthSampleCounts
        );

        if(counts & vk::SampleCountFlagBits::e64) return vk::SampleCountFlagBits::e64;
        if(counts & vk::SampleCountFlagBits::e32) return vk::SampleCountFlagBits::e32;
        if(counts & vk::SampleCountFlagBits::e16) return vk::SampleCountFlagBits::e16;
        if(counts & vk::SampleCountFlagBits::e8) return vk::SampleCountFlagBits::e8;
        if(counts & vk::SampleCountFlagBits::e4) return vk::SampleCountFlagBits::e4;
        if(counts & vk::SampleCountFlagBits::e2) return vk::SampleCountFlagBits::e2;
        return vk::SampleCountFlagBits::e1;
    }
    void createColorResources() {
        vk::Format colorFormat = m_swapChainImageFormat;

        createImage(
            m_swapChainExtent.width,
            m_swapChainExtent.height,
            1,
            m_msaaSamples,
            colorFormat,
            vk::ImageTiling::eOptimal,
            vk::ImageUsageFlagBits::eTransientAttachment |
            vk::ImageUsageFlagBits::eColorAttachment,
            vk::MemoryPropertyFlagBits::eDeviceLocal,
            m_colorImage,
            m_colorImageMemory
        );
        m_colorImageView = createImageView(
            m_colorImage,
            colorFormat,
            vk::ImageAspectFlagBits::eColor,
            1
        );
    }
    /////////////////////////////////////////////////////////////////

    /////////////////////////////////////////////////////////////////
    /// instance data
    struct alignas(16) InstanceData {
        glm::mat4 model;

        static vk::VertexInputBindingDescription getBindingDescription() {
            vk::VertexInputBindingDescription bindingDescription;
            bindingDescription.binding = 1; // binding 1 for instance data
            bindingDescription.stride = sizeof(InstanceData);
            bindingDescription.inputRate = vk::VertexInputRate::eInstance;

            return bindingDescription;
        }

        static std::array<vk::VertexInputAttributeDescription, 4>  getAttributeDescriptions() {
            std::array<vk::VertexInputAttributeDescription, 4> attributeDescriptions;
            for(uint32_t i = 0; i < 4; ++i) {
                attributeDescriptions[i].binding = 1; // binding 1 for instance data
                attributeDescriptions[i].location = 3 + i; // location 3, 4, 5, 6
                attributeDescriptions[i].format = vk::Format::eR32G32B32A32Sfloat;
                attributeDescriptions[i].offset = sizeof(glm::vec4) * i;
            }
            return attributeDescriptions;
        }
    };
    void initInstanceDatas() {
        InstanceData instanceData;
        m_instanceDatas.reserve(BUNNY_NUMBER + 1);
        // room model
        instanceData.model = glm::rotate(
            glm::mat4(1.0f), 
            glm::radians(-90.0f), 
            glm::vec3(1.0f, 0.0f, 0.0f)
        ) *  glm::rotate(
            glm::mat4(1.0f), 
            glm::radians(-90.0f), 
            glm::vec3(0.0f, 0.0f, 1.0f)
        );
        m_instanceDatas.emplace_back( instanceData );

        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_real_distribution<float> dis(-1.0f, 1.0f);
        // initialize BUNNY_NUMBER instances
        for (int i = 0; i < BUNNY_NUMBER; ++i) {
            // randomly generate position and rotation
            instanceData.model = glm::translate(
                glm::mat4(1.0f), 
                glm::vec3(dis(gen), dis(gen), dis(gen))
            ) * glm::rotate(
                glm::mat4(1.0f), 
                glm::radians(dis(gen) * 180.0f), 
                glm::vec3(0.0f, 1.0f, 0.0f)
            );
            m_instanceDatas.emplace_back( instanceData );
        }

        instanceData.model = glm::translate(
            glm::mat4(1.0f),
            glm::vec3(0.0f, 0.0f, 1.2f)
        ) * glm::scale(
            glm::mat4(1.0f),
            glm::vec3(0.2f, 0.2f, 0.2f)
        );
        m_instanceDatas.emplace_back( instanceData );
    }
    void createInstanceBuffer() {
        vk::DeviceSize bufferSize = sizeof(InstanceData) * m_instanceDatas.size();

        vk::raii::DeviceMemory stagingBufferMemory{ nullptr };
        vk::raii::Buffer stagingBuffer{ nullptr };
        createBuffer(bufferSize,
            vk::BufferUsageFlagBits::eTransferSrc,
            vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent,
            stagingBuffer,
            stagingBufferMemory
        );

        void* data = stagingBufferMemory.mapMemory(0, bufferSize);
        memcpy(data, m_instanceDatas.data(), static_cast<size_t>(bufferSize));
        stagingBufferMemory.unmapMemory();

        createBuffer(bufferSize,
            vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eVertexBuffer,
            vk::MemoryPropertyFlagBits::eDeviceLocal,
            m_instanceBuffer,
            m_instanceBufferMemory
        );

        copyBuffer(stagingBuffer, m_instanceBuffer, bufferSize);
    }
    /////////////////////////////////////////////////////////////////

    /////////////////////////////////////////////////////////////////
    /// dynamic uniform buffer
    void initDynamicUboMatrices() {
        m_dynamicUboMatrices.push_back(glm::mat4(1.0f));
        m_dynamicUboMatrices.push_back(glm::mat4(1.0f));
        m_dynamicUboMatrices.push_back(glm::mat4(1.0f));
    }
    void createDynamicUniformBuffers() {
        vk::DeviceSize bufferSize  = sizeof(glm::mat4) * m_dynamicUboMatrices.size();

        m_dynamicUniformBuffers.reserve(MAX_FRAMES_IN_FLIGHT);
        m_dynamicUniformBuffersMemory.reserve(MAX_FRAMES_IN_FLIGHT);
        m_dynamicUniformBuffersMapped.reserve(MAX_FRAMES_IN_FLIGHT);

        for(size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
            m_dynamicUniformBuffers.emplace_back( nullptr );
            m_dynamicUniformBuffersMemory.emplace_back( nullptr );
            m_dynamicUniformBuffersMapped.emplace_back( nullptr );
            createBuffer(bufferSize, 
                vk::BufferUsageFlagBits::eUniformBuffer, 
                vk::MemoryPropertyFlagBits::eHostVisible | 
                vk::MemoryPropertyFlagBits::eHostCoherent, 
                m_dynamicUniformBuffers[i], 
                m_dynamicUniformBuffersMemory[i]
            );

            m_dynamicUniformBuffersMapped[i] = m_dynamicUniformBuffersMemory[i].mapMemory(0, bufferSize);
        }
    }
    void updateDynamicUniformBuffer(uint32_t currentImage) {
        static auto startTime = std::chrono::high_resolution_clock::now();
        auto currentTime = std::chrono::high_resolution_clock::now();
        float time = std::chrono::duration<float, std::chrono::seconds::period>(currentTime - startTime).count();
        startTime = currentTime;

        m_dynamicUboMatrices[1] = glm::rotate(
            m_dynamicUboMatrices[1], 
            glm::radians(time * 60.0f), 
            glm::vec3(0.0f, 1.0f, 0.0f)
        );

        memcpy(
            m_dynamicUniformBuffersMapped[currentImage],
            m_dynamicUboMatrices.data(),
            sizeof(glm::mat4) * m_dynamicUboMatrices.size()
        );
    }
    /////////////////////////////////////////////////////////////////

    /////////////////////////////////////////////////////////////////
    /// pipeline cache
    void createPipelineCache() {
        vk::PipelineCacheCreateInfo pipelineCacheInfo;
        std::vector<char> data;
        std::ifstream in("pipeline_cache.bin", std::ios::binary | std::ios::ate);
        if (in) {
            size_t size = in.tellg();
            in.seekg(0);
            data.resize(size);
            in.read(data.data(), size);
            pipelineCacheInfo.setInitialData<char>(data);
        }
        m_pipelineCache = m_device.createPipelineCache(pipelineCacheInfo);
    }
    void savePipelineCache() {
        // std::vector<uint8_t>
        auto cacheData = m_pipelineCache.getData();
        std::ofstream out("pipeline_cache.bin", std::ios::binary);
        out.write(reinterpret_cast<const char*>(cacheData.data()), cacheData.size());
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
