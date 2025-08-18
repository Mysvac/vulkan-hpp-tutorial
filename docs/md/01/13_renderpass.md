---
title: 渲染通道
comments: true
---
# **渲染通道**

## **渲染通道概念**

在 Vulkan 中，**渲染通道（Render Pass）** 是一个核心概念，用于定义渲染操作的框架，包括帧缓冲（Framebuffer）的附件（Attachments），子通道（Subpasses）和子通道之间的依赖关系，以及数据如何在不同渲染阶段间传递。

一个渲染通道可以包含多个子通道，每个子通道可以绑定一个图形管线，对应一次绘制操作。
可以通过子通道依赖来精确描述各子通道间的同步和资源访问顺序，确保渲染正确性

“附件”表示渲染需要使用到的图像资源，它们实际绑定在帧缓冲上，渲染通道创建时只包含附件的“描述”，这些附件可以在子通道间复用，减少内存带宽消耗。

## **设置**

我们需要告诉 Vulkan 将在渲染时使用的帧缓冲附件，还需要指定将有多少颜色和深度缓冲，每个缓冲使用多少个采样，以及它们的内容在整个渲染操作中应如何处理。

这些信息都封装在一个渲染通道对象中，我们需要为此创建一个新的 `createRenderPass` 函数。

```cpp
void initVulkan() {
    createInstance();
    setupDebugMessenger();
    createSurface();
    selectPhysicalDevice();
    createLogicalDevice();
    createSwapChain();
    createImageViews();
    createRenderPass();
}

void createRenderPass() {

}
```

## **附件描述**

在我们的例子中，只有一个颜色缓冲附件，由交换链中的一个图像表示。

```cpp
vk::AttachmentDescription colorAttachment;
```

注意渲染通道只包含附件的“描述”，而实际的附件对象绑定在帧缓冲上。  

我们会在下一章创建帧缓冲区，你需要保证渲染通道的附件“描述”和帧缓冲实际绑定的“附件”一一对应。

### 1. 颜色附件格式

**必须与交换链图像格式一致**，确保渲染输出与显示兼容，所以使用 `m_swapChainImageFormat` ：

```cpp
colorAttachment.format = m_swapChainImageFormat;
```

### 2. 采样方式

和之前的多重采样一样，设置`e1`即可：

```cpp
colorAttachment.samples = vk::SampleCountFlagBits::e1;
```

### 3. 数据加载与存储操作

对于 `loadOp`，我们将使用清除操作在绘制新帧之前将帧缓冲清除为黑色。对于 `storeOp`，
我们有兴趣在屏幕上看到渲染的三角形，因此我们在这里使用存储操作。

```cpp
colorAttachment.loadOp = vk::AttachmentLoadOp::eClear;
colorAttachment.storeOp = vk::AttachmentStoreOp::eStore;
colorAttachment.stencilLoadOp = vk::AttachmentLoadOp::eDontCare;
colorAttachment.stencilStoreOp = vk::AttachmentStoreOp::eDontCare;
```

| **操作类型**      | **常见选项**                           | **用途**                   |
|---------------|------------------------------------|--------------------------|
| **`loadOp`**  | `vk::AttachmentLoadOp::eLoad`      | 保留附件现有内容（如叠加多帧渲染）。       |
|               | `vk::AttachmentLoadOp::eClear`     | 渲染前清除为指定值（如黑色背景）。        |
|               | `vk::AttachmentLoadOp::eDontCare`  | 忽略现有内容（性能优化，适用于首次写入的附件）。 |
| **`storeOp`** | `vk::AttachmentStoreOp::eStore`    | 保存渲染结果（需后续读取或显示）。        |
|               | `vk::AttachmentStoreOp::eDontCare` | 忽略渲染结果（适用于临时中间附件）。       |

### 4. 图像布局

Vulkan 中的纹理和帧缓冲由 `vk::Image` 对象表示，这些对象具有特定的像素格式。
Vulkan 要求图像按操作类型切换布局以优化内存访问。

**注意：**图像布局并非图像在显存中的数据格式，而是指显卡通过何种方式访问和操作图像。

| **布局类型**                                   | **适用场景**            |
|--------------------------------------------|---------------------|
| `vk::ImageLayout::eUndefined`              | 不关心图像原有内容（如清除前）。    |
| `vk::ImageLayout::eColorAttachmentOptimal` | 作为颜色附件写入时使用。        |
| `vk::ImageLayout::ePresentSrcKHR`          | 渲染完成后准备显示到交换链。      |
| `vk::ImageLayout::eTransferDstOptimal`     | 作为数据拷贝目标时使用（如纹理上传）。 |


