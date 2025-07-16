export module GraphicsPipeline;

import std;
import shaderc;
import vulkan_hpp;


import DataLoader;
import Tools;
import Device;
import RenderPass;

const std::string VERT_CODE = R"_GLSL_(
#version 450

layout(binding = 0) uniform UniformBufferObject {
    mat4 model;
    mat4 view;
    mat4 proj;
} ubo;

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec2 inTexCoord;

layout(location = 0) out vec2 fragTexCoord;

void main() {
    gl_Position = ubo.proj * ubo.view * ubo.model * vec4(inPosition, 1.0);
    fragTexCoord = inTexCoord;
}
)_GLSL_";

const std::string FRAG_CODE = R"_GLSL_(
#version 450

layout(set = 1, binding = 0) uniform sampler2D texSampler;

layout(location = 0) in vec2 fragTexCoord;

layout(location = 0) out vec4 outColor;

void main() {
    outColor = texture(texSampler, fragTexCoord);
}
)_GLSL_";


std::vector<std::uint32_t> compile_shader(
    const std::string& source,              // 着色器源代码
    const shaderc::shader_kind kind,        // 着色器类型
    const std::string& name = "shader"     // 着色器名称（可选，默认为 "shader"）
) {
    // 创建编译器和编译选项
    shaderc::Compiler compiler;
    shaderc::CompileOptions options;

    // 设置编译选项
    options.SetOptimizationLevel(shaderc::optimization_level::shaderc_optimization_level_performance);
    // 设置目标环境
    options.SetTargetEnvironment(shaderc::target_env::shaderc_target_env_vulkan, shaderc::env_version::shaderc_env_version_vulkan_1_4);
    // 设置源语言
    options.SetSourceLanguage(shaderc::source_language::shaderc_source_language_glsl);

    // 编译着色器
    const shaderc::SpvCompilationResult result = compiler.CompileGlslToSpv(
        source, // 着色器源代码
        kind,   // 着色器类型
        name.c_str(), // 着色器名称
        options // 编译选项
    );

    // 检查编译结果
    if (result.GetCompilationStatus() != shaderc::compilation_status::shaderc_compilation_status_success) {
        throw std::runtime_error(result.GetErrorMessage());
    }

    return {result.cbegin(), result.cend()};
}

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
            // const std::vector<std::uint32_t>
            const auto vertex_shader_code = compile_shader(VERT_CODE, shaderc::shader_kind::shaderc_glsl_vertex_shader);
            const auto fragment_shader_code = compile_shader(FRAG_CODE, shaderc::shader_kind::shaderc_glsl_fragment_shader);
            const auto vertex_shader_module = m_device->device().createShaderModule(
                vk::ShaderModuleCreateInfo().setCode( vertex_shader_code )
            );
            const auto fragment_shader_module = m_device->device().createShaderModule(
                vk::ShaderModuleCreateInfo().setCode(fragment_shader_code)
            );

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


