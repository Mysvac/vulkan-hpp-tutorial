export module Swapchain;

import std;
import glfw;
import vulkan_hpp;

import Tools;
import Window;
import Device;

export namespace vht {
    /**
     * @brief 交换链相关
     * @details
     * - 依赖：
     *  - m_window: 窗口系统
     *  - m_device: 逻辑设备与队列
     * - 工作：
     *  - 创建交换链
     *  - 获取交换链图像
     *  - 创建图像视图
     * - 可访问成员：
     *  - swapchain(): 交换链
     *  - images(): 交换链图像
     *  - image_views(): 交换链图像视图
     *  - format(): 交换链图像格式
     *  - extent(): 交换链图像尺寸
     *  - size(): 交换链图像数量
     */
    class Swapchain {
        std::shared_ptr<vht::Window> m_window{ nullptr };
        std::shared_ptr<vht::Device> m_device{ nullptr };
        vk::raii::SwapchainKHR m_swapchain{ nullptr };
        std::vector<vk::Image> m_images;
        std::vector<vk::raii::ImageView> m_image_views;
        vk::Format m_format{};
        vk::Extent2D m_extent{};
    public:
        explicit Swapchain(std::shared_ptr<vht::Window> window, std::shared_ptr<vht::Device> device)
        :   m_window(std::move(window)),
            m_device(std::move(device)) {
            init();
        }

        /**
         * @warning 此函数仅重置交换链和内部图像、图像视图，不重置帧缓冲。请使用渲染通道的 recreate 重置全部内容
         */
        void recreate() {
            m_image_views.clear();
            m_images.clear();
            m_swapchain = nullptr;
            create_swapchain();
            create_image_views();
        }

        [[nodiscard]]
        const vk::raii::SwapchainKHR& swapchain() const { return m_swapchain; }
        [[nodiscard]]
        const std::vector<vk::Image>& images() const { return m_images; }
        [[nodiscard]]
        const std::vector<vk::raii::ImageView>& image_views() const { return m_image_views; }
        [[nodiscard]]
        vk::Format format() const { return m_format; }
        [[nodiscard]]
        vk::Extent2D extent() const { return m_extent; }
        [[nodiscard]]
        std::size_t size() const { return m_images.size(); }
    private:
        void init() {
            create_swapchain();
            create_image_views();
        }

        // 选择交换链的表面格式
        [[nodiscard]]
        static vk::SurfaceFormatKHR choose_format(const std::span<const vk::SurfaceFormatKHR> formats) {
            if (formats.empty()) throw std::runtime_error("failed to find surface formats");
            for (const auto& format : formats) {
                if (format.format == vk::Format::eB8G8R8A8Srgb &&
                    format.colorSpace == vk::ColorSpaceKHR::eSrgbNonlinear
                ) return format;
            }
            return *formats.begin();
        }
        // 选择交换链的呈现模式
        [[nodiscard]]
        static vk::PresentModeKHR choose_present_mode(const std::span<const vk::PresentModeKHR> present_modes) {
            for (const auto& present_mode : present_modes) {
                if (present_mode == vk::PresentModeKHR::eMailbox) {
                    return present_mode;
                }
            }
            return vk::PresentModeKHR::eFifo;
        }
        // 选择交换链的图像尺寸
        [[nodiscard]]
        vk::Extent2D choose_extent(const vk::SurfaceCapabilitiesKHR& capabilities) const {
            if (capabilities.currentExtent.width != std::numeric_limits<std::uint32_t>::max()) {
                return capabilities.currentExtent;
            }
            int width, height;
            glfw::get_framebuffer_size( m_window->ptr(), &width, &height );
            return {
                std::clamp(static_cast<std::uint32_t>(width),capabilities.minImageExtent.width, capabilities.maxImageExtent.width),
                std::clamp(static_cast<std::uint32_t>(height),capabilities.minImageExtent.height,capabilities.maxImageExtent.height)
            };
        }
        // 创建交换链
        void create_swapchain() {
            const auto [capabilities, formats, present_modes] = m_device->swapchain_support();
            const auto format = choose_format( formats );
            const auto present_mode = choose_present_mode( present_modes );
            const auto extent = choose_extent( capabilities );

            std::uint32_t image_count = capabilities.minImageCount + 1;
            if (capabilities.maxImageCount > 0 && image_count > capabilities.maxImageCount ) {
                image_count = capabilities.maxImageCount;
            }


            vk::SwapchainCreateInfoKHR create_info;
            create_info.surface = m_window->surface();
            create_info.minImageCount = image_count;
            create_info.imageFormat = format.format;
            create_info.imageColorSpace = format.colorSpace;
            create_info.imageExtent = extent;
            create_info.imageArrayLayers = 1;
            create_info.imageUsage = vk::ImageUsageFlagBits::eColorAttachment;
            create_info.presentMode = present_mode;
            create_info.preTransform = capabilities.currentTransform;
            create_info.compositeAlpha = vk::CompositeAlphaFlagBitsKHR::eOpaque;
            create_info.clipped = true;
            create_info.oldSwapchain = nullptr;

            const auto [graphics_family, present_family] = m_device->queue_family_indices();
            std::vector<std::uint32_t> queueFamilyIndices { graphics_family.value(), present_family.value() };
            if (graphics_family.value() != present_family.value()) {
                create_info.imageSharingMode = vk::SharingMode::eConcurrent;
                create_info.setQueueFamilyIndices( queueFamilyIndices );
            } else {
                create_info.imageSharingMode = vk::SharingMode::eExclusive;
            }

            m_swapchain = m_device->device().createSwapchainKHR( create_info );
            m_images = m_swapchain.getImages();
            m_format = format.format;
            m_extent = extent;
        }
        // 创建交换链图像视图
        void create_image_views() {
            m_image_views.clear();
            m_image_views.reserve( m_images.size() );
            for (std::size_t i = 0; const auto& image : m_images) {
                // create_image_view 函数在 Tools.cppm 中定义
                m_image_views.emplace_back( vht::create_image_view(
                    m_device->device(),
                    image,
                    m_format,
                    vk::ImageAspectFlagBits::eColor
                ));
                ++i;
            }
        }
    };
}

