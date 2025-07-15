export module DepthImage;

import std;
import vulkan_hpp;

import Tools;
import Device;
import Swapchain;
import CommandPool;

export namespace vht {

    /**
     * @brief 深度图像相关
     * @details
     * - 依赖：
     *  - m_device: 物理/逻辑设备与队列
     *  - m_swapchain: 交换链
     * - 工作：
     *  - 创建深度图像
     *  - 创建深度图像视图
     * - 可访问成员：
     *  - image(): 深度图像
     *  - image_view(): 深度图像视图
     *  - format(): 深度图像格式
     */
    class DepthImage {
        std::shared_ptr<vht::Device> m_device{ nullptr };
        std::shared_ptr<vht::Swapchain> m_swapchain{ nullptr };
        std::shared_ptr<vht::CommandPool> m_command_pool{ nullptr };
        vk::raii::DeviceMemory m_memory{ nullptr };
        vk::raii::Image m_image{ nullptr };
        vk::raii::ImageView m_image_view{ nullptr };
        vk::Format m_format{};
    public:
        explicit DepthImage(
            std::shared_ptr<vht::Device> device,
            std::shared_ptr<vht::Swapchain> swapchain,
            std::shared_ptr<vht::CommandPool> command_pool
        ):  m_device(std::move(device)),
            m_swapchain(std::move(swapchain)),
            m_command_pool(std::move(command_pool)) {
            init();
        }

        [[nodiscard]]
        const vk::raii::Image& image() const { return m_image; }
        [[nodiscard]]
        const vk::raii::ImageView& image_view() const { return m_image_view; }
        [[nodiscard]]
        vk::Format format() const { return m_format; }

        // 重建深度图像和视图
        void recreate() {
            m_image_view = nullptr;
            m_image = nullptr;
            create_depth_resources();
        }
    private:
        void init() {
            find_depth_format();
            create_depth_resources();
        }
        // 查找支持的深度格式
        void find_depth_format() {
            for(const vk::Format format : { vk::Format::eD32Sfloat, vk::Format::eD32SfloatS8Uint, vk::Format::eD24UnormS8Uint }) {
                if(const auto props = m_device->physical_device().getFormatProperties(format);
                    props.optimalTilingFeatures & vk::FormatFeatureFlagBits::eDepthStencilAttachment
                ) {
                    m_format = format;
                    return;
                }
            }
            throw std::runtime_error("failed to find supported format!");
        }
        // 创建深度图像和视图
        void create_depth_resources() {
            create_image(
                m_image,
                m_memory,
                m_device->device(),
                m_device->physical_device(),
                m_swapchain->extent().width,
                m_swapchain->extent().height,
                m_format,
                vk::ImageTiling::eOptimal,
                vk::ImageUsageFlagBits::eDepthStencilAttachment,
                vk::MemoryPropertyFlagBits::eDeviceLocal
            );
            m_image_view = create_image_view(
                m_device->device(),
                m_image,
                m_format,
                vk::ImageAspectFlagBits::eDepth
            );

            transition_image_layout(
                m_command_pool->pool(),
                m_device->device(),
                m_device->graphics_queue(),
                m_image,
                vk::ImageLayout::eUndefined,
                vk::ImageLayout::eAttachmentOptimal,
                vk::ImageAspectFlagBits::eDepth
            );
        }

    };

}


