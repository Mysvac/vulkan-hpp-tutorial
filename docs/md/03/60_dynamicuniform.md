# **动态Uniform缓冲**

## **前言**

上一节的前言部分提到，可以使用 UBO 存放 `enableTexture` 控制字段。
我们之前学习的普通 Uniform 缓冲直接将全部数据绑定为一个对象，想要改变 UBO 内容只能更新缓冲内的数据。

本章我们将学习 **动态Uniform缓冲区** 。
动态 Uniform 缓冲的创建方式和内存修改方式与普通 Uniform 类似，但前者可以包含多个数据段，在命令录制时通过偏移量快速切换数据段从而改变 UBO 的值，无需修改缓冲。

| **特性**  | **普通Uniform缓冲区** | **推送常量 (Push Constants)** | **动态Uniform缓冲区**  |
|----------|----------------------|-------------------------------|-----------------------|
| **数据量** | 中等                  | 极小           | 较大（如数千个矩阵）      |
| **更新频率**    | 较低（如每帧或更慢）    | 极高（每绘制调用均可更新） | 高（如每实例数据）   |
| **延迟**  | 中等（需描述符绑定）| 低（直接嵌入命令缓冲区） | 中等（需描述符与动态偏移绑定）  |
| **性能优化点** | 适合复用数据（如全局矩阵） | 适合高频微小数据（如开关、标量） | 适合批处理大量相似数据（如实例化渲染） |
| **常见用途**   | 视口/投影矩阵、材质参数 | 调试开关、实时标量参数（如时间戳） | 实例变换矩阵、大规模粒子参数 |
| **代码复杂度**  | 中等（需管理描述符池/集） | 简单 | 较高（需管理描述符、需处理动态偏移和内存对齐）  |

> “实例化渲染”章节的实例缓冲需通过暂存缓冲才能修改，主要存放静态（不变的）数据。


本章我们会使用动态UBO，让兔子不断旋转，而房屋保持不变。

## **保存位置信息**

我们需要一个模型矩阵记录兔子的旋转角度，现在添加一个成员变量：

```cpp
std::vector<glm::mat4> m_dynamicUboMatrices;
```

然后创建一个函数初始化它:

```cpp
void initVulkan() {
    ......
    initInstanceDatas();
    initDynamicUboMatrices();
    ......
}
void initDynamicUboMatrices() {
    m_dynamicUboMatrices.push_back(glm::mat4(1.0f));
    m_dynamicUboMatrices.push_back(glm::mat4(1.0f));
}
```

我们创建了两个矩阵，第一个用于记录房屋模型的动态变化，第二个用于记录兔子模型的动态变化。

## **创建动态Uniform**

### 1. 修改描述符布局

修改 `createDescriptorSetLayout` 函数，添加动态UBO的布局绑定信息：

```cpp
void createDescriptorSetLayout() {
    ......

    vk::DescriptorSetLayoutBinding dynamicUboLayoutBinding;
    dynamicUboLayoutBinding.binding = 2;
    dynamicUboLayoutBinding.descriptorType = vk::DescriptorType::eUniformBufferDynamic;
    dynamicUboLayoutBinding.descriptorCount = 1;
    dynamicUboLayoutBinding.stageFlags = vk::ShaderStageFlagBits::eVertex;

    auto bindings = { uboLayoutBinding, samplerLayoutBinding, dynamicUboLayoutBinding };
    vk::DescriptorSetLayoutCreateInfo layoutInfo;
    layoutInfo.setBindings( bindings );

    ......
}
```

描述符布局相关内容可以参考“uniform缓冲”章节的内容。

### 2. 创建缓冲区

和之前的UBO类似，动态UBO也需要缓冲区、内存和映射指针。
我们会在普通UBO之后创建动态UBO，可以将这些成员变量声明在 `m_uniformBuffersMapped` 下方：

```cpp
......
std::vector<vk::raii::DeviceMemory> m_dynamicUniformBuffersMemory;
std::vector<vk::raii::Buffer> m_dynamicUniformBuffers;
std::vector<void*> m_dynamicUniformBuffersMapped;
......
```

然后创建一个新函数 `createDynamicUniformBuffers` 用于初始化它们：

