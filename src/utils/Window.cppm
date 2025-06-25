module;

#include <string>
#include <memory>
#include <stdexcept>

#include <vulkan/vulkan.h>
#include <GLFW/glfw3.h>

export module Window;

import vulkan_hpp;

import Context;

export namespace vht {

    /**
     * @brief 窗口相关
     * @details
     * - 依赖：
     *  - m_context: Vulkan上下文
     * - 工作：
     *  - 创建 GLFW 窗口
     *  - 创建 Vulkan 窗口表面
     *  - 处理窗口调整大小事件
     * - 可访问成员：
     *  - window_ptr(): 获取 GLFW 窗口指针
     *  - surface(): 获取 Vulkan 窗口表面
     *  - framebuffer_resized(): 获取布尔值，表示窗口是否被调整大小
     */
    class Window {
        std::shared_ptr<vht::Context> m_context{ nullptr };
        GLFWwindow* m_window{ nullptr };
        vk::raii::SurfaceKHR m_surface{ nullptr };
        bool m_framebuffer_resized{ false };
    public:
        explicit Window(std::shared_ptr<vht::Context> context)
        : m_context(std::move(context)) {
            init();
        }

        ~Window() {
            m_surface = nullptr;
            if ( m_window ) {
                glfwDestroyWindow(m_window);
                glfwTerminate();
            }
        }

        [[nodiscard]]
        GLFWwindow* ptr() const { return m_window; }
        [[nodiscard]]
        const vk::raii::SurfaceKHR& surface() const { return m_surface; }
        [[nodiscard]]
        bool framebuffer_resized() const { return m_framebuffer_resized; }
        void reset_framebuffer_resized() { m_framebuffer_resized = false; }
    private:
        void init() {
            // 在 Context 中已经初始化了 GLFW
            // if (!glfwInit()) throw std::runtime_error("Failed to initialize GLFW");

            glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
            m_window = glfwCreateWindow(800, 600, "Vulkan", nullptr, nullptr);
            if (!m_window) {
                glfwTerminate();
                throw std::runtime_error("Failed to create GLFW window");
            }
            glfwSetWindowUserPointer(m_window, this);
            glfwSetFramebufferSizeCallback(m_window, framebuffer_resize_callback);

            VkSurfaceKHR surface;
            // vk::SurfaceKHR surface;

            if (const auto res = glfwCreateWindowSurface(*m_context->instance(), m_window, nullptr, &surface);
                res != VK_SUCCESS) {
                throw std::runtime_error("failed to create window surface " + std::to_string(res));
            }
            m_surface = vk::raii::SurfaceKHR{ m_context->instance(), surface };
        }

        /**
         * @brief 窗口尺寸变化时的回调
         */
        static void framebuffer_resize_callback(GLFWwindow* window, int , int ) {
            const auto app = static_cast<Window*>(glfwGetWindowUserPointer(window));
            app->m_framebuffer_resized = true;
        }
    };
}

