module;

#include <memory>
#include <array>
#include <vector>

export module Descriptor;

import vulkan_hpp;

import Config;
import Device;
import ShadowDepthImage;
import ShadowPipeline;
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
        std::shared_ptr<vht::ShadowDepthImage> m_shadow_depth_image{ nullptr };
        std::shared_ptr<vht::ShadowPipeline> m_shadow_pipeline{ nullptr };
        std::shared_ptr<vht::GraphicsPipeline> m_graphics_pipeline;
        std::shared_ptr<vht::UniformBuffer> m_uniform_buffer;
        std::shared_ptr<vht::TextureSampler> m_texture_sampler;
        std::shared_ptr<vht::LightUniformBuffer> m_light_uniform_buffer;
        vk::raii::DescriptorPool m_pool{ nullptr };
        std::vector<vk::raii::DescriptorSet> m_sets;
        std::vector<vk::raii::DescriptorSet> m_shadow_sets;
    public:
        explicit Descriptor(
            std::shared_ptr<vht::Device> device,
            std::shared_ptr<vht::ShadowDepthImage> shadow_depth_image,
            std::shared_ptr<vht::ShadowPipeline> shadow_pipeline,
            std::shared_ptr<vht::GraphicsPipeline> m_graphics_pipeline,
            std::shared_ptr<vht::UniformBuffer> m_uniform_buffer,
            std::shared_ptr<vht::TextureSampler> m_texture_sampler,
            std::shared_ptr<vht::LightUniformBuffer> m_light_uniform_buffer
        ):  m_device(std::move(device)),
            m_shadow_depth_image(std::move(shadow_depth_image)),
            m_shadow_pipeline(std::move(shadow_pipeline)),
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
        const std::vector<vk::raii::DescriptorSet>& shadow_sets() const { return m_shadow_sets; }

    private:
        void init() {
            create_descriptor_pool();
            create_descriptor_sets();
        }
        // 创建描述符池
        void create_descriptor_pool() {
            std::array<vk::DescriptorPoolSize, 2> pool_sizes;
            pool_sizes[0].type = vk::DescriptorType::eUniformBuffer;
            pool_sizes[0].descriptorCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT * 3); // UBO + 2 份 Light UBO
            pool_sizes[1].type = vk::DescriptorType::eCombinedImageSampler;
            pool_sizes[1].descriptorCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT * 2); // 纹理采样器 + 阴影贴图

            vk::DescriptorPoolCreateInfo poolInfo;
            poolInfo.flags = vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet;
            poolInfo.setPoolSizes( pool_sizes );
            poolInfo.maxSets = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT * 2); // 两个管线都需要 MAX_FRAMES_IN_FLIGHT

            m_pool = m_device->device().createDescriptorPool(poolInfo);
        }
        // 创建描述符集
        void create_descriptor_sets() {
            // 为阴影管线创建描述符集布局
            std::vector<vk::DescriptorSetLayout> shadow_layouts(MAX_FRAMES_IN_FLIGHT, *m_shadow_pipeline->descriptor_set_layout());
            vk::DescriptorSetAllocateInfo shadow_alloc_info;
            shadow_alloc_info.descriptorPool = m_pool;
            shadow_alloc_info.setSetLayouts( shadow_layouts );

            m_shadow_sets = m_device->device().allocateDescriptorSets(shadow_alloc_info);

            for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i) {
                vk::DescriptorBufferInfo shadow_light_buffer_info;
                shadow_light_buffer_info.buffer = m_light_uniform_buffer->buffers()[i];
                shadow_light_buffer_info.offset = 0;
                shadow_light_buffer_info.range = sizeof(LightUBO);

                vk::WriteDescriptorSet write;
                write.dstSet = m_shadow_sets[i];
                write.dstBinding = 0;
                write.dstArrayElement = 0;
                write.descriptorType = vk::DescriptorType::eUniformBuffer;
                write.setBufferInfo(shadow_light_buffer_info);

                m_device->device().updateDescriptorSets(write, nullptr);
            }

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

                vk::DescriptorBufferInfo light_buffer_info;
                light_buffer_info.buffer = m_light_uniform_buffer->buffers()[i];
                light_buffer_info.offset = 0;
                light_buffer_info.range = sizeof(LightUBO);

                vk::DescriptorImageInfo depth_map_info;
                depth_map_info.imageLayout = vk::ImageLayout::eShaderReadOnlyOptimal;
                depth_map_info.imageView = m_shadow_depth_image->image_views()[i];
                depth_map_info.sampler = m_shadow_depth_image->sampler();

                std::array<vk::WriteDescriptorSet, 4> writes;
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
                writes[2].dstSet = m_sets[i];
                writes[2].dstBinding = 2;
                writes[2].dstArrayElement = 0;
                writes[2].descriptorType = vk::DescriptorType::eUniformBuffer;
                writes[2].setBufferInfo(light_buffer_info);
                writes[3].dstSet = m_sets[i];
                writes[3].dstBinding = 3;
                writes[3].dstArrayElement = 0;
                writes[3].descriptorType = vk::DescriptorType::eCombinedImageSampler;
                writes[3].setImageInfo(depth_map_info);

                m_device->device().updateDescriptorSets(writes, nullptr);
            }
        }
    };

} // namespace vht

