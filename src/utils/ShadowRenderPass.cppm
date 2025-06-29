module;

#include <memory>
#include <vector>

export module ShadowRenderPass;

import vulkan_hpp;
import Device;
import ShadowDepthImage;

export namespace vht {
    class ShadowRenderPass {
        std::shared_ptr<vht::Device> m_device{ nullptr };
        std::shared_ptr<vht::ShadowDepthImage> m_shadow_depth_image{ nullptr };
        vk::raii::RenderPass m_render_pass{ nullptr };
        std::vector<vk::raii::Framebuffer> m_framebuffers;
        vk::Extent2D m_extent{};  // 使用阴影深度图的宽度和高度
    public:
        explicit ShadowRenderPass(
            std::shared_ptr<vht::Device> device,
            std::shared_ptr<vht::ShadowDepthImage> shadow_depth_image)
        :   m_device(std::move(device)),
            m_shadow_depth_image(std::move(shadow_depth_image)) {
            m_extent.width = m_shadow_depth_image->width();
            m_extent.height = m_shadow_depth_image->height();
            create_render_pass();
            create_framebuffers();
        }

        [[nodiscard]]
        const vk::raii::RenderPass& render_pass() const { return m_render_pass; }
        [[nodiscard]]
        const std::vector<vk::raii::Framebuffer>& framebuffers() const { return m_framebuffers; }
        [[nodiscard]]
        const vk::Extent2D& extent() const { return m_extent; } // 命令缓冲录制时需要使用
    private:
        // 创建渲染通道
        void create_render_pass() {
            vk::AttachmentDescription depth_attachment;
            depth_attachment.format = m_shadow_depth_image->format();
            depth_attachment.samples = vk::SampleCountFlagBits::e1;
            depth_attachment.loadOp = vk::AttachmentLoadOp::eClear;
            depth_attachment.storeOp = vk::AttachmentStoreOp::eStore; // 阴影深度图像需要存储
            depth_attachment.stencilLoadOp = vk::AttachmentLoadOp::eDontCare;
            depth_attachment.stencilStoreOp = vk::AttachmentStoreOp::eDontCare;
            depth_attachment.initialLayout = vk::ImageLayout::eUndefined;
            depth_attachment.finalLayout = vk::ImageLayout::eShaderReadOnlyOptimal; // 后续需要让着色器读取

            vk::AttachmentReference depth_attachment_ref;
            depth_attachment_ref.attachment = 0;
            depth_attachment_ref.layout = vk::ImageLayout::eDepthStencilAttachmentOptimal;

            vk::SubpassDescription subpass;
            subpass.pipelineBindPoint = vk::PipelineBindPoint::eGraphics;
            subpass.setPDepthStencilAttachment( &depth_attachment_ref );

            vk::RenderPassCreateInfo create_info;
            create_info.setAttachments( depth_attachment );
            create_info.setSubpasses( subpass );

            m_render_pass = m_device->device().createRenderPass( create_info );
        }
        // 创建帧缓冲
        void create_framebuffers() {
            vk::FramebufferCreateInfo create_info;
            create_info.renderPass = m_render_pass;
            create_info.width = m_extent.width; // 使用阴影深度图的宽度和高度
            create_info.height = m_extent.height;
            create_info.layers = 1;
            for (const auto& image_view : m_shadow_depth_image->image_views()) {
                create_info.setAttachments( *image_view ); // 记得加 * 操作符
                m_framebuffers.emplace_back(m_device->device().createFramebuffer( create_info ));
            }
        }
    };
}

