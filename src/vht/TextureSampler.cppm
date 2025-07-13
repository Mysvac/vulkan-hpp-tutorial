export module TextureSampler;

import std;
import stbi;
import vulkan_hpp;

import Tools;
import Device;
import CommandPool;

// 纹理路径
const std::string TEXTURE_PATH = "textures/viking_room.png";

export namespace vht {

    /**
     * @brief 纹理采样器
     * @details
     * - 依赖：
     *  - m_device: 逻辑设备与队列
     *  - m_command_pool: 命令池
     * - 工作：
     *  - 创建纹理图像、图像视图和采样器
     * - 可访问成员：
     *  - image(): 获取纹理图像
     *  - image_view(): 获取纹理图像视图
     *  - sampler(): 获取纹理采样器
     */
    class TextureSampler {
        std::shared_ptr<vht::Device> m_device;
        std::shared_ptr<vht::CommandPool> m_command_pool;
        vk::raii::DeviceMemory m_memory{ nullptr };
        vk::raii::Image m_image{ nullptr };
        vk::raii::ImageView m_image_view{ nullptr };
        vk::raii::Sampler m_sampler{ nullptr };
    public:
        explicit TextureSampler(std::shared_ptr<vht::Device> device, std::shared_ptr<vht::CommandPool> command_pool)
        :   m_device(std::move(device)),
            m_command_pool(std::move(command_pool)) {
            init();
        }

        [[nodiscard]]
        const vk::raii::Image& image() const { return m_image; }
        [[nodiscard]]
        const vk::raii::ImageView& image_view() const { return m_image_view; }
        [[nodiscard]]
        const vk::raii::Sampler& sampler() const { return m_sampler; }

    private:
        void init() {
            create_texture_image();
            create_texture_image_view();
            create_texture_sampler();
        }
        // 创建纹理图像
        void create_texture_image() {
            int tex_width, tex_height, tex_channels;
            stbi::uc* pixels = stbi::load(TEXTURE_PATH.c_str(), &tex_width, &tex_height, &tex_channels, stbi::RGB_ALPHA);

            if (!pixels) {
                throw std::runtime_error("failed to load texture image!");
            }
            const vk::DeviceSize image_size = tex_width * tex_height * 4;

            vk::raii::DeviceMemory staging_memory{ nullptr };
            vk::raii::Buffer staging_buffer{ nullptr };

            vht::create_buffer(
                staging_buffer,
                staging_memory,
                m_device->device(),
                m_device->physical_device(),
                image_size,
                vk::BufferUsageFlagBits::eTransferSrc,
                vk::MemoryPropertyFlagBits::eHostVisible |
                vk::MemoryPropertyFlagBits::eHostCoherent
            );

            void* data = staging_memory.mapMemory(0, image_size);
            std::memcpy(data, pixels, static_cast<std::size_t>(image_size));
            staging_memory.unmapMemory();

            stbi::image_free(pixels);

            vht::create_image(
                m_image,
                m_memory,
                m_device->device(),
                m_device->physical_device(),
                tex_width,
                tex_height,
                vk::Format::eR8G8B8A8Srgb,
                vk::ImageTiling::eOptimal,
                vk::ImageUsageFlagBits::eTransferDst |
                vk::ImageUsageFlagBits::eSampled,
                vk::MemoryPropertyFlagBits::eDeviceLocal
            );

            vht::transition_image_layout(
                m_command_pool->pool(),
                m_device->device(),
                m_device->graphics_queue(),
                m_image,
                vk::ImageLayout::eUndefined,
                vk::ImageLayout::eTransferDstOptimal
            );

            vht::copy_buffer_to_image(
                m_command_pool->pool(),
                m_device->device(),
                m_device->graphics_queue(),
                staging_buffer,
                m_image,
                static_cast<std::uint32_t>(tex_width),
                static_cast<std::uint32_t>(tex_height)
            );

            vht::transition_image_layout(
                m_command_pool->pool(),
                m_device->device(),
                m_device->graphics_queue(),
                m_image,
                vk::ImageLayout::eTransferDstOptimal,
                vk::ImageLayout::eShaderReadOnlyOptimal
            );
        }
        // 创建纹理图像视图
        void create_texture_image_view() {
            m_image_view = vht::create_image_view(
                m_device->device(),
                m_image,
                vk::Format::eR8G8B8A8Srgb,
                vk::ImageAspectFlagBits::eColor
            );
        }
        // 创建纹理采样器
        void create_texture_sampler() {
            vk::SamplerCreateInfo create_info;
            create_info.magFilter = vk::Filter::eLinear;
            create_info.minFilter = vk::Filter::eLinear;
            create_info.addressModeU = vk::SamplerAddressMode::eRepeat;
            create_info.addressModeV = vk::SamplerAddressMode::eRepeat;
            create_info.addressModeW = vk::SamplerAddressMode::eRepeat;
            create_info.anisotropyEnable = true;
            const auto properties = m_device->physical_device().getProperties();
            if (const auto features = m_device->physical_device().getFeatures();
                features.samplerAnisotropy
            ) {
                create_info.anisotropyEnable = true;
                create_info.maxAnisotropy = properties.limits.maxSamplerAnisotropy;
            } else {
                create_info.anisotropyEnable = false;
                create_info.maxAnisotropy = 1.0f;
            }
            create_info.borderColor = vk::BorderColor::eIntOpaqueBlack;
            create_info.unnormalizedCoordinates = false;
            create_info.compareEnable = false;
            create_info.compareOp = vk::CompareOp::eAlways;
            create_info.mipmapMode = vk::SamplerMipmapMode::eLinear;
            create_info.mipLodBias = 0.0f;
            create_info.minLod = 0.0f;
            create_info.maxLod = 0.0f;
            m_sampler = m_device->device().createSampler(create_info);
        }

    };
}

