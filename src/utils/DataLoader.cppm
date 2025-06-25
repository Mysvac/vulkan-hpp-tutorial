module;

#include <array>
#include <vector>
#include <unordered_map>
#include <stdexcept>

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/hash.hpp>

#define TINYOBJLOADER_IMPLEMENTATION
#include <tiny_obj_loader.h>

export module DataLoader;

import vulkan_hpp;

// 模型路径
const std::string MODEL_PATH = "models/viking_room.obj";

export namespace vht {

    /**
     * @brief 顶点数据结构
     * @details
     * - pos: 顶点位置
     * - texCoord: 纹理坐标
     * - get_binding_description(): 获取顶点绑定描述符
     * - get_attribute_description(): 获取顶点属性描述符
     */
    struct Vertex {
        glm::vec3 pos;
        glm::vec2 texCoord;

        static vk::VertexInputBindingDescription get_binding_description() {
            return { 0, sizeof(Vertex), vk::VertexInputRate::eVertex };
        }
        static std::array<vk::VertexInputAttributeDescription, 2>  get_attribute_description() {
            std::array<vk::VertexInputAttributeDescription, 2> descriptions;
            descriptions[0].location = 0;
            descriptions[0].binding = 0;
            descriptions[0].format = vk::Format::eR32G32B32Sfloat;
            descriptions[0].offset = offsetof(Vertex, pos);
            descriptions[1].location = 1;
            descriptions[1].binding = 0;
            descriptions[1].format = vk::Format::eR32G32Sfloat;
            descriptions[1].offset = offsetof(Vertex, texCoord);
            return descriptions;
        }
    };


    /**
     * @brief 数据加载器
     * @details
     * - 工作：
     *  - 加载模型数据
     *  - 存储顶点和索引数据
     * - 可访问成员：
     *  - vertices(): 获取顶点数据
     *  - indices(): 获取索引数据
     */
    class DataLoader {
        std::vector<Vertex> m_vertices;
        std::vector<uint32_t> m_indices;
    public:
        DataLoader() {
            load_model();
        }

        [[nodiscard]]
        const std::vector<Vertex>& vertices() const { return m_vertices; }
        [[nodiscard]]
        const std::vector<uint32_t>& indices() const { return m_indices; }
    private:
        // 加载模型数据
        void load_model() {
            tinyobj::attrib_t attrib;
            std::vector<tinyobj::shape_t> shapes;
            std::vector<tinyobj::material_t> materials;
            std::string warn, err;
            if (!tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, MODEL_PATH.c_str())) {
                throw std::runtime_error(warn + err);
            }
            // 使用 unordered_map 进行顶点去重
            static std::unordered_map<
                    Vertex,
                    uint32_t,
                    decltype( [](const Vertex& vertex) -> size_t {
                        return (std::hash<glm::vec3>()(vertex.pos) << 1) ^
                               (std::hash<glm::vec2>()(vertex.texCoord) << 1);
                    } ),
                    decltype( [](const Vertex& vtx_1, const Vertex& vtx_2){
                        return vtx_1.pos == vtx_2.pos && vtx_1.texCoord == vtx_2.texCoord;
                    } )
            > unique_vertices;

            for (const auto& shape : shapes) {
                for (const auto& index : shape.mesh.indices) {
                    Vertex vertex{};
                    vertex.pos = {
                        attrib.vertices[3 * index.vertex_index + 0],
                        attrib.vertices[3 * index.vertex_index + 1],
                        attrib.vertices[3 * index.vertex_index + 2]
                    };
                    vertex.texCoord = {
                        attrib.texcoords[2 * index.texcoord_index + 0],
                        1.0f - attrib.texcoords[2 * index.texcoord_index + 1]
                    };
                    if(const auto it = unique_vertices.find(vertex); it == unique_vertices.end()) {
                        unique_vertices.insert({vertex, static_cast<uint32_t>(m_vertices.size())});
                        m_indices.push_back(static_cast<uint32_t>(m_vertices.size()));
                        m_vertices.push_back(vertex);
                    } else {
                        m_indices.push_back(it->second);
                    }
                }
            }
        }

    };

}


