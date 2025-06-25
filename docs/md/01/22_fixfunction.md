---
title: 固定功能阶段配置
comments: true
---
# **固定功能阶段**

旧的图形 API 为图形管线的大部分阶段提供了默认状态。
但在 Vulkan 中，您必须显式地指定大多数管线状态，因为它们将被烘焙到不可变的管线状态对象中。

在本章中，我们将填写下列所有结构以配置这些固定功能操作。

- 动态状态
- 顶点输入
- 输入装配
- 视口和裁剪矩形
- 光栅化器
- 多重采样
- 深度和模板测试
- 颜色混合
- 管线布局

> 代码都添加在 `createGraphicsPipeline` 函数中

## **动态状态**

虽然 Vulkan 大多数管线状态需要烘焙到管线状态中，
但部分状态允许在绘制时动态修改，提供更高的灵活性。常见动态状态包括：

- 视口尺寸（Viewport）
- 裁剪区域（Scissor）
- 线宽和混合因子

如果您想使用动态状态并保留某些属性，那么需要填写一个 `PipelineDynamicStateCreateInfo` 结构，如下所示：

```cpp
// 启用视口和裁剪矩形动态状态
std::vector<vk::DynamicState> dynamicStates = {
    vk::DynamicState::eViewport,
    vk::DynamicState::eScissor
};

vk::PipelineDynamicStateCreateInfo dynamicState;
dynamicState.setDynamicStates( dynamicStates );
```

这将导致管线创建时暂时忽略这些值的配置，并使您能够（需要）在绘制时指定数据，这样的设置更加灵活且常见。

## **顶点输入**

顶点输入创建信息描述了传递给顶点着色器的顶点数据的格式。它大致通过两种方式描述这一点

- **绑定描述**：数据在内存中的布局（间隔、逐顶点/实例）
- **属性描述**：顶点属性类型、绑定源和偏移量

我们将顶点数据直接硬编码到了顶点着色器中，暂时不需要任何设置：

```cpp
vk::PipelineVertexInputStateCreateInfo vertexInputInfo;
```

> 具体配置将在后续“顶点缓冲”章节详细说明。

## **输入装配**

输入装配创建信息描述了两件事：从顶点绘制哪种类型的几何图形，以及是否应启用图元重启。

前者在 `topology` 成员中指定，常见的值有这些


|   `vk::PrimitiveTopology` |                 含义                          |  
|-----------------------|--------------------------------------------------|
| `ePointList`          | 点集，点来自顶点                                   |  
| `eLineList`           | 每 2 个顶点绘制一条线，不重复使用顶点               |  
| `eLineStrip`          | 每条线的结束顶点用作下一条线的起始顶点               |  
| `eTriangleList`       | 每 3 个顶点绘制一个三角形，不重复使用顶点            |  
| `eTriangleStrip`      | 每个三角形的第二/三个顶点用作下一个三角形的前两个顶点 |  


通常，顶点按索引顺序从顶点缓冲中加载，但使用索引缓冲区，您可以自己指定要使用的索引。
这允许您执行诸如重用顶点之类的优化，这将在后续的“索引缓冲”章节介绍。

对于后者，如果您将 `primitiveRestartEnable` 成员设置为 `true`，则可以通过使用特殊的索引 `0xFFFF` 或 `0xFFFFFFFF` 在 `Strip` 拓扑模式中打断线和三角形。

我们打算在本教程中绘制三角形，因此使用以下结构：

```cpp
vk::PipelineInputAssemblyStateCreateInfo inputAssembly;
inputAssembly.topology = vk::PrimitiveTopology::eTriangleList;
inputAssembly.primitiveRestartEnable = false; // default
```

## **视口和裁剪矩形**

### 1. 静态状态时

视口基本上描述了将渲染输出的帧缓冲区域。这几乎总是 `(0, 0)` 到 `(width, height)` ，在本教程中也将是这种情况。

```cpp
vk::Viewport viewport(
    0.0f, 0.0f,                                     // x y
    static_cast<float>(m_swapChainExtent.width),    // width
    static_cast<float>(m_swapChainExtent.height),   // height
    0.0f, 1.0f                                      // minDepth maxDepth
);
```

