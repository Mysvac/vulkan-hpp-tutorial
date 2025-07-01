module;

#include <array>
#include <vector>
#include <memory>

#include <GLFW/glfw3.h>

export module RenderPass;

import vulkan_hpp;

import Window;
import Device;
import Swapchain;
import DepthImage;
import GBuffer;

export namespace vht {

    /**
     * @brief 渲染通道相关
     * @details
     * - 依赖：
     *  - m_window: 窗口
     *  - m_device: 逻辑设备与队列
     *  - m_swapchain: 交换链
     *  - m_depth_image: 深度图像
     * - 工作：
     *  - 创建渲染通道
     *  - 创建帧缓冲区
     *  - 支持交换链重建
     * - 可访问成员：
     *  - render_pass(): 渲染通道
     *  - framebuffers(): 帧缓冲区列表
     */
    class RenderPass {
        std::shared_ptr<vht::Window> m_window{ nullptr };
        std::shared_ptr<vht::Device> m_device{ nullptr };
        std::shared_ptr<vht::Swapchain> m_swapchain{ nullptr };
        std::shared_ptr<vht::DepthImage> m_depth_image{ nullptr };
        std::shared_ptr<vht::GBuffer> m_g_buffer{ nullptr };
        vk::raii::RenderPass m_render_pass{ nullptr };
        std::vector<vk::raii::Framebuffer> m_framebuffers;
    public:
        explicit RenderPass(
            std::shared_ptr<vht::Window> window,
            std::shared_ptr<vht::Device> device,
            std::shared_ptr<vht::Swapchain> swapchain,
            std::shared_ptr<vht::DepthImage> depth_image,
            std::shared_ptr<vht::GBuffer> g_buffer
        ):  m_window(std::move(window)),
            m_device(std::move(device)),
            m_swapchain(std::move(swapchain)),
            m_depth_image(std::move(depth_image)),
            m_g_buffer(std::move(g_buffer)) {
            init();
        }

        /**
         * @brief 重建交换链与帧缓冲区
         * @details
         * 在窗口大小改变时调用，重新创建交换链和帧缓冲区。
         * 注意 m_swapchain 的 recreate 仅重置交换链和图像视图，不重置帧缓冲区。
         * 此函数调用了它，并额外重置了帧缓冲区。
         */
        void recreate() {
            int width = 0, height = 0;
            glfwGetFramebufferSize(m_window->ptr(), &width, &height);
            while (width == 0 || height == 0) {
                glfwGetFramebufferSize(m_window->ptr(), &width, &height);
                glfwWaitEvents();
            }
            m_device->device().waitIdle();

            m_framebuffers.clear();
            m_swapchain->recreate();
            m_depth_image->recreate();
            m_g_buffer->recreate();
            create_framebuffers();

            m_window->reset_framebuffer_resized();
        }

        [[nodiscard]]
        const vk::raii::RenderPass& render_pass() const { return m_render_pass; }
        [[nodiscard]]
        const std::vector<vk::raii::Framebuffer>& framebuffers() const { return m_framebuffers; }

