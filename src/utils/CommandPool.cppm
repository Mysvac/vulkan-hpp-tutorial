module;

#include <memory>
#include <optional>

export module CommandPool;

import vulkan_hpp;
import Device;

export namespace vht {

    /**
     * @brief 命令池相关
     * @details
     * - 依赖：
     *  - m_device: 物理/逻辑设备与队列
     * - 工作：
     *  - 创建命令池
     * - 可访问成员：
     *  - pool(): 命令池
     */
    class CommandPool {
        std::shared_ptr<vht::Device> m_device;
        vk::raii::CommandPool m_pool{ nullptr };
    public:
        explicit CommandPool(std::shared_ptr<vht::Device> device)
        :   m_device(std::move(device)) {
            init();
        }

        [[nodiscard]]
        const vk::raii::CommandPool& pool() const { return m_pool; }

    private:
        void init() {
            const auto [graphics_family, present_family] = m_device->queue_family_indices();

            vk::CommandPoolCreateInfo create_info;
            create_info.flags = vk::CommandPoolCreateFlagBits::eResetCommandBuffer;
            create_info.queueFamilyIndex =  graphics_family.value();

            m_pool = m_device->device().createCommandPool( create_info );
        }

    };

} // namespace vht

