# **图形管线-总结**

现在我们可以结合之前章节中的所有结构和对象来创建图形管线了！以下是我们现在拥有的对象类型，作为一个快速回顾

- 着色器阶段：定义图形管线可编程阶段功能的着色器模块
- 固定功能状态：定义管线固定功能阶段的结构，例如输入装配、光栅化器、视口和颜色混合
- 管线布局：uniform 值和 push 值被着色器引用，可以在绘制时更新
- 渲染通道：管线阶段引用的附件及其用途

## **添加创建信息**

这些内容结合在一起完全定义了图形管线的功能，
所以我们现在可以开始填写 `vk::GraphicsPipelineCreateInfo` 结构体，
在 `createGraphicsPipeline` 函数的末尾：

```cpp
vk::GraphicsPipelineCreateInfo pipelineInfo;
pipelineInfo.setStages( shaderStages );
```

我们首先填写了 `vk::PipelineShaderStageCreateInfo` 结构体数组

然后我们引用所有描述固定功能阶段的结构体。

```cpp
pipelineInfo.pVertexInputState = &vertexInputInfo;
pipelineInfo.pInputAssemblyState = &inputAssembly;
pipelineInfo.pViewportState = &viewportState;
pipelineInfo.pRasterizationState = &rasterizer;
pipelineInfo.pMultisampleState = &multisampling;
pipelineInfo.pDepthStencilState = nullptr; // Optional
pipelineInfo.pColorBlendState = &colorBlending;
pipelineInfo.pDynamicState = &dynamicState;
```

之后是管线布局，它是一个 Vulkan 句柄，而不是结构体指针。

```cpp
pipelineInfo.layout = m_pipelineLayout;
```

最后我们有对渲染通道的引用，以及此图形管线将要使用的子通道的**索引**。
也可以将此管线与其他的渲染通道一起使用，而不是这个特定的实例，但它们必须与 renderPass兼容。
兼容性的要求在 [这里](https://www.khronos.org/registry/vulkan/specs/1.3-extensions/html/chap8.html#renderpass-compatibility) 描述，
但我们不会在本教程中使用该功能。

```cpp
pipelineInfo.renderPass = m_renderPass;
pipelineInfo.subpass = 0;
```

实际上还有两个参数：`basePipelineHandle` 和 `basePipelineIndex`。

Vulkan 允许您通过派生自现有管线来创建新的图形管线。
管线派生的想法是，当它们与现有管线具有许多共同的功能时，设置管线的成本较低，并且在来自同一父级的管线之间切换也可以更快地完成。
您可以使用 `basePipelineHandle` 指定现有管线的句柄，或者使用 `basePipelineIndex` 引用另一个即将通过索引创建的管线。

现在只有一个管线，所以我们将简单地指定一个空句柄和一个无效索引。
只有当 `vk::PipelineCreateFlagBits::eDerivative` 标志也在 `vk::GraphicsPipelineCreateInfo` 的 flags 字段中指定时，这些值才会被使用。

```cpp
pipelineInfo.basePipelineHandle = nullptr; // Optional
pipelineInfo.basePipelineIndex = -1; // Optional
```

## **创建图形管线**

现在准备好最后一步，创建一个类成员来保存 `vk::raii::Pipeline` 对象

```cpp
vk::raii::Pipeline m_graphicsPipeline{ nullptr };
```

最后创建图形管线

```cpp
m_graphicsPipeline = m_device.createGraphicsPipeline( nullptr, pipelineInfo );
```

`createGraphicsPipeline` 用于创建单个对象，`createGraphicsPipelines`则可以一次性创建多个。

> 注意到我们多传入了一个 `nullptr`，他等于宏 `VK_NULL_HANDLE`。 
> 此位置的参数简写是 `optional<PipelineCache>` 管线缓存。  
> 我们将在管线缓存章节中深入探讨这一点。

## **测试**

现在运行你的程序，确认我们已经可以成功创建管线！

---

我们已经非常接近在屏幕上看到一些东西了。
在接下来的几章中，我们将从交换链图像中设置实际的帧缓冲，并准备绘制命令。

---

**[C++代码](../../codes/01/24_conclusion/main.cpp)**

**[C++代码差异](../../codes/01/24_conclusion/main.diff)**

**[根项目CMake代码](../../codes/01/21_shader/CMakeLists.txt)**

**[shader-CMake代码](../../codes/01/21_shader/shaders/CMakeLists.txt)**

**[shader-vert代码](../../codes/01/21_shader/shaders/shader.vert)**

**[shader-frag代码](../../codes/01/21_shader/shaders/shader.frag)**
