module;

#include <iostream>
#include <array>
#include <vector>
#include <stdexcept>

#include <vulkan/vulkan.h>
#include <GLFW/glfw3.h>

export module Context;

import vulkan_hpp;

// Vulkan 验证层
constexpr std::array<const char*, 1> vulkan_layers { "VK_LAYER_KHRONOS_validation" };

export namespace vht {

    /**
     * @brief Vulkan 初始化，含验证层
     * @details
     * - 工作：
     *  - 创建 Vulkan 上下文
     *  - 创建 Vulkan 实例
     *  - 创建调试信使（如果启用验证层）
     * - 可访问成员：
     *  - context(): vulkan 上下文
     *  - instance(): vulkan 实例
     */
    class Context {
        bool m_enable_validation{};
        vk::raii::Context m_context{};
        vk::raii::Instance m_instance{ nullptr };
        vk::raii::DebugUtilsMessengerEXT m_debug_messenger{ nullptr };
    public:
        explicit Context(const bool validation = false)
        : m_enable_validation(validation) {
            init();
        }

        [[nodiscard]]
        const vk::raii::Context& context() const { return m_context; }
        [[nodiscard]]
        const vk::raii::Instance& instance() const { return m_instance; }
    private:
        void init() {
            if ( !glfwInit() ) throw std::runtime_error("Failed to initialize GLFW");
            vk::ApplicationInfo app_info{
                "Vulkan", 1,
                "MyEngine", 1,
                vk::makeApiVersion(0, 1, 4, 0) // VK_API_VERSION_1_4
            };

            vk::InstanceCreateInfo create_info{ {} , &app_info };
            create_info.flags |= vk::InstanceCreateFlagBits::eEnumeratePortabilityKHR;

            const auto extensions = get_required_extensions();
            create_info.setPEnabledExtensionNames( extensions );

            const auto debug_messenger_create_info = get_debug_messenger_create_info();
            if (m_enable_validation) {
                create_info.setPEnabledLayerNames( vulkan_layers );
                create_info.pNext = &debug_messenger_create_info;
            }
            // 创建实例
            m_instance = m_context.createInstance( create_info );
            // 如果启用了验证层，则创建 调试信使
            if (m_enable_validation) {
                m_debug_messenger = m_instance.createDebugUtilsMessengerEXT( debug_messenger_create_info );
            }
        }

        /**
         * @brief 获取所需的扩展列表
         */
        [[nodiscard]]
        std::vector<const char*> get_required_extensions() const {
            uint32_t count = 0;
            const auto glfw_extensions = glfwGetRequiredInstanceExtensions(&count);
            std::vector<const char *> extensions(glfw_extensions, glfw_extensions + count);
            extensions.emplace_back( vk::KHRPortabilityEnumerationExtensionName );
            if (m_enable_validation) extensions.emplace_back( vk::EXTDebugUtilsExtensionName );
            for (int counter = 0; const auto& it : extensions ) {
                std::cout << "extensions: " << ++counter << " - " << it << std::endl;
            }
            return extensions;
        }

        /**
         * @brief 调试回调函数，仅向错误流输出信息
         */
        static VKAPI_ATTR uint32_t VKAPI_CALL debug_recall(
            vk::DebugUtilsMessageSeverityFlagBitsEXT       severity,
            vk::DebugUtilsMessageTypeFlagsEXT              types,
            vk::DebugUtilsMessengerCallbackDataEXT const * p_data,
            void * p_user_data
        ) {
            std::cerr << "validation layer: " << p_data->pMessage << std::endl;
            return false;
        }

        /**
         * @brief 调试信使信息
         */
        static vk::DebugUtilsMessengerCreateInfoEXT get_debug_messenger_create_info() {
            return {
                {},
                vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning |
                vk::DebugUtilsMessageSeverityFlagBitsEXT::eError,
                vk::DebugUtilsMessageTypeFlagBitsEXT::eGeneral |
                vk::DebugUtilsMessageTypeFlagBitsEXT::eValidation |
                vk::DebugUtilsMessageTypeFlagBitsEXT::ePerformance,
                &debug_recall
            };
        }

    };

}

