module;

#include <memory>
#include <array>
#include <vector>

export module Descriptor;

import vulkan_hpp;

import Config;
import Device;
import GBuffer;
import GraphicsPipeline;
import UniformBuffer;
import TextureSampler;
import LightUniformBuffer;

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
     *  - sets(): 获取描述符集列表
     */
    class Descriptor {
        std::shared_ptr<vht::Device> m_device;
        std::shared_ptr<vht::GBuffer> m_g_buffer;
        std::shared_ptr<vht::GraphicsPipeline> m_graphics_pipeline;
        std::shared_ptr<vht::UniformBuffer> m_uniform_buffer;
        std::shared_ptr<vht::TextureSampler> m_texture_sampler;
        std::shared_ptr<vht::LightUniformBuffer> m_light_uniform_buffer;
        vk::raii::DescriptorPool m_pool{ nullptr };
        std::vector<vk::raii::DescriptorSet> m_sets;
        std::vector<vk::raii::DescriptorSet> m_second_sets;
    public:
        explicit Descriptor(
            std::shared_ptr<vht::Device> device,
            std::shared_ptr<vht::GBuffer> g_buffer,
            std::shared_ptr<vht::GraphicsPipeline> m_graphics_pipeline,
            std::shared_ptr<vht::UniformBuffer> m_uniform_buffer,
            std::shared_ptr<vht::TextureSampler> m_texture_sampler,
            std::shared_ptr<vht::LightUniformBuffer> m_light_uniform_buffer
        ):  m_device(std::move(device)),
            m_g_buffer(std::move(g_buffer)),
            m_graphics_pipeline(std::move(m_graphics_pipeline)),
            m_uniform_buffer(std::move(m_uniform_buffer)),
            m_texture_sampler(std::move(m_texture_sampler)),
            m_light_uniform_buffer(std::move(m_light_uniform_buffer)) {
            init();
        }

        [[nodiscard]]
        const vk::raii::DescriptorPool& pool() const { return m_pool; }
        [[nodiscard]]
        const std::vector<vk::raii::DescriptorSet>& sets() const { return m_sets; }
        [[nodiscard]]
        const std::vector<vk::raii::DescriptorSet>& second_sets() const { return m_second_sets; }

        void recreate() {
            m_sets.clear();
            m_second_sets.clear();
            create_descriptor_sets();
        }

    private:
        void init() {
            create_descriptor_pool();
            create_descriptor_sets();
        }
        // 创建描述符池
        void create_descriptor_pool() {
            std::array<vk::DescriptorPoolSize, 3> pool_sizes;
            pool_sizes[0].type = vk::DescriptorType::eUniformBuffer;
            pool_sizes[0].descriptorCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT * 2); // 包括 Light UBO
            pool_sizes[1].type = vk::DescriptorType::eCombinedImageSampler;
            pool_sizes[1].descriptorCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);
            pool_sizes[2].type = vk::DescriptorType::eInputAttachment;
            pool_sizes[2].descriptorCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT * 3);

            vk::DescriptorPoolCreateInfo poolInfo;
            poolInfo.flags = vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet;
            poolInfo.setPoolSizes( pool_sizes );
            poolInfo.maxSets = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT * 2);

            m_pool = m_device->device().createDescriptorPool(poolInfo);
        }
        // 创建描述符集
        void create_descriptor_sets() {
            std::vector<vk::DescriptorSetLayout> layouts(MAX_FRAMES_IN_FLIGHT, *m_graphics_pipeline->descriptor_set_layout());
            vk::DescriptorSetAllocateInfo alloc_info;
            alloc_info.descriptorPool = m_pool;
            alloc_info.setSetLayouts( layouts );

            m_sets = m_device->device().allocateDescriptorSets(alloc_info);

            for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i) {
                vk::DescriptorBufferInfo buffer_info;
                buffer_info.buffer = m_uniform_buffer->buffers()[i];
                buffer_info.offset = 0;
                buffer_info.range = sizeof(UBO);

                vk::DescriptorImageInfo image_info;
                image_info.imageLayout = vk::ImageLayout::eShaderReadOnlyOptimal;
                image_info.imageView = m_texture_sampler->image_view();
                image_info.sampler = m_texture_sampler->sampler();

                std::array<vk::WriteDescriptorSet, 2> writes;
                writes[0].dstSet = m_sets[i];
                writes[0].dstBinding = 0;
                writes[0].dstArrayElement = 0;
                writes[0].descriptorType = vk::DescriptorType::eUniformBuffer;
                writes[0].setBufferInfo(buffer_info);
                writes[1].dstSet = m_sets[i];
                writes[1].dstBinding = 1;
                writes[1].dstArrayElement = 0;
                writes[1].descriptorType = vk::DescriptorType::eCombinedImageSampler;
                writes[1].setImageInfo(image_info);

                m_device->device().updateDescriptorSets(writes, nullptr);
            }

            std::vector<vk::DescriptorSetLayout> second_layouts(MAX_FRAMES_IN_FLIGHT, *m_graphics_pipeline->second_descriptor_set_layout());
            vk::DescriptorSetAllocateInfo second_alloc_info;
            second_alloc_info.descriptorPool = m_pool;
            second_alloc_info.setSetLayouts( second_layouts );

            m_second_sets = m_device->device().allocateDescriptorSets(second_alloc_info);

            for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i) {
                vk::DescriptorBufferInfo light_buffer_info;
                light_buffer_info.buffer = m_light_uniform_buffer->buffers()[i];
                light_buffer_info.offset = 0;
                light_buffer_info.range = sizeof(LightUBO);

                std::array<vk::DescriptorImageInfo, 3> input_attachments;
                input_attachments[0].imageLayout = vk::ImageLayout::eShaderReadOnlyOptimal;
                input_attachments[0].imageView = m_g_buffer->pos_views();
                input_attachments[1].imageLayout = vk::ImageLayout::eShaderReadOnlyOptimal;
                input_attachments[1].imageView = m_g_buffer->color_views();
                input_attachments[2].imageLayout = vk::ImageLayout::eShaderReadOnlyOptimal;
                input_attachments[2].imageView = m_g_buffer->normal_depth_views();

                std::array<vk::WriteDescriptorSet, 2> writes;
                writes[0].dstSet = m_second_sets[i];
                writes[0].dstBinding = 0;
                writes[0].dstArrayElement = 0;
                writes[0].descriptorType = vk::DescriptorType::eUniformBuffer;
                writes[0].setBufferInfo(light_buffer_info);
                writes[1].dstSet = m_second_sets[i];
                writes[1].dstBinding = 1;
                writes[1].dstArrayElement = 0;
                writes[1].descriptorType = vk::DescriptorType::eInputAttachment;
                writes[1].setImageInfo(input_attachments);

                m_device->device().updateDescriptorSets(writes, nullptr);
            }
        }
    };

} // namespace vht

