module;

#include <array>
#include <vector>
#include <set>
#include <memory>
#include <stdexcept>
#include <optional>

export module Device;

import vulkan_hpp;

import Context;
import Window;

export namespace  vht {
    /**
     * @brief 队列族索引
     * @details
     * - graphics_family: 图形队列族索引
     * - present_family: 呈现队列族索引
     * - is_complete(): 检查是否已找到所需的队列族索引
     */
    struct QueueFamilyIndices {
        std::optional<uint32_t> graphics_family;
        std::optional<uint32_t> present_family;
        [[nodiscard]]
        bool is_complete() const {
            return graphics_family.has_value() && present_family.has_value();
        }
    };

    /**
     * @brief 交换链支持的详细信息
     * @details
     * - capabilities: 交换链的能力
     * - formats: 支持的表面格式
     * - present_modes: 支持的呈现模式
     */
    struct SwapchainSupportDetails {
        vk::SurfaceCapabilitiesKHR capabilities;
        std::vector<vk::SurfaceFormatKHR>  formats;
        std::vector<vk::PresentModeKHR> present_modes;
    };
    /**
     * @brief 设备相关
     * @details
     * - 依赖：
     *  - m_context: Vulkan上下文
     *  - m_window: 窗口相关
     * - 工作：
     *  - 选择合适的物理设备
     *  - 创建逻辑设备和队列
     * - 可访问成员：
     *  - physical_device(): 获取物理设备
     *  - device(): 获取逻辑设备
     *  - graphics_queue(): 获取图形队列
     *  - present_queue(): 获取呈现队列
     *  - swapchain_support(): 获取交换链支持的详细信息
     *  - queue_family_indices(): 获取队列族索引
     */
    class Device {
        std::shared_ptr<vht::Context> m_context{ nullptr };
        std::shared_ptr<vht::Window> m_window{ nullptr };
        vk::raii::PhysicalDevice m_physical_device{ nullptr };
        vk::raii::Device m_device{ nullptr };
        QueueFamilyIndices m_queue_family_indices{};
        vk::raii::Queue m_graphics_queue{ nullptr };
        vk::raii::Queue m_present_queue{ nullptr };
    public:
        explicit Device(std::shared_ptr<vht::Context> context, std::shared_ptr<vht::Window> window)
        :   m_context(std::move(context)),
            m_window(std::move(window)) {
            init();
        }

        [[nodiscard]]
        const vk::raii::PhysicalDevice& physical_device() const { return m_physical_device; }
        [[nodiscard]]
        const vk::raii::Device& device() const { return m_device; }
        [[nodiscard]]
        const vk::raii::Queue& graphics_queue() const { return m_graphics_queue; }
        [[nodiscard]]
        const vk::raii::Queue& present_queue() const { return m_present_queue; }
        [[nodiscard]]
        SwapchainSupportDetails swapchain_support() const { return query_swapchain_support(m_physical_device); }
        [[nodiscard]]
        QueueFamilyIndices queue_family_indices() const { return m_queue_family_indices; }
    private:
        void init() {
            pick_physical_device();
            create_device();
        }

        /**
         * @brief 挑选物理设备
         */
        void pick_physical_device() {
            const auto physical_devices = m_context->instance().enumeratePhysicalDevices();
            if (physical_devices.empty()) throw std::runtime_error("failed to enumerate physical devices");
            for (const auto& it : physical_devices) {
                if (is_device_suitable(it)) {
                    m_physical_device = it;
                    return;
                }
            }
            throw std::runtime_error("failed to get physical device");
        }

        /**
         * @brief 判断物理设备是否可用
         * @param physical_device 物理设备
         * @return bool 是否适合使用
         */
        [[nodiscard]]
        bool is_device_suitable(const vk::raii::PhysicalDevice& physical_device) const {
            // 检查是否支持交换链扩展
            std::set<std::string> extension_set{ vk::KHRSwapchainExtensionName };
            for (const auto& it : physical_device.enumerateDeviceExtensionProperties()) {
                extension_set.erase(it.extensionName);
            }
            if (!extension_set.empty()) return false;
            // 交换链格式支持
            if (const auto supports = query_swapchain_support(physical_device);
                supports.formats.empty() || supports.present_modes.empty()
            ) return false;
            // 队列族支持
            const auto indices = find_queue_families(physical_device);
            return indices.is_complete();
        }

        /**
         * @brief 获取交换链支持的详细信息
         * @param physical_device 物理设备
         * @return 交换链支持的详细信息
         */
        [[nodiscard]]
        SwapchainSupportDetails query_swapchain_support(const vk::raii::PhysicalDevice& physical_device) const {
            return {
                physical_device.getSurfaceCapabilitiesKHR( m_window->surface() ),
                physical_device.getSurfaceFormatsKHR( m_window->surface() ),
                physical_device.getSurfacePresentModesKHR( m_window->surface() )
            };
        }

        /**
         * @brief 查询需要的队列族索引
         * @param physical_device 物理设备
         * @return QueueFamilyIndices 队列族索引
         */
        [[nodiscard]]
        QueueFamilyIndices find_queue_families(const vk::raii::PhysicalDevice& physical_device) const {
            QueueFamilyIndices indices{};
            const auto queue_families = physical_device.getQueueFamilyProperties();
            for (uint32_t i = 0; const auto& queue_family : queue_families) {
                if (queue_family.queueFlags & vk::QueueFlagBits::eGraphics) {
                    indices.graphics_family = i;
                }
                if(physical_device.getSurfaceSupportKHR(i, m_window->surface())){
                    indices.present_family = i;
                }
                if (indices.is_complete()) break;
                ++i;
            }
            return indices;
        }

        /**
         * @brief 创建逻辑设备与队列
         */
        void create_device() {
            // 已在物理设备创建时保证内容不为空
            m_queue_family_indices = find_queue_families(m_physical_device);
            const auto [graphics_family, present_family] = m_queue_family_indices;

            std::set<uint32_t> unique_queue_families { graphics_family.value(), present_family.value() };

            std::vector<vk::DeviceQueueCreateInfo> queue_create_infos;
            queue_create_infos.reserve(unique_queue_families.size());
            float queue_priority = 1.0f;
            for (const uint32_t queue_family : unique_queue_families) {
                vk::DeviceQueueCreateInfo queue_create_info;
                queue_create_info.queueFamilyIndex = queue_family;
                queue_create_info.setQueuePriorities( queue_priority );
                queue_create_infos.emplace_back( queue_create_info );
            }

            vk::PhysicalDeviceFeatures features;
            features.pipelineStatisticsQuery = true;
            features.samplerAnisotropy = true;
            vk::DeviceCreateInfo create_info;
            create_info.setQueueCreateInfos( queue_create_infos );
            create_info.setPEnabledFeatures( &features );
            constexpr std::array<const char*, 1> device_extensions { vk::KHRSwapchainExtensionName };
            create_info.setPEnabledExtensionNames( device_extensions );

            m_device = m_physical_device.createDevice( create_info );
            m_graphics_queue = m_device.getQueue( graphics_family.value(), 0 );
            m_present_queue = m_device.getQueue( present_family.value(), 0 );
        }

    };
}

