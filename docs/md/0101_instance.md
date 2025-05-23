# Vulkan 实例

## RAII上下文初始化

### 1. 创建Context对象

`vk::raii::Context` 的作用是 初始化 Vulkan 的动态加载层（Loader），并提供 Vulkan 函数指针的加载功能。它是 RAII 封装的基础。

**要求：**

1. 我们必须初始化它
2. 只初始化一次
3. 保证它的生命周期覆盖其他Vulkan组件
4. 可以无参构造，不可`nullptr`构造（特殊）

所以我们可以直接将它作为成员变量，类实例化时自动创建并加载上下文：

```cpp
private:
    /// class member
    GLFWwindow* m_window{ nullptr };
    vk::raii::Context m_context;
```

现在直接构建与运行程序，请保证程序不报错。

> RAII机制参考接口介绍章节。


## 创建实例

我们还需要创建一个实例来初始化 Vulkan 库，实例是您的应用程序和 Vulkan 库之间沟通的桥梁。

### 1. 创建成员变量和辅助函数

我们需要一个`vk::raii::Instance`对象管理Vulkan实例，并通过辅助函数创建它

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

> 上面提到大部分 `raii` 资源不支持无参构造，要使用 `nullptr` 初始化表示无效值

### 2. 添加应用程序信息

需要添加应用程序信息结构体，它是可选的，但是填写它能够让驱动程序更好的进行优化。

```cpp
void createInstance(){
    vk::ApplicationInfo applicationInfo( 
        "Hello Triangle",   // pApplicationName
        1,                  // applicationVersion
        "No Engine",        // pEngineName
        1,                  // engineVersion
        VK_API_VERSION_1_1  // apiVersion
    );
}
```

注意到，它并不是raii的，因为它只是个配置信息，不含特殊资源。
所以我们可以无参构造，然后直接修改成员变量，像这样：

```cpp
vk::ApplicationInfo applicationInfo;
applicationInfo.pApplicationName = "xxxx";  // class member is public
applicationInfo.setApplicationVersion(1);   // or use setter()
```

> 后续教程为方便讲解，将同时使用这两种方式！

### 2. 配置基础创建信息

前面提到，Vulkan中资源的创建都依赖对应的 `CreateInfo` 结构体，我们必须先填写它。

```cpp
vk::InstanceCreateInfo createInfo( 
    {},                 // vk::InstanceCreateFlags
    &applicationInfo    // vk::ApplicationInfo*
);
```

**说明**：

1. 任何 `CreateInfo` 都有一个 `flags` 参数。
2. `flags` 参数是位枚举，用于控制特殊行为。
3. `flags`参数提供默认初始化，大部分时候无需修改。
4. 还有其他参数，但都提供了默认初始化，无需手动设置。

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

## 添加扩展以支持GLFW

Vulkan 是一个平台无关的 API，这意味着您需要一个扩展来处理窗口系统接口。

### 1. 获取所需扩展与修改CreateInfo

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

注意这里我们使用了 `setPEnabledExtensionNames` ，它一次性设置了2个参数，效果类型这样：

```cpp
// 扩展的数量
createInfo.enabledExtensionCount = static_cast<uint32_t>(requiredExtensions.size());
// 扩展名数组，每个扩展名都是个字符串字面量
createInfo.ppEnabledExtensionNames = requiredExtensions.data();
```

为了兼容底层C接口，很多配置信息的参数依然是首元素指针+元素数量的方式。
但是vulkan-hpp提供了更高级的成员函数，也就是上面用到的 `setter`：

### 2. 特殊settter函数说明

vulkan-hpp 提供了一些特殊的 setter 成员函数，它们通过 `vk::ArrayProxyNoTemporaries` 模板参数简化了数组参数的设置。
其核心优势在于：

> 这些函数的命名大致是： `set......s` ，末尾`s`表示多个。  
> 假如是 `setAs`，那么类应该有类似 `pA` 和 `aCount` 的成员变量，分别表示开始指针和数量。

1. **自动处理数组参数**  
   - 接收任意连续容器（`std::vector`、`std::array`、原生数组）或初始化列表。
   - 支持直接传入单个元素（自动包装为单元素数组）。
   - 自动计算元素数量并设置指针，无需手动管理 `enabledExtensionCount` 和 `ppEnabledExtensionNames`。

