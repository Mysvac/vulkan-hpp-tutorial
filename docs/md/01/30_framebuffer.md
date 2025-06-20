# **帧缓冲**

## **概念**

帧（Frame），对应屏幕上显示的一张图像。缓冲（Buffer），表示暂时存储，用于中转。

帧缓冲（Framebuffer） 是一个逻辑对象，用于绑定 渲染通道（Render Pass） 所需的图像附件（Attachments），如颜色附件、深度/模板附件等。
它定义了渲染操作的目标输出，是连接 渲染通道 和实际图像内存（如交换链图像、深度缓冲）的桥梁。

| **组件**            | **作用**                                                 | **关系**                                |
|---------------------|---------------------------------------------------------|-----------------------------------------|
| **Render Pass**     | 定义渲染流程（附件描述、子通道、依赖关系）。                 | 帧缓冲必须基于某个 Render Pass 创建。     |
| **Framebuffer**     | 绑定具体的图像视图（`VkImageView`）到 Render Pass 的附件上 | 是 Render Pass 和实际图像之间的桥梁。     |
| **Image/ImageView** | 存储像素数据（如交换链图像、深度缓冲）。                     | 帧缓冲通过 `VkImageView` 引用这些图像。   |

## **创建帧缓冲**

现在，添加新成员变量：

```cpp
std::vector<vk::raii::Framebuffer> m_swapChainFramebuffers;
```

使用一个新的函数 `createFramebuffers` 为这个数组创建对象，该函数在创建图形管线后调用：

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
for (size_t i = 0; i < m_swapChainImageViews.size(); i++) {
    vk::FramebufferCreateInfo framebufferInfo;
    framebufferInfo.renderPass = m_renderPass;
    framebufferInfo.setAttachments( *m_swapChainImageViews[i] );
    framebufferInfo.width = m_swapChainExtent.width;
    framebufferInfo.height = m_swapChainExtent.height;
    framebufferInfo.layers = 1;

    m_swapChainFramebuffers.emplace_back( m_device.createFramebuffer(framebufferInfo) );
}
```

> 注意 `setAttachments` 的参数输入时需要使用 `*`运算符重载 将 `vk::raii::ImageView` 显式转换成 `vk::ImageView` ，
> 否则需要经过2次隐式转换才能变成代理数组，无法转换成功。

如您所见，帧缓冲的创建非常简单。我们首先需要指定帧缓冲需要与哪个 `renderPass` 兼容。
您只能将帧缓冲与它兼容的渲染通道一起使用，这意味着它们**必须使用相同数量和类型的附件**。

`setAttachments()`设置了`attachmentCount` 和 `pAttachments` 参数，指定了渲染通道 `pAttachment` 数组中附件描述对应的实际 `vk::ImageView` 对象。

`width` 和 `height` 参数无需说明，`layers` 指的是图像数组中的层数。我们的交换链图像是单张图像，因此层数是 `1`。

## **测试**

我们拥有渲染所需的所有对象，现在测试程序，应当没有错误发生。

---

在下一章中，我们将编写第一个实际的绘制命令。

---

**[C++代码](../../codes/01/30_framebuffer/main.cpp)**

**[C++代码差异](../../codes/01/30_framebuffer/main.diff)**

**[根项目CMake代码](../../codes/01/21_shader/CMakeLists.txt)**

**[shader-CMake代码](../../codes/01/21_shader/shaders/CMakeLists.txt)**

**[shader-vert代码](../../codes/01/21_shader/shaders/shader.vert)**

**[shader-frag代码](../../codes/01/21_shader/shaders/shader.frag)**
