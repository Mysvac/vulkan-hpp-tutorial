# Vulkan窗口表面

Vulkan作为平台无关的API，需要通过WSI（窗口系统集成）扩展与窗口系统交互。


在本章中，我们讨论的第一个扩展是 `VK_KHR_surface`。
它提供了一个 `vk::SurfaceKHR` 窗口表面对象，该对象表示一种抽象的表面类型，用于呈现渲染后的图像。

实际上我们已经启用了此扩展，因为它在 `glfwGetRequiredInstanceExtensions` 返回的列表中。
该列表还包括我们将在接下来的几章中使用的其他 WSI 扩展。

> 注意 Vulkan 进行离屏渲染不需要窗口系统，也就不需要这些扩展。

## 添加表面成员变量

窗口表面需要在实例创建之后立即创建，因为它会影响物理设备的选择。
由于它还涉及了呈现和渲染的内容，推迟到现在才说明是为了使教程结构更清晰。

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

添加一个函数 `createSurface`，以便在实例创建和 `setupDebugMessenger` 之后从 `initVulkan` 中调用它。

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

## 创建窗口表面

虽然 `vk::raii::SurfaceKHR` 对象及其用法与平台无关，但其创建并非如此，因为它取决于窗口系统的详细信息。
创值得庆幸的是，所需的平台特定扩展已在 `glfwGetRequiredInstanceExtensions` 返回的列表中，无需手动添加。

我们将直接使用 GLFW 的 `glfwCreateWindowSurface`，它自动为我们处理平台的差异。但下面的代码还不能运行！！

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

> 这里 `*m_instance` 的 `*` 是运算符重载，返回内部 `VkInstance` 的引用。  
> 这没有内存泄漏风险，无需担心。

## 扩展队列族支持检查

Vulkan 支持窗口系统基础，但设备可能不支持。 我们需要扩展 `isDeviceSuitable` 以确保设备可以将图像呈现到我们创建的表面。

由于呈现是特定于队列的功能，因此此问题实际上是找到一个支持呈现到表面的队列族。

支持绘制命令的队列族和支持呈现的队列族可能不重叠。因此我们需要修改 `QueueFamilyIndices` 结构体，添加新内容

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

> 请注意，最终它们很可能是同一个队列族，但在整个程序中，我们将把它们视为独立的队列，以便采用统一的方法。  
> 尽管如此，您可以添加逻辑来显式地偏好在同一队列中支持绘制和呈现的物理设备，以提高性能。

## 创建呈现队列

剩下的最后一件事是修改逻辑设备创建过程，从而创建呈现队列。在 `m_graphicsQueue` 下方添加一个成员变量

```cpp
vk::raii::Queue m_presentQueue{ nullptr };
```

接下来，我们需要有多个 `CreateInfo` 结构体，以便从两个队列族创建队列。
一种优雅的方法是创建一个所需队列族的集合。让我们修改 `createLogicalDevice` 函数

```cpp
std::vector<vk::DeviceQueueCreateInfo> queueCreateInfos;
// isDeviceSuitable() ensure queue availability
std::set<uint32_t> uniqueQueueFamilies = {indices.graphicsFamily.value(), indices.presentFamily.value()};

float queuePriority = 1.0f;
for (uint32_t queueFamily : uniqueQueueFamilies) {
    queueCreateInfos.emplace_back( vk::DeviceQueueCreateInfo(
        {},                             // flags
        indices.graphicsFamily.value(), // queueFamilyIndex
        1,                              // queueCount
        &queuePriority    
    ));
}
```

并修改 `vk::DeviceCreateInfo` 以指向创建信息数组

```cpp
vk::DeviceCreateInfo createInfo(
    {},                         // flags
    queueCreateInfos.size(),    // queueCreateInfoCount
    queueCreateInfos.data()     // pQueueCreateInfos
);
```

最后，不要忘了在函数末尾添加创建队列的语句：

```cpp
m_presentQueue = m_device.getQueue( indices.presentFamily.value(), 0 );
```

如果队列族相同，则这两个句柄现在很可能具有相同的值，这依然可以正常运行。

## 测试

现在构建与运行程序，保证没有报错。

在下一章中，我们将研究交换链以及它们如何使我们能够将图像呈现到表面。

---

**[C++代码](../codes/0110_surface/main.cpp)**

**[C++代码差异](../codes/0110_surface/main.diff)**
