export module RenderPass;

import std;
import glfw;
import vulkan_hpp;

import Window;
import Device;
import Swapchain;
import DepthImage;

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
        vk::raii::RenderPass m_render_pass{ nullptr };
        std::vector<vk::raii::Framebuffer> m_framebuffers;
    public:
        explicit RenderPass(
            std::shared_ptr<vht::Window> window,
            std::shared_ptr<vht::Device> device,
            std::shared_ptr<vht::Swapchain> swapchain,
            std::shared_ptr<vht::DepthImage> depth_image
        ):  m_window(std::move(window)),
            m_device(std::move(device)),
            m_swapchain(std::move(swapchain)),
            m_depth_image(std::move(depth_image)) {
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
            glfw::get_framebuffer_size(m_window->ptr(), &width, &height);
            while (width == 0 || height == 0) {
                glfw::get_framebuffer_size(m_window->ptr(), &width, &height);
                glfw::wait_events();
            }
            m_device->device().waitIdle();

            m_framebuffers.clear();
            m_swapchain->recreate();
            m_depth_image->recreate();
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
            vk::AttachmentDescription2 color_attachment;
            color_attachment.format = m_swapchain->format();
            color_attachment.samples = vk::SampleCountFlagBits::e1;
            color_attachment.loadOp = vk::AttachmentLoadOp::eClear;
            color_attachment.storeOp = vk::AttachmentStoreOp::eStore;
            color_attachment.stencilLoadOp = vk::AttachmentLoadOp::eDontCare;
            color_attachment.stencilStoreOp = vk::AttachmentStoreOp::eDontCare;
            color_attachment.initialLayout = vk::ImageLayout::eUndefined;
            color_attachment.finalLayout = vk::ImageLayout::ePresentSrcKHR;

            vk::AttachmentReference2 color_attachment_ref;
            color_attachment_ref.attachment = 0;
            color_attachment_ref.layout = vk::ImageLayout::eAttachmentOptimal;

            vk::AttachmentDescription2 depth_attachment;
            depth_attachment.format = m_depth_image->format();
            depth_attachment.samples = vk::SampleCountFlagBits::e1;
            depth_attachment.loadOp = vk::AttachmentLoadOp::eClear;
            depth_attachment.storeOp = vk::AttachmentStoreOp::eDontCare;
            depth_attachment.stencilLoadOp = vk::AttachmentLoadOp::eDontCare;
            depth_attachment.stencilStoreOp = vk::AttachmentStoreOp::eDontCare;
            depth_attachment.initialLayout = vk::ImageLayout::eUndefined;
            depth_attachment.finalLayout = vk::ImageLayout::eAttachmentOptimal;


            vk::AttachmentReference2 depth_attachment_ref;
            depth_attachment_ref.attachment = 1;
            depth_attachment_ref.layout = vk::ImageLayout::eAttachmentOptimal;

            vk::SubpassDescription2 subpass;
            subpass.pipelineBindPoint = vk::PipelineBindPoint::eGraphics;
            subpass.setColorAttachments( color_attachment_ref );
            subpass.setPDepthStencilAttachment( &depth_attachment_ref );


            vk::StructureChain<vk::SubpassDependency2, vk::MemoryBarrier2> dependency;
            dependency.get()
                .setDependencyFlags(vk::DependencyFlagBits::eByRegion) // 局部依赖化，优化性能，可选
                .setSrcSubpass( vk::SubpassExternal )   // 只需要设置两个子通道的序号
                .setDstSubpass( 0 );
            dependency.get<vk::MemoryBarrier2>()    // 具体同步交给内存屏障
                .setSrcStageMask( vk::PipelineStageFlagBits2::eColorAttachmentOutput | vk::PipelineStageFlagBits2::eEarlyFragmentTests )
                .setSrcAccessMask( vk::AccessFlagBits2::eNone )
                .setDstStageMask( vk::PipelineStageFlagBits2::eColorAttachmentOutput | vk::PipelineStageFlagBits2::eEarlyFragmentTests )
                .setDstAccessMask( vk::AccessFlagBits2::eColorAttachmentWrite | vk::AccessFlagBits2::eDepthStencilAttachmentWrite );

            const auto attachments = { color_attachment, depth_attachment };
            vk::RenderPassCreateInfo2 create_info;
            create_info.setAttachments( attachments );
            create_info.setSubpasses( subpass );
            create_info.setDependencies( dependency.get() );

            m_render_pass = m_device->device().createRenderPass2( create_info );
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
            for (std::size_t i = 0; const auto& image_view : m_swapchain->image_views()) {
                const std::array<vk::ImageView, 2> attachments { image_view, m_depth_image->image_view() };
                create_info.setAttachments( attachments );
                m_framebuffers.emplace_back( m_device->device().createFramebuffer(create_info) );
                ++i;
            }
        }

    };

} // namespace vht

