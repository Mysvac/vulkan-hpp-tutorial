---
title: 查询池
comments: true
---
# **查询池**

**本节内容待重构！！**

## **前言**

Vulkan 的 **查询池\(Query Pool\)** 是一种用于收集 GPU 统计信息和性能数据的机制。通过查询池，你可以获取诸如时间戳、管线统计、采样器使用情况等信息，常用于性能分析和调试。

- 时间戳查询：测量 GPU 上命令执行的时间间隔。
- 管线统计查询：统计顶点数、片元数、着色器调用次数等渲染管线相关数据。
- 遮挡查询：判断绘制的物体是否被可见。

## **基础代码**

请下载并阅读下面的基础代码，这个就是上一节“模块化”的第二部分代码：

**[点击下载](../../codes/04/00_cxxmodule/module_code.zip)**

我们将再次基础上创建一个“查询池”模块，并演示上述的三种查询方式。

## **查询池模块**

### 1. 创建查询池模块

首先，在项目的 `src/utils/` 目录下新建一个文件 `QueryPool.cppm`，并添加以下代码：

```cpp
module;
#include <memory>
export module QueryPool;

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

        [[nodiscard]]
        const vk::raii::QueryPool& timestamp() const { return m_timestamp; }
        [[nodiscard]]
        const vk::raii::QueryPool& statistics() const { return m_statistics; }
        [[nodiscard]]
        const vk::raii::QueryPool& occlusion() const { return m_occlusion; }
    private:
        void init() {
            
        }
    };
}
```

查询池只需要用到逻辑设备，所以我们通过构造函数注入了依赖。
我们还需要三种查询池，分别时间信息、管线统计和遮挡查询，因此创建了三个成员变量并对外提供访问句柄。

> Vulkan raii 对象的大部分操作都是 const 的，我们使用此修饰防止资源被外部移动。

### 2. 创建查询池对象

查询池，顾名思义是个“池”，内部包含多个查询对象。
查询池的创建方式非常简单，我们只需要指定“查询对象”的类型和数量即可，管线统计查询池还需要指定管线阶段：

```cpp
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
```

时间戳查询可以用于测量 GPU 上命令执行的时间间隔，一个时间段需要 2 个时间点（开始和结束），因此我们将查询数量设为 2。
管线统计需要指定要查询的类型，这里我们指定了顶点着色器调用次数，你可以自行浏览可能的枚举值。

“管线统计查询”需要启用设备特性，现在修改 `Device.cppm` 的 `create_device` 函数：

```cpp
vk::PhysicalDeviceFeatures features;
features.pipelineStatisticsQuery = true;
```

查询池的准备工作就是这么简单，接下来就可以使用了。

## **依赖注入**

回顾命令缓冲录制的方式，会有一个 `begin` 和 `end` 的过程。
查询也需要在命令缓冲中进行录制，时间戳查询直接在需要的时候插入时间搓，管线统计查询和遮挡查询则需要使用各自的 `begin` 和 `end` 函数。

我们在很多地方用到了命令录制，本章节只查询最终图像的绘制与呈现信息。
我们的绘制命令位于 `Drawer.cppm` 中，我们要为它注入查询池的依赖。

### 1. Drawer模块

首先修改 `vht::Drawer` 类，添加一个接受 `vht::QueryPool` 的智能指针并通过构造函数初始化：

```cpp
...
import QueryPool; // 导入查询池模块
...
class Drawer {
    ...
    std::shared_ptr<vht::Descriptor> m_descriptor{ nullptr };
    std::shared_ptr<vht::QueryPool> m_query_pool; // 添加查询池成员
    std::vector<vk::raii::Semaphore> m_image_available_semaphores;
    ...
public:    
    explicit Drawer(
        ...
        std::shared_ptr<vht::Descriptor> descriptor,
        std::shared_ptr<vht::QueryPool> query_pool // 添加查询池参数
    ):  ... ,
        m_query_pool(std::move(query_pool)) { // 成员变量初始化
        ...
    }
    ......
};
...
```

### 2. App模块

我们在 `App.cppm` 中创建查询池对象，并将其传递给 `Drawer`：

```cpp
...
import QueryPool; // 导入查询池模块
import Drawer;
...
class App {
    ...
    std::shared_ptr<vht::QueryPool> m_query_pool; // 添加查询池成员
    std::shared_ptr<vht::Drawer> m_drawer{ nullptr };
    ......
    void init() {
        ......
        init_query_pool(); // 初始化查询池
        std::cout << "query pool created" << std::endl;
        init_drawer();
        std::cout << "drawer created" << std::endl;
    }
    ......
    void init_query_pool() { m_query_pool = std::make_shared<vht::QueryPool>( m_device ); }
    void init_drawer() {
        m_drawer = std::make_shared<vht::Drawer>(
             ...
            m_descriptor,
            m_query_pool // 传递查询池
        );
    }
};
```

