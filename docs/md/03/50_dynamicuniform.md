# **动态Uniform缓冲**

## **前言**

上一节我们只有2个物体，使用推送常量更简单高效，但它不适合大量数据。这一节我们会增加更多的几何体，就需要用到动态 Uniform 缓冲。

动态 Uniform 的核心特性是允许通过偏移量复用同一个描述符。
普通 Uniform 缓冲，直接将整个数据绑定为一个对象；而动态 Unifrom 缓冲可以包含多个数据段，内存修改方式与普通 Uniform 相同，但在命令录制时可以通过动态偏移快速切换数据段。

| **特性**  | **普通Uniform缓冲区** | **推送常量 (Push Constants)** | **动态Uniform缓冲区**  |
|----------|----------------------|-------------------------------|-----------------------|
| **数据量** | 中等  | 极小（通常≤128字节） | 大量（如数千个矩阵）      |
| **更新频率**    | 较低（如每帧或更慢）    | 极高（每绘制调用均可更新） | 高（如每实例数据）   |
| **延迟**  | 中等（需通过描述符绑定）| 极低（直接嵌入命令缓冲区） | 中等（需动态偏移绑定）  |
| **性能优化点** | 适合复用数据（如全局矩阵） | 适合高频微小数据（如开关、标量） | 适合批处理大量相似数据（如实例化渲染） |
| **常见用途**   | 视口/投影矩阵、材质参数 | 调试开关、实时标量参数（如时间戳） | 实例变换矩阵、大规模粒子参数 |
| **代码复杂度**  | 中等（需管理描述符池/集） | 简单 | 较高（需管理描述符、需处理动态偏移和内存对齐）  |

在本章中，我们会绘制多个兔子模型，改用 Uniform 缓冲区设置每个模型的模型矩阵。

## **增加模型数**

### 1. 重复加载模型

首先，添加一个静态常量成员，用于记录兔子数量：

```cpp
static constexpr int BUNNY_NUMBER = 5;
```

然后修改 `initVulkan` 函数，循环多次绘制：

```cpp
for (int i = 0; i < BUNNY_NUMBER; ++i) {
    loadModel(BUNNY_PATH);
}
```

> 此写法存在冗余，因为我们需要多次记录索引，但不需要重复读取模型文件，你可以自行优化。

### 2. 记录模型矩阵

现在添加一个成员变量，用于记录每个图像的模型矩阵：

```cpp
std::vector<glm::mat4> m_modelMatrices;
```

然后在 `initVulkan` 中添加一个函数 `initModelMatrices` 用于初始化它：

```cpp
void initVulkan() {
    ...
    for (int i = 0; i < BUNNY_NUMBER; ++i) {
        loadModel(BUNNY_PATH);
    }
    initModelMatrices();
    ...
}
...
void initModelMatrices(){

}
```

我们先采取一种简单的方式，直接随机生成这些兔子的位置：

```cpp
#include <random>

......

void initModelMatrices() {
    m_modelMatrices.reserve(BUNNY_NUMBER+1);
    // 为房子模型添加变换矩阵
    glm::mat4 roomModel = glm::rotate(
        glm::mat4(1.0f), 
        glm::radians(-90.0f), 
        glm::vec3(1.0f, 0.0f, 0.0f)
    )  * glm::rotate(
        glm::mat4(1.0f), 
        glm::radians(-90.0f), 
        glm::vec3(0.0f, 0.0f, 1.0f)
    );
    m_modelMatrices.emplace_back( roomModel );
    // 随机数生成器
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<float> dis(-1.0f, 1.0f);
    // 为兔子模型初始化BUNNY_NUMBER个模型矩阵
    for (int i = 0; i < BUNNY_NUMBER; ++i) {
        glm::mat4 model = glm::mat4(1.0f);
        // 随机的位置与朝向
        model = glm::translate(model, glm::vec3(dis(gen), dis(gen), dis(gen)));
        model = glm::rotate(model, glm::radians(dis(gen) * 180.0f), glm::vec3(0.0f, 1.0f, 0.0f));
        m_modelMatrices.emplace_back( model );
    }
}
```

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

和之前的UBO类似，动态UBO也需要缓冲区、内存和映射指针，现在添加成员变量：

```cpp
......
std::vector<vk::raii::DeviceMemory> m_dynamicUniformBuffersMemory;
std::vector<vk::raii::Buffer> m_dynamicUniformBuffers;
std::vector<void*> m_dynamicUniformBuffersMapped;
......
```

我们会在普通UBO之后创建动态UBO，将这些成员变量声明在 `m_uniformBuffersMapped` 下方即可。

然后创建一个新函数 `createDynamicUniformBuffers` 用于创建它们：

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
    vk::DeviceSize bufferSize  = sizeof(glm::mat4) * m_modelMatrices.size();

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

注意我们绑定的 `buffer` 的大小是 `sizeof(glm::mat4)` \* `m_modelMatrices.size()` ，包含全部模型矩阵。但 `range` 是 `sizeof(glm::mat4)` ，只含一个模型矩阵。我们后续可以通过偏移量决定动态UBO加载哪个矩阵。

### 4. 更新动态Uniform缓冲内容

创建一个新函数 `updateDynamicUniformBuffer` 用于更新缓冲内容：

