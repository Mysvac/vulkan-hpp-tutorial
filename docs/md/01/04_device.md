---
title: 逻辑设备与队列
comments: true
---
# **逻辑设备与队列**

## **创建逻辑设备**

在选择合适的物理设备之后，我们需要设置一个逻辑设备从而与之交互。
在“教程前言”章节提到，逻辑设备是物理设备的抽象，如果您有需要，甚至可以从同一个物理设备创建多个逻辑设备。

### 1. 基础结构

首先添加一个新的类成员 `m_device` 来存储逻辑设备句柄。
现在你的成员变量顺序应该是：

```cpp
GLFWwindow* m_window{ nullptr };
vk::raii::Context m_context;
vk::raii::Instance m_instance{ nullptr };
vk::raii::DebugUtilsMessengerEXT m_debugMessenger{ nullptr };
vk::raii::PhysicalDevice m_physicalDevice{ nullptr };
vk::raii::Device m_device{ nullptr };
```

接下来，添加一个 `createLogicalDevice` 函数并在 `initVulkan` 中调用它：

```cpp
void initVulkan() {
    createInstance();
    setupDebugMessenger();
    selectPhysicalDevice();
    createLogicalDevice();
}

void createLogicalDevice() {

}
```

### 2. 队列创建信息

队列将和逻辑设备同时创建，队列资源由逻辑设备管理，因此我们需要指定队列创建信息，依靠 `vk::DeviceQueueCreateInfo` 结构体：

```cpp
vk::DeviceQueueCreateInfo queueCreateInfo;
```

首先需要指定从哪个队列族中创建队列：

```cpp
const auto indices = findQueueFamilies( m_physicalDevice );
queueCreateInfo.queueFamilyIndex = indices.graphicsFamily.value();
```

还需要指定创建队列的数量，并使用 `0.0` 和 `1.0` 之间的浮点数为它们分配优先级，以影响命令缓冲区执行的调度。即使只有一个队列，这也是必需的。 

```cpp
constexpr float queuePriority = 1.0f;
queueCreateInfo.setQueuePriorities( queuePriority );
```

注意函数内部使用了浮点数的指针，所以不能传入浮点字面量。

> 当前只允许您为每个队列族创建少量队列，但其实一个就够了。因为您可以在多个线程上创建命令缓冲区，然后使用主线程一次性提交它们。

### 3. 指定设备特性

目前我们不需要任何特殊的东西，可以直接使用默认值。 当我们开始使用 Vulkan 做更有趣的事情时，会回来修改到这个结构。

我们之前提到过，物理设备还有很多可选特性，它们的支持性取决于具体的 GPU 硬件。
如果需要使用这些特性，要在逻辑设备创建时显式启用它们，但我们现在还没有需要的特性。

```cpp
vk::PhysicalDeviceFeatures deviceFeatures;
```

后续章节提到的“需要启用 GPU 特性”指得就是这里。

### 4. 创建信息

有了前面的两个结构，我们就可以填写 `vk::DeviceCreateInfo` 结构了。

```cpp
vk::DeviceCreateInfo createInfo;
createInfo.setQueueCreateInfos( queueCreateInfo );
createInfo.pEnabledFeatures = &deviceFeatures;
```

其余信息与 `vk::InstanceCreateInfo` 结构相似，并要求您指定扩展。不同之处在于这次这些是设备特定的。

> 注意，虽然设备特定的“层”已废弃，但仍然需要指定设备特定的扩展与设备特性。

### 5. 验证层

设备特定的层很早就被废弃，你无需也不应该使用它们，以下代码将被 Vulkan 忽略。

```cpp
if constexpr (ENABLE_VALIDATION_LAYER) {
    // 无效代码
    createInfo.setPEnabledLayerNames( REQUIRED_LAYERS );
}
```

### 6. 创建逻辑设备

目前我们不需要任何设备特定的扩展，可以实例化逻辑设备了：

```cpp
m_device = m_physicalDevice.createDevice( createInfo );
```

## **获取队列句柄**

队列与逻辑设备一起自动创建，但我们还没有句柄以与之交互。现在添加一个类成员来存储图形队列的句柄：

```cpp
vk::raii::Queue m_graphicsQueue{ nullptr };
```

我们可以使用 `getQueue` 成员函数来获取每个队列族的队列句柄。
参数是队列族和队列索引。因为我们只从此队列族创建了一个队列，所以简单地使用索引 `0` 。
```cpp
m_graphicsQueue = m_device.getQueue( indices.graphicsFamily.value(), 0 );
```

## **测试**

现在运行程序，确保没有报错。

---

**[C++代码](../../codes/01/04_device/main.cpp)**

**[C++代码差异](../../codes/01/04_device/main.diff)**

**[CMake代码](../../codes/01/00_base/CMakeLists.txt)**

---