    private:
        void init() {
            create_render_pass();
            create_framebuffers();
        }
        // 创建渲染通道
        void create_render_pass() {
            vk::AttachmentDescription color_attachment;
            color_attachment.format = m_swapchain->format();
            color_attachment.samples = vk::SampleCountFlagBits::e1;
            color_attachment.loadOp = vk::AttachmentLoadOp::eClear;
            color_attachment.storeOp = vk::AttachmentStoreOp::eStore;
            color_attachment.stencilLoadOp = vk::AttachmentLoadOp::eDontCare;
            color_attachment.stencilStoreOp = vk::AttachmentStoreOp::eDontCare;
            color_attachment.initialLayout = vk::ImageLayout::eUndefined;
            color_attachment.finalLayout = vk::ImageLayout::ePresentSrcKHR;

            vk::AttachmentReference color_attachment_ref;
            color_attachment_ref.attachment = 0;
            color_attachment_ref.layout = vk::ImageLayout::eColorAttachmentOptimal;

            vk::AttachmentDescription depth_attachment;
            depth_attachment.format = m_depth_image->format();
            depth_attachment.samples = vk::SampleCountFlagBits::e1;
            depth_attachment.loadOp = vk::AttachmentLoadOp::eClear;
            depth_attachment.storeOp = vk::AttachmentStoreOp::eDontCare;
            depth_attachment.stencilLoadOp = vk::AttachmentLoadOp::eDontCare;
            depth_attachment.stencilStoreOp = vk::AttachmentStoreOp::eDontCare;
            depth_attachment.initialLayout = vk::ImageLayout::eUndefined;
            depth_attachment.finalLayout = vk::ImageLayout::eDepthStencilAttachmentOptimal;

            vk::AttachmentReference depth_attachment_ref;
            depth_attachment_ref.attachment = 1;
            depth_attachment_ref.layout = vk::ImageLayout::eDepthStencilAttachmentOptimal;

            vk::AttachmentDescription g_pos_attachment;
            g_pos_attachment.format = m_g_buffer->pos_format();
            g_pos_attachment.samples = vk::SampleCountFlagBits::e1;
            g_pos_attachment.loadOp = vk::AttachmentLoadOp::eClear;
            g_pos_attachment.storeOp = vk::AttachmentStoreOp::eDontCare; // 渲染通道内部使用，不需要 Store
            g_pos_attachment.stencilLoadOp = vk::AttachmentLoadOp::eDontCare;
            g_pos_attachment.stencilStoreOp = vk::AttachmentStoreOp::eDontCare;
            g_pos_attachment.initialLayout = vk::ImageLayout::eUndefined;
            // 最终布局设为最后一个子通道使用时的布局，减少转换开销
            g_pos_attachment.finalLayout = vk::ImageLayout::eShaderReadOnlyOptimal;

            vk::AttachmentDescription g_color_attachment;
            g_color_attachment.format = m_g_buffer->color_format();
            g_color_attachment.samples = vk::SampleCountFlagBits::e1;
            g_color_attachment.loadOp = vk::AttachmentLoadOp::eClear;
            g_color_attachment.storeOp = vk::AttachmentStoreOp::eDontCare;
            g_color_attachment.stencilLoadOp = vk::AttachmentLoadOp::eDontCare;
            g_color_attachment.stencilStoreOp = vk::AttachmentStoreOp::eDontCare;
            g_color_attachment.initialLayout = vk::ImageLayout::eUndefined;
            g_color_attachment.finalLayout = vk::ImageLayout::eShaderReadOnlyOptimal;

            vk::AttachmentDescription g_normal_depth_attachment;
            g_normal_depth_attachment.format = m_g_buffer->normal_depth_format();
            g_normal_depth_attachment.samples = vk::SampleCountFlagBits::e1;
            g_normal_depth_attachment.loadOp = vk::AttachmentLoadOp::eClear;
            g_normal_depth_attachment.storeOp = vk::AttachmentStoreOp::eDontCare;
            g_normal_depth_attachment.stencilLoadOp = vk::AttachmentLoadOp::eDontCare;
            g_normal_depth_attachment.stencilStoreOp = vk::AttachmentStoreOp::eDontCare;
            g_normal_depth_attachment.initialLayout = vk::ImageLayout::eUndefined;
            g_normal_depth_attachment.finalLayout = vk::ImageLayout::eShaderReadOnlyOptimal;

            // 用于第一个子通道的附件引用
            vk::AttachmentReference g_pos_out_ref;
            g_pos_out_ref.attachment = 2; // 后续附件绑定到帧缓冲的实际索引
            g_pos_out_ref.layout = vk::ImageLayout::eColorAttachmentOptimal;
            vk::AttachmentReference g_color_out_ref;
            g_color_out_ref.attachment = 3;
            g_color_out_ref.layout = vk::ImageLayout::eColorAttachmentOptimal;
            vk::AttachmentReference g_normal_depth_out_ref;
            g_normal_depth_out_ref.attachment = 4;
            g_normal_depth_out_ref.layout = vk::ImageLayout::eColorAttachmentOptimal;

            // 用于第二个子通道的附件引用
            vk::AttachmentReference g_pos_input_ref;
            g_pos_input_ref.attachment = 2;
            g_pos_input_ref.layout = vk::ImageLayout::eShaderReadOnlyOptimal;
            vk::AttachmentReference g_color_input_ref;
            g_color_input_ref.attachment = 3;
            g_color_input_ref.layout = vk::ImageLayout::eShaderReadOnlyOptimal;
            vk::AttachmentReference g_normal_depth_input_ref;
            g_normal_depth_input_ref.attachment = 4;
            g_normal_depth_input_ref.layout = vk::ImageLayout::eShaderReadOnlyOptimal;

            std::array<vk::SubpassDescription,2> subpasses;
            subpasses[0].pipelineBindPoint = vk::PipelineBindPoint::eGraphics;
            const auto first_attachments = { g_pos_out_ref, g_color_out_ref, g_normal_depth_out_ref };
            subpasses[0].setColorAttachments( first_attachments );
            subpasses[0].setPDepthStencilAttachment( &depth_attachment_ref );
            subpasses[1].pipelineBindPoint = vk::PipelineBindPoint::eGraphics;
            const auto second_attachments = { g_pos_input_ref, g_color_input_ref, g_normal_depth_input_ref };
            subpasses[1].setInputAttachments( second_attachments );
            subpasses[1].setColorAttachments( color_attachment_ref );


            std::array<vk::SubpassDependency,2> dependencies;
            dependencies[0].srcSubpass = vk::SubpassExternal;
            dependencies[0].srcStageMask = vk::PipelineStageFlagBits::eFragmentShader;
            dependencies[0].srcAccessMask = {};
            dependencies[0].dstSubpass = 0;
            dependencies[0].dstStageMask = vk::PipelineStageFlagBits::eEarlyFragmentTests;
            dependencies[0].dstAccessMask = vk::AccessFlagBits::eDepthStencilAttachmentWrite;
            dependencies[1].srcSubpass = 0;
            dependencies[1].srcStageMask = vk::PipelineStageFlagBits::eColorAttachmentOutput;
            dependencies[1].srcAccessMask = vk::AccessFlagBits::eColorAttachmentWrite;
            dependencies[1].dstSubpass = 1;
            dependencies[1].dstStageMask = vk::PipelineStageFlagBits::eFragmentShader;
            dependencies[1].dstAccessMask = vk::AccessFlagBits::eInputAttachmentRead;


            const auto attachments = {
                color_attachment,
                depth_attachment,
                g_pos_attachment,
                g_color_attachment,
                g_normal_depth_attachment
            };
            vk::RenderPassCreateInfo create_info;
            create_info.setAttachments( attachments );
            create_info.setSubpasses( subpasses );
            create_info.setDependencies( dependencies );

            m_render_pass = m_device->device().createRenderPass( create_info );
        }
        // 创建帧缓冲区
        void create_framebuffers() {
            m_framebuffers.clear();
            m_framebuffers.reserve( m_swapchain->size() );
            vk::FramebufferCreateInfo create_info;
            create_info.renderPass = m_render_pass;
            create_info.width = m_swapchain->extent().width;
            create_info.height = m_swapchain->extent().height;
            create_info.layers = 1;
            for (size_t i = 0; const auto& image_view : m_swapchain->image_views()) {
                std::array<vk::ImageView, 5> attachments {
                    image_view,
                    m_depth_image->image_view(),
                    m_g_buffer->pos_views(),
                    m_g_buffer->color_views(),
                    m_g_buffer->normal_depth_views()
                };
                create_info.setAttachments( attachments );
                m_framebuffers.emplace_back( m_device->device().createFramebuffer(create_info) );
                ++i;
            }
        }

    };

} // namespace vht

