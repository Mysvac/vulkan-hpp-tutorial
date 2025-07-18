---
title: 交换链
comments: true
---
# **交换链**

## **前言**

正如“教程前言”中的比喻，交换链\(SwapChain\)用于处理窗口表面和实际绘制之间的图像中转：

- 交换链是一组等待显示到屏幕的图像队列

- 应用程序从队列获取图像进行渲染，完成后返回队列

- 窗口表面从队列获取渲染完成的图像并呈现到窗口

- 交换链同步图像呈现与屏幕刷新率

> 关于交换链：[Vulkan-Guide \[WSI\]](https://docs.vulkan.org/guide/latest/wsi.html)

## **检查交换链支持**

### 1. 添加扩展

并非所有显卡都支持将图像呈现到屏幕，比如专用于服务器的显卡。

窗口系统并非 Vulkan 核心，所以我们要启用 `VK_KHR_swapchain` 设备扩展并检查支持性。
首先声明一个必需的设备扩展列表，类似于要启用的层列表。

```cpp
constexpr std::array<const char*,1> DEVICE_EXTENSIONS {
    vk::KHRSwapchainExtensionName
};
```

> 常量 `vk::KHRSwapchainExtensionName` 等于 `"VK_KHR_swapchain"`，使用它防止拼写错误。

### 2. 扩展物理设备检查

接下来创建一个新的函数 `checkDeviceExtensionSupport`，该函数将被 `isDeviceSuitable` 调用，作为额外的检查：

```cpp
static bool checkDeviceExtensionSupport(const vk::raii::PhysicalDevice& physicalDevice) {
    return true;
}

bool isDeviceSuitable(const vk::raii::PhysicalDevice& physicalDevice) const {
    const auto indices = findQueueFamilies(physicalDevice);
    const bool extensionsSupported = checkDeviceExtensionSupport(physicalDevice);

    return indices.isComplete() && extensionsSupported;
}
```

### 3. 设备适用性检查

修改函数体以枚举扩展，并检查所有必需的扩展是否都在其中：

```cpp
static bool checkDeviceExtensionSupport(const vk::raii::PhysicalDevice& physicalDevice) {
    // std::vector<vk::ExtensionProperties>
    const auto availableExtensions = physicalDevice.enumerateDeviceExtensionProperties();
    std::set<std::string> requiredExtensions(DEVICE_EXTENSIONS.begin(), DEVICE_EXTENSIONS.end());
    for (const auto& extension : availableExtensions) {
        requiredExtensions.erase(extension.extensionName);
    }
    return requiredExtensions.empty();
}
```

### 4. 启用设备扩展

启用扩展只需要对逻辑设备创建结构进行少量更改，现在修改 `createLogicalDevice` 函数，添加内容：

```cpp
createInfo.setPEnabledExtensionNames( DEVICE_EXTENSIONS );
```

现在运行代码并验证您的显卡是否能够创建交换链。

## **查询交换链支持详情**

仅检查交换链是否可用是不够的，因为它可能与我们的窗口表面不兼容。
一般而言，我们还需要检查三种属性：

- 基本表面功能（交换链中图像的最小/最大数量，图像的最小/最大宽度和高度）
- 可用表面格式（像素格式，色彩空间）
- 可用呈现模式

### 1. 交换链信息存储

我们创建一个结构来存储和传递这些详细信息：

```cpp
struct SwapChainSupportDetails {
    vk::SurfaceCapabilitiesKHR capabilities;
    std::vector<vk::SurfaceFormatKHR>  formats;
    std::vector<vk::PresentModeKHR> presentModes;
};
```

再创建一个新函数 `querySwapChainSupport` 用于填充此结构：

```cpp
SwapChainSupportDetails querySwapChainSupport(const vk::raii::PhysicalDevice& physicalDevice) const {
    SwapChainSupportDetails details;

    return details;
}
```

> 本节仅介绍如何查询这些信息。它们的具体含义和内容将在下一节中讨论。

### 2. 查询可用设置

我们从基本表面功能与可用设置开始，这些属性易于查询。

```cpp
details.capabilities = physicalDevice.getSurfaceCapabilitiesKHR( m_surface );
details.formats = physicalDevice.getSurfaceFormatsKHR( m_surface );
details.presentModes = physicalDevice.getSurfacePresentModesKHR( m_surface );
```

让我们再次扩展 `isDeviceSuitable` 函数来验证交换链支持是否足够。
本教程至少需要一种受支持的图像格式和一种受支持的呈现模式。

```cpp
bool isDeviceSuitable(const vk::raii::PhysicalDevice& physicalDevice) const {
    if (const auto indices = findQueueFamilies(physicalDevice);
        !indices.isComplete()
    ) return false;

    if (const bool extensionsSupported = checkDeviceExtensionSupport(physicalDevice);
        !extensionsSupported
    ) return false;

    if (const auto swapChainSupport = querySwapChainSupport(physicalDevice);
        swapChainSupport.formats.empty() || swapChainSupport.presentModes.empty()
    ) return false;
    
    return true;
}
```

> 注意，我们在验证扩展可用后再查询交换链支持。


## **交换链设置选择**

只要满足上述检查，支持性就足够了。 但可能存在许多不同的优化模式。

我们现在将编写几个函数来找到最佳交换链的设置。有三种类型的设置需要确定

- 交换范围（交换链中图像的分辨率）
- 表面格式（颜色深度）
- 呈现模式（“交换”图像到屏幕的条件）

### 1. 表面格式

创建一个新函数用来查询最佳参数，函数输入是刚刚定义的结构体成员

```cpp
static vk::SurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<vk::SurfaceFormatKHR>& availableFormats) {

}
```

每个 `vk::SurfaceFormatKHR` 条目都包含一个 `format` 和一个 `colorSpace` 成员：

| 成员           | 作用                                               |
|--------------|--------------------------------------------------|
| `format`     | 指定颜色通道和类型                                        |  
|              | 例如，`vk::Format::eB8G8R8A8Srgb` 表示BGRA四通道且每通道占8位。 |  
| `colorSpace` | 指定色彩空间                                           |  
|              | 例如，SRGB使用 `vk::ColorSpaceKHR::eSrgbNonlinear`。   |  


让我们浏览列表，看看首选组合是否可用

```cpp
for (const auto& availableFormat : availableFormats) {
    if (availableFormat.format == vk::Format::eB8G8R8A8Srgb && 
        availableFormat.colorSpace == vk::ColorSpaceKHR::eSrgbNonlinear
    ) return availableFormat;
}
```

如果这也失败了，那么我们根据可用格式的好坏程度排序。大多数情况下，只需选择第一个格式即可。
```cpp
static vk::SurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<vk::SurfaceFormatKHR>& availableFormats) {
    for (const auto& availableFormat : availableFormats) {
        if (availableFormat.format == vk::Format::eB8G8R8A8Srgb &&
            availableFormat.colorSpace == vk::ColorSpaceKHR::eSrgbNonlinear
        ) return availableFormat;
    }
    return availableFormats.at(0);
}
```

### 2. 呈现模式

呈现模式可以说是交换链最重要的设置，它表示将图像显示到屏幕的条件。Vulkan 中有多种可用模式，都位于`vk::PresentModeKHR`中，下面介绍常见的四种：

| `vk::PresentModeKHR` | 含义                                  |
|----------------------|-------------------------------------|
| `eImmediate`         | 图像会立即传输到屏幕，可能会导致图像撕裂。               |  
| `eFifo`              | 先进先出的队列。若队列已满，则程序必须等待。              |  
|                      | 这与现代游戏中的垂直同步最相似。显示刷新的时刻称为“垂直消隐”。    |  
| `eFifoRelaxed`       | 第二种的变体，图像在最终到达时立即传输，可能会导致明显的撕裂。     |  
|                      | 仅当应用程序迟到且队列在上次垂直消隐时为空，此模式才与前一种模式不同。 |  
| `eMailbox`           | 第二种的变体，队列已满时将已排队的图像简单地替换为较新的图像。     |
|                      | 模式可用于尽可能快地渲染帧，通常被称为“三重缓冲”。          |  


只有 `vk::PresentModeKHR::eFifo` 模式保证可用，因此我们将再次编写一个函数，以查找可用的最佳模式：

```cpp
static vk::PresentModeKHR chooseSwapPresentMode(const std::vector<vk::PresentModeKHR>& availablePresentModes) {
    return vk::PresentModeKHR::eFifo;
}
```

如果能耗不是问题，那么 `eMailbox` 是一个较好的折衷方案。
在能耗更重要的移动设备上，您可能需要改用 `eFifo`。

```cpp
static vk::PresentModeKHR chooseSwapPresentMode(const std::vector<vk::PresentModeKHR>& availablePresentModes) {
    for (const auto& availablePresentMode : availablePresentModes) {
        if (availablePresentMode == vk::PresentModeKHR::eMailbox) {
            return availablePresentMode;
        }
    }
    return vk::PresentModeKHR::eFifo;
}
```

现在，可以通过打印输出浏览列表，看看 `eMailbox` 是否可用。

### 3. 交换范围

只剩下一个主要属性，为此我们添加最后一个函数：

```cpp
vk::Extent2D chooseSwapExtent(const vk::SurfaceCapabilitiesKHR& capabilities) const {

}
```

交换范围是交换链图像的分辨率，它几乎总是等于我们窗口的像素分辨率。可能的分辨率范围在 `vk::SurfaceCapabilitiesKHR` 结构中定义。

Vulkan 要求我们通过 `currentExtent` 的成员设置宽度和高度来匹配窗口的分辨率，但是某些窗口管理器提供更自由的设置方式，此时 `currentExtent` 中的宽度和高度会是 `uint32_t` 的最大值，而我们可以自由选择`minImageExtent` 和 `maxImageExtent` 内的分辨率。

GLFW 在测量尺寸时使用两个单位：像素和屏幕坐标。

例如，我们在创建窗口时指定的 `{WIDTH, HEIGHT}` 分辨率是屏幕坐标。
但是 Vulkan 使用像素，因此交换链范围也必须以像素为单位。

不幸的是，如果您使用的是高 DPI 显示器，由于更高的像素密度，窗口的像素分辨率将大于屏幕坐标分辨率。
所以我们只能使用 `glfwGetFramebufferSize` 查询窗口的像素分辨率，然后再将其与最小和最大图像范围进行匹配。

```cpp
...
#include <limits> // Necessary for std::numeric_limits
#include <algorithm> // Necessary for std::clamp

......

vk::Extent2D chooseSwapExtent(const vk::SurfaceCapabilitiesKHR& capabilities) const {
    if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max()) {
        return capabilities.currentExtent;
    }
    int width, height;
    glfwGetFramebufferSize( m_window, &width, &height );
    vk::Extent2D actualExtent (
        static_cast<uint32_t>(width),
        static_cast<uint32_t>(height)
    );

    actualExtent.width = std::clamp(actualExtent.width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width);
    actualExtent.height = std::clamp(actualExtent.height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height);
    return actualExtent;
}
```

> `clamp` 函数用于将 `width` 和 `height` 的值限制在指定的范围之间。

## **创建交换链**

### 1. 基础结构

创建一个 `createSwapChain` 函数，并确保在逻辑设备创建后调用它。

```cpp
void initVulkan() {
    createInstance();
    setupDebugMessenger();
    createSurface();
    selectPhysicalDevice();
    createLogicalDevice();
    createSwapChain();
}

void createSwapChain() {
    const auto [capabilities, formats, presentModes]  = querySwapChainSupport( m_physicalDevice );
    const auto surfaceFormat = chooseSwapSurfaceFormat( formats );
    const auto presentMode = chooseSwapPresentMode( presentModes );
    const auto extent = chooseSwapExtent( capabilities );
}
```

我们还必须指定交换链中拥有多少图像，指定它运行所需的最小数量。 

仅仅使用最小值意味着我们有时可能需要等待驱动程序完成内部操作，然后才能获取另一个图像进行渲染。因此建议设置至少比最小值多一个

```cpp
uint32_t imageCount = capabilities.minImageCount + 1;
```

我们还应该确保在执行此操作时不超过图像的最大数量，其中 0 是一个特殊值，表示没有最大值

```cpp
if (capabilities.maxImageCount > 0 && imageCount > capabilities.maxImageCount) {
    imageCount = capabilities.maxImageCount;
}
```

### 2. 创建配置

然后创建我们熟悉的 `CreateInfo`

```cpp
vk::SwapchainCreateInfoKHR createInfo;
createInfo.surface = m_surface;
createInfo.minImageCount = imageCount;
createInfo.imageFormat = surfaceFormat.format;
createInfo.imageColorSpace = surfaceFormat.colorSpace;
createInfo.imageExtent = extent;
createInfo.imageArrayLayers = 1;
createInfo.imageUsage = vk::ImageUsageFlagBits::eColorAttachment;
```

- `imageArrayLayers` 指定每个图像由多少层组成。除非您正在开发立体 3D 应用程序，否则这始终为 `1`。

- `imageUsage` 位字段指定交换链中图像的操作类型。在本教程中，我们将直接渲染它们，这意味着它们用作颜色附件。我们会在“渲染通道”章节详细介绍附件。

### 3. 队列族信息设置

接下来，我们需要指定如何处理将在多个队列族中使用的交换链图像。
我们将依靠图形队列绘制交换链中的图像，然后在呈现队列上提交它们。

有两种方法可以被从多个队列访问的图像：

| `vk::SharingMode` | 含义                                                  |
|-------------------|-----------------------------------------------------|
| `eExclusive`      | 图像一次由一个队列族拥有，并且必须显式传输所有权，然后才能在另一个队列族中使用它。此选项提供最佳性能。 |  
| `eConcurrent`     | 图像可以在多个队列族之间使用，而无需显式的所有权传输。                         |  


如果队列族不同，我们将使用并发模式。
如果图形队列族和呈现队列族相同（大多数硬件都是这种情况），我们应该使用独占模式，因为并发模式要求您指定至少两个不同的队列族。

```cpp
const auto [graphicsFamily, presentFamily] = findQueueFamilies( m_physicalDevice );
std::vector<uint32_t> queueFamilyIndices { graphicsFamily.value(), presentFamily.value() };
if (graphicsFamily != presentFamily) {
    createInfo.imageSharingMode = vk::SharingMode::eConcurrent;
    createInfo.setQueueFamilyIndices( queueFamilyIndices );
} else {
    createInfo.imageSharingMode = vk::SharingMode::eExclusive;
}
```

### 4. 指定图像变换

我们可以指定是否应将某个变换应用于交换链中的图像，例如顺时针旋转 90 度或水平翻转。

要指定您不希望进行任何变换，只需如下方式设置。

```cpp
createInfo.preTransform = capabilities.currentTransform;
```

`compositeAlpha` 字段指定 alpha 通道是否与窗口系统中的其他窗口混合。您几乎总是希望忽略 alpha 通道，可以如下设置

```cpp
createInfo.compositeAlpha = vk::CompositeAlphaFlagBitsKHR::eOpaque;
```

`presentMode` 成员直接指定上面的presentMode。

```cpp
createInfo.presentMode = presentMode;
```

如果将 `clipped` 成员设置为 `true`，则意味着忽略被遮挡的像素的颜色。
我们启用剪裁以获得最佳性能

```cpp
createInfo.clipped = true;
```

这只剩下一个字段， `oldSwapChain` 。

使用 Vulkan，您的交换链可能会在应用程序运行时变为无效或未优化，例如调整了窗口大小。
在这种情况下，需要重新创建交换链，并且必须在此字段中指定对旧交换链的引用。
这是一个复杂的主题，我们将在以后的章节中了解更多信息。现在，我们假设只会创建一个交换链。

```cpp
createInfo.oldSwapchain = nullptr;
```

### 5. 创建交换链

现在添加一个类成员 `m_swapChain` 以存储交换链对象，你的成员变量应该类似这样：

```cpp
GLFWwindow* m_window{ nullptr };
vk::raii::Context m_context;
vk::raii::Instance m_instance{ nullptr };
vk::raii::DebugUtilsMessengerEXT m_debugMessenger{ nullptr };
vk::raii::SurfaceKHR m_surface{ nullptr };
vk::raii::PhysicalDevice m_physicalDevice{ nullptr };
vk::raii::Device m_device{ nullptr };
vk::raii::Queue m_graphicsQueue{ nullptr };
vk::raii::Queue m_presentQueue{ nullptr };
vk::raii::SwapchainKHR m_swapChain{ nullptr };
```

然后在函数中创建交换链

```cpp
m_swapChain = m_device.createSwapchainKHR( createInfo );
```

现在运行应用程序以确保交换链可成功创建。

## **检索交换链图像**

交换链现在已创建，因此剩下的就是检索其中 `vk::Image` 的句柄。我们将在后面的章节中在渲染操作期间引用这些句柄。添加一个类成员来存储句柄

```cpp
std::vector<vk::Image> m_swapChainImages;
```

下面在 `createSwapChain` 函数的末尾创建交换链：

```cpp
m_swapChainImages = m_swapChain.getImages();
```

> `vk::image` 的资源释放由交换链管理，交换链释放时会自动释放此资源，所以无需RAII。  
> 情况类似`PhysicalDevice`，但物理设备加了`raii::`、这里却没有，可能是因为 `raii` 封装中物理设备需要特殊的成员函数。

## **存储交换链参数**

最后一件事，将我们为交换链图像选择的格式 `vk::Format` 和范围 `vk::Extent2D` 存储在成员变量中，我们以后的章节需要用到它们。

```cpp
// class member
vk::raii::SwapchainKHR m_swapChain{ nullptr };
std::vector<vk::Image> m_swapChainImages;
vk::Format m_swapChainImageFormat{};
vk::Extent2D m_swapChainExtent{};

// createSwapchain() function
m_swapChainImageFormat = surfaceFormat.format;
m_swapChainExtent = extent;
```

## **测试**

现在运行代码保证程序正常。

我们现在有了一组可以绘制并呈现到窗口的图像。
下一章将开始介绍如何将图像设置为渲染目标，然后开始研究实际的图形管线和绘制命令。

---

**[C++代码](../../codes/01/11_swapchain/main.cpp)**

**[C++代码差异](../../codes/01/11_swapchain/main.diff)**

**[CMake代码](../../codes/01/00_base/CMakeLists.txt)**

---
