export module InputAssembly;

import std;
import vulkan_hpp;

import DataLoader;
import Tools;
import Device;
import CommandPool;

export namespace vht {

    /**
     * @brief 输入装配相关
     * @details
     * - 依赖：
     *  - m_data_loader: 数据加载器
     *  - m_device: 逻辑设备与队列
     *  - m_command_pool: 命令池
     * - 工作：
     *  - 将模型数组载入缓冲区
     * - 可访问成员：
     *  - vertex_buffer(): 顶点缓冲区
     *  - index_buffer(): 索引缓冲区
     */
    class InputAssembly {
        std::shared_ptr<vht::DataLoader> m_data_loader{ nullptr };
        std::shared_ptr<vht::Device> m_device{ nullptr };
        std::shared_ptr<vht::CommandPool> m_command_pool{ nullptr };
        vk::raii::DeviceMemory m_vertex_memory{ nullptr };
        vk::raii::Buffer m_vertex_buffer{ nullptr };
        vk::raii::DeviceMemory m_index_memory{ nullptr };
        vk::raii::Buffer m_index_buffer{ nullptr };
    public:
        explicit InputAssembly(
            std::shared_ptr<vht::DataLoader> data_loader,
            std::shared_ptr<vht::Device> device,
            std::shared_ptr<vht::CommandPool> command_pool
        ):  m_data_loader(std::move(data_loader)),
            m_device(std::move(device)),
            m_command_pool(std::move(command_pool)) {
            init();
        }
        [[nodiscard]]
        const vk::raii::Buffer& vertex_buffer() const { return m_vertex_buffer; }
        [[nodiscard]]
        const vk::raii::Buffer& index_buffer() const { return m_index_buffer; }

    private:
        void init() {
            create_vertex_buffer();
            create_index_buffer();
        }
        // 创建顶点缓冲区
        void create_vertex_buffer() {
            const vk::DeviceSize buffer_size = sizeof(vht::Vertex) * m_data_loader->vertices().size();
            vk::raii::DeviceMemory staging_memory{ nullptr };
            vk::raii::Buffer staging_buffer{ nullptr };
            vht::create_buffer(
                staging_buffer,
                staging_memory,
                m_device->device(),
                m_device->physical_device(),
                buffer_size,
                vk::BufferUsageFlagBits::eTransferSrc,
                vk::MemoryPropertyFlagBits::eHostVisible |
                vk::MemoryPropertyFlagBits::eHostCoherent
            );
            void* data = staging_memory.mapMemory(0, buffer_size);
            std::memcpy(data, m_data_loader->vertices().data(), buffer_size);
            staging_memory.unmapMemory();
            vht::create_buffer(
                m_vertex_buffer,
                m_vertex_memory,
                m_device->device(),
                m_device->physical_device(),
                buffer_size,
                vk::BufferUsageFlagBits::eTransferDst |
                vk::BufferUsageFlagBits::eVertexBuffer,
                vk::MemoryPropertyFlagBits::eDeviceLocal
            );
            vht::copy_buffer(
                m_command_pool->pool(),
                m_device->device(),
                m_device->graphics_queue(),
                staging_buffer,
                m_vertex_buffer,
                buffer_size
            );
        }
        // 创建索引缓冲区
        void create_index_buffer() {
            const vk::DeviceSize buffer_size = sizeof(std::uint32_t) * m_data_loader->indices().size();
            vk::raii::DeviceMemory staging_memory{ nullptr };
            vk::raii::Buffer staging_buffer{ nullptr };
            vht::create_buffer(
                staging_buffer,
                staging_memory,
                m_device->device(),
                m_device->physical_device(),
                buffer_size,
                vk::BufferUsageFlagBits::eTransferSrc,
                vk::MemoryPropertyFlagBits::eHostVisible |
                vk::MemoryPropertyFlagBits::eHostCoherent

            );
            void* data = staging_memory.mapMemory(0, buffer_size);
            std::memcpy(data, m_data_loader->indices().data(), static_cast<std::size_t>(buffer_size));
            staging_memory.unmapMemory();
            vht::create_buffer(
                m_index_buffer,
                m_index_memory,
                m_device->device(),
                m_device->physical_device(),
                buffer_size,
                vk::BufferUsageFlagBits::eTransferDst |
                vk::BufferUsageFlagBits::eIndexBuffer,
                vk::MemoryPropertyFlagBits::eDeviceLocal
            );
            vht::copy_buffer(
                m_command_pool->pool(),
                m_device->device(),
                m_device->graphics_queue(),
                staging_buffer,
                m_index_buffer,
                buffer_size
            );
        }

    };
}