渲染通道能够根据设置自动处理图像布局转换，`initialLayout` 指定开始之前的布局，`finalLayout` 指定完成后**自动转换**到的布局：

```cpp
colorAttachment.initialLayout = vk::ImageLayout::eUndefined;
colorAttachment.finalLayout = vk::ImageLayout::ePresentSrcKHR;
```

对 `initialLayout` 使用 `eUndefined` 意味着我们不在乎图像之前的布局是什么。
此特殊值不能保证图像的内容会被保留，但这没关系，因为我们本就会主动清除内容。
我们希望图像在使用交换链渲染后即可用于呈现，所以我们使用 `ePresentSrcKHR` 作为 `finalLayout`。

## **渲染通道组成**

渲染通道由附件、子通道和子通道依赖组成。

### 1. 附件引用

附件引用为子通道提供，它告诉了子通道使用哪些附件，然后指定它们的布局（并在需要时自动转换图像布局）。

```cpp
vk::AttachmentReference colorAttachmentRef;
colorAttachmentRef.attachment = 0;
colorAttachmentRef.layout = vk::ImageLayout::eColorAttachmentOptimal;
```

`attachment` 参数通过其在附件描述数组中的索引来指定要引用的附件。
我们的附件描述数组将由单个 `vk::AttachmentDescription` 组成，因此其索引为 `0`。

`layout` 指定我们希望附件在此子通道期间具有的布局，我们打算使用该附件充当颜色缓冲，故使用 `eColorAttachmentOptimal` 布局，显卡通过此方式访问图像能获得最佳性能。

### 2. 子通道

一个子通道绑定一个图形管线，并通过“附件引用”绑定附件资源，可通过子通道依赖显式定义子通道间的执行顺序和内存依赖。
它将复杂的渲染过程拆分为多个逻辑阶段（多个子通道），每个子通道可以独立处理特定的渲染任务，从而显著提高渲染效率。

对于第一个三角形，我们只需一个图形管线、单个子通道。子通道使用 `vk::SubpassDescription` 结构描述：

```cpp
vk::SubpassDescription subpass;
subpass.pipelineBindPoint = vk::PipelineBindPoint::eGraphics;
```

Vulkan 未来也可能支持计算管线，因此我们必须明确指定它绑定的是图形管线。

接下来，我们指定对颜色附件的引用：

```cpp
subpass.setColorAttachments( colorAttachmentRef );
```

还有以下其他类型的附件可以由子通道引用

- `pInputAttachments`：可从着色器读取的附件（输入附件）
- `pResolveAttachments`：用于多重采样颜色附件的附件
- `pDepthStencilAttachment`：用于深度和模板数据的附件
- `pPreserveAttachments`：此子通道未使用、但必须保留其数据的附件

### 3. 子通道依赖

不同的子通道可能可以部分并行以获得最高性能，此时需要通过**子通道依赖\(Subpass Dependencies\)**控制执行顺序。

如果不指定子通道依赖，它使用默认的隐式依赖，当前子通道管线的开始依赖外部子通道管线的结束，即顺序执行。

此外，图像布局转换实际上也由子通道依赖控制，我们会在“渲染与呈现”章节深入讨论这一点。

## **渲染通道**

现在已经描述了附件和引用它的基本子通道，我们可以创建渲染通道本身了。
创建一个新的类成员变量存储 `vk::raii::RenderPass` 对象：

```cpp
vk::raii::RenderPass m_renderPass{ nullptr };
```

然后可以通过使用附件和子通道数组填充 `vk::RenderPassCreateInfo` 结构来创建渲染通道对象。
`vk::AttachmentReference` 对象使用此数组的索引引用附件。

```cpp
vk::RenderPassCreateInfo renderPassInfo;
renderPassInfo.setAttachments( colorAttachment );
renderPassInfo.setSubpasses( subpass );

m_renderPass = m_device.createRenderPass(renderPassInfo);
```

## **测试**

现在可以运行程序，请保证没有错误发生。

下一章我们将创建帧缓冲区，将渲染通道与交换链的图像资源关联起来。

---

**[C++代码](../../codes/01/13_renderpass/main.cpp)**

**[C++代码差异](../../codes/01/13_renderpass/main.diff)**

**[CMake代码](../../codes/01/00_base/CMakeLists.txt)**

---

