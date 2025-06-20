# **物理设备与队列族**

在创建实例之后，我们需要查找并选择系统中合适的物理设备\(通常为 GPU \)。
实际上，我们可以选择任意数量的显卡并同时使用它们，但在本教程中，我们只使用第一张满足我们需求的显卡。

## **选择物理设备**

### 1. 成员变量与辅助函数

首先在 `m_debugMessenger` 下方添加一个成员变量管理物理设备句柄：

```cpp
vk::raii::PhysicalDevice m_physicalDevice{ nullptr };
```

然后添加一个函数 `pickPhysicalDevice`，并在 `initVulkan` 函数中调用它：

```cpp
void initVulkan() {
    createInstance();
    setupDebugMessenger();
    pickPhysicalDevice();
}

void pickPhysicalDevice() {

}
```

### 2. 获取可用设备列表

```cpp
void pickPhysicalDevice() {
    // std::vector<vk::raii::PhysicalDevice>
    auto physicalDevices = m_instance.enumeratePhysicalDevices();
    if(physicalDevices.empty()){
        throw std::runtime_error("failed to find GPUs with Vulkan support!");
    }
}
```

> 你可能见过 `vk::raii::PhysicalDevices` 类型，它末尾多了个 `s`。  
> 他实际上继承了 `std::vector<vk::raii::PhysicalDevice>` ，二者功能基本一致。

### 3. 设备适用性检查

现在我们需要评估每个设备，并检查它们是否适合满足要求。为此我们引入一个新函数：

```cpp
bool isDeviceSuitable(const vk::raii::PhysicalDevice& physicalDevice) {
    return true;
}
```

### 4. 选择第一个合适的设备

遍历并挑选一个满足的物理设备，如果没有就抛出异常：

```cpp
for (const auto& it : physicalDevices) {
    if (isDeviceSuitable(it)) {
        m_physicalDevice = it;
        break;
    }
}
if(m_physicalDevice == nullptr){
    throw std::runtime_error("failed to find a suitable GPU!");
}
```

注意到 `vk::raii::PhysicalDevice` 可以直接拷贝赋值，这很特殊。
因为物理设备句柄实际由 `vk::Instance` 管理，所以 `vk::raii::PhysicalDevice` 销毁时没有调用任何 `vkDestory` 。

## **设备评估标准**

### 1. 设备属性与特性查询

在 `isDeviceSuitable` 函数中，我们可以这样获取物理设备的属性：

```cpp
vk::PhysicalDeviceProperties properties = physicalDevice.getProperties();
vk::PhysicalDeviceFeatures features = physicalDevice.getFeatures();
```

- `Properties` : 基本设备属性，例如名称、类型和支持的 Vulkan 版本。
- `Features` : 可选功能（如纹理压缩、64 位浮点数和多视口渲染）的支持。

假设我们的应用程序需要支持几何着色器的独立显卡。那么可以这样写

```cpp
bool isDeviceSuitable(const vk::raii::PhysicalDevice& physicalDevice) {
    auto properties = physicalDevice.getProperties();
    auto features = physicalDevice.getFeatures();

    return features.geometryShader && 
        properties.deviceType == vk::PhysicalDeviceType::eDiscreteGpu;
}
```

### 2. 评分机制示例

你还可以创建自己的评分机制，然后挑选最好的显卡，像这样：

```cpp
int rateDeviceSuitability(const vk::raii::PhysicalDevice& physicalDevice) {
    auto properties = physicalDevice.getProperties();
    auto features = physicalDevice.getFeatures();

    // 必要性功能检查
    if (!features.geometryShader) {
        return 0;
    }

    int score = 0;
    // 独立显卡加分
    if (properties.deviceType == vk::PhysicalDeviceType::eDiscreteGpu) {
        score += 1000;
    }
    // 最大纹理尺寸影响图形质量加分
    score += properties.limits.maxImageDimension2D;

    return score;
}
```

###  注意！！

作为教程的开始部分，我们目前仅需要 Vulkan 支持，下面暂时使用这个最简单的判断函数：

```cpp
bool isDeviceSuitable(const vk::raii::PhysicalDevice& physicalDevice) {
    return true;
}
```

在下一节，我们将讨论第一个真正需要检查的功能。

## **队列族管理**

Vulkan 中的几乎每个操作，从绘制到上传纹理，都需要将命令提交到队列。

### 1. 队列查找函数

