---
title: 着色器模块
comments: true
---
#  **着色器**

## **着色器模块**

### 1. 着色器介绍

在 Vulkan 中，着色器代码需要使用 SPIR-V 字节码格式，
这与传统的 [GLSL](https://en.wikipedia.org/wiki/OpenGL_Shading_Language) 
或 [HLSL](https://en.wikipedia.org/wiki/High-Level_Shading_Language) 
等高级着色语言不同。SPIR-V 是 Khronos 组织制定的一种中间表示格式，具有以下优势：

1. **跨平台兼容性**：避免了不同厂商对 GLSL 解释的差异
2. **编译器简化**：GPU 厂商只需处理标准化的字节码
3. **性能优化**：支持更精细的编译器优化

然而，这并不意味着我们需要手动编写此字节码。
我们将使用 GLSL 语法编写代码，并使用 Google 的 `glslc` 工具将代码编译成 SPIR-V 文件。

`glslc` 的优点是它使用与 GCC 和 Clang 等知名编译器相同的参数格式，并包含一些额外的功能，例如 includes 。
它们都已包含在 Vulkan SDK 中，因此您无需下载任何额外的程序。

### 2. 编码语言介绍

GLSL 是一种具有 C 风格语法的着色语言。

此语言也提供了提供了 int/float/bool 等基础类型，还有我们熟悉的 if/for/while 以及结构体和（支持重载的）函数。
重要的是，它还提供了 vec 向量和 mat 矩阵类型以及各种数学计算函数。

它编写的程序包含一个主入口函数\(可以不是`main`\)，该函数会针对每个对象进行调用。
比如顶点着色器会对每个顶点调用一遍主函数，片段着色器则是对每个片段（像素）调用一遍主函数。

与常规编程语言不同，GLSL并不依赖参数输入和返回值输出，而是通过全局变量来实现输入与输出操作。
语言本身隐含了一些变量用于输入和输出顶点颜色等数据，还可以定义一些全局变量传输自己需要的数据，你很快就会看到。


## **创建着色器代码文件**

正如上一章提到的，我们需要编写一个顶点着色器和一个片段着色器。

1. **顶点着色器 (shader.vert)**：
    - 处理每个顶点数据
    - 输出裁剪坐标和颜色

2. **片段着色器 (shader.frag)**：
    - 处理每个像素片段
    - 输出最终颜色值

现在，让我们在项目根目录创建 `shaders` 文件夹，用于存放着色器源代码，你也可以使用自己喜欢的文件夹名。

然后，在`shaders/`文件夹中创建两个文件：`graphics.vert.glsl` 和 `graphics.frag.glsl`。 
注意文件名甚至后缀都是完全任意的，但作者推荐“任意名称+着色器阶段+着色器语法”的三段式命名。

现在你的项目结构应该像这样：

```
│
├── CMakeLists.txt          # 主CMake配置文件
│
├── shaders/                # 着色器源代码目录
│   │
│   ├── graphics.vert.glsl  # 顶点着色器
│   └── graphics.frag.glsl  # 片段着色器
│
└── src/                    # 源代码目录
    │
    └── main.cpp            # 主程序入口
```

### 1. 顶点着色器

顶点着色器处理每个传入的顶点。
它以世界位置、颜色、法线和纹理坐标等属性作为输入。
输出是裁剪坐标中的最终位置以及需要传递给片段着色器的属性，例如颜色和纹理坐标。
然后，这些值将由光栅化器在片段上进行插值，以产生平滑的渐变。

**裁剪坐标** 是来自顶点着色器的四维向量，随后通过将整个向量除以其最后一个分量进行归一化。
这些归一化设备坐标是 **[齐次坐标](https://en.wikipedia.org/wiki/Homogeneous_coordinates)** ，
它们将帧缓冲映射到 [-1, 1] \* [-1, 1] 坐标系，如下所示

![device_coordinates](../../images/0121/normalized_device_coordinates.png)

如果您之前涉足过计算机图形学，那么您应该已经熟悉这些概念。
如果您以前使用过 OpenGL，那么您会注意到 Y 坐标的符号现在已翻转（Y轴朝下）。
Z 坐标现在使用的范围与 Direct3D 中的范围相同，从 0 到 1。

对于我们的第一个三角形，我们不会应用任何变换，将直接把三个顶点的位置指定为归一化设备坐标，以创建以下形状：

![triangle](../../images/0121/triangle_coordinates.svg)

通常，这些坐标将存储在顶点缓冲中，但在 Vulkan 中创建顶点缓冲并用数据填充它并非易事。
因此，我决定将其推迟到画出第一个基础三角形之后。
在此期间，我们将做一些不太寻常的事情：直接在顶点着色器代码中硬编码坐标。代码如下所示：

```glsl
#version 450
// Vulkan 中的 GLSL 语法需要指定为 450 版本

vec2 positions[3] = vec2[](
    vec2(0.0, -0.5),
    vec2(0.5, 0.5),
    vec2(-0.5, 0.5)
);

void main() {
    gl_Position = vec4(positions[gl_VertexIndex], 0.0, 1.0);
}
```

`main` 函数会为每个顶点调用。

内置的 `gl_VertexIndex` 变量包含当前顶点的索引。
这通常是顶点缓冲的索引，但在我们的例子中，它将是硬编码顶点数组的索引。
每个顶点的位置从着色器中的常量数组访问，并与虚拟的 `z` 和 `w` 分量组合以生成裁剪坐标中的位置。

内置变量 `gl_Position` 用作输出。


### 2. 片段着色器

顶点着色器计算好了每个顶点的位置，它们会组成一个个三角形。
光栅化阶段会计算出这些三角形投影到图像上的区域，区域包含片段\(像素\)，片段着色器将对每个片段调用以确定色彩信息。

一个简单的片段着色器，为整个三角形输出红色，如下所示：

```glsl
#version 450

layout(location = 0) out vec4 outColor;

void main() {
    outColor = vec4(1.0, 0.0, 0.0, 1.0);
}
```

`main` 函数为每个片段调用，就像顶点着色器的 `main` 函数为每个顶点调用一样。

GLSL 中的颜色是 4 分量向量，R、G、B 和 alpha 通道，都在 `[0, 1]` 范围内。

与顶点着色器中的 `gl_Position` 不同，没有内置变量来输出当前片段的颜色，您必须为每个帧缓冲指定自己的输出变量。

上面的 `layout(location = 0)` 修饰符指定帧缓冲的附件索引，它链接到了索引为 `0` 的附件，也就是我们的颜色附件（对应交换链图像）。

> 注意，变量名是不重要的，重要的是变量类型和 location 的值。
> 只要保证类型（数据格式）和 location 都一致，就能一个地方 out ，另一个地方 in 。
> 一个 location 只能放置一种数据。

### 3. 逐顶点颜色

将整个三角形变成红色不是很吸引人，变成下面这样的效果是不是更漂亮？

![triangle_color](../../images/0121/triangle_coordinates_colors.png)

我们必须对两个着色器进行一些更改才能实现此目的。
首先，我们需要为三个顶点指定不同的颜色。
顶点着色器现在应该包含一个颜色数组，就像它对位置所做的那样：

```glsl
vec3 colors[3] = vec3[](
    vec3(1.0, 0.0, 0.0),
    vec3(0.0, 1.0, 0.0),
    vec3(0.0, 0.0, 1.0)
);
```

现在我们只需要将这些逐顶点颜色传递给片段着色器，以便它可以将其插值输出到帧缓冲。
向顶点着色器添加颜色输出并在 `main` 函数中写入它：

```glsl
layout(location = 0) out vec3 fragColor;

void main() {
    gl_Position = vec4(positions[gl_VertexIndex], 0.0, 1.0);
    fragColor = colors[gl_VertexIndex];
}
```

接下来，我们需要在片段着色器中添加一个匹配的输入：

```glsl
layout(location = 0) in vec3 fragColor;

void main() {
    outColor = vec4(fragColor, 1.0);
}
```

`main` 函数已修改为输出颜色以及 `alpha` 值。
图形管线将使用三个顶点的数据，自动插值生成内部片段的 `fragColor` ，从而产生平滑的渐变。

### 4. 编译着色器

现在 `graphics.vert.glsl` 的内容应该是：

```glsl
#version 450

layout(location = 0) out vec3 fragColor;

vec2 positions[3] = vec2[](
    vec2(0.0, -0.5),
    vec2(0.5, 0.5),
    vec2(-0.5, 0.5)
);

vec3 colors[3] = vec3[](
    vec3(1.0, 0.0, 0.0),
    vec3(0.0, 1.0, 0.0),
    vec3(0.0, 0.0, 1.0)
);

void main() {
    gl_Position = vec4(positions[gl_VertexIndex], 0.0, 1.0);
    fragColor = colors[gl_VertexIndex];
}
```

`graphics.frag.glsl` 的内容应该是：

```glsl
#version 450

layout(location = 0) in vec3 fragColor;

layout(location = 0) out vec4 outColor;

void main() {
    outColor = vec4(fragColor, 1.0);
}
```

我们现在将使用 glslc 程序将它们编译为 SPIR-V 字节码。命令大概是这样的：

```shell
# window
xxx/VulkanSDK/x.x.x.x/Bin/glslc.exe -fshader-stage=vert graphics.vert.glsl -o graphics.vert.spv
xxx/VulkanSDK/x.x.x.x/Bin/glslc.exe -fshader-stage=frag graphics.frag.glsl -o graphics.frag.spv
# linux
/home/user/VulkanSDK/x.x.x.x/x86_64/bin/glslc -fshader-stage=vert graphics.vert.glsl -o graphics.vert.spv
/home/user/VulkanSDK/x.x.x.x/x86_64/bin/glslc -fshader-stage=frag graphics.frag.glsl -o graphics.frag.spv
```

这两个命令告诉编译器读取 GLSL 源文件，并使用 `-o`（输出）标志输出 SPIR-V 字节码文件。

如果您的着色器包含语法错误，那么编译器会告诉您行号和问题。
可以尝试省略分号并再次运行编译脚本。
还可以尝试在没有任何参数的情况下运行编译器，以查看它支持哪些类型的标志。

它还可以将字节码输出为人类可读的格式，以便您可以准确地了解您的着色器正在做什么以及在此阶段已应用的任何优化。

在命令行上编译着色器是最直接的选择之一，也是我们将在本教程中使用的选择，但也可以直接从您自己的代码中编译着色器。
Vulkan SDK 包含 libshaderc ，这是一个从您的程序中将 GLSL 代码编译为 SPIR-V 的库，我们会在进阶章节介绍。

### 5. CMake编译着色器

直接使用命令行显然不够优秀，且写明路径导致无法跨平台，所以我们借助 CMake 执行命令。

> 注意此部分是可选的，你完全可以在每次修改后手动编译。

现在让我们在 `shaders/` 文件夹中创建新的 `CMakeLists.txt`，内容如下所示：

```cmake
cmake_minimum_required(VERSION 4.0.0)

find_package(Vulkan REQUIRED)

set(SHADER_DIR ${CMAKE_CURRENT_SOURCE_DIR})
set(STAGE_VERT -fshader-stage=vert)
set(STAGE_FRAG -fshader-stage=frag)
set(GRAPHICS_VERT_SHADER ${SHADER_DIR}/graphics.vert.glsl)
set(GRAPHICS_FRAG_SHADER ${SHADER_DIR}/graphics.frag.glsl)
set(GRAPHICS_SPIRV_VERT ${SHADER_DIR}/graphics.vert.spv)
set(GRAPHICS_SPIRV_FRAG ${SHADER_DIR}/graphics.frag.spv)

add_custom_command(
     OUTPUT ${GRAPHICS_SPIRV_VERT}
     COMMAND ${Vulkan_GLSLC_EXECUTABLE} ${STAGE_VERT} ${GRAPHICS_VERT_SHADER} -o ${GRAPHICS_SPIRV_VERT}
     COMMENT "Compiling graphics.vert.glsl to graphics.vert.spv"
     DEPENDS ${GRAPHICS_VERT_SHADER}
)

add_custom_command(
     OUTPUT ${GRAPHICS_SPIRV_FRAG}
     COMMAND ${Vulkan_GLSLC_EXECUTABLE} ${STAGE_FRAG} ${GRAPHICS_FRAG_SHADER} -o ${GRAPHICS_SPIRV_FRAG}
     COMMENT "Compiling graphics.frag.glsl to graphics.frag.spv"
     DEPENDS ${GRAPHICS_FRAG_SHADER}
)

add_custom_target(CompileShaders ALL
    DEPENDS ${GRAPHICS_SPIRV_VERT} ${GRAPHICS_SPIRV_FRAG}
)
```

我们通过 `find_package(Vulkan REQUIRED)` 命令找到包后，会提供一系列的变量。
而 `Vulkan_GLSLC_EXECUTABLE` 变量指定了 `glslc` 的路径。

我们通过设置变量，避免了在命令中硬编码文件名。然后通过添加命令和目标，使我们可以通过统一命令执行着色器的编译。

现在还需要在项目根目录的 `CMakeLists.txt` 包含子目录，才能自动加入这一目标。
请在项目根目录的 `CMakeLists.txt` 末尾中添加一条语句。

```cmake
add_subdirectory(shaders)
```

现在配置与构建项目，`shaders/` 下应该生成了 `graphics.frag.spv` 和 `graphics.vert.spv` 两个文件。

## **加载着色器**

### 1. 读取文件

现在我们有了一种生成 SPIR-V 着色器的方法，是时候将它们加载到我们的程序中，以便在某个时候将它们插入到图形管线中。
我们首先编写一个简单的辅助函数，从文件中加载二进制数据。

```cpp
#include <fstream>

// ...

static std::vector<char> readFile(const std::string& filename) {
    std::ifstream file(filename, std::ios::ate | std::ios::binary);

    if (!file.is_open()) {
        throw std::runtime_error("failed to open file!");
    }
}
```

`readFile` 函数将从指定文件中读取所有字节，并将它们作为由 `std::vector` 管理的数组返回。我们首先使用两个标志打开文件

- `ate`：从文件末尾开始读取
- `binary`：将文件作为二进制文件读取（避免文本转换）

从文件末尾开始读取的优点是我们可以读取位置从而确定文件的大小并分配缓冲区。

```cpp
const size_t fileSize = file.tellg();
std::vector<char> buffer(fileSize);
```

之后，可以 `seekg` 回到文件开头并一次读取所有字节。

```cpp
file.seekg(0);
file.read(buffer.data(), fileSize);
```

最后关闭文件并返回字节：

```cpp
file.close(); // optional

return buffer;
```

在 `createGraphicsPipeline` 函数中使用它，以加载两个着色器的字节码：

```cpp
 void createGraphicsPipeline() {
     const auto vertShaderCode = readFile("shaders/graphics.vert.spv");
     const auto fragShaderCode = readFile("shaders/graphics.frag.spv");
     std::cout << "vertShaderCode.size(): " << vertShaderCode.size() << std::endl;
     std::cout << "fragShaderCode.size(): " << fragShaderCode.size() << std::endl;
 }
```

我们通过打印缓冲区的大小并检查它们是否与字节的实际文件大小匹配，确保着色器已正确加载。

> 注意我们使用了相对路径，这要求你运行可执行程序时，当前路径必须位于项目根目录。  
> 你可以将着色器文件夹复制一份到你的执行目录。但推荐的做法是修改代码编辑器的启动项配置，设置目标文件运行目录为项目根目录。

### 2. 创建着色器模块

在将着色器代码传递给管线之前，必须将其包装在 `vk::ShaderModule` 对象中。
让我们创建一个辅助函数 `createShaderModule` 来执行此操作：

```cpp
vk::raii::ShaderModule createShaderModule(const std::vector<char>& code) const {

}
```

该函数将采用带有字节码的缓冲区作为参数，并从中创建 `vk::raii::ShaderModule` 。

创建着色器模块很简单，我们只需要指定字节码缓冲区的开始指针和缓冲区长度。
此信息在 `vk::ShaderModuleCreateInfo` 结构中指定。

需要注意的一点是，字节码的大小以字节为单位指定，但字节码指针是 `uint32_t` 指针，而不是 `char` 指针。
因此，我们需要使用 `reinterpret_cast` 强制转换指针，如下所示：

```cpp
vk::ShaderModuleCreateInfo createInfo;
createInfo.codeSize = code.size();
createInfo.pCode = reinterpret_cast<const uint32_t*>(code.data());
```

当您执行这样的强制转换时，还需要确保数据满足 `uint32_t` 的对齐要求。
幸运的是，数据存储在 `std::vector` 中，其中默认分配器已经确保数据满足最坏情况的对齐要求。

> 你无法使用 `setCode()` ，它只接受 `uint32_t` 类型的代理数组。

然后我们创建 `vk::raii::ShaderModule` 并返回即可。

```cpp
return m_device.createShaderModule(createInfo);
```

着色器模块只是我们着色器字节码的薄包装，
从 SPIR-V 字节码到 GPU 机器码的编译链接过程在图形管线创建才发生，
这意味着我们可以在管线创建完成后可以立即销毁着色器模块，
所以我们将它们作为函数中的局部变量而不是类成员：

```cpp
void createGraphicsPipeline() {
    const auto vertShaderCode = readFile("shaders/graphics.vert.spv");
    const auto fragShaderCode = readFile("shaders/graphics.frag.spv");
    
    vk::raii::ShaderModule vertShaderModule = createShaderModule(vertShaderCode);
    vk::raii::ShaderModule fragShaderModule = createShaderModule(fragShaderCode);
}
```

### 3. 创建着色器阶段

要实际使用着色器，我们需要通过 `vk::PipelineShaderStageCreateInfo` 结构将它们分配给特定的管线阶段，作为实际管线创建过程的一部分。

我们将从填充顶点着色器的结构开始，同样在 `createGraphicsPipeline` 函数中指定：

```cpp
vk::PipelineShaderStageCreateInfo vertShaderStageInfo;
vertShaderStageInfo.stage = vk::ShaderStageFlagBits::eVertex;
vertShaderStageInfo.module = vertShaderModule;
vertShaderStageInfo.pName = "main";
```

`stage` 字段指定了着色器在哪个阶段工作。

接下来的两个字段指定着色器的代码模块以及要调用的函数，称为入口点。
这意味着可以将多个片段着色器组合到一个着色器模块中，并使用不同的入口点来区分它们的行为。
我们使用标准的 `main` 入口。

还有一个可选成员 `pSpecializationInfo`，它用于设置特化常量，我们会在进阶章节介绍。

片段着色器和上面的代码差不多，记得修改 `stage` 字段。

```cpp
vk::PipelineShaderStageCreateInfo fragShaderStageInfo;
fragShaderStageInfo.stage = vk::ShaderStageFlagBits::eFragment;
fragShaderStageInfo.module = fragShaderModule;
fragShaderStageInfo.pName = "main";
```

最后，定义一个包含这两个结构的数组，我们稍后将在实际的管线创建步骤中使用它。

```cpp
const auto shaderStages = { vertShaderStageInfo, fragShaderStageInfo };
```

## **测试**

这就是描述管线的可编程阶段的全部内容，你可以尝试运行程序，不应报错。

本章我们学习了两个可编程阶段。在下一章中，我们将研究图形管线的固定功能阶段。

---

**[C++代码](../../codes/01/21_shader/main.cpp)**

**[C++代码差异](../../codes/01/21_shader/main.diff)**

**[根项目CMake代码](../../codes/01/21_shader/CMakeLists.txt)**

**[根项目CMake代码差异](../../codes/01/21_shader/CMakeLists.diff)**

**[shader-CMake代码](../../codes/01/21_shader/shaders/CMakeLists.txt)**

**[shader-vert代码](../../codes/01/21_shader/shaders/graphics.vert.glsl)**

**[shader-frag代码](../../codes/01/21_shader/shaders/graphics.frag.glsl)**

---
