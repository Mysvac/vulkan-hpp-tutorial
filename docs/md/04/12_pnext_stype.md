---
title: pNext与sType
comments: true
---

# **pNext与sType**

## **前言**

`pNext` 是 Vulkan API 中的一个重要概念，它是指向下一个结构体的 `void*` 指针，用于扩展结构体功能。`void *` 丢失了类型信息，因此需要使用 `sType` 成员来标识结构体的类型。

> C 接口的结构体需要手动设置 `sType` ， C++ 封装的类构造函数会自动设置 `sType` 成员。

我们在创建验证层时就使用了它，将实例创建信息的 `pNext` 指向了验证层的创建信息结构体。

这种方式允许开发者在现有结构体的基础上添加自定义数据或使用扩展功能。
我们虽然不会为 Vulkan 增加新功能，但依然需要了解它的原理，以便在使用其他扩展时能够正确处理 `pNext` 链。

## **基础代码**

本章代码使用“C++模块化”章节**第一部分**的最终代码，即仅 Vulkan 模块化的代码：

**[点击下载](../../codes/04/00_cxxmodule/vk_module_demo.zip)**

`pNext` 链不需要窗口渲染，我们直接使用一个简单项目演示。

## **基本原理**

Vulkan 中的此类结构体，都保证 `sType` 在结构体的第一个位置， `pNext` 位于第二个位置。
因此可以使用 `pNext` 构成单向链表（最后一个 `pNext` 指向 `nullptr`），然后根据 `sType` 判定类型。

> 并非所有配置信息都支持 `pNext` 链，比如 `vk::ApplicationInfo` 就没有 `pNext` 成员。

你可以设计一个结构体，包含 `sType` 和 `pNext` 成员，然后强制转换其它数据的类型：

```cpp
import std;
import vulkan_hpp;

struct MyInfo {
    vk::StructureType sType;
    void* pNext;
};

int main() {
    const vk::raii::Context ctx;
    constexpr vk::InstanceCreateInfo create_info;
    // 强制转换为 MyInfo 结构体
    const auto my_info = reinterpret_cast<const MyInfo *>( &create_info );
    std::println("sType is app info ? {}", my_info->sType == vk::StructureType::eApplicationInfo);
    // ↑ false
    std::println("sType is instance info ? {}", my_info->sType == vk::StructureType::eInstanceCreateInfo);
    // ↑ true
    std::println("pNext is nullptr ? {}", my_info->pNext == nullptr);
    // ↑ true
}
```

可以使用非常粗暴的 `switch/if` 语句判断 `sType` 的值，根据不同的值调用不同的 `reinterpret_cast` ，即可访问完整的结构体信息：

```cpp
switch (my_info->sType) {
    case vk::StructureType::eDeviceCreateInfo: {
        const auto tmp = reinterpret_cast<const vk::DeviceCreateInfo *>( &my_info );
        std::println("{}", tmp->enabledLayerCount);
    } break;
    case vk::StructureType::eInstanceCreateInfo: {
        const auto tmp = reinterpret_cast<const vk::InstanceCreateInfo *>( &my_info );
        std::println("{}", tmp->enabledExtensionCount);
    } break;
    default: break;
}
```

## **迭代器**

Vulkan 提供了两个类型用于遍历它们：`vk::BaseInStructure` 和 `vk::BaseOutStructure`。

它们的结构与上面的 `MyInfo` 几乎一样，二者的区别在于 `pNext` 指针的类型不同：

- `vk::BaseInStructure：const void* pNext`; 只读，用于遍历输入链（如创建信息），不能修改链内容。
- `vk::BaseOutStructure：void* pNext`; 可写，用于遍历输出链（如查询结果），可以修改链内容。

所以你可以这样使用它们：

```cpp
import std;
import vulkan_hpp;

// 读取 pNext 链（只读）
void print(const vk::BaseInStructure* p) {
    while (p) {
        std::cout << "sType: " << static_cast<int>(p->sType) << std::endl;
        p = p->pNext;
    }
}

// 构建/修改 pNext 链（可写），在链的末尾添加一个节点
void push_back(vk::BaseOutStructure* head, vk::BaseOutStructure* node) {
    while (head->pNext) {
        head = static_cast<vk::BaseOutStructure*>(head->pNext);
    }
    head->pNext = node;
    node->pNext = nullptr;
}

int main() {
    const vk::raii::Context ctx;
    vk::InstanceCreateInfo create_info;

    vk::DebugUtilsMessengerCreateInfoEXT debug_info;
    push_back(reinterpret_cast<vk::BaseOutStructure*>(&create_info),
              reinterpret_cast<vk::BaseOutStructure*>(&debug_info));
    vk::BufferCreateInfo buffer_info;
    push_back(reinterpret_cast<vk::BaseOutStructure*>(&create_info),
              reinterpret_cast<vk::BaseOutStructure*>(&buffer_info));

    print(reinterpret_cast<const vk::BaseInStructure*>(&create_info));
}
```

## **获取 GPU 特性**

实际上我们一般不需要手动遍历 `pNext` 链，但通过上面的学习，你可以更好的理解它，

下面以新版功能查询 `PhysicalDeviceFeatures2` 为你演示 `pNext` 在 Vulkan 中的使用。

### 1. 旧版语法

在 Vulkan 1.0 版本中，我们是这样查询物理设备特性的：

```cpp
const auto physical_devices = instance.enumeratePhysicalDevices();
for (const auto& it : physical_devices) {
    const vk::PhysicalDeviceFeatures features = it.getFeatures();
    std::println("{}", features.alphaToOne);
    std::println("{}", features.geometryShader);
    std::println("{}", features.tessellationShader);
    // ...
}
```