请记住，交换链及其图像的大小可能与窗口的 `WIDTH` 和 `HEIGHT` 不同。
交换链图像稍后将用作帧缓冲，因此我们应该坚持使用它们的大小。

`minDepth` 和 `maxDepth` 值指定用于帧缓冲的深度值范围。
这些值必须在 `[0.0f, 1.0f]` 范围内，但 `minDepth` 可能高于 `maxDepth` 。
如果您没有做任何特殊的事情，那么您应该坚持使用 `0.0f` 和 `1.0f` 的标准值。

虽然视口定义了从图像到帧缓冲的转换，但裁剪矩形定义了实际存储像素的区域。
光栅化器将丢弃裁剪矩形之外的任何像素。它们的功能类似于过滤器，而不是转换，下图说明了差异：

![scissor](../../images/0122/viewports_scissors.png)

> 左图将图像 Y 轴进行了缩放\(保留内容\)，而右图直接删掉了一半内容。

因此，如果我们想绘制到整个帧缓冲，可以指定一个覆盖它的裁剪矩形：

```cpp
vk::Rect2D scissor(
    {0, 0},             // offset
    m_swapChainExtent   // Extent2D
);
```

需要使用 `vk::PipelineViewportStateCreateInfo` 结构在管线中设置视口和裁剪矩形。

```cpp
// 第一个参数是开始序号
vk::PipelineViewportStateCreateInfo viewportState;
viewportState.setViewports( 0, viewport );
viewportState.setScissors( 0, scissor );
```

静态状态使得此管线的视口和裁剪矩形不可变，想对这些值进行任何更改都需要创建新管线。

### 2. 动态状态时

我们启用了动态状态，可以在绘制时指定视口和裁剪矩形，而现在只需指定它们的计数，**此时无需创建 `scissor` 和 `viewport`** ：

```cpp
// 不需要创建scissor和viewport
vk::PipelineViewportStateCreateInfo viewportState;
viewportState.viewportCount = 1;
viewportState.scissorCount = 1;
```

当然，即使启用了动态状态，也可以像静态状态一样设置，但它们的内容会被忽略，实际只有数量字段有效：

```cpp
// 需要创建了scissor和viewport，但结构体内容被忽略
vk::PipelineViewportStateCreateInfo viewportState;
viewportState.setViewports( 0, viewport );
viewportState.setScissors( 0, scissor );
```

使用动态状态，甚至可以在单个命令缓冲区中指定不同的视口或裁剪矩形。
无论您如何设置它们，都可以在某些显卡上使用多个视口和裁剪矩形，通过结构体成员引用它们的数组。

## **光栅化器**

光栅化器获取顶点着色器中的顶点形成的几何图形，称之为“**图元**”，并将其拆分为片段，以便由片段着色器着色。
它还执行“深度测试、面剔除和裁剪测试”等内容，且可以配置输出的片段是包含整个多边形还是仅边缘线条。
这些都使用 `vk::PipelineRasterizationStateCreateInfo` 结构进行配置。

需要的参数较多，所以我们逐个设置并解释：

```cpp
vk::PipelineRasterizationStateCreateInfo rasterizer;
rasterizer.depthClampEnable = false;
```

如果 `depthClampEnable` 设置为 `true`，则超出近平面和远平面的片段将被视作边界位置的片段(clamp)，而不是丢弃它们。
这在某些特殊情况下很有用，例如阴影贴图。

```cpp
rasterizer.rasterizerDiscardEnable = false;
```

如果 `rasterizerDiscardEnable` 设置为 `true`，则几何图形永远不会通过光栅化器阶段。这基本上禁用了对帧缓冲的任何输出。

```cpp
rasterizer.polygonMode = vk::PolygonMode::eFill;
```

`polygonMode` 确定如何将几何图元转换为片段，至少有三种常见模式可用：

|   `vk::PolygonMode`   |   功能   |    说明   |
|------------------------|----------------|--------------------| 
| `eFill`  | 生成覆盖多边形内部区域的片段 |  默认，片段覆盖所有内容 |
| `eLine`  | 仅多边形的边被渲染为线段 |  线的宽度由 `vk::LineWidth` 字段控制 |
| `ePoint` | 仅多边形的顶点被渲染为点片段  |  点的大小可由着色器的 `gl_PointSize` 控制 |