```cpp
void initVulkan() {
    ...
    createUniformBuffers();
    createDynamicUniformBuffers();
    ...
}
void createDynamicUniformBuffers() {

}
```

函数内容和 `createUniformBuffers` 几乎一样：

```cpp
void createDynamicUniformBuffers() {
    vk::DeviceSize bufferSize  = sizeof(glm::mat4) * m_dynamicUboMatrices.size();

    m_dynamicUniformBuffers.reserve(MAX_FRAMES_IN_FLIGHT);
    m_dynamicUniformBuffersMemory.reserve(MAX_FRAMES_IN_FLIGHT);
    m_dynamicUniformBuffersMapped.reserve(MAX_FRAMES_IN_FLIGHT);

    for(size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        m_dynamicUniformBuffers.emplace_back( nullptr );
        m_dynamicUniformBuffersMemory.emplace_back( nullptr );
        m_dynamicUniformBuffersMapped.emplace_back( nullptr );
        createBuffer(bufferSize, 
            vk::BufferUsageFlagBits::eUniformBuffer, 
            vk::MemoryPropertyFlagBits::eHostVisible | 
            vk::MemoryPropertyFlagBits::eHostCoherent, 
            m_dynamicUniformBuffers[i], 
            m_dynamicUniformBuffersMemory[i]
        );

        m_dynamicUniformBuffersMapped[i] = m_dynamicUniformBuffersMemory[i].mapMemory(0, bufferSize);
    }
}
```

别忘了在 `cleanup` 中结束映射：

```cpp
void cleanup() {
    ......
    for(const auto& it : m_dynamicUniformBuffersMemory){
        it.unmapMemory();
    }
    ......
}
```

### 3. 修改描述符池与集合

描述符集需从描述池分配，所以我们先修改描述符池，调整 `createDescriptorPool` 函数：

```cpp
void createDescriptorPool() {
    std::array<vk::DescriptorPoolSize, 3> poolSizes;
    ......
    poolSizes[2].type = vk::DescriptorType::eUniformBufferDynamic;
    poolSizes[2].descriptorCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);
    ......
}
```

然后调整 `createDescriptorSets` 函数，用于实际分配并修改描述符集：

```cpp
void createDescriptorSets() {
    ......

    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i) {
        ......

        vk::DescriptorBufferInfo dynamicBufferInfo;
        dynamicBufferInfo.buffer = m_dynamicUniformBuffers[i];
        dynamicBufferInfo.offset = 0;
        dynamicBufferInfo.range = sizeof(glm::mat4);

        ......

        std::array<vk::WriteDescriptorSet, 3> descriptorWrites;

        ......

        descriptorWrites[2].dstSet = m_descriptorSets[i];
        descriptorWrites[2].dstBinding = 2;
        descriptorWrites[2].dstArrayElement = 0;
        descriptorWrites[2].descriptorType = vk::DescriptorType::eUniformBufferDynamic;
        descriptorWrites[2].setBufferInfo(dynamicBufferInfo);

        ......
    }
}
```

注意 `buffer` 的大小是 `sizeof(glm::mat4)` \* `m_dynamicUboMatrices.size()` ，包含全部矩阵。但 `range` 是 `sizeof(glm::mat4)` ，只含一个矩阵。我们后续可以通过偏移量决定动态UBO加载哪个矩阵。

### 4. 更新动态Uniform缓冲内容

创建一个新函数 `updateDynamicUniformBuffer` 用于更新缓冲内容：

```cpp
void updateDynamicUniformBuffer(uint32_t currentImage) {
    static auto startTime = std::chrono::high_resolution_clock::now();
    auto currentTime = std::chrono::high_resolution_clock::now();
    float time = std::chrono::duration<float, std::chrono::seconds::period>(currentTime - startTime).count();
    startTime = currentTime;

    m_dynamicUboMatrices[1] = glm::rotate(
        m_dynamicUboMatrices[1], 
        glm::radians(time * 60.0f), 
        glm::vec3(0.0f, 1.0f, 0.0f)
    );

    memcpy(
        m_dynamicUniformBuffersMapped[currentImage],
        m_dynamicUboMatrices.data(),
        sizeof(glm::mat4) * m_dynamicUboMatrices.size()
    );
}
```

