# **图像视图**

## **前言**

要使用任何 `vk::Image`，都必须先创建一个 `vk::ImageView` 对象。

ImageView 从字面上看就是图像的视图。
它描述了如何访问图像以及访问图像的哪一部分，例如它是否应该被视为没有 mipmap 深度级别的 2D 纹理图像。

在本章中，我们将为交换链中的每个图像创建一个图像视图，以便稍后将它们用作渲染目标。

## **创建图像视图**

### 1. 添加基本结构

首先添加一个类成员来存储图像视图。注意图像视图需要手动回收资源，所以我们使用 RAII 封装。

```cpp
std::vector<vk::raii::ImageView> m_swapChainImageViews;
```

创建 `createImageViews` 函数并在交换链创建后立即调用它。

```cpp
void initVulkan() {
    createInstance();
    setupDebugMessenger();
    createSurface();
    pickPhysicalDevice();
    createLogicalDevice();
    createSwapChain();
    createImageViews();
}

void createImageViews() {

}
```

### 2. 批量创建图像视图

设置循环以迭代所有交换链图像，图像视图需要一个一个创建。

```cpp
m_swapChainImageViews.reserve( m_swapChainImages.size() );
for (size_t i = 0; i < m_swapChainImages.size(); i++) {
    // ......
}
```

> 注意`resize`是不行的，因为不支持无参构造。需要使用`reserve`仅分配空间而不实例化内容。

图像视图创建的参数在 `vk::ImageViewCreateInfo` 结构中指定，前几个参数很简单：

```cpp
vk::ImageViewCreateInfo createInfo;
createInfo.image = m_swapChainImages[i];
createInfo.viewType = vk::ImageViewType::e2D;
createInfo.format = m_swapChainImageFormat;
```

- `viewType` 参数允许您将图像视为 1D、2D、3D 和立方体贴图。

- `components` 字段我们未设置（默认），它允许您调换颜色通道。例如，您可以将所有通道映射到红色通道以获得单色纹理。

下面设置 `subresourceRange` 字段，它描述了图像的用途以及应访问图像的哪一部分。

```cpp
createInfo.subresourceRange.aspectMask = vk::ImageAspectFlagBits::eColor;
createInfo.subresourceRange.baseMipLevel = 0;
createInfo.subresourceRange.levelCount = 1;
createInfo.subresourceRange.baseArrayLayer = 0;
createInfo.subresourceRange.layerCount = 1;
```

第一个字段指定图像存放的数据类型，我们存放的是“色彩”数据。
此外还有“深度”和“模板”，我们会在“深度缓冲”章节介绍。

我们的图像将用在没有 `mipmap` 级别和多层颜色的目标上。
注意没有额外 `mip` 级别和层次，但是原图自身占一个级别和层次，所以 `levelCount` 和 `layerCount` 为 `1` 。

> 如果您正在开发立体的 3D 应用程序，那么您将创建一个具有多层的交换链。
> 然后，您可以为每个图像创建多个图像视图，通过访问不同的图层来表示左眼和右眼的视图。

现在，创建图像视图：

```cpp
m_swapChainImageViews.emplace_back( m_device.createImageView(createInfo) );
```
## **测试**

现在运行程序保证没有异常。

---

图像视图足以将图像用作纹理，但它还不能完全用作渲染目标。这需要一个额外的间接步骤，称为帧缓冲。但在它之前我们还需设置图形管线。

---

**[C++代码](../../codes/01/12_imageview/main.cpp)**

**[C++代码差异](../../codes/01/12_imageview/main.diff)**

**[CMake代码](../../codes/01/00_base/CMakeLists.txt)**
