module;

#include <iostream>
#include <memory>

// #include <vulkan/vulkan.h>
#include <GLFW/glfw3.h>

export module App;

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
import QueryPool;
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
        std::shared_ptr<vht::QueryPool> m_query_pool; // 添加查询池成员
        std::shared_ptr<vht::Drawer> m_drawer{ nullptr };
    public:
        void run() {
            init();
            std::cout << "begin draw" << std::endl;
            while (!glfwWindowShouldClose(m_window->ptr())) {
                glfwPollEvents();
                m_drawer->draw();
            }
            std::cout << "device waitIdle" << std::endl;
            m_device->device().waitIdle();
            std::cout << "finished" << std::endl;
        }
    private:
        void init() {
            init_data_loader();
            init_vulkan();
            init_window();
            std::cout << "window created" << std::endl;
            init_device();
            std::cout << "device created" << std::endl;
            init_swapchain();
            std::cout << "swapchain created" << std::endl;
            init_depth_image();
            std::cout << "depth image created" << std::endl;
            init_render_pass();
            std::cout << "render pass created" << std::endl;
            init_graphics_pipeline();
            std::cout << "graphics pipeline created" << std::endl;
            init_command_pool();
            std::cout << "command pool created" << std::endl;
            init_input_assembly();
            std::cout << "input assembly created" << std::endl;
            init_uniform_buffer();
            std::cout << "uniform buffer created" << std::endl;
            init_texture_sampler();
            std::cout << "texture sampler created" << std::endl;
            init_descriptor();
            std::cout << "descriptor created" << std::endl;
            init_query_pool(); // 初始化查询池
            std::cout << "query pool created" << std::endl;
            init_drawer();
            std::cout << "drawer created" << std::endl;
        }
        void init_data_loader() { m_data_loader = std::make_shared<vht::DataLoader>(); }
        void init_vulkan() { m_context = std::make_shared<vht::Context>( true ); }
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
        void init_query_pool() { m_query_pool = std::make_shared<vht::QueryPool>( m_device ); }
        void init_drawer() {
            m_drawer = std::make_shared<vht::Drawer>(
                m_data_loader,
                m_window,
                m_device,
                m_swapchain,
                m_render_pass,
                m_graphics_pipeline,
                m_command_pool,
                m_input_assembly,
                m_uniform_buffer,
                m_descriptor,
                m_query_pool // 传递查询池
            );
        }
    };
}