```cpp
void updateDynamicUniformBuffer(uint32_t currentImage) {
    vk::DeviceSize modelSize = sizeof(glm::mat4);

    for (size_t i = 0; i < m_modelMatrices.size(); ++i) {
        memcpy(
            static_cast<char*>(m_dynamicUniformBuffersMapped[currentImage]) + i * modelSize,
            &m_modelMatrices[i],
            modelSize
        );
    }
}
```

我们可以在 `drawFrame` 函数中使用它：

```cpp
void drawFrame() {
    ......
    updateUniformBuffer(m_currentFrame);
    updateDynamicUniformBuffer(m_currentFrame);
    ......
}
```

> 实际上，我们只需要在缓冲内容变换时更新。
> 由于我们目前的场景是静止的，你可以将它放在 `initVulkan` 中，只集体更新一次。

### 5. 记录命令

现在需要修改 `recordCommandBuffer` ，使用动态Unifrom缓冲的模型矩阵，而不是推送常量：

```cpp
// 删除循环外的描述符集绑定
// commandBuffer.bindDescriptorSets(
//     vk::PipelineBindPoint::eGraphics, 
//     m_pipelineLayout,
//     0,
//     *m_descriptorSets[m_currentFrame],
//     nullptr
// );

for(size_t index = 0; const uint32_t firstIndex : m_indicesOffsets) {
    // 推送常量用于控制是否纹理采样
    PushConstantData pcData;
    pcData.enableTexture = (index == 0 ? 1 : 0);
    commandBuffer.pushConstants<uint32_t>(
        m_pipelineLayout,
        vk::ShaderStageFlagBits::eFragment,
        0, // offset
        pcData.enableTexture
    );
    // 动态UBO控制模型矩阵
    uint32_t dynamicOffset = index * sizeof(glm::mat4);
    commandBuffer.bindDescriptorSets(
        vk::PipelineBindPoint::eGraphics,
        m_pipelineLayout,
        0,
        *m_descriptorSets[m_currentFrame],
        dynamicOffset
    );
    commandBuffer.drawIndexed(
        index + 1 == m_indicesOffsets.size() ? m_indices.size() - firstIndex : m_indicesOffsets[index + 1] - firstIndex,
        1,
        firstIndex,
        0,
        0
    );
    ++index;
}
```

我们将描述符集的绑定放在了循环内，唯一的修改是我们使用了最后一个参数“动态偏移量”。

### 6. 修改推送常量

我们需要删除推送常量的冗余内容并调整相关管线布局的配置。

首先是 `PushConstantData` 结构体可以改为：

```cpp
struct PushConstantData {
    uint32_t enableTexture;
};
```

然后修改 `createGraphicsPipeline` 中的推送常量配置：

```cpp
vk::PushConstantRange pushConstantRange;
pushConstantRange.stageFlags = vk::ShaderStageFlagBits::eFragment;
pushConstantRange.offset = 0;
pushConstantRange.size = sizeof(uint32_t);

vk::PipelineLayoutCreateInfo pipelineLayoutInfo;
pipelineLayoutInfo.setPushConstantRanges( pushConstantRange );
```

### 7. 修改着色器

现在回到顶点着色器，删除推送常量的模型矩阵，改用动态UBO的模型矩阵：

```glsl
......

layout(binding = 2) uniform DynamicUBO {
    mat4 model;
} dynamicUbo;

......

void main() {
    gl_Position = ubo.proj * ubo.view * dynamicUbo.model * vec4(inPosition, 1.0);
    ......
}

```

还需要修改片段着色器中推送常量的偏移量：

```glsl
layout(push_constant) uniform PushConstants {
    uint enableTexture;
} pc;
```

## **测试**

现在你可以运行程序，在房子周围会有5只兔子，且每次重启程序后位置和朝向都不同，类似这样：

![random_bunny](../../images/0350/random_bunny.png)

---

## **最后**

你现在掌握的 Vulkan 基本原理知识应该足以开始探索更多功能，例如

- 实例渲染\(Instanced rendering\)
- 分离的图像和采样器描述符\(Separate images and sampler descriptors\)
- 管线缓存\(Pipeline cache\)
- 多线程命令缓冲生成\(Multi-threaded command buffer generation\)
- 多个子通道\(Multiple subpasses\)
- 计算着色器\(Compute shaders\)

当前的程序可以通过多种方式扩展，例如添加 Blinn-Phong 光照、后期处理效果和阴影贴图。
你应该能够从其他 API 的教程中学习这些效果是如何工作。尽管 Vulkan API 很细致，但许多概念仍然是相同的。

> 理论上，作者会慢慢更新这些内容，你可以看看Github仓库的最近更新时间。

---

**[C++代码](../../codes/03/50_dynamicuniform/main.cpp)**

**[C++代码差异](../../codes/03/50_dynamicuniform/main.diff)**

**[根项目CMake代码](../../codes/03/50_dynamicuniform/CMakeLists.txt)**

**[shader-CMake代码](../../codes/03/50_dynamicuniform/shaders/CMakeLists.txt)**

**[shader-vert代码](../../codes/03/50_dynamicuniform/shaders/shader.vert)**

**[shader-vert代码差异](../../codes/03/50_dynamicuniform/shaders/vert.diff)**

**[shader-frag代码](../../codes/03/50_dynamicuniform/shaders/shader.frag)**

**[shader-frag代码差异](../../codes/03/50_dynamicuniform/shaders/frag.diff)**
