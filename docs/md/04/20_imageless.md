---
title: 无图像帧缓冲
comments: true
---
# **无图像帧缓冲**

## **前言**

**无图像帧缓冲\(Imageless Framebuffer\)**是 Vulkan 1.2 引入的一项核心特性，允许开发者在创建帧缓冲时不绑定具体的附件资源（但需要指定图像格式和其他参数），这些图像视图将在命令缓冲录制时动态绑定。

这使得帧缓冲的创建更加灵活，还可以减少帧缓冲的创建和销毁开销，尤其是在需要频繁切换渲染目标的场景中。

> 关于无图像帧缓冲：[Vulkan-Guide \[imageless framebuffer\]](https://docs.vulkan.org/guide/latest/extensions/VK_KHR_imageless_framebuffer.html)

## **基础代码**

请下载并阅读下面的基础代码，这是“C++模块化“章节的第二部分代码，但使用了同步2语法：

**[点击下载](../../codes/04/00_cxxmodule/module_code2.zip)**

构建运行，将显示我们熟悉的房屋模型：

![right_room](../../images/0310/right_room.png)

## **启用设备特性**

无图像帧缓冲需要启用设备特性，修改 `Device.cppm` 中的逻辑设备创建代码：

```cpp
// 1.2 起作为核心特性
device_create_info.get<vk::PhysicalDeviceVulkan12Features>()
    .setTimelineSemaphore( true )
    .setImagelessFramebuffer( true ); // 启用无图像帧缓冲特性
```

## **创建无图像帧缓冲**

无图像帧缓冲不包含具体的图像对象，而是和渲染通道一样只包含图像格式等参数。
这带来的一大好处是，资源格式一样时可以复用同一个无图像帧缓冲，而不需要为每个图像创建一份。

因此我们现在只需要一个帧缓冲对象，可以修改 `RenderPass.cppm` 中的成员变量：

```cpp
vk::raii::Framebuffer m_framebuffer{ nullptr }; // 修改成员变量，只需要一个帧缓冲对象
...
void recreate() {
    ...
    m_framebuffer = nullptr; // 修改交换链重建函数
    ...
}
...
[[nodiscard]]       // 修改对外接口
const vk::raii::Framebuffer& framebuffer() const { return m_framebuffer; }
```

可以修改帧缓冲的创建函数名，去掉 `s` ：

```cpp
void recreate() {
    ...
    create_framebuffer();   // 去除末尾的 s
    ...
}
...
void init() {
    create_render_pass();
    create_framebuffer();
}
...
void create_framebuffer() { // 创建帧缓冲

}
```

然后填写创建函数，无图像帧缓冲的创建需要在原先的基础上使用 `pNext` 链接 `FramebufferAttachmentsCreateInfo` 结构体：

```cpp
vk::StructureChain<
    vk::FramebufferCreateInfo,
    vk::FramebufferAttachmentsCreateInfo
> create_info;
```

首先填写第一个结构体，它和普通帧缓冲的创建类似，但需要设置 `flags` ，无需绑定图像视图：

```cpp
create_info.get()
    .setFlags( vk::FramebufferCreateFlagBits::eImageless )
    .setRenderPass( m_render_pass )
    .setAttachmentCount( 2 )    // 只需设置附件数，无需绑定具体资源
    .setHeight( m_swapchain->extent().height )
    .setWidth( m_swapchain->extent().width )
    .setLayers( 1 );
```

虽然不需要绑定图像视图，但仍需指定附件数和帧缓冲大小。

然后需要设置第二个结构体，它只用于引用附件信息，所以需要先创建附件信息结构体：

```cpp
const vk::Format swapchain_format = m_swapchain->format();
const vk::Format depth_format = m_depth_image->format();
std::array<vk::FramebufferAttachmentImageInfo, 2> image_infos;
image_infos[0].height = m_swapchain->extent().height; // 宽高和帧缓冲一致
image_infos[0].width = m_swapchain->extent().width;
image_infos[0].layerCount = 1;
image_infos[0].usage = vk::ImageUsageFlagBits::eColorAttachment;
image_infos[0].setViewFormats( swapchain_format ); // 需要设置图像格式
image_infos[1].height = m_swapchain->extent().height;
image_infos[1].width = m_swapchain->extent().width;
image_infos[1].layerCount = 1;
image_infos[1].usage = vk::ImageUsageFlagBits::eDepthStencilAttachment;
image_infos[1].setViewFormats( depth_format );
```

所有信息都应该和图像格式保持一致，宽高还需与帧缓冲一致。
此处还剩一个字段 `flags` ，它也需要和图像创建时的 `flags` 保持一致，我们的图像没有设置此字段，所以可以忽略。

然后可以将这两个结构体添加到结构体链的第二个元素中，并创建帧缓冲：

```cpp
create_info.get<vk::FramebufferAttachmentsCreateInfo>()
    .setAttachmentImageInfos( image_infos );

m_framebuffer = m_device->device().createFramebuffer( create_info.get() );
```

## **命令录制**

使用无图像帧缓冲时，需要在命令录制（开始渲染通道）时绑定具体的图像视图，需修改 `Drawer.cppm` 中的 `record_command_buffer` 函数。

绑定具体的附件也需要使用 `pNext` 链，我们先填充基础信息：

```cpp
std::array<vk::ClearValue, 2> clear_values;
clear_values[0] = vk::ClearColorValue{ 0.0f, 0.0f, 0.0f, 1.0f };
clear_values[1] = vk::ClearDepthStencilValue{ 1.0f ,0 };

vk::StructureChain<
    vk::RenderPassBeginInfo,
    vk::RenderPassAttachmentBeginInfo
> render_pass_begin_info;

render_pass_begin_info.get()
    .setRenderPass( m_render_pass->render_pass() )
    .setFramebuffer( m_render_pass->framebuffer() ) // 只有单个帧缓冲，无需索引
    .setRenderArea( vk::Rect2D{ vk::Offset2D{0, 0}, m_swapchain->extent() } )
    .setClearValues( clear_values );
```

这些信息和之前的开始信息几乎一样，但我们使用了无图像帧缓冲。

第二个结构体用于绑定具体的附件，它需要使用交换链和深度图像的图像视图，但 `Drawer` 模块没有导入深度图像模块，所以请先添加依赖：

```cpp
// Drawer.cppm
import DepthImage; // 导入深度图像模块
...
class Drawer {
    ...
    std::shared_ptr<vht::DepthImage> m_depth_image{ nullptr }; // 成员变量
    ...
    std::shared_ptr<vht::DepthImage> depth_image, // 构造函数参数
    ...
    m_depth_image(std::move(depth_image)), // 成员初始化
    ...
};

// App.cppm
void init_drawer() {
    m_drawer = std::make_shared<vht::Drawer>(
        ...
        m_depth_image,
        ...
    );
}
```

现在可以正常填写第二个结构体了：

```cpp
std::array<vk::ImageView, 2> attachments{
    m_swapchain->image_views()[image_index],
    m_depth_image->image_view()
};

render_pass_begin_info.get<vk::RenderPassAttachmentBeginInfo>()
    .setAttachments( attachments );
```

是的，此结构体就是这么简单，它仅用于绑定具体的图像视图。

最后一件事，略微调整渲染通道的开始函数：

```cpp
command_buffer.beginRenderPass( render_pass_begin_info.get(), vk::SubpassContents::eInline);
```

现在你可以编译并运行代码，应该能够看到熟悉的渲染结果。

![right_room](../../images/0310/right_room.png)

## **最后**

“无图像帧缓冲”并没有带来太多额外的工作，但它确实减少了帧缓冲的数量，提高了资源复用性。

现在你可以浏览官方文档，了解更多关于无图像帧缓冲的细节和使用场景。

---

**[基础代码](../../codes/04/00_cxxmodule/module_code2.zip)**

**[Device.cppm（修改）](../../codes/04/20_imageless/Device.cppm)**

**[Device.diff（差异文件）](../../codes/04/20_imageless/Device.diff)**

**[RenderPass.cppm（修改）](../../codes/04/20_imageless/RenderPass.cppm)**

**[RenderPass.diff（差异文件）](../../codes/04/20_imageless/RenderPass.diff)**

**[Drawer.cppm（修改）](../../codes/04/20_imageless/Drawer.cppm)**

**[Drawer.diff（差异文件）](../../codes/04/20_imageless/Drawer.diff)**

**[App.cppm（修改）](../../codes/04/20_imageless/App.cppm)**

**[App.diff（差异文件）](../../codes/04/20_imageless/App.diff)**

---
