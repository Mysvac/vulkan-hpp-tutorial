module;

#include <string>
#include <array>
#include <vector>
#include <unordered_map>
#include <stdexcept>

#include <iostream>
#include <format>

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
const std::string MARRY_DIR = "models/Marry";
const std::string MARRY_OBJ_PATH = "models/Marry/Marry.obj";
const std::string FLOOR_DIR = "models/floor";
const std::string FLOOR_OBJ_PATH = "models/floor/floor.obj";

export namespace vht {

    /**
     * @brief 顶点数据结构
     * @details
     * - pos: 顶点位置
     * - texCoord: 纹理坐标
     * - normal: 法线
     * - get_binding_description(): 获取顶点绑定描述符
     * - get_attribute_description(): 获取顶点属性描述符
     */
    struct Vertex {
        glm::vec3 pos;
        glm::vec3 normal;
        glm::vec2 texCoord;

        static vk::VertexInputBindingDescription get_binding_description() {
            return { 0, sizeof(Vertex), vk::VertexInputRate::eVertex };
        }
        static std::array<vk::VertexInputAttributeDescription, 3>  get_attribute_description() {
            std::array<vk::VertexInputAttributeDescription, 3> descriptions;
            for (auto& desc : descriptions) desc.binding = 0;
            descriptions[0].location = 0;
            descriptions[0].format = vk::Format::eR32G32B32Sfloat;
            descriptions[0].offset = offsetof(Vertex, pos);
            descriptions[1].location = 1; // 顶点输入 法线的属性描述
            descriptions[1].format = vk::Format::eR32G32B32Sfloat;
            descriptions[1].offset = offsetof(Vertex, normal);
            descriptions[2].location = 2;
            descriptions[2].format = vk::Format::eR32G32Sfloat;
            descriptions[2].offset = offsetof(Vertex, texCoord);
            return descriptions;
        }
    };

    /**
     * @brief 实例数据结构
     * @details
     * - na: 材质的高光指数
     * - ka: 材质的环境光颜色
     * - kd: 材质的漫反射颜色
     * - ks: 材质的镜面反射颜色
     * - get_binding_description(): 获取绑定描述符
     * - get_attribute_description(): 获取属性描述符
     */
    struct InstanceData {
        float na;
        glm::vec3 ka;
        glm::vec3 kd;
        glm::vec3 ks;

