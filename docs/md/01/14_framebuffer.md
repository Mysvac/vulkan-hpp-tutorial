---
title: 帧缓冲区
comments: true
---
# **帧缓冲**

## **概念**

**帧缓冲\(Framebuffer\)**是一个逻辑对象，用于绑定“渲染通道\(Render Pass\)”所需的图像附件，如颜色附件、深度/模板附件等。

| **组件**              | **作用**                                   | **关系**                      |
|---------------------|----------------------------------------------|-----------------------------|
| **Render Pass**     | 定义渲染流程（附件描述、子通道、依赖关系）。          | 帧缓冲必须基于某个 Render Pass 创建。   |
| **Framebuffer**     | 绑定具体的图像资源（附件资源）                      | 是 Render Pass 和实际图像之间的桥梁。   |
| **Image/ImageView** | 存储像素数据（如交换链图像、深度缓冲）。            | 帧缓冲通过 `VkImageView` 引用这些图像。 |

## **创建帧缓冲**

现在，添加新成员变量：

```cpp
std::vector<vk::raii::Framebuffer> m_swapChainFramebuffers;
```

使用一个新的函数 `createFramebuffers` 为这个数组创建对象：

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
    createFramebuffers();
}

// ...

void createFramebuffers() {

}
```

首先调整容器大小以容纳所有帧缓冲，注意使用 `reserve` 而非 `resize` ：

```cpp
m_swapChainFramebuffers.reserve( m_swapChainImageViews.size() );
```

然后，遍历图像视图并从中创建帧缓冲：

```cpp
vk::FramebufferCreateInfo framebufferInfo;
framebufferInfo.renderPass = m_renderPass;
framebufferInfo.width = m_swapChainExtent.width;
framebufferInfo.height = m_swapChainExtent.height;
framebufferInfo.layers = 1;
for (const auto& imageView : m_swapChainImageViews) {
    framebufferInfo.setAttachments( *imageView );
    m_swapChainFramebuffers.emplace_back( m_device.createFramebuffer(framebufferInfo) );
}
```

> 注意 `setAttachments` 的参数输入时需要使用 `*` 运算符将 `vk::raii::ImageView` 显式转换成 `vk::ImageView` ，否则需要 2 次隐式转换才能变成代理数组，无法转换成功。

如您所见，帧缓冲的创建非常简单。我们首先需要指定帧缓冲需要与哪个 `renderPass` 兼容。
您只能将帧缓冲与它兼容的渲染通道一起使用，这意味着它们**必须使用相同数量和类型的附件**。

`setAttachments()`设置了`attachmentCount` 和 `pAttachments` 参数，指定了渲染通道 `pAttachment` 附件描述数组对应的实际附件对象 `vk::ImageView` 。

`width` 和 `height` 参数无需说明，`layers` 指的是图像数组中的层数。我们的交换链图像是单张图像，因此层数是 `1`。

## **测试**

现在测试程序，应当没有错误发生。

从下一章开始，我们将介绍图形管线。

---

**[C++代码](../../codes/01/14_framebuffer/main.cpp)**

**[C++代码差异](../../codes/01/14_framebuffer/main.diff)**

**[CMake代码](../../codes/01/00_base/CMakeLists.txt)**

---