第二个矩阵将用于兔子，我们让其不断旋转，通过时间计算旋转角度保证转速均匀。

> 我们只修改了第二个矩阵，所以 `memcpy` 可以部分复制，你可以自行修改代码。

现在可以在 `drawFrame` 函数中使用它：

```cpp
void drawFrame() {
    ......
    updateUniformBuffer(m_currentFrame);
    updateDynamicUniformBuffer(m_currentFrame);
    ......
}
```

### 5. 记录命令

现在修改 `recordCommandBuffer` 函数，在两次绘制前分别调用 `bindDescriptorSets` 函数：

```cpp
......

uint32_t dynamicOffset = 0;
commandBuffer.bindDescriptorSets(
    vk::PipelineBindPoint::eGraphics, 
    m_pipelineLayout,
    0,
    *m_descriptorSets[m_currentFrame],
    dynamicOffset
);

uint32_t enableTexture = 1; 
commandBuffer.pushConstants<uint32_t>( ... );
commandBuffer.drawIndexed( ... );

dynamicOffset = sizeof(glm::mat4);
commandBuffer.bindDescriptorSets(
    vk::PipelineBindPoint::eGraphics, 
    m_pipelineLayout,
    0,
    *m_descriptorSets[m_currentFrame],
    dynamicOffset
);

enableTexture = 0;
commandBuffer.pushConstants<uint32_t>( ... );
commandBuffer.drawIndexed( ... );

......
```

所有的描述符绑定共用一个 `bindDescriptorSets` 函数，我们需要在两次绘制前使用不同的偏移量重新绑定描述符集。

第一次调用时的偏移量为 `0` ，所以动态 UBO 将加载动态 Uniform 缓冲中的第一个矩阵，也就是没有修改过的单位矩阵。
第二次调用时的偏移量为 `sizeof(glm::mat4)` ，所以会加载第二个矩阵，也就是旋转矩阵。

### 6. 修改着色器

现在回到顶点着色器，接收动态UBO的模型矩阵并使用：

```glsl
......

layout(binding = 2) uniform DynamicUBO {
    mat4 model;
} dynamicUbo;

......

void main() {
    gl_Position = ubo.proj * ubo.view * inModel * dynamicUbo.model * vec4(inPosition, 1.0);
    ......
}
```

如果 `dynamicUbo.model` 是单位矩阵，则没有任何效果。如果是旋转矩阵，会让模型先旋转再参与后续变换。

> 如果你调换 `inModel` 和 `dynamicUbo.model` 的位置，会看到不同的旋转方式。

## **测试**

现在你可以运行程序，将看到每个兔子模型都在缓慢自转，而房屋保持静止。

![rotating_bunny](../../images/0360/rotating_bunny.gif)

---

## **最后**

你现在掌握的 Vulkan 基本知识应该足以开始探索更多功能，例如

- 分离的图像和采样器描述符\(Separate images and sampler descriptors\)
- 管线缓存\(Pipeline cache\)
- 多线程命令缓冲生成\(Multi-threaded command buffer generation\)
- 多个子通道\(Multiple subpasses\)
- 计算着色器\(Compute shaders\)

当前的程序可以通过多种方式扩展，例如添加 Blinn-Phong 光照、后期处理效果和阴影贴图。
你应该能够从其他 API 的教程中学习这些效果是如何工作。尽管 Vulkan API 很细致，但许多概念仍然是相同的。

> 理论上，作者会慢慢更新这些内容，你可以看看Github仓库的最近更新时间。

---

**[C++代码](../../codes/03/60_dynamicuniform/main.cpp)**

**[C++代码差异](../../codes/03/60_dynamicuniform/main.diff)**

**[根项目CMake代码](../../codes/03/50_pushconstant/CMakeLists.txt)**

**[shader-CMake代码](../../codes/03/50_pushconstant/shaders/CMakeLists.txt)**

**[shader-vert代码](../../codes/03/60_dynamicuniform/shaders/shader.vert)**

**[shader-vert代码差异](../../codes/03/60_dynamicuniform/shaders/vert.diff)**

**[shader-frag代码](../../codes/03/60_dynamicuniform/shaders/shader.frag)**
