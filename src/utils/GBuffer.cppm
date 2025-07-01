module;

#include <memory>
#include <vector>

export module GBuffer;

import vulkan_hpp;

import Config;
import Utility;
import Device;
import Swapchain;

export namespace vht {

    /**
     * @brief GBuffer 类
     * @details
     * - pos 顶点位置缓冲区
     * - color 原色彩缓冲区
     * - normal_depth 法线和深度缓冲区
     * - recreate(): 重新创建 GBuffer 资源
     */
    class GBuffer {
        std::shared_ptr<vht::Device> m_device{ nullptr };
        std::shared_ptr<vht::Swapchain> m_swapchain{ nullptr };
        // 用于记录顶点位置的资源
        vk::raii::DeviceMemory m_pos_memory{ nullptr };
        vk::raii::Image m_pos_image{ nullptr };
        vk::raii::ImageView m_pos_view{ nullptr };
        vk::Format m_pos_format{ vk::Format::eR32G32B32A32Sfloat }; // 四通道保证支持，三通道可能不被所有 GPU 支持
        // 用于记录原色彩的资源
        vk::raii::DeviceMemory m_color_memory{ nullptr };
        vk::raii::Image m_color_image{ nullptr };
        vk::raii::ImageView m_color_view{ nullptr };
        vk::Format m_color_format{ vk::Format::eR8G8B8A8Srgb }; // 色彩格式，使用 sRGB 格式以便于显示
        // 用于记录法线和深度的资源
        vk::raii::DeviceMemory m_normal_depth_memory{ nullptr };
        vk::raii::Image m_normal_depth_image{ nullptr };
        vk::raii::ImageView m_normal_depth_view{ nullptr };
        vk::Format m_normal_depth_format{ vk::Format::eR32G32B32A32Sfloat }; // 四通道保证支持，三通道可能不被所有 GPU 支持
    public:
        explicit GBuffer(std::shared_ptr<vht::Device> device, std::shared_ptr<vht::Swapchain> swapchain)
        :   m_device(std::move(device)),
            m_swapchain(std::move(swapchain)) {
            create_pos_images();
            create_color_images();
            create_normal_depth_images();
        }

        void recreate() {
            m_pos_view.clear();
            m_pos_image.clear();
            m_pos_memory.clear();
            m_color_view.clear();
            m_color_image.clear();
            m_color_memory.clear();
            m_normal_depth_view.clear();
            m_normal_depth_image.clear();
            m_normal_depth_memory.clear();
            create_pos_images();
            create_color_images();
            create_normal_depth_images();
        }


        [[nodiscard]]
        const vk::raii::Image& pos_images() const { return m_pos_image; }
        [[nodiscard]]
        const vk::raii::ImageView& pos_views() const { return m_pos_view; }
        [[nodiscard]]
        vk::Format pos_format() const { return m_pos_format; }
        [[nodiscard]]
        const vk::raii::Image& color_images() const { return m_color_image; }
        [[nodiscard]]
        const vk::raii::ImageView& color_views() const { return m_color_view; }
        [[nodiscard]]
        vk::Format color_format() const { return m_color_format; }
        [[nodiscard]]
        const vk::raii::Image& normal_depth_images() const { return m_normal_depth_image; }
        [[nodiscard]]
        const vk::raii::ImageView& normal_depth_views() const { return m_normal_depth_view; }
        [[nodiscard]]
        vk::Format normal_depth_format() const { return m_normal_depth_format; }

    private:
        void create_pos_images() {
            create_image(
                m_pos_image,
                m_pos_memory,
                m_device->device(),
                m_device->physical_device(),
                m_swapchain->extent().width,
                m_swapchain->extent().height,
                m_pos_format,
                vk::ImageTiling::eOptimal,
                vk::ImageUsageFlagBits::eColorAttachment |  // 需要作为色彩写入附件
                vk::ImageUsageFlagBits::eInputAttachment,   // 需要作为输入附件
                vk::MemoryPropertyFlagBits::eDeviceLocal
            );
            m_pos_view = create_image_view(
                m_device->device(),
                m_pos_image,
                m_pos_format,
                vk::ImageAspectFlagBits::eColor // 依然作为色彩
            );
        }

        void create_color_images() {
            create_image(
                m_color_image,
                m_color_memory,
                m_device->device(),
                m_device->physical_device(),
                m_swapchain->extent().width,
                m_swapchain->extent().height,
                m_color_format,
                vk::ImageTiling::eOptimal,
                vk::ImageUsageFlagBits::eColorAttachment |  // 需要作为色彩写入附件
                vk::ImageUsageFlagBits::eInputAttachment,   // 需要作为输入附件
                vk::MemoryPropertyFlagBits::eDeviceLocal
            );
            m_color_view = create_image_view(
                m_device->device(),
                m_color_image,
                m_color_format,
                vk::ImageAspectFlagBits::eColor // 依然作为色彩
            );
        }

        void create_normal_depth_images() {
            create_image(
                m_normal_depth_image,
                m_normal_depth_memory,
                m_device->device(),
                m_device->physical_device(),
                m_swapchain->extent().width,
                m_swapchain->extent().height,
                m_normal_depth_format,
                vk::ImageTiling::eOptimal,
                vk::ImageUsageFlagBits::eColorAttachment |  // 需要作为色彩写入附件
                vk::ImageUsageFlagBits::eInputAttachment,   // 需要作为输入附件
                vk::MemoryPropertyFlagBits::eDeviceLocal
            );
            m_normal_depth_view = create_image_view(
                m_device->device(),
                m_normal_depth_image,
                m_normal_depth_format,
                vk::ImageAspectFlagBits::eColor // 依然作为色彩
            );
        }
    };
}
