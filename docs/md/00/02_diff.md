# **接口介绍**

## **接口概述**

Vulkan提供了两种编程接口风格：

1. **原生C接口**：定义在`vulkan/vulkan.h`头文件中

2. **C++封装**：Vulkan-hpp提供现代C++封装，位于：
    - `vulkan/vulkan.hpp`：基础封装
    - `vulkan/vulkan_raii.hpp`：RAII资源管理封装

> 注意：`vulkan.hpp`已包含原生头文件，无需重复引入。

## 接口风格对比

Vulkan-hpp封装与底层C风格接口略有不同，下面将为你粗略展示二者的差异。

**你目前不需要记住这些内容**，因为在后续的代码编写过程中，你很快就会熟悉它们。


### C风格接口特点

1. **命名规范**：
    - 所有类型/宏以`VK`或`Vk`开头
    - 示例：`VkFormat`, `VkInstance`, `VkXxxxCreateInfo`

2. **常量表示**：
    - 使用宏定义布尔值、扩展名和枚举值等
    - 示例：`VK_TRUE`, `VK_EXT_DEBUG_UTILS_EXTENSION_NAME`

3. **错误处理**：
    - 通过返回值`VkResult`判断执行状态
    - 输出参数通过指针返回

4. **资源管理**：
    ```cpp
    // 创建CreateInfo结构体并填写参数
    VkInstanceCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    // ...flags和其他参数设置

    // 创建对象并检查错误
    VkInstance instance;
    if (vkCreateInstance(&createInfo, nullptr, &instance) != VK_SUCCESS) {
        throw std::runtime_error("创建失败");
    }

    // 必须手动释放
    vkDestroyInstance(instance, nullptr);
    ```

> **为何如此复杂？**  
> 因为底层接口使用C编写，没有命名空间，没有类，没有错误处理和RAII。

### C++风格接口特点

1. **命名空间**：
    - 所有类型位于`vk`命名空间
    - RAII封装位于`vk::raii`

2. **改进特性**：
    - 使用 `Flags` 、`Bit` 、`e` 区分特殊的枚举类型。
    - 构造函数和资源创建函数直接返回对象
    - 提供RAII支持，无需手动清理资源
    - 使用异常机制处理错误
    - 更加现代的语法（成员函数，命名空间，默认参数...）

3. **资源管理示例**：
    ```cpp
    try {
        vk::InstanceCreateInfo createInfo(
            // sType无需设置
            ... // flags和其他参数提供默认构造
        );

        // RAII方式创建
        vk::raii::Instance instance = context.createInstance(createInfo);

        // 自动管理生命周期
    } catch (const vk::SystemError& err) {
        // 异常处理
    }
    ```

## **vk::raii基本原理**

`vk::raii::` 是 Vulkan-HPP 中提供的 RAII 包装器，用于自动化资源管理。

### 核心设计理念

1. **资源封装**：
    - 每个 `vk::raii::` 类型（如 `vk::raii::Semaphore`）内部存储对应的 Vulkan 原始类型（如 `vk::Semaphore`）的指针
    - 在析构时自动调用相应的 `destroy` 或释放函数

2. **构造控制**：
    - 默认构造函数通常被禁用（防止无效资源）
    - 允许用 `nullptr` 构造（表示空资源）
    - 禁用拷贝构造（小部分资源允许拷贝）
    - 允许移动构造（资源管理权转移）


### 隐式转换

```cpp
vk::raii::Semaphore semaphore = ...;  // RAII 对象
vk::Semaphore rawSemaphore = semaphore;  // 隐式转换为原始类型
```

> 注意 `vk::Semaphore`只是句柄，没有资源。资源将在 `vk::raii::Semaphore` 被析构时释放。

### 显式转换

当需要多次转换时，可以使用`*`运算符进行显式转换，这实际是重载了成员运算符。

```cpp
vk::raii::Semaphore semaphore = ...;
someFunction(*semaphore);  // 使用 * 运算符获取内部原始引用
// *semaphore 返回 vk::raii::Semaphore&
```


## **核心差异总结**

| 特性            | C接口                          | C++封装                       |
|-----------------|-------------------------------|------------------------------|
| 命名空间        | 全局前缀(Vk/VK)               | vk命名空间                   |
| 枚举值          | 宏定义                        | 类枚举成员(e前缀)            |
| 资源创建        | 分离Create/Destroy函数        | 构造函数/RAII自动管理        |
| 错误处理        | 返回值检查                    | 异常机制                     |
| 扩展名          | 保持宏定义不变                | 保持宏定义不变               |
| 结构体初始化    | 需手动设置sType               | 构造函数自动处理             |

> 提示：本教程将主要使用C++ RAII封装，既保持现代C++风格，又能简化资源管理。

---

下面让我们开始绘制第一个三角形！！

---
