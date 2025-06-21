# **顶点输入描述**

## **前言**

在后面几章，我们将使用内存中的**顶点缓冲区\(vertex buffers\)**数据代替Shader中的硬编码数据。

图形管线的第一个阶段是输入装配，如果你回顾“图形管线-固定功能”章节，会看到一个“**顶点输入**”模块。
我们要做的是将顶点数据放入顶点缓冲区，然后将此缓冲区绑定到管线的顶点输入。

我们将从最简单的方式开始，创建主机\(CPU\)可见缓冲区然后直接用 `memcpy` 将顶点数据复制进去。
之后我们将了解如何使用暂存缓冲区\(staging buffers\)将顶点数据复制进高性能显存中。

## **顶点着色器**

首先我们需要改变顶点着色器的代码，不再包含硬编码的顶点数据。
顶点数据将从外部获取，通过`in`关键字：

```glsl
#version 450

layout(location = 0) in vec2 inPosition;
layout(location = 1) in vec3 inColor;

layout(location = 0) out vec3 fragColor;

void main() {
    gl_Position = vec4(inPosition, 0.0, 1.0);
    fragColor = inColor;
}
```

`inPosition` 和 `inColor` 是顶点参数，这些数据将从管线的顶点输入获取。

我们之前提过，一个`location`只能放一个资源，所以我们的位置和颜色信息需要放在不同的`location`中。
注意 `inPosition` 是“顶点输入->顶点着色器”，`fragColor`是“顶点着色器->片段着色器”，所以二者不冲突。

特殊的是，一个槽位最多 16 字节，因此某些类型需要多个槽位。比如使用`dvec3`时，后一个变量的索引至少要高 2 ：

```glsl
layout(location = 0) in dvec3 inPosition;
layout(location = 2) in vec3 inColor;
```

