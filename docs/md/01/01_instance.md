---
title: 上下文与实例
comments: true
---
# **Vulkan实例**

## **RAII上下文初始化**

`vk::raii::Context` 的作用是 **初始化 Vulkan 的动态加载层\(Loader\)**，自动加载全局函数指针，它是 RAII 封装的基础。

> 在C风格接口中，部分扩展函数需要通过加载函数获取函数指针，无法直接通过函数名调用。  
> 而 `vk::raii::Context` 隐藏了这些加载操作，让我们可以直接调用相关类的成员函数。

- 我们必须初始化它，且只初始化一次
- 保证它的生命周期覆盖其他 Vulkan 组件
- 可以无参构造，不可 `nullptr` 构造（特殊）

根据上面的要求，我们可以将它作为成员变量，在类实例化时自动创建并加载上下文：

```cpp
private:
    /// class member
    GLFWwindow* m_window{ nullptr };
    vk::raii::Context m_context;
```

现在直接构建与运行程序，请保证程序不出错。

> RAII机制参考“接口介绍”章节。


## **创建实例**

还需要创建一个**实例**来初始化 Vulkan 库，实例是您的应用程序和 Vulkan 库之间沟通的桥梁。

### 1. 创建成员变量和辅助函数

我们需要一个 `vk::raii::Instance` 对象管理 Vulkan 实例，通过辅助函数创建它：

```cpp
/// class member
GLFWwindow* m_window{ nullptr };
vk::raii::Context m_context;
vk::raii::Instance m_instance{ nullptr };

void initVulkan() {
    createInstance();
}

void createInstance(){

}
```

> 之前提到大部分 `raii` 资源不支持无参构造，要使用 `nullptr` 初始化表示无资源。

### 2. 添加应用程序信息

然后添加应用程序信息结构体，它是可选的，但是填写它能够让驱动程序更好的进行优化。

```cpp
void createInstance(){
    vk::ApplicationInfo applicationInfo( 
        "Hello Triangle",   // pApplicationName
        1,                  // applicationVersion
        "No Engine",        // pEngineName
        1,                  // engineVersion
        vk::makeApiVersion(0, 1, 4, 0)  // apiVersion
    );
}
```

> 版本号也可直接用宏 `VK_API_VERSION_X_X` ，API 版本可以低于 Vulkan 版本，但不能高于。

注意到，它并不是RAII的，因为它只是个配置信息，不含特殊资源。
所以我们可以无参构造，然后直接修改成员变量，像这样：

```cpp
vk::ApplicationInfo applicationInfo;
applicationInfo.pApplicationName = "xxxx";  // 可以直接修改成员变量
applicationInfo.setApplicationVersion(1);   // 也可以使用 setter 函数
```

> 后续教程为方便讲解，将同时使用这两种方式！

### 2. 配置基础创建信息

Vulkan中资源的创建都依赖对应的 `CreateInfo` 结构体，我们必须先填写它。

```cpp
vk::InstanceCreateInfo createInfo( 
    {},                 // vk::InstanceCreateFlags
    &applicationInfo    // vk::ApplicationInfo*
);
```

`flags` 参数是标志位，用于控制特殊行为，默认认初始化为空，大多时候无需修改。
还有其他参数，但都提供了默认初始化，无需手动设置。

注意到，`&applicationInfo`传入指针，需要注意生命周期！

1. `CreateInfo`仅用于提供配置信息。
2. `CreateInfo`在创建对应资源后就无用了。
3. 只需要保证指针在创建对象时有效。

### 3. 创建实例

```cpp
// 现在要求applicationInfo和createInfo还存在
m_instance = m_context.createInstance( createInfo );
// 现在不再需要createInfo和applicationInfo，可以销毁资源
```

> 创建实例失败会抛出 `vk::SystemError` 异常，而我们已在主函数捕获，无需在此处理。

实际创建方式有2种，构造函数和父对象成员函数。 所以你还可以像这样创建：
```cpp
m_instance = vk::raii::Instance( m_context, createInfo );
```

> 本文档统一使用成员函数创建子对象。

## **添加扩展以支持GLFW**

Vulkan 是一个平台无关的 API，这意味着您需要一个扩展来处理窗口系统接口。

### 1. 获取所需扩展

GLFW 有一个方便的内置函数，可以返回它需要的扩展，我们可以将其传递给结构体

```cpp
uint32_t glfwExtensionCount = 0;
const char** glfwExtensions;
glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);
// 将参数包装成数组
std::vector<const char*> requiredExtensions( glfwExtensions, glfwExtensions + glfwExtensionCount );
// 使用特殊的setter，可以直接传入数组
createInfo.setPEnabledExtensionNames( requiredExtensions );
```

