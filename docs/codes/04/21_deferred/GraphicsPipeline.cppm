module;

#include <memory>
#include <vector>

export module GraphicsPipeline;

import vulkan_hpp;

import DataLoader;
import Utility;
import Device;
import RenderPass;

export namespace vht {

    /**
     * @brief 图形管线相关
     * @details
     * - 依赖：
     *  - m_device: 逻辑设备与队列
     *  - m_render_pass: 渲染通道
     * - 工作：
     *  - 创建描述符集布局
     *  - 创建图形管线布局和图形管线
     * - 可访问成员：
     *  - descriptor_set_layout(): 描述符集布局
     *  - pipeline_layout(): 管线布局
     *  - pipeline(): 图形管线
     */
    class GraphicsPipeline {
        std::shared_ptr<vht::Device> m_device;
        std::shared_ptr<vht::RenderPass> m_render_pass;
        vk::raii::DescriptorSetLayout m_descriptor_set_layout{ nullptr };
        vk::raii::PipelineLayout m_pipeline_layout{ nullptr };
        vk::raii::Pipeline m_pipeline{ nullptr };
        vk::raii::DescriptorSetLayout m_second_descriptor_set_layout{ nullptr };
        vk::raii::PipelineLayout m_second_pipeline_layout{ nullptr };
        vk::raii::Pipeline m_second_pipeline{ nullptr };
    public:
        explicit GraphicsPipeline(std::shared_ptr<vht::Device> device, std::shared_ptr<vht::RenderPass> render_pass)
        :   m_device(std::move(device)),
            m_render_pass(std::move(render_pass)) {
            init();
        }

        [[nodiscard]]
        const vk::raii::DescriptorSetLayout& descriptor_set_layout() const { return m_descriptor_set_layout; }
        [[nodiscard]]
        const vk::raii::PipelineLayout& pipeline_layout() const { return m_pipeline_layout; }
        [[nodiscard]]
        const vk::raii::Pipeline& pipeline() const { return m_pipeline; }
        [[nodiscard]]
        const vk::raii::DescriptorSetLayout& second_descriptor_set_layout() const { return m_second_descriptor_set_layout; }
        [[nodiscard]]
        const vk::raii::PipelineLayout& second_pipeline_layout() const { return m_second_pipeline_layout; }
        [[nodiscard]]
        const vk::raii::Pipeline& second_pipeline() const { return m_second_pipeline; }