队列有不同的类型，这些类型源自不同的队列族，并且每个队列族仅允许一些特定的命令。
例如，可能有一个队列族仅允许处理计算命令，或者一个队列族仅允许与内存传输相关的命令。

为此，我们将添加一个新函数 `findQueueFamilies` ，用于查找我们需要的所有队列族。

现在我们只打算查找支持图形命令的队列族，因此该函数可能如下所示：

```cpp
uint32_t findQueueFamilies(const vk::raii::PhysicalDevice& physicalDevice) {
    // 查找图像队列族
}
```

队列族的查找返回整数索引，由于我们后面需要找的队列不止一个，可以返回一个结构体：

```cpp
struct QueueFamilyIndices {
    uint32_t graphicsFamily;
};

QueueFamilyIndices findQueueFamilies(const vk::raii::PhysicalDevice& physicalDevice) {
    QueueFamilyIndices indices;
    // 查找图像队列族
    return indices;
}
```

### 2. 更好的队列存储

此函数可能找不到有用的队列族。
但是有时候找不到也可以正常执行，比如我们可能希望使用具有专用传输队列族的设备，但不强制要求。

不应该使用魔术值来指示队列族的不存在，因为 `uint32_t` 的任何值都可能是有效的队列族索引，包括 `0`。
幸运的是，C++17 引入了一种数据结构 `std::optional<>` 来区分值存在与不存在的情况，它可以这样使用：

```cpp
std::optional<uint32_t> graphicsFamily;

std::cout << std::boolalpha << graphicsFamily.has_value() << std::endl; // false

graphicsFamily = 0;

std::cout << std::boolalpha << graphicsFamily.has_value() << std::endl; // true
```

于是我们可以将代码修改成这样：

```cpp
......

#include <optional>

......

struct QueueFamilyIndices {
    std::optional<uint32_t> graphicsFamily;
};

QueueFamilyIndices findQueueFamilies(const vk::raii::PhysicalDevice& physicalDevice) {
    QueueFamilyIndices indices;
    // Assign index to queue families that could be found
    return indices;
}
```

### 3. 实现队列族查找

我们现在可以开始实现 `findQueueFamilies`，使用 `getQueueFamilyProperties` 成员函数即可：

```cpp
// std::vector<vk::QueueFamilyProperties>
auto queueFamilies = physicalDevice.getQueueFamilyProperties();
```

`vk::QueueFamilyProperties`只包含基本信息，包括支持的操作类型以及该族可创建的队列数量，但在这里已经足够了。

我们需要找到至少一个支持 `vk::QueueFlagBits::eGraphics` 的队列族。

```cpp
for (int i = 0; const auto& queueFamily : queueFamilies) {
    if (queueFamily.queueFlags & vk::QueueFlagBits::eGraphics) {
        indices.graphicsFamily = i;
    }

    ++i;
}
```

> 这里用到了C++20的初始化语句。

### 4. 改进设备适用性检查

现在我们可以在 `isDeviceSuitable` 函数中使用它作为检查，以确保设备可以处理我们想要使用的命令：

```cpp
bool isDeviceSuitable(const vk::raii::PhysicalDevice& physicalDevice) {
    QueueFamilyIndices indices = findQueueFamilies(physicalDevice);

    return indices.graphicsFamily.has_value();
}
```

为了更方便一点，我们可以在结构体中添加一个通用检查：

```cpp
struct QueueFamilyIndices {
    std::optional<uint32_t> graphicsFamily;

    bool isComplete() {
        return graphicsFamily.has_value();
    }
};

......

bool isDeviceSuitable(const vk::raii::PhysicalDevice& physicalDevice) {
    QueueFamilyIndices indices = findQueueFamilies(physicalDevice);

    return indices.isComplete();
}
```

我们现在可以使用它，在需要的时候从 `findQueueFamilies` 中提前退出：

```cpp
for (int i = 0; const auto& queueFamily : queueFamilies) {
     ...

    if (indices.isComplete()) {
        break;
    }

    ++i;
}
```

> 为什么不直接赋值后就退出？因为我们后面还需查找其他队列族。

## **测试**

现在构建与运行代码，虽然程序还是和之前一样的效果，但不应报错。

---

我们现在足以找到合适的物理设备，下一步是创建逻辑设备以与之交互。

---

**[C++代码](../../codes/01/03_physicaldevice/main.cpp)**

**[C++代码差异](../../codes/01/03_physicaldevice/main.diff)**

**[CMake代码](../../codes/01/00_base/CMakeLists.txt)**
