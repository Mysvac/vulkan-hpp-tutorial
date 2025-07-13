export module Drawer;

import std;
import vulkan_hpp;

import Config;
import DataLoader;
import Window;
import Device;
import Swapchain;
import RenderPass;
import GraphicsPipeline;
import CommandPool;
import InputAssembly;
import UniformBuffer;
import Descriptor;

export namespace vht {

    /**
     * @brief 绘制相关
     * @details
     * - 依赖：
     *  - m_data_loader: 数据加载器
     *  - m_window: 窗口与表面
     *  - m_device: 物理/逻辑设备与队列
     *  - m_swapchain: 交换链
     *  - m_render_pass: 渲染通道与帧缓冲
     *  - m_graphics_pipeline: 图形管线与描述布局
     *  - m_command_pool: 命令池
     *  - m_input_assembly: 输入装配（顶点缓冲和索引缓冲）
     *  - m_uniform_buffer: uniform 缓冲区
     *  - m_descriptor: 描述符集与池
     * - 工作：
     *  - 创建同步对象（信号量和栅栏）
     *  - 创建命令缓冲区
     *  - 绘制函数 draw()
     */
    class Drawer {
        std::shared_ptr<vht::DataLoader> m_data_loader{ nullptr };
        std::shared_ptr<vht::Window> m_window{ nullptr };
        std::shared_ptr<vht::Device> m_device{ nullptr };
        std::shared_ptr<vht::Swapchain> m_swapchain{ nullptr };
        std::shared_ptr<vht::RenderPass> m_render_pass{ nullptr };
        std::shared_ptr<vht::GraphicsPipeline> m_graphics_pipeline{ nullptr };
        std::shared_ptr<vht::CommandPool> m_command_pool{ nullptr };
        std::shared_ptr<vht::InputAssembly> m_input_assembly{ nullptr };
        std::shared_ptr<vht::UniformBuffer> m_uniform_buffer{ nullptr };
        std::shared_ptr<vht::Descriptor> m_descriptor{ nullptr };
        std::vector<vk::raii::Semaphore> m_image_available_semaphores;
        std::vector<vk::raii::Semaphore> m_render_finished_semaphores;
        std::vector<vk::raii::Fence> m_in_flight_fences;
        std::vector<vk::raii::CommandBuffer> m_command_buffers;
        int m_current_frame = 0;
    public:
        explicit Drawer(
            std::shared_ptr<vht::DataLoader> data_loader,
            std::shared_ptr<vht::Window> window,
            std::shared_ptr<vht::Device> device,
            std::shared_ptr<vht::Swapchain> swapchain,
            std::shared_ptr<vht::RenderPass> render_pass,
            std::shared_ptr<vht::GraphicsPipeline> graphics_pipeline,
            std::shared_ptr<vht::CommandPool> command_pool,
            std::shared_ptr<vht::InputAssembly> input_assembly,
            std::shared_ptr<vht::UniformBuffer> uniform_buffer,
            std::shared_ptr<vht::Descriptor> descriptor
        ):  m_data_loader(std::move(data_loader)),
            m_window(std::move(window)),
            m_device(std::move(device)),
            m_swapchain(std::move(swapchain)),
            m_render_pass(std::move(render_pass)),
            m_graphics_pipeline(std::move(graphics_pipeline)),
            m_command_pool(std::move(command_pool)),
            m_input_assembly(std::move(input_assembly)),
            m_uniform_buffer(std::move(uniform_buffer)),
            m_descriptor(std::move(descriptor)) {
            init();
        }


        void draw() {
            // 等待当前帧的栅栏，即确保上一个帧的绘制完成
            if( const auto res = m_device->device().waitForFences( *m_in_flight_fences[m_current_frame], true, std::numeric_limits<std::uint64_t>::max() );
                res != vk::Result::eSuccess
            ) throw std::runtime_error{ "waitForFences in drawFrame was failed" };

            // 获取交换链的下一个图像索引
            std::uint32_t image_index;
            try{
                auto [res, idx] = m_swapchain->swapchain().acquireNextImage(std::numeric_limits<std::uint64_t>::max(), m_image_available_semaphores[m_current_frame]);
                image_index = idx;
            } catch (const vk::OutOfDateKHRError&){
                m_render_pass->recreate();
                return;
            }
            // 重置当前帧的栅栏，延迟到此处等待，防止上方 return 导致死锁
            m_device->device().resetFences( *m_in_flight_fences[m_current_frame] );
            // 更新 uniform 缓冲区
            m_uniform_buffer->update_uniform_buffer(m_current_frame);
            // 重置当前帧的命令缓冲区，并记录新的命令
            m_command_buffers[m_current_frame].reset();
            record_command_buffer(m_command_buffers[m_current_frame], image_index);
            // 设置绘制命令的提交信息
            vk::SubmitInfo submit_info;
            submit_info.setWaitSemaphores( *m_image_available_semaphores[m_current_frame] );
            std::array<vk::PipelineStageFlags,1> waitStages = { vk::PipelineStageFlagBits::eColorAttachmentOutput };
            submit_info.setWaitDstStageMask( waitStages );
            submit_info.setCommandBuffers( *m_command_buffers[m_current_frame] );
            submit_info.setSignalSemaphores( *m_render_finished_semaphores[m_current_frame] );
            // 提交命令缓冲区到图形队列
            m_device->graphics_queue().submit(submit_info, m_in_flight_fences[m_current_frame]);
            // 设置呈现信息
            vk::PresentInfoKHR present_info;
            present_info.setWaitSemaphores( *m_render_finished_semaphores[m_current_frame] );
            present_info.setSwapchains( *m_swapchain->swapchain() );
            present_info.pImageIndices = &image_index;
            // 提交呈现之类
            try{
                if(  m_device->present_queue().presentKHR(present_info) == vk::Result::eSuboptimalKHR ) {
                    m_render_pass->recreate();
                }
            } catch (const vk::OutOfDateKHRError&){
                m_render_pass->recreate();
            }
            // 检查窗口是否被调整大小
            if( m_window->framebuffer_resized() ){
                m_render_pass->recreate();
            }
            // 更新飞行中的帧索引
            m_current_frame = (m_current_frame + 1) % MAX_FRAMES_IN_FLIGHT;
        }