    private:
        void init() {
            create_descriptor_set_layout();
            create_graphics_pipeline();
            create_second_descriptor_set_layout();
            create_second_graphics_pipeline();
        }
        // 创建描述符集布局
        void create_descriptor_set_layout() {
            vk::DescriptorSetLayoutBinding ubo_layout_binging;
            ubo_layout_binging.binding = 0;
            ubo_layout_binging.descriptorType = vk::DescriptorType::eUniformBuffer;
            ubo_layout_binging.descriptorCount = 1;
            ubo_layout_binging.stageFlags = vk::ShaderStageFlagBits::eVertex;

            vk::DescriptorSetLayoutBinding sampler_layout_binding;
            sampler_layout_binding.binding = 1;
            sampler_layout_binding.descriptorType = vk::DescriptorType::eCombinedImageSampler;
            sampler_layout_binding.descriptorCount = 1;
            sampler_layout_binding.stageFlags = vk::ShaderStageFlagBits::eFragment;

            const auto bindings = { ubo_layout_binging, sampler_layout_binding };
            vk::DescriptorSetLayoutCreateInfo layoutInfo;
            layoutInfo.setBindings( bindings );

            m_descriptor_set_layout = m_device->device().createDescriptorSetLayout( layoutInfo );
        }
        void create_second_descriptor_set_layout() {
            vk::DescriptorSetLayoutBinding light_ubo_layout_binging;
            light_ubo_layout_binging.binding = 0;
            light_ubo_layout_binging.descriptorType = vk::DescriptorType::eUniformBuffer;
            light_ubo_layout_binging.descriptorCount = 1;
            light_ubo_layout_binging.stageFlags = vk::ShaderStageFlagBits::eFragment;
            vk::DescriptorSetLayoutBinding input_layout_binging; // 输入附件绑定
            input_layout_binging.binding = 1;
            input_layout_binging.descriptorType = vk::DescriptorType::eInputAttachment;
            input_layout_binging.descriptorCount = 3; // 有三个输入附件
            input_layout_binging.stageFlags = vk::ShaderStageFlagBits::eFragment;
            const auto bindings = { light_ubo_layout_binging, input_layout_binging };
            vk::DescriptorSetLayoutCreateInfo layoutInfo;
            layoutInfo.setBindings( bindings );
            m_second_descriptor_set_layout = m_device->device().createDescriptorSetLayout( layoutInfo );
        }
        // 创建图形管线
        void create_graphics_pipeline() {
            const auto vertex_shader_code = vht::read_shader("shaders/vert.spv");
            const auto fragment_shader_code = vht::read_shader("shaders/frag.spv");
            const auto vertex_shader_module = vht::create_shader_module(m_device->device(), vertex_shader_code);
            const auto fragment_shader_module = vht::create_shader_module(m_device->device(), fragment_shader_code);
            vk::PipelineShaderStageCreateInfo vertex_shader_create_info;
            vertex_shader_create_info.stage = vk::ShaderStageFlagBits::eVertex;
            vertex_shader_create_info.module = vertex_shader_module;
            vertex_shader_create_info.pName = "main";

            vk::PipelineShaderStageCreateInfo fragment_shader_create_info;
            fragment_shader_create_info.stage = vk::ShaderStageFlagBits::eFragment;
            fragment_shader_create_info.module = fragment_shader_module;
            fragment_shader_create_info.pName = "main";

            const auto shader_stages = { vertex_shader_create_info, fragment_shader_create_info };

            const auto dynamic_states = { vk::DynamicState::eViewport, vk::DynamicState::eScissor };
            vk::PipelineDynamicStateCreateInfo dynamic_state;
            dynamic_state.setDynamicStates(dynamic_states);

            auto binding_description = vht::Vertex::get_binding_description();
            auto attribute_description = vht::Vertex::get_attribute_description();
            auto instance_binding_description = vht::InstanceData::get_binding_description();
            auto instance_attribute_description = vht::InstanceData::get_attribute_description();
            auto binding_descriptions = { binding_description, instance_binding_description };
            std::vector<vk::VertexInputAttributeDescription> attribute_descriptions;
            attribute_descriptions.insert(attribute_descriptions.end(), attribute_description.begin(), attribute_description.end());
            attribute_descriptions.insert(attribute_descriptions.end(), instance_attribute_description.begin(), instance_attribute_description.end());

            vk::PipelineVertexInputStateCreateInfo vertex_input;
            vertex_input.setVertexBindingDescriptions(binding_descriptions);
            vertex_input.setVertexAttributeDescriptions(attribute_descriptions);

            vk::PipelineInputAssemblyStateCreateInfo input_assembly;
            input_assembly.topology = vk::PrimitiveTopology::eTriangleList;

            vk::PipelineViewportStateCreateInfo viewport_state;
            viewport_state.viewportCount = 1;
            viewport_state.scissorCount = 1;

            vk::PipelineDepthStencilStateCreateInfo depth_stencil;
            depth_stencil.depthTestEnable = true;
            depth_stencil.depthWriteEnable = true;
            depth_stencil.depthCompareOp = vk::CompareOp::eLess;
            depth_stencil.depthBoundsTestEnable = false; // Optional
            depth_stencil.stencilTestEnable = false; // Optional

            vk::PipelineRasterizationStateCreateInfo rasterizer;
            rasterizer.depthClampEnable = false;
            rasterizer.rasterizerDiscardEnable = false;
            rasterizer.polygonMode = vk::PolygonMode::eFill;
            rasterizer.lineWidth = 1.0f;
            rasterizer.cullMode = vk::CullModeFlagBits::eBack;
            rasterizer.frontFace = vk::FrontFace::eCounterClockwise;
            rasterizer.depthBiasEnable = false;

            vk::PipelineMultisampleStateCreateInfo multisampling;
            multisampling.rasterizationSamples =  vk::SampleCountFlagBits::e1;
            multisampling.sampleShadingEnable = false;  // default

            std::array<vk::PipelineColorBlendAttachmentState, 3> color_blend_attachments;
            for (auto& att : color_blend_attachments) {
                att.blendEnable = false;
                att.colorWriteMask = vk::FlagTraits<vk::ColorComponentFlagBits>::allFlags;
            }

            vk::PipelineColorBlendStateCreateInfo color_blend;
            color_blend.logicOpEnable = false;
            color_blend.logicOp = vk::LogicOp::eCopy;
            color_blend.setAttachments( color_blend_attachments );

            vk::PushConstantRange pushConstantRange;
            pushConstantRange.stageFlags = vk::ShaderStageFlagBits::eFragment;
            pushConstantRange.offset = 0;
            pushConstantRange.size = sizeof(int32_t);

            vk::PipelineLayoutCreateInfo layout_create_info;
            layout_create_info.setPushConstantRanges( pushConstantRange );
            layout_create_info.setSetLayouts( *m_descriptor_set_layout );
            m_pipeline_layout = m_device->device().createPipelineLayout( layout_create_info );

            vk::GraphicsPipelineCreateInfo create_info;
            create_info.layout = m_pipeline_layout;

            create_info.setStages( shader_stages );
            create_info.pVertexInputState =  &vertex_input;
            create_info.pInputAssemblyState = &input_assembly;
            create_info.pDynamicState = &dynamic_state;
            create_info.pViewportState = &viewport_state;
            create_info.pDepthStencilState = &depth_stencil;
            create_info.pRasterizationState = &rasterizer;
            create_info.pMultisampleState = &multisampling;
            create_info.pColorBlendState = &color_blend;

            create_info.renderPass = m_render_pass->render_pass();
            create_info.subpass = 0;

            m_pipeline = m_device->device().createGraphicsPipeline( nullptr, create_info );
        }

