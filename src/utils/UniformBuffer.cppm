module;

#include <cmath>
#include <memory>
#include <chrono>
#include <vector>


#include <GLFW/glfw3.h>

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

export module UniformBuffer;

import vulkan_hpp;

import Config;
import Utility;
import Window;
import Device;
import Swapchain;

export namespace vht {

    struct alignas(16) UBO {
        glm::mat4 model;
        glm::mat4 view;
        glm::mat4 proj;
    };

    /**
     * @brief Uniform Buffer Object (UBO) 相关
     * @details
     * - 依赖：
     *  - m_window: 窗口
     *  - m_device: 物理/逻辑设备与队列
     *  - m_swapchain: 交换链
     * - 工作：
     *  - 创建 Uniform Buffer
     *  - 分配内存
     *  - 映射内存
     * - 可访问成员：
     *  - uniform_buffers(): Uniform Buffer 列表
     *  - uniform_mapped(): 映射的 Uniform Buffer 数据指针列表
     */
    class UniformBuffer {
        std::shared_ptr<vht::Window> m_window{ nullptr };
        std::shared_ptr<vht::Device> m_device{ nullptr };
        std::shared_ptr<vht::Swapchain> m_swapchain{ nullptr };
        std::vector<vk::raii::DeviceMemory> m_memories;
        std::vector<vk::raii::Buffer> m_buffers;
        std::vector<void*> m_mapped;
        glm::vec3 m_cameraPos{ 2.0f, 2.0f, 2.0f };
        glm::vec3 m_cameraUp{ 0.0f, 1.0f, 0.0f };
        float m_pitch = -35.0f;
        float m_yaw = -135.0f;
        float m_cameraMoveSpeed = 1.0f;
        float m_cameraRotateSpeed = 25.0f;
    public:
        explicit UniformBuffer(
            std::shared_ptr<vht::Window> window,
            std::shared_ptr<vht::Device> device,
            std::shared_ptr<vht::Swapchain> swapchain
        ):  m_window(std::move(window)),
            m_device(std::move(device)),
            m_swapchain(std::move(swapchain)){
            init();
        }

        // 析构时清除映射
        ~UniformBuffer() {
            for (const auto& memory : m_memories) {
                memory.unmapMemory();
            }
        }

        [[nodiscard]]
        const std::vector<vk::raii::Buffer>& buffers() const { return m_buffers; }
        [[nodiscard]]
        const std::vector<void*>& mapped() const { return m_mapped; }
        // 更新 Uniform Buffer
        void update_uniform_buffer(const int current_frame ) {
            update_camera();

            glm::vec3 front;
            front.x = std::cos(glm::radians(m_yaw)) * std::cos(glm::radians(m_pitch));
            front.y = std::sin(glm::radians(m_pitch));
            front.z = std::sin(glm::radians(m_yaw)) * std::cos(glm::radians(m_pitch));
            front = glm::normalize(front);
            UBO ubo{};
            ubo.model = glm::rotate(
                    glm::mat4(1.0f),
                    glm::radians(-90.0f),
                    glm::vec3(1.0f, 0.0f, 0.0f)
            );
            ubo.model *= glm::rotate(
                    glm::mat4(1.0f),
                    glm::radians(-90.0f),
                    glm::vec3(0.0f, 0.0f, 1.0f)
            );
            ubo.view = glm::lookAt(
                    m_cameraPos,
                    m_cameraPos + front,
                    m_cameraUp
            );
            ubo.proj = glm::perspective(
                    glm::radians(45.0f),
                    static_cast<float>(m_swapchain->extent().width) / static_cast<float>(m_swapchain->extent().height),
                    0.1f,
                    10.0f
            );
            ubo.proj[1][1] *= -1;
            memcpy(m_mapped[current_frame], &ubo, sizeof(UBO));
        }
    private:
        void init() {
            create_uniform_buffer();
        }
        // 创建 Uniform Buffer
        void create_uniform_buffer() {
            m_buffers.reserve(MAX_FRAMES_IN_FLIGHT);
            m_memories.reserve(MAX_FRAMES_IN_FLIGHT);
            m_mapped.reserve(MAX_FRAMES_IN_FLIGHT);
            for(size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
                constexpr vk::DeviceSize bufferSize  = sizeof(UBO);
                m_buffers.emplace_back( nullptr );
                m_memories.emplace_back( nullptr );
                m_mapped.emplace_back( nullptr );
                create_buffer(
                    m_buffers[i],
                    m_memories[i],
                    m_device->device(),
                    m_device->physical_device(),
                    bufferSize,
                    vk::BufferUsageFlagBits::eUniformBuffer,
                    vk::MemoryPropertyFlagBits::eHostVisible |
                    vk::MemoryPropertyFlagBits::eHostCoherent
                );
                m_mapped[i] = m_memories[i].mapMemory(0, bufferSize);
            }
        }
        // 创建缓冲区
        void update_camera() {
            static auto start_time = std::chrono::high_resolution_clock::now();
            const auto current_time = std::chrono::high_resolution_clock::now();
            const float time = std::chrono::duration<float, std::chrono::seconds::period>(current_time - start_time).count();
            start_time = current_time;

            glm::vec3 front;
            front.x = std::cos(glm::radians(m_yaw)) * std::cos(glm::radians(m_pitch));
            front.y = 0.0f;
            front.z = std::sin(glm::radians(m_yaw)) * std::cos(glm::radians(m_pitch));
            front = glm::normalize(front);

            if (glfwGetKey(m_window->ptr(), GLFW_KEY_W) == GLFW_PRESS)
                m_cameraPos += front * m_cameraMoveSpeed * time;
            if (glfwGetKey(m_window->ptr(), GLFW_KEY_S) == GLFW_PRESS)
                m_cameraPos -= front * m_cameraMoveSpeed * time;
            if (glfwGetKey(m_window->ptr(), GLFW_KEY_A) == GLFW_PRESS)
                m_cameraPos -= glm::normalize(glm::cross(front, m_cameraUp)) * m_cameraMoveSpeed * time;
            if (glfwGetKey(m_window->ptr(), GLFW_KEY_D) == GLFW_PRESS)
                m_cameraPos += glm::normalize(glm::cross(front, m_cameraUp)) * m_cameraMoveSpeed * time;
            if (glfwGetKey(m_window->ptr(), GLFW_KEY_SPACE) == GLFW_PRESS)
                m_cameraPos += m_cameraUp * m_cameraMoveSpeed * time;
            if (glfwGetKey(m_window->ptr(), GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS)
                m_cameraPos -= m_cameraUp *m_cameraMoveSpeed * time;

            if (glfwGetKey(m_window->ptr(), GLFW_KEY_UP) == GLFW_PRESS)
                m_pitch += m_cameraRotateSpeed * time;
            if (glfwGetKey(m_window->ptr(), GLFW_KEY_DOWN) == GLFW_PRESS)
                m_pitch -= m_cameraRotateSpeed * time;
            if (glfwGetKey(m_window->ptr(), GLFW_KEY_LEFT) == GLFW_PRESS)
                m_yaw   -= m_cameraRotateSpeed * time;
            if (glfwGetKey(m_window->ptr(), GLFW_KEY_RIGHT) == GLFW_PRESS)
                m_yaw   += m_cameraRotateSpeed * time;

            m_yaw = std::fmodf(m_yaw + 180.0f, 360.0f);
            if (m_yaw < 0.0f) m_yaw += 360.0f;
            m_yaw -= 180.0f;

            if (m_pitch > 89.0f) m_pitch = 89.0f;
            if (m_pitch < -89.0f) m_pitch = -89.0f;
        }

    };
}