注意这里我们使用了 `setPEnabledExtensionNames` ，它一次性设置了2个参数，等于这样：

```cpp
// 扩展的数量
createInfo.enabledExtensionCount = static_cast<uint32_t>(requiredExtensions.size());
// 扩展名数组首地址指针
createInfo.ppEnabledExtensionNames = requiredExtensions.data();
```

### 2. 特殊setter函数说明

由于数组类型传参时会隐式退化成指针，底层C风格接口都使用“开始指针+元素数量”的方式引用数组。
vulkan-hpp 需要调用底层C接口，所以这些配置信息采用相同的方式记录。

但 vulkan-hpp 提供了一些特殊的 `setter` 成员函数，它们通过 `vk::ArrayProxyNoTemporaries` 模板参数简化了数组参数的设置，
这些函数能够自动处理数组参数：

   - 接收任意连续容器（`std::vector`、`std::array`、原生数组）或初始化列表。
   - 支持直接传入单个元素（自动包装为单元素数组）。
   - 自动计算元素数量并设置指针，无需手动管理 `....Count` 和 `p....s`。

> 实际上，每个成员变量还有自身的 `setter` ，他们和直接赋值是等效的。  
> 为了方便区分，如果是单参数，本教程直接赋值而不用 `setter` 函数。  

### 3. 测试与运行

现在尝试构建与运行项目，非MacOS不应该出现错误。

## **处理MacOS的错误**

> 特征：err.code() 为  `vk::Result::eErrorIncompatibleDriver`

在使用最新 MoltenVK SDK 的 MacOS 上，可能抛出异常，因为 MacOS 在运行 Vulkan 时必须启用转换层扩展。

**解决方案：**

1. 修改 `CreateInfo` 
2. 添加 `vk::KHRPortabilityEnumerationExtensionName` 扩展
3. 添加 `vk::InstanceCreateFlagBits::eEnumeratePortabilityKHR` 标志位


通常代码可能如下所示
```cpp
std::vector<const char*> requiredExtensions( glfwExtensions, glfwExtensions + glfwExtensionCount );
requiredExtensions.emplace_back(vk::KHRPortabilityEnumerationExtensionName);

createInfo.setPEnabledExtensionNames( requiredExtensions );
createInfo.flags |= vk::InstanceCreateFlagBits::eEnumeratePortabilityKHR;
```

现在重新尝试构建与运行项目，不应该出现错误。


## **检查扩展支持**

创建实例时有扩展不支持会抛出异常，异常代码为 `vk::Result::eErrorExtensionNotPresent` 。

我们可以主动检查哪些扩展是支持的，使用 `enumerateInstanceExtensionProperties` 函数，会返回一个 `std::vector` ，表示支持的扩展类型列表。

每个 `vk::ExtensionProperties` 结构体包含扩展的名称和版本。我们可以用一个简单的 for 循环列出它们：
```cpp
// std::vector<vk::ExtensionProperties>
auto extensions = m_context.enumerateInstanceExtensionProperties();
std::cout << "available extensions:\n";

for (const auto& extension : extensions) {
    std::cout << '\t' << extension.extensionName << std::endl;
}
```

> `m_context.enumerate...()`和`vk::enumerate...()`的效果是一样的。


您可以将此代码添加到 `createInstance` 函数中，以查看支持的扩展列表。

> 挑战：尝试创建一个函数，检查 GLFW 需要的所有扩展是否都在支持的扩展列表中。

## **清理资源**

- `CreateInfo`和 `ApplicationInfo` 是简单结构，不含其他资源，自然析构即可。

- C风格接口必须手动调用相关 `Destroy` 函数释放 `VkInstance` 等特殊资源。

- 我们使用了 `vk::raii`，不需要在 `cleanup` 中手动清理资源。

**注意：**

1. 本文档依靠RAII的析构释放资源，将按照声明顺序反序析构。

2. 建议保证析构时先析构子实例，再析构父实例。 

3. 由1+2，需要保证成员变量的声明顺序正确。

当你不确定成员变量声明顺序时，可以参考每章最下方的样例代码。

> 实际上也可以使用 `m_xxx = nullptr` 显式清理资源，此时无需在意声明顺序。

---

在继续更复杂的步骤之前，是时候通过验证层来评估我们的程序了。

---

**[C++代码](../../codes/01/01_instance/main.cpp)**

**[C++代码差异](../../codes/01/01_instance/main.diff)**

**[CMake代码](../../codes/01/00_base/CMakeLists.txt)**

---