        static vk::VertexInputBindingDescription get_binding_description() {
            return { 1, sizeof(InstanceData), vk::VertexInputRate::eInstance };
        }
        static std::array<vk::VertexInputAttributeDescription, 4> get_attribute_description() {
            std::array<vk::VertexInputAttributeDescription, 4> descriptions;
            for (auto& desc : descriptions) desc.binding = 1; // 实例数据的绑定描述符
            descriptions[0].location = 3; // na
            descriptions[0].format = vk::Format::eR32Sfloat;
            descriptions[0].offset = offsetof(InstanceData, na);
            descriptions[1].location = 4; // ka
            descriptions[1].format = vk::Format::eR32G32B32Sfloat;
            descriptions[1].offset = offsetof(InstanceData, ka);
            descriptions[2].location = 5; // kd
            descriptions[2].format = vk::Format::eR32G32B32Sfloat;
            descriptions[2].offset = offsetof(InstanceData, kd);
            descriptions[3].location = 6; // ks
            descriptions[3].format = vk::Format::eR32G32B32Sfloat;
            descriptions[3].offset = offsetof(InstanceData, ks);
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
     *  - instances(): 获取实例数据
     *  - index_offsets(): 获取索引偏移
     *  - index_counts(): 获取索引数量
     */
    class DataLoader {
        std::vector<Vertex> m_vertices;
        std::vector<uint32_t> m_indices;
        std::vector<InstanceData> m_instances; // 实例数据，材质参数
        std::vector<uint32_t> m_index_offsets; // 使用索引偏移记录不同实例开始索引
        std::vector<uint32_t> m_index_counts; // 记录每个实例的索引数量
    public:
        DataLoader() {
            load_model(MARRY_DIR,MARRY_OBJ_PATH);
            load_model(FLOOR_DIR,FLOOR_OBJ_PATH);
        }

        [[nodiscard]]
        const std::vector<Vertex>& vertices() const { return m_vertices; }
        [[nodiscard]]
        const std::vector<uint32_t>& indices() const { return m_indices; }
        [[nodiscard]]
        const std::vector<InstanceData>& instances() const { return m_instances; }
        [[nodiscard]]
        const std::vector<uint32_t>& index_offsets() const { return m_index_offsets; }
        [[nodiscard]]
        const std::vector<uint32_t>& index_counts() const { return m_index_counts; }

    private:
        // 加载模型数据
        void load_model(const std::string& dir, const std::string& filename) {
            tinyobj::attrib_t attrib;
            std::vector<tinyobj::shape_t> shapes;
            std::vector<tinyobj::material_t> materials;
            std::string warn, err;
            if (!tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, filename.c_str(), dir.c_str())) {
                throw std::runtime_error(warn + err);
            }
            // 获取模型的索引偏移
            m_index_offsets.push_back(static_cast<uint32_t>(m_indices.size()));

            // 加载材质数据
            if (materials.empty()) {
                InstanceData instance_data{
                    128,
                        {0.9f, 0.9f, 0.9f}, // ka
                        {0.8f, 0.8f, 0.8f}, // kd
                        {0.8f, 0.8f, 0.8f}  // ks
                };
                m_instances.push_back(instance_data);
            } else {
                InstanceData instance_data{
                    materials[0].shininess,
                        {materials[0].ambient[0], materials[0].ambient[1], materials[0].ambient[2]}, // ka
                        {materials[0].diffuse[0], materials[0].diffuse[1], materials[0].diffuse[2]}, // kd
                        {materials[0].specular[0], materials[0].specular[1], materials[0].specular[2]}  // ks
                };
                m_instances.push_back(instance_data);
            }
            std::cout << std::format(" mtl: Ns {} Ka {} {} {} Kd {} {} {} Ks {} {} {}",
                m_instances.rbegin()->na, m_instances.rbegin()->ka.x, m_instances.rbegin()->ka.y, m_instances.rbegin()->ka.z,
                m_instances.rbegin()->kd.x, m_instances.rbegin()->kd.y, m_instances.rbegin()->kd.z,
                m_instances.rbegin()->ks.x, m_instances.rbegin()->ks.y, m_instances.rbegin()->ks.z
            ) << std::endl;

            // 使用 unordered_map 进行顶点去重
            std::unordered_map<
                    Vertex,
                    uint32_t,
                    decltype( [](const Vertex& vertex) -> size_t {
                        return (std::hash<glm::vec3>()(vertex.pos) << 1) ^ std::hash<glm::vec3>()(vertex.normal) ^
                               (std::hash<glm::vec2>()(vertex.texCoord) << 1);
                    } ),
                    decltype( [](const Vertex& vtx_1, const Vertex& vtx_2){
                        return vtx_1.pos == vtx_2.pos && vtx_1.normal == vtx_2.normal && vtx_1.texCoord == vtx_2.texCoord;
                    } )
            > unique_vertices;

            for (const auto& shape : shapes) {
                for (const auto&[vertex_index, normal_index, texcoord_index] : shape.mesh.indices) {
                    Vertex vertex{};
                    vertex.pos = { // 获取顶点位置
                        attrib.vertices[3 * vertex_index + 0],
                        attrib.vertices[3 * vertex_index + 1],
                        attrib.vertices[3 * vertex_index + 2]
                    };

                    if (!attrib.texcoords.empty()) { // 获取纹理坐标
                        vertex.texCoord = {
                            attrib.texcoords[2 * texcoord_index + 0],
                            1.0f - attrib.texcoords[2 * texcoord_index + 1]
                        };
                    } else vertex.texCoord = {0.0f, 0.0f}; // 默认纹理坐标

                    if (!attrib.normals.empty()) {  // 获取法线坐标
                        vertex.normal = {
                            attrib.normals[3 * normal_index + 0],
                            attrib.normals[3 * normal_index + 1],
                            attrib.normals[3 * normal_index + 2]
                        };
                    } else vertex.normal = {0.0f, 0.0f, 1.0f}; // 默认法线
                    // 顶点去重并加入索引列表
                    if(const auto it = unique_vertices.find(vertex); it == unique_vertices.end()) {
                        unique_vertices.insert({vertex, static_cast<uint32_t>(m_vertices.size())});
                        m_indices.push_back(static_cast<uint32_t>(m_vertices.size()));
                        m_vertices.push_back(vertex);
                    } else {
                        m_indices.push_back(it->second);
                    }

                }
            }
            // 记录当前模型的索引数量
            m_index_counts.push_back(static_cast<uint32_t>(m_indices.size() - m_index_offsets.back()));
        }

    };

}


