# **渲染通道**

## **渲染通道概念**

在 **Vulkan API** 中，**渲染通道（Render Pass）** 是一个核心概念，用于定义渲染操作的框架，包括帧缓冲（Framebuffer）的附件（Attachments）、子通道（Subpasses）之间的依赖关系以及数据如何在不同渲染阶段传递。

### 1. 渲染通道的核心作用

- **描述渲染目标**：定义颜色、深度、模板等附件的格式和用途（如输入、输出或保留）。  
- **组织子通道**：将渲染过程分解为多个逻辑阶段，避免冗余的内存读写（如延迟渲染中的 G-Buffer 生成和光照计算）。  
- **优化性能**：通过明确的依赖关系，指导驱动进行内存布局转换（如从 `COLOR_ATTACHMENT_OPTIMAL` 到 `SHADER_READ_OPTIMAL`）。  


### 2. 与其他 Vulkan 组件的关系

| **组件**               | **与渲染通道的关系**                                                                 |
|------------------------|------------------------------------------------------------------------------------|
| **管线（Pipeline）**    | 管线创建时需要绑定到特定的渲染通道和子通道，确定着色器如何访问附件（如 `layout(location=0) out vec4 outColor`）。 |
| **帧缓冲（Framebuffer）** | 帧缓冲的附件必须与渲染通道中定义的附件匹配（格式、数量）。渲染通道是帧缓冲的“蓝图”。 |
| **命令缓冲（Command Buffer）** | 在命令缓冲中，`vkCmdBeginRenderPass` 和 `vkCmdEndRenderPass` 之间记录渲染命令，依赖渲染通道的配置。 |
| **同步（Synchronization）** | 子通道依赖和外部依赖（如与交换链图像的同步）通过 `VkSubpassDependency` 和管线屏障（Barrier）管理。 |

## **设置**

在我们完成管线的创建之前，我们需要告诉 Vulkan 将在渲染时使用的帧缓冲附件。
我们还需要指定将有多少颜色和深度缓冲，每个缓冲使用多少个采样，以及它们的内容在整个渲染操作中应如何处理。

所有这些信息都封装在一个渲染通道对象中，我们需要为此创建一个新的 `createRenderPass` 函数。
在 `createGraphicsPipeline` **之前**从 `initVulkan` 调用此函数：

```cpp
void initVulkan() {
    createInstance();
    setupDebugMessenger();
    createSurface();
    pickPhysicalDevice();
    createLogicalDevice();
    createSwapChain();
    createImageViews();
    createRenderPass();
    createGraphicsPipeline();
}

// ...

void createRenderPass() {

}
```

## **附件描述**

在我们的例子中，我们将只有一个颜色缓冲附件，由交换链中的一个图像表示。

```cpp
vk::AttachmentDescription colorAttachment;
```

> 注意渲染通道只包含附件的“描述”，而实际的附件对象绑定在帧缓冲上。  
> 你需要保证渲染通道的附件“描述”和帧缓冲实际绑定的“附件”一一对应。

### 1. 颜色附件格式

**必须与交换链图像格式一致**，确保渲染输出与显示兼容。所以我们使用`m_swapChainImageFormat`。

```cpp
colorAttachment.format = m_swapChainImageFormat;
```

### 2. 采样方式

和之前的多重采样一样，设置`e1`即可：

```cpp
colorAttachment.samples = vk::SampleCountFlagBits::e1;
```

### 3. 数据加载与存储操作


| **操作类型**       | **常见选项**                       | **用途**                                                                 |
|--------------------|-----------------------------------|--------------------------------------------------------------------------|
| **`loadOp`**       | `vk::AttachmentLoadOp::eLoad`     | 保留附件现有内容（如叠加多帧渲染）。                                       |
|                    | `vk::AttachmentLoadOp::eClear`    | 渲染前清除为指定值（如黑色背景）。                                |
|                    | `vk::AttachmentLoadOp::eDontCare` | 忽略现有内容（性能优化，适用于首次写入的附件）。                            |
| **`storeOp`**      | `vk::AttachmentStoreOp::eStore`   | 保存渲染结果（需后续读取或显示）。                                |
|                    | `vk::AttachmentStoreOp::eDontCare`| 丢弃渲染结果（适用于临时中间附件）。   

对于 `loadOp`，我们将使用清除操作在绘制新帧之前将帧缓冲清除为黑色。对于 `storeOp`，
我们有兴趣在屏幕上看到渲染的三角形，因此我们在这里使用存储操作。

```cpp
colorAttachment.loadOp = vk::AttachmentLoadOp::eClear;
colorAttachment.storeOp = vk::AttachmentStoreOp::eStore;
colorAttachment.stencilLoadOp = vk::AttachmentLoadOp::eDontCare;
colorAttachment.stencilStoreOp = vk::AttachmentStoreOp::eDontCare;
```

深度/模板附件的配置与此类似，但使用 `stencilLoadOp`/`stencilStoreOp`（模板缓冲未使用时设为 `eDontCare`）。


### 4. 图像布局

