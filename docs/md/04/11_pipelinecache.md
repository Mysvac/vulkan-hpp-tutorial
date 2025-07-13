---
title: 管线缓存
comments: true
---
# **管线缓存**

## **前言**

管线缓存（Pipeline Cache）是一种用于加速管线对象创建的机制。

Vulkan 管线对象的创建过程非常复杂且耗时，尤其是在首次创建时。
而管线缓存可以将创建过程中产生的中间数据缓存下来，避免每次启动程序或重新创建管线时都从头开始编译和优化。

> 关于管线缓存：[Vulkan-Guide \[pipeline cache\]](https://docs.vulkan.org/guide/latest/pipeline_cache.html)

## **基础代码**

请下载并阅读下面的基础代码，这是“C++模块化”章节的第二部分代码：

**[点击下载](../../codes/04/00_cxxmodule/module_code.zip)**

## **创建管线缓存**

我们目前只有一个管线，为其添加管线缓存的方式非常简单。

### 1. 添加成员

转到 `GraphicsPipeline.cppm` 文件，添加一个成员变量管理缓存句柄，将它放在管线对象的上方：

```cpp
vk::raii::PipelineCache m_pipeline_cache{ nullptr };
```

> 它只在图形管线创建时使用，你也可以将他作为管线创建函数的局部变量。

### 2. 创建

在管线创建函数 `create_graphics_pipeline` 之前，添加一个函数 `create_pipeline_cache` ：

```cpp
void init() {
    create_descriptor_set_layout();
    create_pipeline_cache();
    create_graphics_pipeline();
}

void create_pipeline_cache() {
    vk::PipelineCacheCreateInfo pipelineCacheInfo;
    m_pipeline_cache = m_device->device().createPipelineCache(pipelineCacheInfo);
}
```

目前我们还没有保存任何缓存数据，无需读取本地文件，直接创建即可。

### 3. 使用


在“管线创建”章节的最后，我提到过管线创建函数的第一个参数就是管线缓存，当时我们使用了 `nullptr` 表示无缓存。
现在回到 `create_graphics_pipeline` 函数，将其改为我们刚刚创建的管线缓存对象：

```cpp
void create_graphics_pipeline() {
    ......

    m_pipeline = m_device->device().createGraphicsPipeline( m_pipeline_cache, create_info );
}
```

### 4. 测试

现在重新运行程序，应该和上一章的效果没有任何区别。

## **保存与加载缓存**

上面虽然使用了管线缓存，但缓存的内容是空的，没有起到任何作用。
我们需要在管线创建完成后，将管线信息保存到本地，以便下一次运行时读取。

### 1. 保存管线信息

创建一个 `save_pipeline_cache` 函数用于保存管线数据，在 `create_graphics_pipeline` 后立即调用：

```cpp
void init() {
    create_descriptor_set_layout();
    create_pipeline_cache();
    create_graphics_pipeline();
    save_pipeline_cache();
}
void save_pipeline_cache() {
    // std::vector<uint8_t>
    const auto cacheData = m_pipeline_cache.getData();
    std::ofstream out("pipeline_cache.data", std::ios::binary); // 文件后缀任意
    out.write(reinterpret_cast<const char*>(cacheData.data()), cacheData.size());
}
```

注意到，我们直接从缓存对象 `m_pipeline_cache` 中读取的数据，因为我们在创建管线时绑定了缓存，管线会将相关数据存入缓存对象。

### 2. 读取本地缓存

现在可以修改 `create_pipeline_cache` 函数，查询本地是否存在缓冲文件。
如果存在就读取缓冲，如果不存在就直接创建。

```cpp
    void create_pipeline_cache() {
        vk::PipelineCacheCreateInfo pipelineCacheInfo;
        std::vector<char> data;
        if (std::ifstream in("pipeline_cache.data", std::ios::binary | std::ios::ate);
            in
        ) {
            const size_t size = in.tellg();
            in.seekg(0);
            data.resize(size);
            in.read(data.data(), size);
            pipelineCacheInfo.setInitialData<char>(data);
            std::println("Pipeline cache loaded from file.");
        }
        m_pipeline_cache = m_device->device().createPipelineCache(pipelineCacheInfo);
    }
```

如果文件存在，会读取本地数据并存入 `data` 变量中，并通过 `setInitialData` 模板函数设置数据的指针和长度。
`createInfo` 保存的是指针和长度信息，所以要保证 `data` 在创建缓存时还有效，不能放在 `if` 块中。

## **测试**

现在运行程序，应该看到一样的内容。

![right_room](../../images/0310/right_room.png)

由于我们的程序比较简单，你可能察觉不到创建速度的差异，但它在实际项目中非常重要。

---

**[GraphicsPipeline.cppm](../../codes/04/11_pipelinecache/GraphicsPipeline.cppm)**

**[GraphicsPipeline.diff\(差异文件\)](../../codes/04/11_pipelinecache/GraphicsPipeline.diff)**

---