```cpp
rasterizer.lineWidth = 1.0f;
```

`lineWidth` 成员很简单，它以片段数为单位描述线条绘制的粗细，默认使用 1.0f ，支持的最大线宽取决于硬件。

```cpp
rasterizer.cullMode = vk::CullModeFlagBits::eBack;
rasterizer.frontFace = vk::FrontFace::eClockwise;
```

- `cullMode` 变量确定要使用的面剔除类型，剔除指不显示，您可以禁用剔除、剔除正面、剔除背面或两者都剔除。
- `frontFace` 变量指定要被视为正面的顶点绘制顺序，可以是顺时针或逆时针。

```cpp
rasterizer.depthBiasEnable = false;
```

光栅化器可以通过添加常量值或根据片段的斜率对其进行偏置来更改深度值。
这有时用于阴影贴图，但我们不会使用它，所以将 `depthBiasEnable` 设置为 `false`。

## **多重采样**

使用 `vk::PipelineMultisampleStateCreateInfo` 结构配置多重采样，这是 **抗锯齿/防走样** 的方法之一，需要启用 GPU 功能。

它的工作原理非常简单，将单个像素拆分成多个区域，判断各小区域是否在图元的有效范围内\(在范围内就正常色彩，不在就无色\)，最后取平均值以保证几何体边缘色彩平滑过渡。

因为它不需要多次运行片段着色器（如果只有一个多边形映射到一个像素），所以它比简单地渲染到更高的分辨率然后缩小分辨率要高效得多。

```cpp
vk::PipelineMultisampleStateCreateInfo multisampling;
multisampling.rasterizationSamples =  vk::SampleCountFlagBits::e1;
multisampling.sampleShadingEnable = false;
```

> 我们将在后面的“多重采样”章节中重新讨论它，现在先保持最简单的单采样状态。

## **深度和模板测试**

如果您正在使用深度或模板缓冲，那么您需要使用 `vk::PipelineDepthStencilStateCreateInfo` 配置深度和模板测试。
我们现在没有，所以后面会简单地传递一个 `nullptr` 代替它。

> 我们将在后面的“深度缓冲”章节重新讨论它。

## **颜色混合**

在片段着色器返回颜色后，需要将其与帧缓冲区中已有的颜色组合。这种转换称为颜色混合，有两种方法可以做到这一点

- 混合旧值和新值以产生最终颜色
- 使用按位运算组合旧值和新值

有两种类型的结构体可以配置颜色混合。第一个 `vk::PipelineColorBlendAttachmentState` 包含每个附加帧缓冲的配置，
第二个 `vk::PipelineColorBlendStateCreateInfo` 包含全局颜色混合设置。
在我们的例子中，只有一个帧缓冲，且无需混合。

```cpp
vk::PipelineColorBlendAttachmentState colorBlendAttachment;
colorBlendAttachment.blendEnable = false; // default
colorBlendAttachment.colorWriteMask = (
    vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG | 
    vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA
);
```

> 第二个参数手动指定了全部的位掩码，实际上Vulkan-hpp提供了一个`vk::FlagTraits`模板用于定义“全量”掩码，可以这样使用：  
> `colorWriteMask = vk::FlagTraits<vk::ColorComponentFlagBits>::allFlags;`


上述结构允许您配置第一种颜色混合方式，具体效果最好使用以下伪代码进行演示：

```cpp
if (blendEnable) {
    finalColor.rgb = (srcColorBlendFactor * newColor.rgb) <colorBlendOp> (dstColorBlendFactor * oldColor.rgb);
    finalColor.a = (srcAlphaBlendFactor * newColor.a) <alphaBlendOp> (dstAlphaBlendFactor * oldColor.a);
} else {
    finalColor = newColor;
}

finalColor = finalColor & colorWriteMask;
```

如果 `blendEnable` 设置为 `false`，则来自片段着色器的新颜色将未经修改地传递。
否则，将执行两个混合操作以计算新颜色。最后将结果颜色与 `colorWriteMask` 进行 AND 运算，以确定实际传递哪些通道。

