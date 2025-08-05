---
title: C++模块化
comments: true
---
# **C++模块化**

## **前言**

为了优化项目结构，我们将引入 C++20 的模块功能。模块化可以帮助我们更好地组织代码，减少编译时间，并避免宏定义的污染。

本章我们先介绍 Vulkan-Hpp 提供的模块接口文件，然后为你提供一个模块化的项目示例，最后介绍其他常用库的模块化方法。

迷枵是忠实的现代 C++ 用户，考虑后决定直接使用 C++23 标准库模块，面向未来。
C++ 23 标准库模块的使用可以参考 **[这里](https://mysvac.com/archives/191)** 。

## **Vulkan模块**

### 1. 基础代码

在开始内容阅读之前，请下载下面这份超级简单的基础代码：

**[点击下载](../../codes/04/00_cxxmodule/base_code.zip)**

请仔细浏览基础代码，目前的内容非常简单，仅输出 `Hello, World!` 。
本文件使用了标准库模块，如果你不清楚如何启用标准库模块，请浏览前言部分提供的链接。

### 2. 辅助CMake文件

Vulkan-Hpp 提供了模块文件，并在 [官方仓库](https://github.com/KhronosGroup/Vulkan-Hpp) 的 README 中给出了使用说明。

目前还不支持预构建的模块，我们需要使用 vulkan 文件夹下的 `vulkan.cppm` 手动编译。
我们的基础代码有个 `cmake` 文件夹是空的，现在在里面创建一个 `VulkanHppModule.cmake` 文件。

```
项目根目录/
│
├── CMakeLists.txt     # 主CMake配置文件
├── CMakePresets.json  # CMake预设文件
├── cmake/             # CMake脚本目录
│   │
│   └── VulkanHppModule.cmake # Vulkan模块脚本
└── src/          # 源代码目录
    │
    └── main.cpp  # 主程序入口
```

### 3. 编译脚本

由于 vulkan-hpp 已经提供了 `vulkan.cppm` 文件，实际的模块编译非常简单，直接使用它即可。

首先查找库并判断版本，我假设你的 Vulkan 版本是  1.4 以上：

```cmake
# VulkanHppModule.cmake
# IF 判断，防止重复编译
if(NOT TARGET VulkanHppModule)
    find_package(Vulkan REQUIRED)
    message(Vulkan SDK version: ${Vulkan_VERSION})
    if(${Vulkan_VERSION} VERSION_LESS "1.4.0") # 版本判断，可选
        message(FATAL_ERROR "Vulkan SDK too old (required ≥ 1.4.0)")
    endif()
endif()
```

成功找到库后，我们可以通过 `Vulkan_INCLUDE_DIR` 找到模块文件并编译它：

```cmake
if(NOT TARGET VulkanHppModule)
    find_package(Vulkan REQUIRED)
    # 版本判断，此处忽略，可自行添加
    add_library( VulkanHppModule )
    target_sources(VulkanHppModule PUBLIC
            FILE_SET CXX_MODULES
            BASE_DIRS ${Vulkan_INCLUDE_DIR}
            FILES ${Vulkan_INCLUDE_DIR}/vulkan/vulkan.cppm
    )
    target_link_libraries( VulkanHppModule PRIVATE Vulkan::Vulkan )
endif()
```

我们添加了一个库目标 `VulkanHppModule` ，后面可以在主 CMake 配置中链接它。
编译模块需要用到 `CXX_MODULES` 文件集，这里直接找到了 SDK 提供的模块文件并使用它进行编译。

如果你查看过 `vulkan.cppm` 的内容，你会发现它只是导入了 Vulkan 头文件并在 `export` 块内用 `using` 将标识符导出，所以我们要为它提供头文件和源文件，即 `Vulkan::Vulkan` 。

有些必要的宏定义在头文件中，但是我们链接 `Vulkan::Vulkan` 时使用 `PRIVATE` 保证了封闭性，使用此库的人无法访问头文件。
一种解决方案是在为它链接一个 `PUBLIC` 的仅头文件库：

```cmake
target_link_libraries( VulkanHppModule PRIVATE Vulkan::Vulkan )
target_link_libraries( VulkanHppModule PUBLIC Vulkan::Headers )
```

现在还剩下最后一件事，就是编译配置。我们在“实例”章节提到过 Vulkan-hpp 需要使用“动态加载器”自动加载函数指针，此外还有一些可选的编译配置比如“禁用异常”可以按需配置：

```cmake
# VulkanHppModule.cmake
if(NOT TARGET VulkanHppModule)

    find_package(Vulkan REQUIRED)
    # 版本判断，此处忽略，可自行添加
    add_library( VulkanHppModule )
    target_sources(VulkanHppModule PUBLIC
            FILE_SET CXX_MODULES
            BASE_DIRS ${Vulkan_INCLUDE_DIR}
            FILES ${Vulkan_INCLUDE_DIR}/vulkan/vulkan.cppm
    )
    target_link_libraries( VulkanHppModule PRIVATE Vulkan::Vulkan )
    target_link_libraries( VulkanHppModule PUBLIC Vulkan::Headers )

    target_compile_definitions(VulkanHppModule PUBLIC
            VULKAN_HPP_DISPATCH_LOADER_DYNAMIC=1 # 启用动态加载器
            # 其他可选配置参考 vulkan-hpp 仓库的 README
    )
endif()
```

最后还要在 `CMakeLists.txt` 中使用它：

```cmake
# CMakeLists.txt
cmake_minimum_required(VERSION 4.0.0)

project(HelloCppModule LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_MODULE_STD 1)

include(cmake/VulkanHppModule.cmake)

add_executable(main src/main.cpp)

target_link_libraries(main PRIVATE VulkanHppModule)
```

### 4. 编译测试

现在可以回到 `main.cpp` 文件添加一些测试代码：

```cpp
import std;
import vulkan_hpp;

int main() {
    const vk::raii::Context ctx; // 初始化上下文

    constexpr vk::ApplicationInfo app_info = {
        "My App", 1,
        "My Engine", 1,
        vk::makeApiVersion(1, 4, 0, 0)
    };
    const vk::InstanceCreateInfo create_info{ {}, &app_info };
    const vk::raii::Instance instance = ctx.createInstance(create_info);

    std::println("Physical Device: ");
    for(const auto physicalDevices = instance.enumeratePhysicalDevices();
        const auto& physicalDevice : physicalDevices
    ) {
        std::println("\t{}", std::string_view{ physicalDevice.getProperties().deviceName });
    }
}
```

我们尝试初始化上下文、创建实例和获得可用的物理设备信息。

现在你可以尝试编译项目并运行，使用编辑器集成或者前面介绍的手动编译都可以。

> 编译失败可以尝试删除 `build` 文件夹后重新编译，或者更新 Vulkan 版本。

正常运行应该看到类似下面的信息：

```
NVIDIA GeForce RTX ...
Intel(R) UHD Graphics ...
...... 或其他显卡设备
```

最终代码：[下载](../../codes/04/00_cxxmodule/vk_module_demo.zip)

## **项目模块化示例**

请下载下面的示例代码，此代码将“移动摄像机”的代码进行了模块化拆分，使得结构更加清晰：

**[点击下载](../../codes/04/00_cxxmodule/module_code.zip)**

我们后面的章节可能将此代码作为基础代码进行扩展，请务必下载并查看，它的最终效果与之前“移动摄像机”章节的最终效果完全相同：

![right_room](../../images/0310/right_room.png)

浏览此代码有助于你了解 Vulkan 各组件的依赖关系，因为它们现在严格分离在多个模块文件中，而不像之前一样挤在一起。

## **其他库的模块化**

参考上一部分 **[项目模块化示例代码](../../codes/04/00_cxxmodule/module_code.zip)** ，在 `src/third` 中为你提供了其他四个库的模块接口文件。

---

第一部分：

**[最终代码](../../codes/04/00_cxxmodule/vk_module_demo.zip)**

**[CMakeLists.txt样例](../../codes/04/00_cxxmodule/CMakeLists.txt)**

**[VulkanHppModule.cmake样例](../../codes/04/00_cxxmodule/cmake/VulkanHppModule.cmake)**

第二部分：

**[module_code.zip](../../codes/04/00_cxxmodule/module_code.zip)**

---
