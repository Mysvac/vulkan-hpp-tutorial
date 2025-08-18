---
title: 接口介绍
comments: true
---
# **接口介绍**

> 本节主要讲述 C 与 C++ 接口的差异，您可以暂时跳过，感兴趣时再回来查看。

## **接口概述**

Vulkan 提供了两种编程接口风格：

1. **原生C接口**：定义在 `vulkan/vulkan.h` 头文件中

2. **C++封装**：Vulkan-hpp提供现代C++封装，位于：
    - `vulkan/vulkan.hpp`：基础封装，内部包含 `vulkan.h`
    - `vulkan/vulkan_raii.hpp`：基于 RAII 的资源管理封装

本章以 vulkan-hpp 封装为主，介绍其核心概念和使用方法。
你可以将本章视作接口的概览，暂时无需记住内容，可以结合后续章节的代码示例逐步理解。

## **命名风格**

### 1. 与C接口的命名差异

vulkan-hpp 封装和 C 接口都采样“大/小驼峰”方式命名，且类型名保持高度一致:

```cpp
VkInstance c_instance;  // C 接口中的实例类型
vk::Instance cpp_instance;  // C++ 封装中的实例类型，具有命名空间区分
```

### 2. 命名空间

vulkan-hpp 使用 `vk` 命名空间来组织所有类型和函数， RAII 封装处于 `vk::raii` 命名空间。
例：

```cpp
vk::Instance instance;  // vk 命名空间下的 Instance 类型
vk::SystemError error;  // vk 命名空间下的 SystemError 异常类型
vk::raii::Context context;  // RAII 封装在 vk::raii 命名空间下
```

### 3. 枚举类型

Vulkan-hpp 中的枚举值使用“类枚举”风格，枚举值以 `e` 开头，如果类名带 `Bits` 则表示通过其他类型实现了“仿”位运算。
例：

```cpp
auto flag_1 = vk::SwapchainCreateFlagBitsKHR::eDeferredMemoryAllocationEXT;
auto flag_2 = vk::SwapchainCreateFlagBitsKHR::eProtected;
vk::SwapchainCreateFlagKHR flags = flag_1 | flag_2;  // 仿位运算
```

全局和成员运算符 `|`、`&` 重载允许了这些枚举值进行位运算，`FlagBits` 运算的结果是对应的 `Flag` 类型。
`Flag` 实际是一个“类”而不是枚举类或整型数据，因此作者称其为“仿”位运算。

### 4. 全局常量

C 风格中有很多特殊值使用宏定义，而 vulkan-hpp 中使用全局常量代替这些宏，它们直接位于 `vk` 命名空间下。

```cpp
VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME // C 风格的宏定义
vk::KHRPortabilityEnumerationExtensionName ; // 这是 vulkan-hpp 中的提供全局常量
```

## **资源管理**

### 1. 基本概念

Vulkan 使用显式资源管理，所有资源（如实例、设备、缓冲区等）都需要手动创建和销毁。
所有的资源创建都需要一个 `info` 结构体来提供配置信息，如 `vk::InstanceCreateInfo`、`vk::DeviceCreateInfo`、`vk::MemoryAllocateInfo` 等。

### 2. C 风格资源管理

在 C 风格接口中，资源的创建和销毁通常分为这几步：

```cpp
VkInstanceCreateInfo createInfo = {};  // 创建信息
createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;  // 设置类型字段 sType
// 设置其他字段，如应用信息、扩展名等
VkInstance instance;
if (vkCreateInstance(&createInfo, nullptr, &instance) != VK_SUCCESS) {
    throw std::runtime_error("Failed to create instance");
} // 创建实例并检查是否成功
// 使用 instance 进行其他操作
vkDestroyInstance(instance, nullptr);  // 销毁实例
```

由于 C 没有类，结构体无法自动构造，你需要手动设置每个字段。

### 3. C++ 风格资源管理

C++ 的基础封装的使用与 C 类似，但类型提供了成员变量初始化，部分字段无需设置：

```cpp
vk::InstanceCreateInfo createInfo; // 创建信息，无需设置 sType
// 其他字段提供默认值，可修改
vk::Instance instance = vk::createInstance(createInfo); // 使用全局函数创建实例
// 使用 instance 进行其他操作
instance.destroy();  // 必须手动销毁实例，因为此对象只是一个句柄，并不管理资源
```

### 4. RAII 封装

RAII 封装对象使用普通 C++ 封装相同的 `info` 结构体，但提供了自动资源管理：

```cpp
vk::raii::Context context; // RAII 封装的上下文，初始化 RAII 封装需要的函数指针
vk::InstanceCreateInfo createInfo; // 创建信息，同 C++ 基础封装
vk::raii::Instance instance = context.createInstance(createInfo); // RAII 封装创建实例
// 使用 instance 进行其他操作
// RAII 封装会在析构时自动销毁实例
```

> RAII 对象通常使用父对象成员函数或自身的构造函数创建。

### 5. 异常处理

C 风格接口通常通过返回值来指示错误，而 C++ 封装使用异常处理机制：

```cpp
// C 风格中
if (vkCreateInstance(&createInfo, nullptr, &instance) != VK_SUCCESS) {
    // .... 处理错误
}

// C++ 封装中
try {
    vk::raii::Instance instance = context.createInstance(createInfo);
} catch (const vk::SystemError& e) {
    std::cerr << "Failed to create instance: " << e.what() << std::endl;
    // 处理异常
}
```

在部分情况下，C++ 封装也会使用错误码，我们会在后面的章节提及。

## **vk::raii基本原理**

`vk::raii` 是 Vulkan-hpp 中提供的 RAII 包装器，用于自动化资源管理。

### 核心设计理念

每个 `vk::raii::` 类型（如 `vk::raii::Semaphore`）内部存储对应的 Vulkan 原始句柄（如 `VkSemaphore`），在析构函数中调用相应的 `destroy` 函数释放资源。 RAII 类型通常：

- 默认构造函数通常被禁用（防止无效资源）
- 允许用 `nullptr` 构造（表示空资源）
- 禁用拷贝构造（小部分资源允许拷贝）
- 允许移动构造（资源管理权转移）

### 隐式转换

raii 类型可以隐式转换为非 raii 的仅句柄类型，此时资源依然由 raii 类型管理。

```cpp
vk::raii::Semaphore raii_semaphore = ...;  // RAII 对象
vk::Semaphore raw_semaphore = raii_semaphore;  // 隐式转换为原始类型
// 资源将在 raii_semaphore 销毁时释放
```

### 显式转换

使用 `*` 运算符可以将 raii 类型转换为 vulkan-hpp 的仅句柄类型，再用一次 `*` 则转换成 C 接口的仅句柄类型。
这实际是重载了成员运算符，在某些必须显式转换的场合很有用。

```cpp
vk::raii::Semaphore raii_semaphore = ...;
need_raw_semaphore(*raii_semaphore);  // *raii_semaphore 返回 vk::Semaphore&
VkSemaphore ctype_semaphore = **raii_semaphore;
```

## **核心差异总结**

| 特性     | C接口                | C++封装         |
|--------|--------------------|---------------|
| 命名空间   | 全局前缀(Vk/VK)        | vk命名空间        |
| 枚举值    | 宏定义                | 类枚举成员(e前缀)    |
| 资源创建   | 分离Create/Destroy函数 | 构造函数/RAII自动管理 |
| 错误处理   | 返回值检查              | 异常机制          |
| 扩展名    | 保持宏定义不变            | 保持宏定义不变       |
| 结构体初始化 | 需手动设置大部分内容         | 构造函数提供成员初始化   |

---
