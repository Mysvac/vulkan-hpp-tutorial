module;

#include <memory>
#include <vector>

export module ShadowDepthImage;

import vulkan_hpp;

import Config;
import Utility;
import Device;

export namespace vht {

    /**
     * @brief 阴影深度图像相关
     * @details
     * - 依赖：
     *  - m_device: 物理/逻辑设备与队列
     *  - m_swapchain: 交换链
     * - 工作：
     *  - 创建阴影深度图像
     *  - 创建阴影深度图像视图
     * - 可访问成员：
     *  - image(): 阴影深度图像
     *  - image_view(): 阴影深度图像视图
     *  - format(): 阴影深度图像格式
     *  - width(): 阴影深度图像宽度
     *  - height(): 阴影深度图像高度
     */
    class ShadowDepthImage {
        std::shared_ptr<vht::Device> m_device{ nullptr };
        std::vector<vk::raii::DeviceMemory> m_memories;
        std::vector<vk::raii::Image> m_images;
        std::vector<vk::raii::ImageView> m_image_views;
        vk::raii::Sampler m_sampler{ nullptr };
        vk::Format m_format{ vk::Format::eD32Sfloat }; // 默认格式为 D32Sfloat
        uint32_t m_width{ 2000 }; // 深度图的大小，越大效果越好
        uint32_t m_height{ 1600 };
    public:
        explicit ShadowDepthImage(std::shared_ptr<vht::Device> device)
        :   m_device(std::move(device)) {
            init();
        }

        [[nodiscard]]
        const std::vector<vk::raii::Image>& images() const { return m_images; }
        [[nodiscard]]
        const std::vector<vk::raii::ImageView>& image_views() const { return m_image_views; }
        [[nodiscard]]
        const vk::raii::Sampler& sampler() const { return m_sampler; }
        [[nodiscard]]
        vk::Format format() const { return m_format; }
        [[nodiscard]]
        uint32_t width() const { return m_width; }
        [[nodiscard]]
        uint32_t height() const { return m_height; }

    private:
        void init() {
            create_depth_resources();
            create_sampler();
        }

        // 创建深度图像和视图
        void create_depth_resources() {
            m_images.reserve(MAX_FRAMES_IN_FLIGHT);
            m_image_views.reserve(MAX_FRAMES_IN_FLIGHT);
            m_memories.reserve(MAX_FRAMES_IN_FLIGHT);
            for (int i=0; i<vht::MAX_FRAMES_IN_FLIGHT ;++i) {
                m_memories.emplace_back(nullptr);
                m_images.emplace_back(nullptr);
                m_image_views.emplace_back(nullptr);
                create_image(
                    m_images.back(),
                    m_memories.back(),
                    m_device->device(),
                    m_device->physical_device(),
                    m_width,
                    m_height,
                    m_format,
                    vk::ImageTiling::eOptimal,
                    vk::ImageUsageFlagBits::eDepthStencilAttachment |
                    vk::ImageUsageFlagBits::eSampled,
                    vk::MemoryPropertyFlagBits::eDeviceLocal
                );
                m_image_views.back() = create_image_view(
                    m_device->device(),
                    m_images.back(),
                    m_format,
                    vk::ImageAspectFlagBits::eDepth
                );
            }
        }
        // 创建采样器
        void create_sampler() {
            vk::SamplerCreateInfo create_info;
            create_info.magFilter = vk::Filter::eLinear;
            create_info.minFilter = vk::Filter::eLinear;
            create_info.mipmapMode = vk::SamplerMipmapMode::eNearest;
            create_info.addressModeU = vk::SamplerAddressMode::eClampToEdge;
            create_info.addressModeV = vk::SamplerAddressMode::eClampToEdge;
            create_info.addressModeW = vk::SamplerAddressMode::eClampToEdge;
            create_info.compareEnable = true;
            create_info.compareOp = vk::CompareOp::eLess;
            create_info.borderColor = vk::BorderColor::eFloatOpaqueWhite;

            m_sampler = m_device->device().createSampler( create_info );
        }

    };
}


