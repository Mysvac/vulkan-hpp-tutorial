module;

#include <memory>
#include <vector>

export module ShadowPipeline;

import vulkan_hpp;
import DataLoader;
import Utility;
import Device;
import ShadowRenderPass;

export namespace vht {
    class ShadowPipeline {
        std::shared_ptr<vht::Device> m_device;
        std::shared_ptr<vht::ShadowRenderPass> m_shadow_render_pass;
        vk::raii::DescriptorSetLayout m_descriptor_set_layout{ nullptr };
        vk::raii::PipelineLayout m_pipeline_layout{ nullptr };
        vk::raii::Pipeline m_pipeline{ nullptr };
    public:
        explicit ShadowPipeline(std::shared_ptr<vht::Device> device, std::shared_ptr<vht::ShadowRenderPass> shadow_render_pass)
        :   m_device(std::move(device)),
            m_shadow_render_pass(std::move(shadow_render_pass)) {
            create_descriptor_set_layout();
            create_graphics_pipeline();
        }

        [[nodiscard]]
        const vk::raii::DescriptorSetLayout& descriptor_set_layout() const { return m_descriptor_set_layout; }
        [[nodiscard]]
        const vk::raii::PipelineLayout& pipeline_layout() const { return m_pipeline_layout; }
        [[nodiscard]]
        const vk::raii::Pipeline& pipeline() const { return m_pipeline; }

    private:
        void create_descriptor_set_layout() {
            vk::DescriptorSetLayoutBinding light_ubo_layout_binging;
            light_ubo_layout_binging.binding = 0;
            light_ubo_layout_binging.descriptorType = vk::DescriptorType::eUniformBuffer;
            light_ubo_layout_binging.descriptorCount = 1;
            light_ubo_layout_binging.stageFlags = vk::ShaderStageFlagBits::eVertex;

            vk::DescriptorSetLayoutCreateInfo layoutInfo;
            layoutInfo.setBindings( light_ubo_layout_binging );

            m_descriptor_set_layout = m_device->device().createDescriptorSetLayout( layoutInfo );
        }
        // 创建图形管线
        void create_graphics_pipeline() {
            const auto vertex_shader_code = vht::read_shader("shaders/shadow.spv");
            const auto vertex_shader_module = vht::create_shader_module(m_device->device(), vertex_shader_code);
            // 只需要顶点着色器
            vk::PipelineShaderStageCreateInfo vertex_shader_create_info;
            vertex_shader_create_info.stage = vk::ShaderStageFlagBits::eVertex;
            vertex_shader_create_info.module = vertex_shader_module;
            vertex_shader_create_info.pName = "main";
            const std::vector<vk::PipelineShaderStageCreateInfo> shader_stages = { vertex_shader_create_info };

            // 顶点输入状态（只需要顶点位置，不需要实例材质数据）
            auto binding_description = vht::Vertex::get_binding_description();
            auto attribute_description = vht::Vertex::get_attribute_description();
             vk::PipelineVertexInputStateCreateInfo vertex_input;
            vertex_input.setVertexBindingDescriptions(binding_description);
            vertex_input.setVertexAttributeDescriptions(attribute_description);
            // 输入装配状态
            vk::PipelineInputAssemblyStateCreateInfo input_assembly;
            input_assembly.topology = vk::PrimitiveTopology::eTriangleList;
            // 视口与裁剪
            // 直接使用静态状态，因为我们的深度图像大小不会变化
            const vk::Viewport viewport(
                0.0f, 0.0f,          // x, y
                static_cast<float>(m_shadow_render_pass->extent().width),    // width
                static_cast<float>(m_shadow_render_pass->extent().height),   // height
                0.0f, 1.0f      // minDepth maxDepth
            );
            const vk::Rect2D scissor(
                vk::Offset2D{0, 0},              // offset
                m_shadow_render_pass->extent()         // extent
            );
            vk::PipelineViewportStateCreateInfo viewport_state;
            viewport_state.setViewports(viewport);
            viewport_state.setScissors(scissor);
            // 深度与模板测试
            vk::PipelineDepthStencilStateCreateInfo depth_stencil;
            depth_stencil.depthTestEnable = true;
            depth_stencil.depthWriteEnable = true;
            depth_stencil.depthCompareOp = vk::CompareOp::eLess;
            // 光栅化器
            vk::PipelineRasterizationStateCreateInfo rasterizer;
            rasterizer.depthClampEnable = false;
            rasterizer.rasterizerDiscardEnable = false;
            rasterizer.polygonMode = vk::PolygonMode::eFill;
            rasterizer.lineWidth = 1.0f;
            rasterizer.cullMode = vk::CullModeFlagBits::eBack;
            rasterizer.frontFace = vk::FrontFace::eCounterClockwise;
            rasterizer.depthBiasEnable = false;
            // 多重采样
            vk::PipelineMultisampleStateCreateInfo multisampling;
            multisampling.rasterizationSamples =  vk::SampleCountFlagBits::e1;
            multisampling.sampleShadingEnable = false;  // default
            // 管线布局，引用描述符集布局
            vk::PipelineLayoutCreateInfo layout_create_info;
            layout_create_info.setSetLayouts( *m_descriptor_set_layout );
            m_pipeline_layout = m_device->device().createPipelineLayout( layout_create_info );
            // 创建图形管线
            vk::GraphicsPipelineCreateInfo create_info;
            create_info.layout = m_pipeline_layout;

            create_info.setStages( shader_stages );
            create_info.pVertexInputState =  &vertex_input;
            create_info.pInputAssemblyState = &input_assembly;
            create_info.pViewportState = &viewport_state;
            create_info.pDepthStencilState = &depth_stencil;
            create_info.pRasterizationState = &rasterizer;
            create_info.pMultisampleState = &multisampling;
            // 不需要颜色混合阶段
            // 不需要管线动态状态

            create_info.renderPass = m_shadow_render_pass->render_pass();
            create_info.subpass = 0;

            m_pipeline = m_device->device().createGraphicsPipeline( nullptr, create_info );
        }

    };
}
