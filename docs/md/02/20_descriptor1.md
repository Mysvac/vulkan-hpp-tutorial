---
title: 描述符与UBO
comments: true
---
# **描述符布局与缓冲**

## **前言**

我们在之前的章节中介绍了顶点输入，它必须由顶点着色器获取，片段着色器需要由顶点着色器转发数据。
此外，顶点输入的数据通常使用“设备本地缓存”获得最高性能，导致我们需要使用暂存缓冲进行更新，非常麻烦。

有没有什么方式，能让着色器直接访问显存中的数据？
这就用到了本节将介绍的**资源描述符(resource descriptors)**。
资源描述符用于描述着色器需要的资源，让着色器可以**直接**访问显存中的数据，对应着色器代码中的 `uniform` 关键字。

| 名称                            | 含义                               |
|-------------------------------|----------------------------------|
| 描述符\(Descriptor\)             | 对资源的抽象引用，告诉着色器如何访问某个资源           |
| 描述符集布局\(DescriptorSetLayout\) | 图形管线\(布局\)的一部分，定义所有描述符的类型、数量和绑定点 |
| 描述符集\(DescriptorSet\)         | 实际描述符的集合，描述符必须以描述符集的方式绑定管线       |
| 描述符池\(Descriptor Pool\)       | 用于分配描述符的内存池                      |

在本章中，我们会使用一个缓冲区存放 MVP\(模型-视图-投影\) 变换矩阵，使用这个矩阵让我们的正方形不断旋转。
并通过描述符允许顶点着色器访问 MVP 矩阵并使用。

描述符的类型很多，我们这一章使用统一缓冲对象(UBO, uniform buffer objects)。
后面几章会看到其他描述符类型，但它们的基本操作流程是一样的。


## **顶点着色器**