您可以在 [OpenGL wiki](https://www.khronos.org/opengl/wiki/Layout_Qualifier_(GLSL)) 中找到有关 layout 限定符的更多信息。

## **顶点数据**

现在回到C++代码，我们要将顶点数据从着色器代码移动到C++程序代码的数组中。

首先导入 GLM 库头文件，此库包含线性代数所需的工具，比如向量和矩阵。
我们将使用这些类型来指定位置和颜色向量。

```cpp
#include <glm/glm.hpp>
```

现在创建一个新的结构体`Vertex`用于存放顶点数据

```cpp
struct Vertex {
    glm::vec2 pos;
    glm::vec3 color;
};
```

GLM 提供的 C++ 类型的内存布局与着色器中的类型完全匹配，这让我们无需调整位域信息。

现在使用 `Vertex` 结构来指定顶点数据数组。

```cpp
inline static const std::vector<Vertex> vertices = {
    {{0.0f, -0.5f}, {1.0f, 0.0f, 0.0f}},
    {{0.5f, 0.5f}, {0.0f, 1.0f, 0.0f}},
    {{-0.5f, 0.5f}, {0.0f, 0.0f, 1.0f}}
};
```

我们使用的位置和颜色值与之前完全相同，但现在它们组合成一个顶点数组，这被称为“交错顶点属性”。

下一步要告诉 Vulkan 这些数据绑定到顶点输入后如何传递给顶点着色器。
我们需要两个结构体传达这些信息。

## **绑定描述**

第一个结构体是 `vk::VertexInputBindingDescription` ，添加一个静态成员函数用于填充信息：

```cpp
struct Vertex {
    glm::vec2 pos;
    glm::vec3 color;

    static vk::VertexInputBindingDescription getBindingDescription() {
        vk::VertexInputBindingDescription bindingDescription;

        return bindingDescription;
    }
};
```

顶点输入可以一次性绑定多个缓冲区，用一个数组表示，使用 `binding` 字段指定缓冲区索引。
我们目前只有一个缓冲区，所以这个缓冲区将在数组的 0 号位：

```cpp
bindingDescription.binding = 0;
```

> 具体的缓冲区数组将在下一节创建。

“绑定描述”结构体主要描述了顶点着色器从输入的缓冲区中加载数据的“速率”。
它规定了单个数据条目的字节数，以及要在每个顶点还是每个实例时切换一条数据：

```cpp
bindingDescription.stride = sizeof(Vertex);
bindingDescription.inputRate = vk::VertexInputRate::eVertex;
```

`stride`参数则指定一个条目的字节数，`inputRate` 参数具有以下两种枚举值：

| `vk::VertexInputRate` | 意义 |  
|------|------|
| `eVertex` | 在处理每个顶点时读取一条 |
| `eInstance` | 在处理每个实例时读取一条 |

实例化渲染会在后续章节介绍，我们的缓冲区存放顶点数据，所以使用 `eVertex` 。

## **属性描述**

第二个结构体是 `vk::VertexInputAttributeDescription` ，它描述了如何处理顶点的输入数据。

我们在顶点着色器中使用了两个 `location` ，分别接收两种不同类型的数据。
而“属性描述”结构体做的就是这件事，将 C++ 类中的数据与顶点着色器中的数据一一对应，每块数据的大小以及对应哪个 `location` 。


我们依然添加一个静态成员函数：

```cpp
// ......
static std::array<vk::VertexInputAttributeDescription, 2>  getAttributeDescriptions() {
    std::array<vk::VertexInputAttributeDescription, 2> attributeDescriptions;

    return attributeDescriptions;
}
```

一般而言，一个“属性描述”对应着色器中的一个 `location` ，所以我们需要两个属性描述。

第一个字段是 `binding` ，它的作用和“绑定描述”中的完全一致，你应该指定相同的值：

```cpp
attributeDescriptions[0].binding = 0;
```

> 顶点输入需要三个数组，缓冲区数组+绑定描述数组+参数描述数组，因此每个绑定描述和参数描述都需要指定对应的缓冲区索引。

然后设置实际的属性信息：

```cpp
attributeDescriptions[0].location = 0;
attributeDescriptions[0].format = vk::Format::eR32G32Sfloat;
attributeDescriptions[0].offset = offsetof(Vertex, pos);
```

- `location` 参数对应着色器中的 `layout(location = ...)` 。

- `format` 参数表示了数据的格式，特殊的是需要使用颜色格式的枚举值。下面给出常见shader格式与颜色枚举的对应关系，你应该能够理解对应关系：

| Shader类型 | 颜色格式枚举 | 说明 |
|------------|-------------|------|
| `float` | `vk::Format::eR32Sfloat` | 32位浮点数，刚好一个R32。 |
| `double` | `vk::Format::eR64Sfloat` | 64位浮点数，刚好一个R64。 |
| `vec2` | `vk::Format::eR32G32Sfloat` | 两个32位浮点，对应RG双通道。 |
| `vec3` | `vk::Format::eR32G32B32Sfloat` | 三个对应三通道。 |
| `vec4` | `vk::Format::eR32G32B32A32Sfloat` | 四个对应四通道。 |
| `ivec2` | `vk::Format::eR32G32Sint` | S表示有符号，int表示类型。 |
| `uvec4` | `vk::Format::eR32G32B32A32Uint` | U表示无符号数。 |

- `offset` 参数指定了当前数据段的开始位置对应的偏移量，我们使用C/C++的标准宏`offsetof`获取偏移量信息。

色彩属性用类似的方式描述：

```cpp
attributeDescriptions[1].binding = 0;
attributeDescriptions[1].location = 1;
attributeDescriptions[1].format = vk::Format::eR32G32B32Sfloat;
attributeDescriptions[1].offset = offsetof(Vertex, color);
```

> 注意这里的 `format` 对应的是 `vec3` 。

## **管线顶点输入**

我们现在需要设置图形管线的配置，让它接收这些数据。现在修改 `createGraphicsPipeline` 函数，找到 `vertexInputInfo` 变量并添加信息：

```cpp
vk::PipelineVertexInputStateCreateInfo vertexInputInfo;

auto bindingDescription = Vertex::getBindingDescription();
auto attributeDescriptions = Vertex::getAttributeDescriptions();
// 使用setter自动代理数组，同时填充开始指针和数量两个成员变量
vertexInputInfo.setVertexBindingDescriptions(bindingDescription);
vertexInputInfo.setVertexAttributeDescriptions(attributeDescriptions);
```

就像上面说的，我们需要三个数据，缓冲区数组+绑定描述数组+属性描述数组。
我们还没有创建缓冲区，所以现在只设置了后两者。

## **最后**

现在管线已准备好接受指定格式的顶点数据并将其传递给顶点着色器。

但是如果你启用验证层并运行，会看到它提示没有绑定顶点缓冲。下一节我们将创建顶点缓冲并将数据移入，保证GPU可以正常访问它。

> 如果你忘记重新编译着色器，可能没有报错。

---

**[C++代码](../../codes/02/00_vertexinput/main.cpp)**

**[C++代码差异](../../codes/02/00_vertexinput/main.diff)**

**[根项目CMake代码](../../codes/02/00_vertexinput/CMakeLists.txt)**

**[shader-CMake代码](../../codes/02/00_vertexinput/shaders/CMakeLists.txt)**

**[shader-vert代码](../../codes/02/00_vertexinput/shaders/shader.vert)**

**[shader-vert代码差异](../../codes/02/00_vertexinput/shaders/vert.diff)**

**[shader-frag代码](../../codes/02/00_vertexinput/shaders/shader.frag)**
