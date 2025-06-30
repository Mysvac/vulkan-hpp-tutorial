---
title: 管线缓存
comments: true
---
# **管线缓存**

## **前言**

管线缓存（Pipeline Cache）是一种用于加速管线对象创建的机制。由于 Vulkan 的管线对象创建过程非常复杂且耗时，尤其是在首次创建时。管线缓存可以将创建过程中产生的中间数据缓存下来，避免每次启动程序或重新创建管线时都从头开始编译和优化。

> 关于管线缓存：[Vulkan-Guide \[pipeline cache\]](https://docs.vulkan.net.cn/guide/latest/pipeline_cache.html)

## **创建管线缓存**

我们目前只有一个管线，为其添加管线缓存的方式非常简单。

### 1. 添加成员

可以添加一个成员变量管理缓存句柄，将它放在管线对象的上方：

```cpp
......
vk::raii::PipelineCache m_pipelineCache{ nullptr };
vk::raii::Pipeline m_graphicsPipeline{ nullptr };
......
```

> 它只在图形管线创建时使用，你也可以将他作为管线创建函数的局部变量。

### 2. 创建

在管线创建函数 `createGraphicsPipeline` 之前，添加一个函数 `createPipelineCache` ：

```cpp
void initVulkan() {
    ......
    createPipelineCache();
    createGraphicsPipeline();
    ......
}
void createPipelineCache() {
    vk::PipelineCacheCreateInfo pipelineCacheInfo;
    m_pipelineCache = m_device.createPipelineCache(pipelineCacheInfo);
}
```

目前我们还没有保存任何缓存数据，无需读取本地文件，直接创建即可。

### 3. 使用


在“图像管线”章节的总结部分，我提到过管线创建函数的第一个参数就是管线缓存，当时我们使用了 `nullptr` 表示无缓存。现在回到 `createGraphicsPipeline` 函数，将其改为我们刚刚创建的管线缓存对象：

```cpp
void createGraphicsPipeline() {
    ......

    m_graphicsPipeline = m_device.createGraphicsPipeline( m_pipelineCache, pipelineInfo );
}
```

### 4. 测试

现在重新运行程序，应该和上一章的效果没有任何区别。

## **保存与加载缓存**

上面虽然使用了管线缓存，但缓存的内容是空的，没有起到任何作用。
我们需要在管线创建完成后，将管线信息保存到本地，以便下一次运行时读取。

### 1. 保存管线信息

创建一个 `savePipelineCache` 函数用于保存管线数据，在 `createGraphicsPipeline` 后立即调用：

```cpp
void initVulkan() {
    ......
    createPipelineCache();
    createGraphicsPipeline();
    savePipelineCache();
    ......
}
void savePipelineCache() {
    // std::vector<uint8_t>
    auto cacheData = m_pipelineCache.getData();
    std::ofstream out("pipeline_cache.bin", std::ios::binary);
    out.write(reinterpret_cast<const char*>(cacheData.data()), cacheData.size());
}
```

注意到，我们直接从缓存对象 `m_pipelineCache` 中读取的数据，因为我们在创建管线时绑定了缓存，管线会将相关数据存入缓存对象。

### 2. 读取本地缓存

现在可以修改 `createPipelineCache` 函数，查询本地是否存在缓冲文件。
如果存在就读取缓冲，如果不存在就直接创建。

```cpp
void createPipelineCache() {
    vk::PipelineCacheCreateInfo pipelineCacheInfo;
    std::vector<char> data;
    std::ifstream in("pipeline_cache.bin", std::ios::binary | std::ios::ate);
    if (in) {
        size_t size = in.tellg();
        in.seekg(0);
        data.resize(size);
        in.read(data.data(), size);
        pipelineCacheInfo.setInitialData<char>(data);
    }
    m_pipelineCache = m_device.createPipelineCache(pipelineCacheInfo);
}
```

如果文件存在，会读取本地数据并存入 `data` 变量中，并通过 `setInitialData` 模板函数设置数据的指针和长度。
`createInfo` 保存的是指针和长度信息，所以要保证 `data` 在创建缓存时还有效，不能放在 `if` 块中。

## **测试**

现在运行程序，应该能看到和上一章一样的内容。

![final_crate](../../images/0370/final_crate.png)

> 由于我们的程序比较简单，你可能察觉不到创建速度的差异。

---

## **注意**

我们的 `main.cpp` 已经变得非常庞大，不适合再添加内容了。

对于后面的进阶章节，我们会在前言部分给出代码框架。
请读者先阅读基础代码，然后根据章节内容为框架添加代码。

---

**[C++代码](../../codes/03/80_pipelinecache/main.cpp)**

**[C++代码差异](../../codes/03/80_pipelinecache/main.diff)**

**[根项目CMake代码](../../codes/03/50_pushconstant/CMakeLists.txt)**

**[shader-CMake代码](../../codes/03/50_pushconstant/shaders/CMakeLists.txt)**

**[shader-vert代码](../../codes/03/60_dynamicuniform/shaders/shader.vert)**

**[shader-frag代码](../../codes/03/70_separatesampler/shaders/shader.frag)**

---