`features` 结构体包含 Vulkan 1.0 的 GPU 特性列表，每个成员变量都对应一个特性支持情况。

现在 Vulkan 已经更新到 1.4 了，如果每次大版本更新都修改 `PhysicalDeviceFeatures` 类型和相关代码，则会出现很多兼容性问题。
另外，此方式显然只能查询官方提供的特性，结构体不会包含某些显卡厂商提供的扩展或自定义特性。

### 2. 新版语法

一种好的方式是，将 Vulkan 每个版本的特性放在独立的结构体中，然后通过 `pNext` 连成链表，并一次性填充全部内容。

`PhysicalDeviceFeatures2` 做的就是这么一件事情，它内部包含 `PhysicalDeviceFeatures` 结构体，即 1.0 版本的特性。
然后它还可以通过 `pNext` 链接其他结构体，比如 `PhysicalDeviceVulkan11Features` ，即 1.1 版本的特性信息。

将它们都链起来后，就可以通过 `getFeatures2` 一次性填充所有特性信息：

```cpp
// 注意使用非 RAII 的物理设备对象
vk::PhysicalDevice physical_device = ...;
vk::PhysicalDeviceFeatures2 features2;
vk::PhysicalDeviceVulkan11Features features11;
vk::PhysicalDeviceVulkan12Features features12;
vk::PhysicalDeviceVulkan13Features features13;
features2.setPNext(&features11);
features11.setPNext(&features12);
features12.setPNext(&features13);
physical_device.getFeatures2(&features2);
// ↑ 自动填充四个版本的全部特征
```

注意，上面的物理设备是 `vk::PhysicalDevice` 类型，而不是 `vk::raii::PhysicalDevice` 。
如果你是使用本章节提供的框架，此代码可能无法运行，因为此函数需要 `dispatch loader` ，而它被我们的 `vk::raii::Context` 管理。

### 3. RAII 语法

我们使用 RAII 封装，它提供了更好的可用语法：

```cpp
const std::vector<vk::raii::PhysicalDevice> physical_devices = instance.enumeratePhysicalDevices();
for (const auto& it : physical_devices) {
    // vk::StructureChain< ... >
    const auto features = it.getFeatures2<  // 使用 RAII 的物理设备对象
        vk::PhysicalDeviceFeatures2,
        vk::PhysicalDeviceVulkan11Features,
        vk::PhysicalDeviceVulkan12Features,
        vk::PhysicalDeviceVulkan13Features
    >();
    
}
```

RAII 封装提供了一个模板函数，此函数返回一个 `vk::StructureChain<>` 对象。
它根据模板形参列表，在内部自动生成了一条 `pNext` 链并调用底层 C 函数。

> RAII 封装通过这种方式尽量减少用户需要进行的指针操作。

我们可以这样获取内部元素：

```cpp
const auto feature2 = features.get(); // 获取第一个元素可以省略模板
// const auto feature2 = features.get<vk::PhysicalDeviceFeatures2>(); 同上
const auto feature11 = features.get<vk::PhysicalDeviceVulkan11Features>();
const auto feature12 = features.get<vk::PhysicalDeviceVulkan12Features>();
const auto feature13 = features.get<vk::PhysicalDeviceVulkan13Features>();

std::println("{}", feature2.features.shaderFloat64);
std::println("{}", feature11.storageInputOutput16);
std::println("{}", feature12.descriptorIndexing);
std::println("{}", feature13.synchronization2);
```

### 4. 启用 GPU 特性

上面已经介绍了如何查询高版本可用特性，那么如何启用它们呢？

显然我们需要使用 `pNext` 链将特性链接起来，但 `PhysicalDeviceFeatures` 字段并没有 `pNext` 成员，因此我们需要将特性链接到 `vk::DeviceCreateInfo` 上。


```cpp
vk::StructureChain<
    vk::DeviceCreateInfo,   // 以 CreateInfo 作为链的起点
    vk::PhysicalDeviceFeatures2, // 注意是 Feature2 才有 pNext 字段
    vk::PhysicalDeviceVulkan12Features
> create_info_chain;

create_info_chain.get() // 模板空默认返回链起点
    .setQueueCreateInfos( queue_create_infos )  // set 返回自身引用，可以链式调用
    .setPEnabledExtensionNames( vk::KHRSwapchainExtensionName );
create_info_chain.get<vk::PhysicalDeviceFeatures2>().features
    .setSamplerAnisotropy( true );  // 设置需要启用的 1.0 版本特性
create_info_chain.get<vk::PhysicalDeviceVulkan12Features>()
    .setTimelineSemaphore( true );  // 设置需要的高版本特性

// 创建逻辑设备
m_device = m_physical_device.createDevice( create_info_chain.get() );
```

你甚至可以查询全部 GPU 可用特性，然后将 `CreateInfo` 的 `pNext` 成员指向它，直接启用全部可用特性：

```cpp
const auto features = m_physical_device.getFeatures2<  // 使用 RAII 的物理设备对象
    vk::PhysicalDeviceFeatures2,
    vk::PhysicalDeviceVulkan11Features,
    vk::PhysicalDeviceVulkan12Features,
    vk::PhysicalDeviceVulkan13Features
>();

vk::DeviceCreateInfo create_info;
// 设置需要的扩展名 和 队列创建信息 ......
// 将 pNext 链指向需要启用的高版本特性链
create_info.pNext = &features.get();

m_device = m_physical_device.createDevice( create_info );
```


## **最后**

Vulkan 还有很多以 `2` 结尾的 API ，它们都用到了这项技术。 
虽然我们目前只介绍了 GPU 特征查询，但通过本章“原理”部分的学习，你应该很容易学会其他这些 API 的使用。

---