2. **推荐用法**  
   - **直接传递容器**：将扩展列表存入 `std::vector` 后传入：
     ```cpp
     std::vector<const char*> extensions = glfwGetRequiredInstanceExtensions(...);
     createInfo.setPEnabledExtensionNames(extensions);
     ```
   - **使用初始化列表**：直接构造列表传递：
     ```cpp
     createInfo.setPEnabledExtensionNames({
         VK_KHR_SURFACE_EXTENSION_NAME,
         VK_EXT_DEBUG_UTILS_EXTENSION_NAME
     });
     ```
   - **混合容器与单元素**：灵活组合多种形式：
     ```cpp
     std::vector<const char*> baseExtensions = {...};
     createInfo.setPEnabledExtensionNames(baseExtensions); // 容器
     createInfo.addEnabledExtension(VK_ANOTHER_EXTENSION); // 单个扩展（假设存在 add 方法）
     ```

3. **底层原理**  
   - `vk::ArrayProxyNoTemporaries` 是一个轻量代理，不拥有数据，仅包装现有容器。
   - 调用 `setPEnabledExtensionNames` 时，代理自动提取 `.size()` 和 `.data()`，并赋值给底层 C 结构体的 `enabledExtensionCount` 和 `ppEnabledExtensionNames`。

4. **注意事项**  
   - **生命周期管理**：确保传入的字符串指针在 Vulkan 实例创建期间有效（通常静态字符串安全）。
   - **避免临时对象**：代理不接受临时容器（如直接传 `std::vector{...}`），需显式定义变量或使用右值引用适配。

> 实际上，每个成员变量还有自身的 `setter` ，他们和直接赋值是等效的。
> 
> 为了方便区分，如果是单参数，我们直接赋值而不用 `setter` 函数。  
> 如果使用了 `setter` ，那么大概率是上述情况。

### 3. 测试与运行

现在尝试构建与运行项目，非MacOS不应该出现错误。

## 处理MacOS的错误

> 特征：err.code() 为  `vk::Result::eErrorIncompatibleDriver`

如果在使用最新 MoltenVK sdk 的 MacOS 上，可能抛出异常，因为MacOS是运行Vulkan必须启用转换层扩展。

**解决方案：**

1. 添加 `VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME` 扩展
2. 修改 `CreateInfo`
3. 添加标志位 `vk::InstanceCreateFlagBits::eEnumeratePortabilityKHR`


通常代码可能如下所示
```cpp
// ......
std::vector<const char*> requiredExtensions( glfwExtensions, glfwExtensions + glfwExtensionCount );
requiredExtensions.emplace_back(VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME);

createInfo.setPEnabledExtensionNames( requiredExtensions );
createInfo.flags |= vk::InstanceCreateFlagBits::eEnumeratePortabilityKHR;
// ......
```

现在重新尝试构建与运行项目，不应该出现错误。


## 检查扩展支持

不支持扩展时，创建 `instance` 就会抛出异常，异常代码为 `vk::Result::eErrorExtensionNotPresent`。

我们可以主动检查哪些扩展是支持的，使用 `enumerateInstanceExtensionProperties` 函数，会返回一个 `std::vector` ，表示支持的扩展类型。

每个 `vk::ExtensionProperties` 结构体包含扩展的名称和版本。我们可以用一个简单的 for 循环列出它们
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

## 清理资源

- `CreateInfo`和 `ApplicationInfo` 是简单结构，不含其他资源，自然析构即可。

- C风格接口必须手动调用相关 `Destory` 函数释放 `VkInstance` 等特殊资源。

- 我们使用了 `vk::raii`，不需要在 `cleanup` 中手动清理资源。



**注意！！**

1. 依靠RAII的析构释放资源，将按照声明顺序反序析构。

2. 建议保证析构时先析构子实例，再析构父实例。 

3. 由1+2，需要保证成员变量的声明顺序正确。

## 测试

现在运行代码，保证没有出错，出现问题可以参考下面的样例代码。

在继续实例创建之后更复杂的步骤之前，是时候通过查看验证层来评估我们的调试选项了。

---

**[C++代码](../codes/0101_instance/main.cpp)**

**[C++代码差异](../codes/0101_instance/main.diff)**
