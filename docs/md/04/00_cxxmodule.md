---
title: C++模块化
comments: true
---
# **C++模块化**

## **前言**

C++20标准引入了模块功能，它可以加速项目构建，避免宏传播污染，是更加现代的编译方式。
而 Vulkan-hpp 也提供了内置的模块封装，我们将在本章介绍如何使用它。

> 理论上可以加快，但由于C++社区模块化进度缓慢，标准库仍在实验中，导致大量头文件需放于全局模块片段。
> 而模块编译有顺序要求，多因素综合反而减缓了编译速度。  
> 但无论如何，模块化是未来的方向，且使用它编写的代码更加“优雅”可观。

本章将分为三部分，第一部分介绍 `vulkam.cppm` 模块文件的用法，第二部分给出一个模块化的示例项目，第三部分介绍其他工具的模块化（待定）。

> 建议使用 CLion / Visual Studio 等编辑器，它们可以自动识别 CMake 预设且对 C++ 模块有良好的支持（ VSCode 智能感知对模块支持不佳）。


## **Vulkan模块**

### 1. 基础代码

在开始内容阅读之前，请下载下面这份超级简单的基础代码：

**[点击下载](../../codes/04/00_cxxmodule/base_code.zip)**

请仔细浏览基础代码，目前的内容非常简单，仅输出 `Hello, World!` 。

也可以选择手动编译运行，请查看 `CMakePresets.json` 文件并选择合适的预设，以 MSVC 为例：

```shell
cmake --preset "win-x64"
cmake --build --preset "win-x64-build-debug"
./build/Debug/main.exe
```

你将看到输出 `Hello, World!` 。

### 2. 辅助CMake文件

Vulkan-hpp提供了模块文件，并在[官方仓库](https://github.com/KhronosGroup/Vulkan-Hpp)的 README 中给出了使用说明。

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
set(CMAKE_CXX_STANDARD 20)

include(cmake/VulkanHppModule.cmake)

add_executable(main src/main.cpp)

target_link_libraries(main PRIVATE VulkanHppModule)
```

模块的编译需要 C++20 标准，我们已在 `CMakeLists.txt` 中使用 `set` 设置。

### 4. 编译测试

现在可以回到 `main.cpp` 文件添加一些测试代码：

```cpp
#include <iostream>
#include <vulkan/vulkan_hpp_macros.hpp> # 此头文件包含hpp封装常用的部分宏
import vulkan_hpp;

int main() {
    vk::raii::Context ctx; // 初始化上下文
    
    vk::ApplicationInfo app_info = {
        "My App", 1,
        "My Engine", 1,
        vk::makeApiVersion(1, 0, 0, 0)
    };
    vk::InstanceCreateInfo create_info{ {}, &app_info };
    vk::raii::Instance instance = ctx.createInstance(create_info);

    auto physicalDevices = instance.enumeratePhysicalDevices();
    for (const auto& physicalDevice : physicalDevices) {
        std::cout << physicalDevice.getProperties().deviceName << std::endl;
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

Vulkan 的模块化就到这里，后面会介绍本课程其他可能用到的库的模块化方式。

## **项目模块化示例**

请下载下面的示例代码，此代码将“移动摄像机”的代码进行了模块化拆分，使得结构更加清晰：

**[点击下载](../../codes/04/00_cxxmodule/module_code.zip)**

我们后面的章节可能将此代码作为基础代码进行扩展，请务必下载并查看，它的最终效果与之前“移动摄像机”章节的最终效果完全相同：

![right_room](../../images/0310/right_room.png)

即使已经过去多年，26标准即将到来，C++20的模块编译依然不是一件容易成功的事。
本实例代码在作者的几台设备上都可以正常编译运行，但在你的设备上可能由于 Vulkan、CMake 或其他依赖库的版本问题导致编译失败，或许需要根据自己的环境进行一些调整。

即使无法运行，你也可以直接查看 `src/` 文件夹内的代码。
这很有助于你了解 Vulkan 各组件的依赖关系，因为它们现在严格分离在多个模块文件中，而不像之前一样挤在一起。

## **其他工具的模块化**

### 1. GLM模块

// TODO

### 2. GLFW模块

// TODO

### 3. stb_image模块

// TODO

### 4. tinyobjloader模块

// TODO

### 5. VkMemoryAllocator模块

// TODO

### 6. imgui模块

// TODO

---

第一部分：

**[main.cpp样例](../../codes/04/00_cxxmodule/src/main.cpp)**

**[CMakeLists.txt样例](../../codes/04/00_cxxmodule/CMakeLists.txt)**

**[CMakePresets.json样例](../../codes/04/00_cxxmodule/CMakePresets.json)**

**[VulkanHppModule.cmake样例](../../codes/04/00_cxxmodule/cmake/VulkanHppModule.cmake)**

第二部分：

**[module_code.zip](../../codes/04/00_cxxmodule/module_code.zip)**

> 请下载后本地查看（UTF-8），浏览器查看文件可能出现中文乱码。

---