## **时间戳查询**

### 1. 插入时间戳查询

首先是时间戳查询，假设我们需要测量整个命令的执行时间，我们可以在命令缓冲的开始和结束处添加时间戳查询：
回到 `Drawer.cppm` 的 `record_command_buffer` 函数，修改首尾部分代码：

```cpp

void record_command_buffer( ... ) {
    command_buffer.begin( vk::CommandBufferBeginInfo{} );
    // 重置查询池，索引 0 和 1
    command_buffer.resetQueryPool( m_query_pool->timestamp(), 0, 2 ); 
    command_buffer.writeTimestamp( // 在管线顶部写入时间戳
        vk::PipelineStageFlagBits::eTopOfPipe, 
        m_query_pool->timestamp(), // 查询池
        0   // 查询池内部的查询索引
    );
    
    ......
    // 开始渲染通道，绘制，结束渲染通道
    ......
    
    command_buffer.writeTimestamp( // 在管线底部写入时间戳
        vk::PipelineStageFlagBits::eBottomOfPipe, 
        m_query_pool->timestamp(), 1 // 索引 1
    );
    command_buffer.end();
}
```

查询池内的查询在每次用于命令缓冲录制时都需要重置（包括第一次使用）。
我们交由命令缓冲，即 GPU 来重置查询池。使用查询池自身的 `reset` 成员函数则需要 `hostQueryReset` 特性 。

### 2. 读取时间戳查询结果

现在创建一个新函数，可以将它封装在 `QueryPool` 类中并公开访问权限：

```cpp
...
#include <vector>
#include <iostream>
......

void print_delta_time() const {
    // 返回值是 vk::Result 和 std::vector<uint64_t> 的 pair
    auto [result, vec]= m_timestamp.getResults<uint64_t>(
        0,                          // 查询的起始索引
        2,                          // 查询的数量
        2 * sizeof(uint64_t),       // 查询结果总数组的大小
        sizeof(uint64_t),           // 查询结果的单个元素的大小
        vk::QueryResultFlagBits::e64 |      // 64位结果
        vk::QueryResultFlagBits::eWait      // 等待查询结果
    );
    // 返回数组的元素数是 总数组大小 / 单个元素大小
    const uint64_t delta = vec.at(1) - vec.at(0); // 计算时间差，纳秒
    std::cout << "Time consumption per frame: " << (delta / 1000) << " microsecond" << std::endl;
}
```

`getResults` 函数用于批量获取查询结果，第二个返回值即查询结果的数组，第一个返回值是查询结果的状态。

### 3. 调用时间戳查询

现在回到 `Drawer.cppm` 模块。

查询命令需要等待命令执行完成后调用，也就需要 GPU 和 CPU 之间的同步，但我们的围栏只绑定在呈现命令上，无法等待绘制命令。
为了方便，本章直接在 `draw` 函数末尾使用 `pipeline.waitIdle()` 等待 GPU 执行完成。

此外，帧数过快导致输出数量太多，我们可以添加一个计数器，控制输出频率：

```cpp
void draw() {
    ......
    
    // 等待绘制命令完成
    m_device->graphics_queue().waitIdle();
    static int counter = 0;
    if( ++counter % 2500 == 0 ){ // 2500 帧输出一次，可自行调整
        m_query_pool->print_delta_time(); // 输出单次绘制耗时
    }
}
```

我们没有控制帧数上限，不同设备的性能差异较大，请自行调整输出频率。

> 注意，这样的等待方式对性能影响较大，此处仅方便演示，实际应用中应使用更合适的同步方式。

现在运行程序，你应该可以看到每隔一段时间输出一次（单次）绘制的耗时。

## **管线统计查询**

管线统计查询可以查询管线各个阶段的统计信息，例如细分、顶点和片段着色器的调用次数等，几何着色器的输出图元数等。
本章节顶点色器调用次数为例，我们在创建查询池时已经指定了查询类型。

### 1. 插入管线统计查询

与时间戳查询类似，我们需要在命令缓冲的开始和结束处插入管线统计查询：

