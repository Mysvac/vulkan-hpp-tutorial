module;
// offsetof 宏需要从 <cstddef> 头文件中导入。等到 C++26 静态反射支持后才会有更好的替代方案
#include <cstddef>
export module DataLoader;

import std;
import vulkan_hpp;
import glm;
import tinyobj;

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
        bool operator<(const Vertex& other) const {
            return std::tie(pos.x, pos.y, pos.z, texCoord.x, texCoord.y)
                 < std::tie(other.pos.x, other.pos.y, other.pos.z, other.texCoord.x, other.texCoord.y);
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
        std::vector<std::uint32_t> m_indices;
    public:
        DataLoader() {
            load_model();
        }

        [[nodiscard]]
        const std::vector<Vertex>& vertices() const { return m_vertices; }
        [[nodiscard]]
        const std::vector<std::uint32_t>& indices() const { return m_indices; }
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
            std::map<Vertex, std::uint32_t> unique_vertices;

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
                        unique_vertices.insert({vertex, static_cast<std::uint32_t>(m_vertices.size())});
                        m_indices.push_back(static_cast<std::uint32_t>(m_vertices.size()));
                        m_vertices.push_back(vertex);
                    } else {
                        m_indices.push_back(it->second);
                    }
                }
            }
        }

    };

}


