---
title: 窗口表面
comments: true
---
# **窗口表面**

## **前言**

Vulkan作为平台无关的API，需要通过WSI（窗口系统集成）扩展与窗口系统交互。

在本章中，我们讨论的第一个扩展是 `VK_KHR_surface` 。
它提供了一个 `vk::SurfaceKHR` 窗口表面对象，该对象表示一种抽象的表面类型，用于呈现渲染后的图像。

实际上我们已经启用了此扩展，因为它在 `glfwGetRequiredInstanceExtensions` 返回的列表中。
该列表还包括我们将在接下来的几章中使用的其他 WSI 扩展。

> 注意 Vulkan 进行离屏渲染不需要窗口系统，也就不需要这些扩展。

## **创建窗口表面**

### 1. 基础结构

窗口表面需要在实例创建之后立即创建，因为它会影响物理设备的选择。
由于它还涉及了呈现和渲染的内容，推迟到现在才说明是为了使教程结构更加清晰。

首先**在调试回调下方**添加一个 `m_surface` 类成员。现在成员变量列表应该是：

```cpp
GLFWwindow* m_window{ nullptr };
vk::raii::Context m_context;
vk::raii::Instance m_instance{ nullptr };
vk::raii::DebugUtilsMessengerEXT m_debugMessenger{ nullptr };
vk::raii::SurfaceKHR m_surface{ nullptr };
vk::raii::PhysicalDevice m_physicalDevice{ nullptr };
vk::raii::Device m_device{ nullptr };
vk::raii::Queue m_graphicsQueue{ nullptr };
```

添加一个函数 `createSurface`，在实例创建和 `setupDebugMessenger` 之后调用它。

```cpp
void initVulkan() {
    createInstance();
    setupDebugMessenger();
    createSurface();
    pickPhysicalDevice();
    createLogicalDevice();
}

void createSurface() {

}
```

### 2. 创建资源

虽然 `vk::raii::SurfaceKHR` 对象及其用法与平台无关，但其创建并非如此，因为它取决于窗口系统的详细信息。
好消息是，所需的平台特定扩展已在 `glfwGetRequiredInstanceExtensions` 返回的列表中，无需手动添加。

我们将直接使用 GLFW 的 `glfwCreateWindowSurface` 函数，它自动为我们处理平台的差异，但下面的代码还不能运行：

```cpp
if (glfwCreateWindowSurface( m_instance, m_window, nullptr, &m_surface) != VK_SUCCESS) {
    throw std::runtime_error("failed to create window surface!");
}
```

因为 `glfwCreateWindowSurface` 接受的是原始的C风格的`Intsance`和`Surface`，所以我们必须这么做：

```cpp
void createSurface() {
    VkSurfaceKHR cSurface;

    if (glfwCreateWindowSurface( *m_instance, m_window, nullptr, &cSurface ) != VK_SUCCESS) {
        throw std::runtime_error("failed to create window surface!");
    }
    m_surface = vk::raii::SurfaceKHR( m_instance, cSurface );
}
```

这里 `*m_instance` 返回内部 `vk::Instance` 的引用，然后可以隐式转换为 `VkInstance` 。  

我们使用 C 接口的 `VkSurfaceKHR` 类型接受句柄，并将句柄交给 `raii` 封装类进行管理。

## **呈现队列**

Vulkan 支持窗口系统，但设备可能不支持。
我们需要扩展 `isDeviceSuitable` 以确保设备可以将图像呈现到我们创建的表面。

由于呈现是特定队列的功能，此问题实际上是寻找一个支持呈现到表面的队列族。

### 1. 获取呈现队列句柄

支持绘制的队列族和支持呈现的队列族可能不重叠，因此需要修改 `QueueFamilyIndices` 结构体，添加新内容：

```cpp
struct QueueFamilyIndices {
    std::optional<uint32_t> graphicsFamily;
    std::optional<uint32_t> presentFamily;

    bool isComplete() {
        return graphicsFamily.has_value() && presentFamily.has_value();
    }
};
```

下面修改 `findQueueFamilies` 函数，以查找具有呈现到我们的窗口表面的能力的队列族。
将下面的代码放在与 `vk::QueueFlagBits::eGraphic` 相同的循环中

```cpp
if(physicalDevice.getSurfaceSupportKHR(i, m_surface)){
    indices.presentFamily = i;
}
```

注意，它们最终很可能是同一个队列族，但在整个程序中，我们可以将它们视为独立的个体使用。  

尽管如此，您可以添加额外的逻辑，偏好那些具有同时支持绘制和呈现功能的队列族的物理设备，从而提高性能。

### 2. 创建呈现队列

剩下的最后一件事是修改逻辑设备创建过程，从而创建呈现队列。在 `m_graphicsQueue` 下方添加一个成员变量：

```cpp
vk::raii::Queue m_presentQueue{ nullptr };
```

接下来，我们需要多个 `CreateInfo` 结构体，以便从两个队列族创建队列。
一种优雅的方法是创建一个所需队列族的集合。让我们修改 `createLogicalDevice` 函数

```cpp
std::vector<vk::DeviceQueueCreateInfo> queueCreateInfos;
// isDeviceSuitable() 函数已经保证了队列族可用
std::set<uint32_t> uniqueQueueFamilies = {indices.graphicsFamily.value(), indices.presentFamily.value()};

float queuePriority = 1.0f;
for (uint32_t queueFamily : uniqueQueueFamilies) {
    vk::DeviceQueueCreateInfo queueCreateInfo;
    queueCreateInfo.queueFamilyIndex = queueFamily;
    queueCreateInfo.setQueuePriorities( queuePriority );
    queueCreateInfos.emplace_back( queueCreateInfo );
}
```

然后修改 `setQueueCreateInfos` 以指向创建信息数组：（参数变量名加个`s`）

```cpp
vk::DeviceCreateInfo createInfo;
createInfo.setQueueCreateInfos( queueCreateInfos );
createInfo.pEnabledFeatures = &deviceFeatures;
```

最后，不要忘了在函数末尾获取队列句柄：

```cpp
m_presentQueue = m_device.getQueue( indices.presentFamily.value(), 0 );
```

如果队列族相同，这两个句柄很可能具有相同的值，但这依然可以正常运行。

## **测试**

现在构建与运行程序，保证没有报错。

---

**[C++代码](../../codes/01/10_surface/main.cpp)**

**[C++代码差异](../../codes/01/10_surface/main.diff)**

**[CMake代码](../../codes/01/00_base/CMakeLists.txt)**

---
