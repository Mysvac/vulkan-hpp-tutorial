export module GraphicsPipeline;

import std;
import vulkan_hpp;

import DataLoader;
import Tools;
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
        std::vector<vk::raii::DescriptorSetLayout> m_descriptor_set_layouts;
        vk::raii::PipelineLayout m_pipeline_layout{ nullptr };
        vk::raii::Pipeline m_pipeline{ nullptr };
    public:
        explicit GraphicsPipeline(std::shared_ptr<vht::Device> device, std::shared_ptr<vht::RenderPass> render_pass)
        :   m_device(std::move(device)),
            m_render_pass(std::move(render_pass)) {
            init();
        }

        [[nodiscard]]
        const std::vector<vk::raii::DescriptorSetLayout>& descriptor_set_layouts() const { return m_descriptor_set_layouts; }
        [[nodiscard]]
        const vk::raii::PipelineLayout& pipeline_layout() const { return m_pipeline_layout; }
        [[nodiscard]]
        const vk::raii::Pipeline& pipeline() const { return m_pipeline; }

    private:
        void init() {
            create_descriptor_set_layout();
            create_graphics_pipeline();
        }
        // 创建描述符集布局
        void create_descriptor_set_layout() {
            vk::DescriptorSetLayoutBinding uboLayoutBinding;
            uboLayoutBinding.binding = 0;
            uboLayoutBinding.descriptorType = vk::DescriptorType::eUniformBuffer;
            uboLayoutBinding.descriptorCount = 1;
            uboLayoutBinding.stageFlags = vk::ShaderStageFlagBits::eVertex;

            vk::DescriptorSetLayoutCreateInfo uboLayoutInfo;
            uboLayoutInfo.setBindings( uboLayoutBinding );
            m_descriptor_set_layouts.emplace_back( m_device->device().createDescriptorSetLayout( uboLayoutInfo ) );

            vk::DescriptorSetLayoutBinding samplerLayoutBinding;
            samplerLayoutBinding.binding = 0;
            samplerLayoutBinding.descriptorType = vk::DescriptorType::eCombinedImageSampler;
            samplerLayoutBinding.descriptorCount = 1;
            samplerLayoutBinding.stageFlags = vk::ShaderStageFlagBits::eFragment;
            vk::DescriptorSetLayoutCreateInfo samplerLayoutInfo;

            samplerLayoutInfo.setBindings( samplerLayoutBinding );
            m_descriptor_set_layouts.emplace_back( m_device->device().createDescriptorSetLayout( samplerLayoutInfo ) );
        }
        // 创建图形管线
        void create_graphics_pipeline() {
            const auto vertex_shader_code = vht::read_shader("shaders/graphics.vert.spv");
            const auto fragment_shader_code = vht::read_shader("shaders/graphics.frag.spv");
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

            const auto binding_description = vht::Vertex::get_binding_description();
            const auto attribute_description = vht::Vertex::get_attribute_description();
            vk::PipelineVertexInputStateCreateInfo vertex_input;
            vertex_input.setVertexBindingDescriptions(binding_description);
            vertex_input.setVertexAttributeDescriptions(attribute_description);

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

            vk::PipelineColorBlendAttachmentState color_blend_attachment;
            color_blend_attachment.blendEnable = false; // default
            color_blend_attachment.colorWriteMask = vk::FlagTraits<vk::ColorComponentFlagBits>::allFlags;

            vk::PipelineColorBlendStateCreateInfo color_blend;
            color_blend.logicOpEnable = false;
            color_blend.logicOp = vk::LogicOp::eCopy;
            color_blend.setAttachments( color_blend_attachment );

            vk::PipelineLayoutCreateInfo layout_create_info;

            // 将 raii 类型转换回 vk::DescriptorSetLayout
            const auto set_layouts = m_descriptor_set_layouts
                | std::views::transform([](const auto& layout) -> vk::DescriptorSetLayout { return layout; })
                | std::ranges::to<std::vector>();

            layout_create_info.setSetLayouts( set_layouts );
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
    };
}