        void create_second_graphics_pipeline() {
            // 修改着色器名
            const auto vertex_shader_code = vht::read_shader("shaders/second_vert.spv");
            const auto fragment_shader_code = vht::read_shader("shaders/second_frag.spv");
            const auto vertex_shader_module = vht::create_shader_module(m_device->device(), vertex_shader_code);
            const auto fragment_shader_module = vht::create_shader_module(m_device->device(), fragment_shader_code);
            vk::PipelineShaderStageCreateInfo vertex_shader_create_info;
            vertex_shader_create_info.stage = vk::ShaderStageFlagBits::eVertex;
            vertex_shader_create_info.module = vertex_shader_module;
            vertex_shader_create_info.pName = "main";
            vk::PipelineShaderStageCreateInfo fragment_shader_create_info;
            fragment_shader_create_info.stage = vk::ShaderStageFlagBits::eFragment;
            fragment_shader_create_info.module = fragment_shader_module;
            fragment_shader_create_info.pName = "main";
            const auto shader_stages = { vertex_shader_create_info, fragment_shader_create_info };

            const auto dynamic_states = { vk::DynamicState::eViewport, vk::DynamicState::eScissor };
            vk::PipelineDynamicStateCreateInfo dynamic_state;
            dynamic_state.setDynamicStates(dynamic_states);


            // 不需要顶点输入数据
            vk::PipelineVertexInputStateCreateInfo vertex_input;

            vk::PipelineInputAssemblyStateCreateInfo input_assembly;
            input_assembly.topology = vk::PrimitiveTopology::eTriangleList;

            vk::PipelineViewportStateCreateInfo viewport_state;
            viewport_state.viewportCount = 1;
            viewport_state.scissorCount = 1;

            // 不需要深度测试
            // vk::PipelineDepthStencilStateCreateInfo depth_stencil;

            vk::PipelineRasterizationStateCreateInfo rasterizer;
            rasterizer.depthClampEnable = false;
            rasterizer.rasterizerDiscardEnable = false;
            rasterizer.polygonMode = vk::PolygonMode::eFill;
            rasterizer.lineWidth = 1.0f;
            rasterizer.cullMode = vk::CullModeFlagBits::eBack;
            rasterizer.frontFace = vk::FrontFace::eCounterClockwise;
            rasterizer.depthBiasEnable = false;

            vk::PipelineMultisampleStateCreateInfo multisampling;
            multisampling.rasterizationSamples =  vk::SampleCountFlagBits::e1;
            multisampling.sampleShadingEnable = false;  // default

            vk::PipelineColorBlendAttachmentState color_blend_attachment;
            color_blend_attachment.blendEnable = false; // default
            color_blend_attachment.colorWriteMask = vk::FlagTraits<vk::ColorComponentFlagBits>::allFlags;

            vk::PipelineColorBlendStateCreateInfo color_blend;
            color_blend.logicOpEnable = false;
            color_blend.logicOp = vk::LogicOp::eCopy;
            color_blend.setAttachments( color_blend_attachment );

            vk::PipelineLayoutCreateInfo layout_create_info;
            layout_create_info.setSetLayouts( *m_second_descriptor_set_layout );
            m_second_pipeline_layout = m_device->device().createPipelineLayout( layout_create_info );

            vk::GraphicsPipelineCreateInfo create_info;
            create_info.layout = m_second_pipeline_layout;

            create_info.setStages( shader_stages );
            create_info.pVertexInputState =  &vertex_input;
            create_info.pInputAssemblyState = &input_assembly;
            create_info.pDynamicState = &dynamic_state;
            create_info.pViewportState = &viewport_state;
            create_info.pRasterizationState = &rasterizer;
            create_info.pMultisampleState = &multisampling;
            create_info.pColorBlendState = &color_blend;

            create_info.renderPass = m_render_pass->render_pass();
            create_info.subpass = 1;    // 绑定第二个子通道

            m_second_pipeline = m_device->device().createGraphicsPipeline( nullptr, create_info );
        }
    };
}