首先修改我们的顶点着色器代码，包含我们上面提到的统一缓冲对象(UBO)。
这里假设你已经熟悉了 MVP 变换，这是[GAMES101-现代计算机图形学入门](https://www.bilibili.com/video/BV1X7411F744)最初几节课的内容。


```glsl
#version 450

layout(binding = 0) uniform UniformBufferObject {
    mat4 model;
    mat4 view;
    mat4 proj;
} ubo;

layout(location = 0) in vec2 inPosition;
layout(location = 1) in vec3 inColor;

layout(location = 0) out vec3 fragColor;

void main() {
    gl_Position = ubo.proj * ubo.view * ubo.model * vec4(inPosition, 0.0, 1.0);
    fragColor = inColor;
}
```

我们使用了三个变换来获得最终的裁剪坐标。

注意 `uniform`、`in` 和 `out` 三种变量的声明顺序是任意的。
`binding` 和 `location` 指令类似，我们将在描述符布局中引用此绑定。

> `UniformBufferObject` 自定义的类型名，可以任意编写。

## **描述符集布局**

下一步要在 C++ 代码中定义 UBO 然后告诉 Vulkan 它在顶点着色器中对应的描述符。

### 1. 数据格式

首先添加一个结构体：

```cpp
struct UniformBufferObject {
    glm::mat4 model;
    glm::mat4 view;
    glm::mat4 proj;
};
```

我们使用 GLM 的类型，它的内存布局完全匹配着色器中的定义，所以我们可以直接使用 `memcpy` 。

> 类型名不重要，重要的是内存布局。

### 2. 辅助函数

我们需要在图形管线创建时指定描述符的细节，就像我们为顶点参数指定了`location`一样。
现在创建一个新函数`createDescriptorSetLayout`，在图形管线创建之前调用：

```cpp
void initVulkan() {
    ...
    createDescriptorSetLayout();
    createGraphicsPipeline();
    ...
}

...

void createDescriptorSetLayout() {

}
```

### 3. 描述符绑定信息

“描述符”的绑定信息结构体和“顶点输入”的绑定信息结构体不同，顶点输入的绑定信息用于描述数据的传输“数率”，而描述符的绑定信息用于指示资源可以被哪个着色器访问、通过什么方式访问以及数据的大小。

所有绑定信息都通过`vk::DescriptorSetLayoutBinding`结构体指定：

```cpp
vk::DescriptorSetLayoutBinding uboLayoutBinding;
uboLayoutBinding.binding = 0;
```

我们使用 `binding=0` ，对应着色器中的 `layout(binding = 0)` 。

顶点输入的 `binding` 对应缓冲区数组的索引，而描述符的绑定对应着色器的 `layout(binding = ...)` 。
可以这样理解，着色器的每个 `layout(binding = ...)` 对应一个窗口，着色器通过不同的窗口访问不同的资源，我们也需要将不同的内存资源放在不同的窗口。

然后设置描述符类型和数量：

```cpp
uboLayoutBinding.descriptorType = vk::DescriptorType::eUniformBuffer;
uboLayoutBinding.descriptorCount = 1;
```

第一个参数指定描述符的类型，本章使用 `eUniformBuffer`。
着色器变量支持数组类型的UBO，而我们只有一个对象，所以第二个参数指定为 1 。

我们还需要指定描述符将在哪些着色阶段被引用，使用 `vk::ShaderStageFlagBits` 位掩码。
我们只在顶点着色器使用，所以使用`eVertex`：

```cpp
uboLayoutBinding.stageFlags = vk::ShaderStageFlagBits::eVertex;
```

`pImmutableSamplers` 字段仅与图像采样相关的描述符有关，暂时无需设置。

### 4. 创建描述符布局

我们需要使用`vk::DescriptorSetLayout`对象定义描述符集合的布局信息，所以我们在`m_pipelineLayout`上面创建新成员变量：

```cpp
vk::raii::DescriptorSetLayout m_descriptorSetLayout{ nullptr };
vk::raii::PipelineLayout m_pipelineLayout{ nullptr };
```

下面填写`CreateInfo`结构体，并创建此对象：

```cpp
vk::DescriptorSetLayoutCreateInfo layoutInfo;
layoutInfo.setBindings( uboLayoutBinding );

m_descriptorSetLayout = m_device.createDescriptorSetLayout( layoutInfo );
```

### 5. 修改管线布局

我们需要在管线创建期间指定描述符布局，现在回到 `createGraphicsPipeline` 函数修改 `PipelineLayoutCreateInfo` 以引用布局对象：

```cpp
vk::PipelineLayoutCreateInfo pipelineLayoutInfo;
pipelineLayoutInfo.setSetLayouts(*m_descriptorSetLayout);
m_pipelineLayout = m_device.createPipelineLayout( pipelineLayoutInfo );
```

## **Uniform 缓冲区**

现在可以创建一个 `uniform` 缓冲区来存储 MVP 变换矩阵。
我们选择将每帧新数据直接复制到 uniform 缓冲，因此不需要暂存缓冲。
（此时使用暂存缓冲只会带来额外开销、降低性能。）

我们需要多个缓冲区，因为可能有多个帧同时在飞行中，我们不想在上一帧仍在读取时更新缓冲区以准备下一帧。
所以我们需要和飞行帧数一样多的 uniform 缓冲区，现在添加新成员：

```cpp
vk::raii::DeviceMemory m_indexBufferMemory{ nullptr };
vk::raii::Buffer m_indexBuffer{ nullptr };
std::vector<vk::raii::DeviceMemory> m_uniformBuffersMemory;
std::vector<vk::raii::Buffer> m_uniformBuffers;
std::vector<void*> m_uniformBuffersMapped;
```

现在创建一个函数`createUniformBuffers`，在`createIndexBuffer`之后调用

```cpp
void initVulkan() {
    ...
    createVertexBuffer();
    createIndexBuffer();
    createUniformBuffers();
}

void createUniformBuffers() {
    constexpr vk::DeviceSize bufferSize  = sizeof(UniformBufferObject);

    m_uniformBuffers.reserve(MAX_FRAMES_IN_FLIGHT);
    m_uniformBuffersMemory.reserve(MAX_FRAMES_IN_FLIGHT);
    m_uniformBuffersMapped.reserve(MAX_FRAMES_IN_FLIGHT);

    for(size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        m_uniformBuffers.emplace_back( nullptr );
        m_uniformBuffersMemory.emplace_back( nullptr );
        m_uniformBuffersMapped.emplace_back( nullptr );
        createBuffer(bufferSize,
            vk::BufferUsageFlagBits::eUniformBuffer,
            vk::MemoryPropertyFlagBits::eHostVisible |
            vk::MemoryPropertyFlagBits::eHostCoherent,
            m_uniformBuffers[i],
            m_uniformBuffersMemory[i]
        );

        m_uniformBuffersMapped[i] = m_uniformBuffersMemory[i].mapMemory(0, bufferSize);
    }
}
```

我们在创建后立即使用 `mapMemory` 映射缓冲区获取一个指针，稍后可以将数据写入其中。
缓冲区在应用程序的整个生命周期内都映射到此指针。
这种技术称为“**持久映射\(persistent mapping\)**”，在所有 Vulkan 实现上都有效。
不必每次需要更新时都映射缓冲区，这可以提高性能，因为映射不是免费的。

我们需要在程序结束的时候关闭映射：


```cpp
void cleanup() {
    for(const auto& it : m_uniformBuffersMemory){
        it.unmapMemory();
    }

    glfwDestroyWindow( m_window );
    glfwTerminate();
}
```

## **更新uniform数据**

创建一个新函数`updateUniformBuffer`，并在`drawFrame`函数中添加调用，在提交下一帧之前:

```cpp
void drawFrame() {
    ...

    updateUniformBuffer(m_currentFrame);

    m_commandBuffers[m_currentFrame].reset();
    recordCommandBuffer(m_commandBuffers[m_currentFrame], imageIndex);

    ...
}

...

void updateUniformBuffer(const uint32_t currentImage) const {

}
```

此函数将每帧生成一个新的变换，以使几何体旋转起来。我们需要包含两个新的头文件来实现此功能

```cpp
#include <chrono>

#define GLM_FORCE_RADIANS
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
```

`glm/gtc/matrix_transform.hpp` 头文件公开了可用于生成模型变换（如 `glm::rotate`）、视图变换（如 `glm::lookAt`）和投影变换（如 `glm::perspective`）的函数。
`GLM_FORCE_RADIANS` 宏是必要的，以确保像 `glm::rotate` 这样的函数使用弧度作为参数，以避免任何可能的混淆。

`chrono` 标准库头文件公开了执行精确计时的函数。
我们将使用它来确保几何体每秒旋转 90 度，而与帧速率无关。

首先使用一些逻辑来计算自渲染开始以来以浮点精度表示的时间（秒）。

```cpp
void updateUniformBuffer(const uint32_t currentImage) const {
    static auto startTime = std::chrono::high_resolution_clock::now();
    const auto currentTime = std::chrono::high_resolution_clock::now();
    const float time = std::chrono::duration<float, std::chrono::seconds::period>(currentTime - startTime).count();
}
```

我们现在将在 `uniform` 缓冲区对象中定义模型、视图和投影变换。

模型旋转将是围绕 Z 轴的简单旋转，使用 `time` 变量：

```cpp
UniformBufferObject ubo{};
ubo.model = glm::rotate(
    glm::mat4(1.0f), 
    time * glm::radians(90.0f), 
    glm::vec3(0.0f, 0.0f, 1.0f)
);
```

`glm::rotate` 函数接受现有变换、旋转角度和旋转轴作为参数，`glm::mat4(1.0f)` 构造函数返回一个单位矩阵，使用 `time * glm::radians(90.0f)` 的旋转角度实现了每秒旋转 90 度的目的。

对于视图变换，我们可以从上方以 45 度角观察几何体。
`glm::lookAt` 函数接受眼睛位置、中心位置和向上轴作为参数。

```cpp
ubo.view = glm::lookAt(
    glm::vec3(2.0f, 2.0f, 2.0f), 
    glm::vec3(0.0f, 0.0f, 0.0f), 
    glm::vec3(0.0f, 0.0f, 1.0f)
);
```

我选择使用垂直视野为 45 度的透视投影。
其他参数是纵横比、近平面和远平面。
重要的是使用当前的交换链范围来计算纵横比，以考虑调整大小后窗口的新宽度和高度。

```cpp
ubo.proj = glm::perspective(
    glm::radians(45.0f),
    static_cast<float>(m_swapChainExtent.width) / static_cast<float>(m_swapChainExtent.height),
    0.1f,
    20.0f
);
```

GLM 最初是为 OpenGL 设计的，其中裁剪坐标的 Y 坐标是反转的。
弥补这一点的最简单方法是在投影矩阵中翻转 Y 轴的缩放因子的符号。
如果你不这样做，那么图像将倒置渲染。

```cpp
ubo.proj[1][1] *= -1;
```

现在定义了所有变换，所以我们可以将 `uniform` 缓冲区对象中的数据复制到当前的 `uniform` 缓冲区。
这与顶点缓冲的做法几乎相同，只是没有暂存缓冲。
如前所述，我们只映射 `uniform` 缓冲区一次，所以我们可以直接写入它，而无需再次映射

```cpp
memcpy(m_uniformBuffersMapped[currentImage], &ubo, sizeof(ubo));
```

使用 `UBO` 而不是将频繁变化的值传递给着色器是最有效的方法。
将少量数据缓冲区传递给着色器的一种更有效的方法是推送常量。
我们会在以后的章节中介绍它。

---

现在着色器还无法访问uniform资源。
在下一章中，我们将研究描述符集，它实际将 `vk::Buffer` 绑定到 `uniform` 缓冲区描述符，以便着色器可以访问此变换数据。

---

**[C++代码](../../codes/02/20_descriptor1/main.cpp)**

**[C++代码差异](../../codes/02/20_descriptor1/main.diff)**

**[根项目CMake代码](../../codes/02/20_descriptor1/CMakeLists.txt)**

**[shader-CMake代码](../../codes/02/20_descriptor1/shaders/CMakeLists.txt)**

**[shader-vert代码](../../codes/02/20_descriptor1/shaders/graphics.vert.glsl)**

**[shader-vert代码差异](../../codes/02/20_descriptor1/shaders/graphics.vert.diff)**

**[shader-frag代码](../../codes/02/20_descriptor1/shaders/graphics.frag.glsl)**

---