```cpp
void record_command_buffer( ... ) const {
    command_buffer.begin( vk::CommandBufferBeginInfo{} );
    // 重置查询池，起始索引 0， 数量 1
    command_buffer.resetQueryPool( m_query_pool->statistics(), 0, 1 );
    command_buffer.beginQuery(m_query_pool->statistics(), 0); // 绑定管线统计信息查询
    
    ......
    
    command_buffer.endQuery(m_query_pool->statistics(), 0);
    command_buffer.end();
}
```

可以看到管线统计查询的方式非常简单，只需要选定查询池和查询索引即可，因为我们在创建查询池时已经指定了查询类型（顶点着色器调用次数查询）。

### 2. 读取管线统计查询结果

再次回到 `QueryPool.cppm` 模块，我们添加一个函数来读取管线统计查询结果：

```cpp
void print_statistics() const {
    // 返回值是 vk::Result 和 uint64_t ， 注意函数是 getResult, 末尾没有 s ，返回单个结果
    auto [result, count] = m_statistics.getResult<uint64_t>(
        0,                          // 查询的起始索引
        1,                          // 查询的数量
        sizeof(uint64_t),           // 查询结果的单个元素的大小
        vk::QueryResultFlagBits::e64 |      // 64位结果
        vk::QueryResultFlagBits::eWait      // 等待查询结果
    );
    std::cout << "Vertex shader invocations: " << count << std::endl;
}
```

`getResult` 函数用于获取单个查询结果，末尾没有 `s` 。

### 3. 调用管线统计查询

然后可以回到 `Drawer.cppm` 模块，在 `draw` 函数末尾调用管线统计查询：

```cpp
m_device->graphics_queue().waitIdle();
static int counter = 0;
if( ++counter % 2500 == 0 ){
    m_query_pool->print_delta_time();
    m_query_pool->print_statistics();
}
```

然后可以运行程序，移动摄像头或转动视角，你应该会看到每次输出的顶点着色器调用次数保持不变。

## **遮挡查询**

遮挡查询的工作原理是通过 GPU 统计渲染管线中通过深度和模板测试的片段数（即可见像素数）。

### 1. 插入遮挡查询

遮挡查询的使用方式和管线统计查询非常相似，修改 `record_command_buffer` 函数：

```cpp
    void record_command_buffer(const vk::raii::CommandBuffer& command_buffer, const uint32_t image_index) const {
        command_buffer.begin( vk::CommandBufferBeginInfo{} );
        // 遮挡性查询
        command_buffer.resetQueryPool( m_query_pool->occlusion(), 0, 1 );
        command_buffer.beginQuery(m_query_pool->occlusion(), 0);
        
        ......
        
        command_buffer.endQuery(m_query_pool->occlusion(), 0);
        command_buffer.end();
    }
```

### 2. 读取遮挡查询结果

回到 `QueryPool.cppm` 模块，添加一个函数来读遮挡性查询结果：

```cpp
void print_occlusion() const {
    auto [result, count] = m_occlusion.getResult<uint64_t>(
        0,                          // 查询的起始索引
        1,                          // 查询的数量
        sizeof(uint64_t),           // 查询结果的单个元素的大小
        vk::QueryResultFlagBits::e64 |      // 64位结果
        vk::QueryResultFlagBits::eWait      // 等待查询结果
    );
    std::cout << "Occlusion query result: " << count << std::endl;
}
```

### 3. 调用遮挡查询

然后可以回到 `Drawer.cppm` 模块，在 `draw` 函数末尾调用遮挡查询：

```cpp
m_device->graphics_queue().waitIdle();
static int counter = 0;
if( ++counter % 2500 == 0 ){
    m_query_pool->print_delta_time();
    m_query_pool->print_statistics();
    m_query_pool->print_occlusion();
}
```

再次运行程序，可以看到遮挡查询的输出，当你视角转动或移动摄像机位置，应该看到不同的输出。


---

**[初始代码集](../../codes/04/00_cxxmodule/module_code.zip)**

**[QueryPool.cppm（新增）](../../codes/04/10_querypool/cppm/QueryPool.cppm)**

**[Device.cppm（修改）](../../codes/04/10_querypool/cppm/Device.cppm)**

**[Device.diff（差异文件）](../../codes/04/10_querypool/cppm/Device.diff)**

**[Drawer.cppm（修改）](../../codes/04/10_querypool/cppm/Drawer.cppm)**

**[Drawer.diff（差异文件）](../../codes/04/10_querypool/cppm/Drawer.diff)**

**[App.cppm（修改）](../../codes/04/10_querypool/cppm/App.cppm)**

**[App.diff（差异文件）](../../codes/04/10_querypool/cppm/App.diff)**

> 请下载后本地查看（UTF-8），浏览器查看文件可能出现中文乱码。

---


