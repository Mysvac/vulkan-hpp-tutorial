module;


#include <memory>
#include <vector>

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

export module LightUniformBuffer;

import vulkan_hpp;

import Config;
import Utility;
import Device;

export namespace vht {

    struct LightUBO {
        glm::mat4 model{ glm::mat4(1.0f) }; // 光源模型矩阵
        glm::mat4 view{};  // 视图矩阵
        glm::mat4 proj{};  // 投影矩阵
        alignas(16) glm::vec3 light_pos{ 2.0f, 4.2f, 3.2f }; // 光源位置
        alignas(16) glm::vec3 light_color{ 1.0f, 1.0f, 1.0f }; // 光源颜色
        alignas(16) glm::vec3 view_pos{}; // 摄像机位置
    };

    /**
     * @brief Light Uniform Buffer Object (Light UBO) 相关
     * @details
     * - 依赖：无
     * - 工作：创建 Light UBO
     */
    class LightUniformBuffer {
        std::shared_ptr<vht::Device> m_device{ nullptr };
        std::vector<vk::raii::DeviceMemory> m_memories;
        std::vector<vk::raii::Buffer> m_buffers;
        std::vector<void*> m_mapped;
    public:
        explicit LightUniformBuffer(std::shared_ptr<vht::Device> device)
        :   m_device(std::move(device)) {
            init();
        }

        ~LightUniformBuffer() {
            for (const auto& memory : m_memories) {
                memory.unmapMemory();
            }
        }

        [[nodiscard]]
        const std::vector<vk::raii::Buffer>& buffers() const { return m_buffers; }

        // 更新摄像机位置，在 Drawer 类的 draw 函数中调用，计算机位置来自 UniformBuffer 类
        void update(const int current_frame, const glm::vec3 view_pos) const {
            LightUBO light_ubp;
            light_ubp.view_pos = view_pos;
            light_ubp.view = glm::lookAt(
                light_ubp.light_pos,
                glm::vec3(0.0f, 0.0f, 0.0f),
                glm::vec3(0.0f, 1.0f, 0.0f)
            );
            light_ubp.proj = glm::perspective(glm::radians(90.0f), 1.0f, 0.1f, 60.0f);
            light_ubp.proj[1][1] *= -1;
            memcpy(m_mapped[current_frame], &light_ubp, sizeof(LightUBO));
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
                constexpr vk::DeviceSize bufferSize  = sizeof(LightUBO);
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
    };

}

