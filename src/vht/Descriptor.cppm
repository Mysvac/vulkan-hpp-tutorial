export module Descriptor;

import std;
import vulkan_hpp;

import Config;
import Device;
import GraphicsPipeline;
import UniformBuffer;
import TextureSampler;

export namespace vht {

    /**
     * @brief 描述符集管理类
     * @details
     * - 依赖：
     *  - m_device: 逻辑设备
     *  - m_graphics_pipeline: 图形管线
     *  - m_uniform_buffer: Uniform Buffer对象
     *  - m_texture_sampler: 纹理采样器对象
     * - 工作：
     *  - 创建描述符池和描述符集
     * - 可访问成员：
     *  - pool(): 获取描述符池
     *  - ubo_sets(): 获取UBO描述符集列表
     *  - texture_set(): 获取纹理描述符集
     */
    class Descriptor {
        std::shared_ptr<vht::Device> m_device;
        std::shared_ptr<vht::GraphicsPipeline> m_graphics_pipeline;
        std::shared_ptr<vht::UniformBuffer> m_uniform_buffer;
        std::shared_ptr<vht::TextureSampler> m_texture_sampler;
        vk::raii::DescriptorPool m_pool{ nullptr };
        std::vector<vk::raii::DescriptorSet> m_ubo_sets;
        vk::raii::DescriptorSet m_texture_set{ nullptr };
    public:
        explicit Descriptor(
            std::shared_ptr<vht::Device> device,
            std::shared_ptr<vht::GraphicsPipeline> m_graphics_pipeline,
            std::shared_ptr<vht::UniformBuffer> m_uniform_buffer,
            std::shared_ptr<vht::TextureSampler> m_texture_sampler
        ):  m_device(std::move(device)),
            m_graphics_pipeline(std::move(m_graphics_pipeline)),
            m_uniform_buffer(std::move(m_uniform_buffer)),
            m_texture_sampler(std::move(m_texture_sampler)) {
            init();
        }

        [[nodiscard]]
        const vk::raii::DescriptorPool& pool() const { return m_pool; }
        [[nodiscard]]
        const std::vector<vk::raii::DescriptorSet>& ubo_sets() const { return m_ubo_sets; }
        [[nodiscard]]
        const vk::raii::DescriptorSet& texture_set() const { return m_texture_set; }

    private:
        void init() {
            create_descriptor_pool();
            create_descriptor_sets();
        }
        // 创建描述符池
        void create_descriptor_pool() {
            std::array<vk::DescriptorPoolSize, 2> pool_sizes;
            pool_sizes[0].type = vk::DescriptorType::eUniformBuffer;
            pool_sizes[0].descriptorCount = static_cast<std::uint32_t>(MAX_FRAMES_IN_FLIGHT);
            pool_sizes[1].type = vk::DescriptorType::eCombinedImageSampler;
            pool_sizes[1].descriptorCount = 1;

            vk::DescriptorPoolCreateInfo poolInfo;
            poolInfo.flags = vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet;
            poolInfo.setPoolSizes( pool_sizes );
            poolInfo.maxSets = static_cast<std::uint32_t>(MAX_FRAMES_IN_FLIGHT + 1);

            m_pool = m_device->device().createDescriptorPool(poolInfo);
        }
        // 创建描述符集
        void create_descriptor_sets() {
            std::vector<vk::DescriptorSetLayout> layouts(MAX_FRAMES_IN_FLIGHT, *m_graphics_pipeline->descriptor_set_layouts().at(0));
            vk::DescriptorSetAllocateInfo alloc_info;
            alloc_info.descriptorPool = m_pool;
            alloc_info.setSetLayouts( layouts );

            m_ubo_sets = m_device->device().allocateDescriptorSets(alloc_info);

            for (std::size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i) {
                vk::DescriptorBufferInfo buffer_info;
                buffer_info.buffer = m_uniform_buffer->buffers()[i];
                buffer_info.offset = 0;
                buffer_info.range = sizeof(UBO);

                vk::WriteDescriptorSet write;
                write.dstSet = m_ubo_sets[i];
                write.dstBinding = 0;
                write.dstArrayElement = 0;
                write.descriptorType = vk::DescriptorType::eUniformBuffer;
                write.setBufferInfo(buffer_info);

                m_device->device().updateDescriptorSets(write, nullptr);
            }

            alloc_info.setSetLayouts(*m_graphics_pipeline->descriptor_set_layouts().at(1));
            m_texture_set =  std::move(m_device->device().allocateDescriptorSets(alloc_info).at(0));

            vk::DescriptorImageInfo image_info;
            image_info.imageLayout = vk::ImageLayout::eShaderReadOnlyOptimal;
            image_info.imageView = m_texture_sampler->image_view();
            image_info.sampler = m_texture_sampler->sampler();

            vk::WriteDescriptorSet write;
            write.dstSet = m_texture_set;
            write.dstBinding = 0;
            write.dstArrayElement = 0;
            write.descriptorType = vk::DescriptorType::eCombinedImageSampler;
            write.setImageInfo(image_info);

            m_device->device().updateDescriptorSets(write, nullptr);
        }
    };

} // namespace vht

