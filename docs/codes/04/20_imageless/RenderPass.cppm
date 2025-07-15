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
        vk::raii::Framebuffer m_framebuffer{ nullptr };
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

            m_framebuffer = nullptr; // 重置帧缓冲区
            m_swapchain->recreate();
            m_depth_image->recreate();
            create_framebuffer();

            m_window->reset_framebuffer_resized();
        }

        [[nodiscard]]
        const vk::raii::RenderPass& render_pass() const { return m_render_pass; }
        [[nodiscard]]
        const vk::raii::Framebuffer& framebuffer() const { return m_framebuffer; }

    private:
        void init() {
            create_render_pass();
            create_framebuffer();
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

            dependency.get()    // 设置子通道依赖
                .setDependencyFlags(vk::DependencyFlagBits::eByRegion) // 局部依赖化，优化性能，可选
                .setSrcSubpass( vk::SubpassExternal )   // 只需要设置两个子通道序号
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
        void create_framebuffer() {
            vk::StructureChain<
                vk::FramebufferCreateInfo,
                vk::FramebufferAttachmentsCreateInfo
            > create_info;

            create_info.get()
                .setFlags( vk::FramebufferCreateFlagBits::eImageless )
                .setRenderPass( m_render_pass )
                .setAttachmentCount( 2 )    // 只需设置附件数，无需绑定具体资源
                .setHeight( m_swapchain->extent().height )
                .setWidth( m_swapchain->extent().width )
                .setLayers( 1 );

            const vk::Format swapchain_format = m_swapchain->format();
            const vk::Format depth_format = m_depth_image->format();
            std::array<vk::FramebufferAttachmentImageInfo, 2> image_infos;
            image_infos[0].height = m_swapchain->extent().height; // 宽高和帧缓冲一致
            image_infos[0].width = m_swapchain->extent().width;
            image_infos[0].layerCount = 1;
            image_infos[0].usage = vk::ImageUsageFlagBits::eColorAttachment;
            image_infos[0].setViewFormats( swapchain_format ); // 需要设置图像格式
            image_infos[1].height = m_swapchain->extent().height;
            image_infos[1].width = m_swapchain->extent().width;
            image_infos[1].layerCount = 1;
            image_infos[1].usage = vk::ImageUsageFlagBits::eDepthStencilAttachment;
            image_infos[1].setViewFormats( depth_format );

            create_info.get<vk::FramebufferAttachmentsCreateInfo>()
                .setAttachmentImageInfos( image_infos );

            m_framebuffer = m_device->device().createFramebuffer( create_info.get() );
        }

    };

} // namespace vht