Vulkan 中的纹理和帧缓冲由 `vk::Image` 对象表示，这些对象具有特定的像素格式。
Vulkan 要求图像按操作类型切换布局以优化内存访问：

| **布局类型**                   | **适用场景**                                   |
|-------------------------------|----------------------------------------------|
| `vk::ImageLayout::eUndefined`    | 不关心图像原有内容（如清除前）。  |
| `vk::ImageLayout::eColorAttachmentOptimal` | 作为颜色附件写入时使用。                     |
| `vk::ImageLayout::ePresentSrcKHR`          | 渲染完成后准备显示到交换链。    |
| `vk::ImageLayout::eTransferDstOptimal`     | 作为数据拷贝目标时使用（如纹理上传）。        |

我们将在纹理章节中更深入地讨论这个主题，但现在重要的是要知道图像需要转换为特定的布局，这些布局适合它们接下来要参与的操作。

```cpp
colorAttachment.initialLayout = vk::ImageLayout::eUndefined;
colorAttachment.finalLayout = vk::ImageLayout::ePresentSrcKHR;
```

- `initialLayout` 指定图像在渲染通道开始之前将具有的布局。
- `finalLayout` 指定渲染通道完成时自动转换到的布局。

对 `initialLayout` 使用 `eUndefined` 意味着我们不在乎图像之前的布局是什么。
此特殊值的不能保证图像的内容会被保留，但这没关系，因为我们无论如何都要清除它。
我们希望图像在使用交换链渲染后即可用于呈现，所以我们使用 `ePresentSrcKHR` 作为 `finalLayout`。

## **渲染通道组成**

渲染通道由附件、子通道和子通道依赖组成。

### 1. 附件

- 定义帧缓冲中每个图像视图（如颜色、深度缓冲）的格式（`VkFormat`）、加载/存储操作（Load/Store Op）。

```cpp
vk::AttachmentReference colorAttachmentRef;
colorAttachmentRef.attachment = 0;
colorAttachmentRef.layout = vk::ImageLayout::eColorAttachmentOptimal;
```

`attachment` 参数通过其在附件描述数组中的索引来指定要引用的附件。
我们的数组由单个 `vk::AttachmentDescription` 组成，因此其索引为 `0`。

`layout` 指定我们希望附件在此子通道期间具有的布局，当子通道启动时，Vulkan 自动将附件转换为此布局。
我们打算使用该附件充当颜色缓冲，并使用 `eColorAttachmentOptimal` 布局提供最佳性能。

> 注意这里引用的实际是“附件描述”，而此附件本身（`vk::Image`）实际绑定在帧缓冲上。

### 2. 子通道

**子通道的作用：**

- 将渲染过程划分为多个阶段，每个子通道引用一组附件（作为输入/输出/保留）。

- 避免中间结果写回内存，直接复用数据（如深度缓冲在多个子通道间共享）。

- 示例（延迟渲染）：
    - Subpass 1：写入 G-Buffer（颜色、法线、深度）。
    - Subpass 2：读取 G-Buffer，计算光照。

对于第一个三角形，我们只使用单个子通道，子通道使用 `vk::SubpassDescription` 结构描述

```cpp
vk::SubpassDescription subpass;
subpass.pipelineBindPoint = vk::PipelineBindPoint::eGraphics;
```

Vulkan 未来也可能支持计算子通道，因此我们必须明确说明这是一个图形子通道。

接下来，我们指定对颜色附件的引用

```cpp
subpass.setColorAttachments( colorAttachmentRef );
```

片段着色器中使用 `layout(location = 0) out vec4 outColor` 指令引用的就是此数组！

以下其他类型的附件可以由子通道引用

- `pInputAttachments`：从着色器读取的附件
- `pResolveAttachments`：用于多重采样颜色附件的附件
- `pDepthStencilAttachment`：用于深度和模板数据的附件
- `pPreserveAttachments`：此子通道未使用的附件，但必须保留其数据的附件


### 3. 子通道依赖

子通道依赖（Subpass Dependencies）定义子通道之间的执行顺序和内存访问规则，解决资源竞争问题。
由于我们只有单个子通道，暂时无需处理依赖问题。

## **渲染通道**

现在已经描述了附件和引用它的基本子通道，我们可以创建渲染通道本身。
创建一个新的类成员变量来保存 `vk::raii::RenderPass` 对象，就在 `m_pipelineLayout` 变量的**上方**

```cpp
vk::raii::RenderPass m_renderPass{ nullptr };
vk::raii::PipelineLayout m_pipelineLayout{ nullptr };
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

---

这里进行了很多工作，但在下一章中，我们会将所有内容都结合在一起，最终创建图形管线对象！

---

**[C++代码](../../codes/01/23_renderpass/main.cpp)**

**[C++代码差异](../../codes/01/23_renderpass/main.diff)**

**[根项目CMake代码](../../codes/01/21_shader/CMakeLists.txt)**

**[shader-CMake代码](../../codes/01/21_shader/shaders/CMakeLists.txt)**

**[shader-vert代码](../../codes/01/21_shader/shaders/shader.vert)**

**[shader-frag代码](../../codes/01/21_shader/shaders/shader.frag)**