    private:
        void init() {
            create_sync_object();
            create_command_buffers();
        }
        // 创建命令缓冲区
        void create_command_buffers() {
            vk::CommandBufferAllocateInfo alloc_info;
            alloc_info.commandPool = m_command_pool->pool();
            alloc_info.level = vk::CommandBufferLevel::ePrimary;
            alloc_info.commandBufferCount = MAX_FRAMES_IN_FLIGHT;

            m_command_buffers = m_device->device().allocateCommandBuffers(alloc_info);
        }
        // 创建同步对象（信号量和栅栏）
        void create_sync_object() {
            vk::SemaphoreCreateInfo semaphore_create_info;
            vk::FenceCreateInfo fence_create_info{ vk::FenceCreateFlagBits::eSignaled  };
            m_image_available_semaphores.reserve( MAX_FRAMES_IN_FLIGHT );
            m_render_finished_semaphores.reserve( MAX_FRAMES_IN_FLIGHT );
            m_in_flight_fences.reserve( MAX_FRAMES_IN_FLIGHT );
            for(std::size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i){
                m_image_available_semaphores.emplace_back( m_device->device(), semaphore_create_info );
                m_render_finished_semaphores.emplace_back( m_device->device(),  semaphore_create_info );
                m_in_flight_fences.emplace_back( m_device->device() , fence_create_info );
            }
        }
        // 记录命令缓冲区
        void record_command_buffer(const vk::raii::CommandBuffer& command_buffer, const std::uint32_t image_index) const {
            command_buffer.begin( vk::CommandBufferBeginInfo{} );

            vk::RenderPassBeginInfo render_pass_begin_info;
            render_pass_begin_info.renderPass = m_render_pass->render_pass();
            render_pass_begin_info.framebuffer = m_render_pass->framebuffers()[image_index];

            render_pass_begin_info.renderArea.offset = vk::Offset2D{0, 0};
            render_pass_begin_info.renderArea.extent = m_swapchain->extent();

            std::array<vk::ClearValue, 2> clear_values;
            clear_values[0] = vk::ClearColorValue{ 0.0f, 0.0f, 0.0f, 1.0f };
            clear_values[1] = vk::ClearDepthStencilValue{ 1.0f ,0 };
            render_pass_begin_info.setClearValues( clear_values );

            command_buffer.beginRenderPass( render_pass_begin_info, vk::SubpassContents::eInline);

            command_buffer.bindPipeline( vk::PipelineBindPoint::eGraphics, m_graphics_pipeline->pipeline() );

            const vk::Viewport viewport(
                0.0f, 0.0f,         // x, y
                static_cast<float>(m_swapchain->extent().width),    // width
                static_cast<float>(m_swapchain->extent().height),   // height
                0.0f, 1.0f      // minDepth maxDepth
            );
            command_buffer.setViewport(0, viewport);

            const vk::Rect2D scissor(
                vk::Offset2D{0, 0},     // offset
                m_swapchain->extent()         // extent
            );
            command_buffer.setScissor(0, scissor);

            command_buffer.bindVertexBuffers( 0, *m_input_assembly->vertex_buffer(), vk::DeviceSize{ 0 } );
            command_buffer.bindIndexBuffer( m_input_assembly->index_buffer(), 0, vk::IndexType::eUint32 );

            const std::array<vk::DescriptorSet,2> descriptor_sets = {
                m_descriptor->ubo_sets()[m_current_frame],
                m_descriptor->texture_set()
            };

            command_buffer.bindDescriptorSets(
                vk::PipelineBindPoint::eGraphics,
                m_graphics_pipeline->pipeline_layout(),
                0,
                descriptor_sets,
                nullptr
            );

            command_buffer.drawIndexed(static_cast<std::uint32_t>(m_data_loader->indices().size()), 1, 0, 0, 0);

            command_buffer.endRenderPass();
            command_buffer.end();
        }
    };
}
