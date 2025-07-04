module;

#include <memory>

export module DepthImage;

import vulkan_hpp;

import Utility;
import Device;
import Swapchain;

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
        std::shared_ptr<vht::Swapchain> m_Swapchain{ nullptr };
        vk::raii::DeviceMemory m_memory{ nullptr };
        vk::raii::Image m_image{ nullptr };
        vk::raii::ImageView m_image_view{ nullptr };
        vk::Format m_format{ vk::Format::eD32Sfloat }; // 默认格式为 D32Sfloat
    public:
        explicit DepthImage(std::shared_ptr<vht::Device> device, std::shared_ptr<vht::Swapchain> swapchain)
        :   m_device(std::move(device)),
            m_Swapchain(std::move(swapchain)) {
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
            create_depth_resources();
        }

        // 创建深度图像和视图
        void create_depth_resources() {
            create_image(
                m_image,
                m_memory,
                m_device->device(),
                m_device->physical_device(),
                m_Swapchain->extent().width,
                m_Swapchain->extent().height,
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
        }

    };

}


