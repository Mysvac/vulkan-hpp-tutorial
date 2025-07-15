export module QueryPool;

import std;
import vulkan_hpp;
import Device;

export namespace vht {
    class QueryPool {
        std::shared_ptr<vht::Device> m_device;
        vk::raii::QueryPool m_timestamp{ nullptr };
        vk::raii::QueryPool m_statistics{ nullptr };
        vk::raii::QueryPool m_occlusion{ nullptr };
    public:
        explicit QueryPool(std::shared_ptr<vht::Device> device)
        :   m_device(std::move(device)) {
            init();
        }

        void print_delta_time() const {
            // 返回值是 vk::Result 和 std::vector<uint64_t> 的 pair
            const auto [result, vec]= m_timestamp.getResults<std::uint64_t>(
                0,                          // 查询的起始索引
                2,                          // 查询的数量
                2 * sizeof(std::uint64_t),       // 查询结果总数组的大小
                sizeof(std::uint64_t),           // 查询结果的单个元素的大小
                vk::QueryResultFlagBits::e64 |      // 64位结果
                vk::QueryResultFlagBits::eWait      // 等待查询结果
            );
            // 返回数组的元素数是 总数组大小 / 单个元素大小
            const std::uint64_t delta = vec.at(1) - vec.at(0); // 计算时间差，纳秒
            std::println("Time consumption per frame:  {} microsecond", delta / 1000);
        }
        void print_statistics() const {
            // 返回值是 vk::Result 和 uint64_t ， 注意函数是 getResult, 末尾没有 s ，返回单个结果
            const auto [result, count] = m_statistics.getResult<std::uint64_t>(
                0,                          // 查询的起始索引
                1,                          // 查询的数量
                sizeof(std::uint64_t),           // 查询结果的单个元素的大小
                vk::QueryResultFlagBits::e64 |      // 64位结果
                vk::QueryResultFlagBits::eWait      // 等待查询结果
            );
            std::println("Vertex shader invocations: {}", count);
        }
        void print_occlusion() const {
            const auto [result, count] = m_occlusion.getResult<std::uint64_t>(
                0,                          // 查询的起始索引
                1,                          // 查询的数量
                sizeof(std::uint64_t),           // 查询结果的单个元素的大小
                vk::QueryResultFlagBits::e64 |      // 64位结果
                vk::QueryResultFlagBits::eWait      // 等待查询结果
            );
            std::println("Occlusion query result:  {}", count);
        }

        [[nodiscard]]
        const vk::raii::QueryPool& timestamp() const { return m_timestamp; }
        [[nodiscard]]
        const vk::raii::QueryPool& statistics() const { return m_statistics; }
        [[nodiscard]]
        const vk::raii::QueryPool& occlusion() const { return m_occlusion; }
    private:
        void init() {
            create_timestamp_pool();
            create_statistics_pool();
            create_occlusion_pool();
        }
        void create_timestamp_pool() {
            vk::QueryPoolCreateInfo create_info;
            create_info.queryType = vk::QueryType::eTimestamp; // 指定类型
            create_info.queryCount = 2; // 内部“查询”的数量，时间段需要2个时间点，所以需要2个查询
            m_timestamp = m_device->device().createQueryPool( create_info );
        }
        void create_statistics_pool() {
            vk::QueryPoolCreateInfo create_info;
            create_info.queryType = vk::QueryType::ePipelineStatistics; // 指定类型
            create_info.queryCount = 1; // 指定内部可用的“查询”的数量
            // 指定要查询的信息类型，这里指定顶点着色器调用次数
            create_info.pipelineStatistics = vk::QueryPipelineStatisticFlagBits::eVertexShaderInvocations;
            m_statistics = m_device->device().createQueryPool( create_info );
        }
        void create_occlusion_pool() {
            vk::QueryPoolCreateInfo create_info;
            create_info.queryType = vk::QueryType::eOcclusion; // 指定类型
            create_info.queryCount = 1; // 指定内部可用的“查询”的数量
            m_occlusion = m_device->device().createQueryPool( create_info );
        }
    };
}