使用颜色混合最常见的方式是实现 alpha 混合，如果希望根据新颜色的不透明度将其与旧颜色混合。
然后应按如下方式计算 `finalColor`

```
finalColor.rgb = newAlpha * newColor + (1 - newAlpha) * oldColor;
finalColor.a = newAlpha.a;
```

这可以通过以下参数来实现

```cpp
colorBlendAttachment.blendEnable = true;
colorBlendAttachment.srcColorBlendFactor = vk::BlendFactor::eSrcAlpha;
colorBlendAttachment.dstColorBlendFactor = vk::BlendFactor::eOneMinusConstantAlpha;
colorBlendAttachment.colorBlendOp = vk::BlendOp::eAdd;
colorBlendAttachment.srcAlphaBlendFactor = vk::BlendFactor::eOne;
colorBlendAttachment.dstAlphaBlendFactor = vk::BlendFactor::eZero;
colorBlendAttachment.alphaBlendOp = vk::BlendOp::eAdd;
```

您可以在规范的 `vk::BlendFactor` 和 `vk::BlendOp` 枚举中找到所有可能的操作。

> 注意本教程使用的是第一种简单的无混合方式。

第二个结构体引用所有帧缓冲的结构体数组，并允许您设置混合常量，您可以在上述计算中将其用作混合因子。

```cpp
vk::PipelineColorBlendStateCreateInfo colorBlending;
colorBlending.logicOpEnable = false;
colorBlending.logicOp = vk::LogicOp::eCopy;
colorBlending.setAttachments( colorBlendAttachment );
```

如果您想使用第二种混合方法（按位组合），则应将 `logicOpEnable` 设置为 `true`。
然后可以在 `logicOp` 字段中指定按位运算。
请注意，这将自动禁用第一种方法，就好像您已为每个附加的帧缓冲的 `blendEnable` 设置为 `false` 一样！
`colorWriteMask` 字段在此模式依然生效，以确定帧缓冲中的哪些通道将实际受到影响。

也可以禁用两种模式，就像这里做的一样。在此情况下，片段颜色将未经修改地写入帧缓冲。

## **创建管线布局**

您可以在着色器中使用 `uniform` 值，这些值是类似于动态状态变量的全局变量，可以在绘制时更改，以更改着色器的行为，而无需重新创建它们。
它们通常用于将变换矩阵传递给顶点着色器，或在片段着色器中创建纹理采样器。

> 我们会在后续的“uniform缓冲”章节详细介绍它。

这些 `uniform` 值需要在管线创建期间通过创建 `vk::PipelineLayout` 对象来指定。
即使我们在未来的章节中才会使用它们，现在仍然需要创建一个空的管线布局。

创建一个类成员来保存此对象，因为我们将在稍后的章节从其他函数中引用它：

```c++
vk::raii::PipelineLayout m_pipelineLayout{ nullptr };
```

然后在 `createGraphicsPipeline` 函数中直接创建对象

```cpp
vk::PipelineLayoutCreateInfo pipelineLayoutInfo;
m_pipelineLayout = m_device.createPipelineLayout( pipelineLayoutInfo );
```

该结构还可以指定推送常量，这是给着色器传递动态值的另一种方式，同样会在之后的章节介绍。


## **最后**

这就是所有固定功能状态的全部内容！
从头开始设置所有这些工作量很大，但优点是我们现在几乎完全了解了图形管线中发生的一切！
这减少了因某些组件的默认状态不是您期望的那样而遇到意外行为的可能性。

但是在我们最终创建图形管线之前，还需要创建一个对象，那就是 **渲染通道** 。

---

**[C++代码](../../codes/01/22_fixfunction/main.cpp)**

**[C++代码差异](../../codes/01/22_fixfunction/main.diff)**

**[根项目CMake代码](../../codes/01/21_shader/CMakeLists.txt)**

**[shader-CMake代码](../../codes/01/21_shader/shaders/CMakeLists.txt)**

**[shader-vert代码](../../codes/01/21_shader/shaders/shader.vert)**

**[shader-frag代码](../../codes/01/21_shader/shaders/shader.frag)**

---
