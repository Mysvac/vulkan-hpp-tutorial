# Vulkan逻辑设备
在选择要使用的物理设备之后，我们需要设置一个逻辑设备来与之交互。

创建逻辑设备，你需要：

1. 指定需要创建的队列
2. 指定要启用的设备特性
3. 验证层设置（兼容性考虑）

如果您有需要，甚至可以从同一个物理设备创建多个逻辑设备。

## 基础结构
首先添加一个新的类成员 `m_device` 来存储逻辑设备句柄。
现在你的成员变量顺序应该是

```cpp
GLFWwindow* m_window{ nullptr };
vk::raii::Context m_context;
vk::raii::Instance m_instance{ nullptr };
vk::raii::DebugUtilsMessengerEXT m_debugMessenger{ nullptr };
vk::raii::PhysicalDevice m_physicalDevice{ nullptr };
vk::raii::Device m_device{ nullptr };
```

接下来，添加一个从 `initVulkan` 调用的 `createLogicalDevice` 函数。

```cpp
void initVulkan() {
    createInstance();
    setupDebugMessenger();
    pickPhysicalDevice();
    createLogicalDevice();
}

void createLogicalDevice() {

}
```

## 指定队列创建信息

首先指定需要创建的队列，创建 `vk::DeviceQueueCreateInfo`。

```cpp
QueueFamilyIndices indices = findQueueFamilies( m_physicalDevice );

vk::DeviceQueueCreateInfo queueCreateInfo(
    {},                             // flags
    indices.graphicsFamily.value(), // queueFamilyIndex
    1                               // queueCount
);
```

- `queueFamilyIndex`：从物理设备查询获得的队列族索引
- `queueCount`：要从队列族创建的队列数

> 当前只允许您为每个队列族创建少量队列，但其实一个就够了。因为您可以在多个线程上创建命令缓冲区，然后使用主线程一次性提交它们。


Vulkan 允许您使用介于 `0.0` 和 `1.0` 之间的浮点数为队列分配优先级，以影响命令缓冲区执行的调度。即使只有一个队列，这也是必需的。 

```cpp
float queuePriority = 1.0f;
queueCreateInfo.pQueuePriorities = &queuePriority;
```

> 注意传入的是浮点数指针，你需要保证使用时指针有效。我也不知道为什么这么设计。

## 指定设备特性

目前我们不需要任何特殊的东西，可以直接使用默认值。 当我们开始使用 Vulkan 做更有趣的事情时，会回来修改到这个结构。

```c++
vk::PhysicalDeviceFeatures deviceFeatures;
```

## 创建逻辑设备

有了前面的两个结构，我们就可以开始填写主要的 `vk::DeviceCreateInfo` 结构了。

```cpp
vk::DeviceCreateInfo createInfo(
    {},                 // flags
    1,                  // queueCreateInfoCount
    &queueCreateInfo    // pQueueCreateInfos
);
createInfo.pEnabledFeatures = &deviceFeatures;
```

其余信息与 `vk::InstanceCreateInfo` 结构相似，并要求您指定扩展和验证层。不同之处在于这次这些是设备特定的。

之前提到，现在验证层已不再区分实例和特定，这意味着下面的设置将被忽略。但为了与旧的实现兼容，最好还是设置它们。

```cpp
if (enableValidationLayers) {
    createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
    createInfo.ppEnabledLayerNames = validationLayers.data();
}
```

目前我们不需要任何设备特定的扩展。

就这样，我们可以实例化逻辑设备了。
```c++
m_device = m_physicalDevice.createDevice( createInfo );
```

## 检索队列句柄

队列与逻辑设备一起自动创建，但我们还没有句柄来与之交互。首先添加一个类成员来存储图形队列的句柄
```c++
vk::raii::Queue m_graphicsQueue{ nullptr };
```

我们可以使用 `device->getQueue` 成员函数来检索每个队列族的队列句柄。
参数是队列族和队列索引。因为我们只从此队列族创建一个队列，所以简单地使用索引 `0`。
```c++
m_graphicsQueue = m_device.getQueue( indices.graphicsFamily.value(), 0 );
```

## 测试

现在运行程序，确保没有报错。

有了逻辑设备和队列句柄，我们现在实际上可以开始使用显卡来做事了！在接下来的几章中，我们将设置资源以将结果呈现到窗口系统。

---

**[C++代码](../codes/0104_device/main.cpp)**

**[C++代码差异](../codes/0104_device/main.diff)**
