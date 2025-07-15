export module App;

import std;
import glfw;
import vulkan_hpp;

import DataLoader;
import Context;
import Window;
import Device;
import Swapchain;
import DepthImage;
import RenderPass;
import GraphicsPipeline;
import CommandPool;
import InputAssembly;
import UniformBuffer;
import TextureSampler;
import Descriptor;
import Drawer;

export namespace vht {
    class App {
        std::shared_ptr<vht::DataLoader> m_data_loader{ nullptr };
        std::shared_ptr<vht::Context> m_context{ nullptr };
        std::shared_ptr<vht::Window> m_window{ nullptr };
        std::shared_ptr<vht::Device> m_device{ nullptr };
        std::shared_ptr<vht::Swapchain> m_swapchain{ nullptr };
        std::shared_ptr<vht::DepthImage> m_depth_image{ nullptr };
        std::shared_ptr<vht::RenderPass> m_render_pass{ nullptr };
        std::shared_ptr<vht::GraphicsPipeline> m_graphics_pipeline{ nullptr };
        std::shared_ptr<vht::CommandPool> m_command_pool{ nullptr };
        std::shared_ptr<vht::InputAssembly> m_input_assembly{ nullptr };
        std::shared_ptr<vht::UniformBuffer> m_uniform_buffer{ nullptr };
        std::shared_ptr<vht::TextureSampler> m_texture_sampler{ nullptr };
        std::shared_ptr<vht::Descriptor> m_descriptor{ nullptr };
        std::shared_ptr<vht::Drawer> m_drawer{ nullptr };
    public:
        void run() {
            init();
            while (!glfw::window_should_close(m_window->ptr())) {
                glfw::poll_events();
                m_drawer->draw();
            }
            std::println("device waitIdle");
            m_device->device().waitIdle();
            std::println("finished");
        }
    private:
        void init() {
            init_data_loader();
            init_context();
            init_window();
            std::println("window created");
            init_device();
            std::println("device created");
            init_swapchain();
            std::println("swapchain created");
            init_depth_image();
            std::println("depth image created");
            init_render_pass();
            std::println("render pass created");
            init_graphics_pipeline();
            std::println("graphics pipeline created");
            init_command_pool();
            std::println("command pool created");
            init_uniform_buffer();
            std::println("uniform buffer created");
            init_texture_sampler();
            std::println("texture sampler created");
            init_input_assembly();
            std::println("input assembly created");
            init_descriptor();
            std::println("descriptor created");
            init_drawer();
            std::println("drawer created");
        }
        void init_data_loader() { m_data_loader = std::make_shared<vht::DataLoader>(); }
        void init_context() { m_context = std::make_shared<vht::Context>( true ); }
        void init_window() { m_window = std::make_shared<vht::Window>( m_context ); }
        void init_device() { m_device = std::make_shared<vht::Device>( m_context, m_window ); }
        void init_swapchain() { m_swapchain = std::make_shared<vht::Swapchain>( m_window, m_device ); }
        void init_depth_image() { m_depth_image = std::make_shared<vht::DepthImage>( m_device, m_swapchain ); }
        void init_render_pass() { m_render_pass = std::make_shared<vht::RenderPass>( m_window, m_device, m_swapchain, m_depth_image ); }
        void init_graphics_pipeline() { m_graphics_pipeline = std::make_shared<vht::GraphicsPipeline>( m_device, m_render_pass ); }
        void init_command_pool() { m_command_pool = std::make_shared<vht::CommandPool>( m_device ); }
        void init_input_assembly() { m_input_assembly = std::make_shared<vht::InputAssembly>( m_data_loader, m_device, m_command_pool ); }
        void init_uniform_buffer() { m_uniform_buffer = std::make_shared<vht::UniformBuffer>( m_window, m_device, m_swapchain ); }
        void init_texture_sampler() { m_texture_sampler = std::make_shared<vht::TextureSampler>( m_device, m_command_pool ); }
        void init_descriptor() { m_descriptor = std::make_shared<vht::Descriptor>( m_device, m_graphics_pipeline, m_uniform_buffer, m_texture_sampler ); }
        void init_drawer() {
            m_drawer = std::make_shared<vht::Drawer>(
                m_data_loader,
                m_window,
                m_device,
                m_swapchain,
                m_depth_image,
                m_render_pass,
                m_graphics_pipeline,
                m_command_pool,
                m_input_assembly,
                m_uniform_buffer,
                m_descriptor
            );
        }
    };
}
