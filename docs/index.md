# 文档简介

**文档正在制作中······**

## 前言
这是Vulkan的C++入门文档。  
我们将基于Vulkan-Hpp封装提供的raii相关内容进行介绍。

*制作本文档的原因是作者发现没有完整的`vulkan-hpp-raii`基础示例*

> 本教程使用AI优化语言组织，请注意甄别错误!

## 说明

### 项目结构
- `src` 中存放Cpp完整代码。
- `shaders`中存放shader代码。
- `docs` 中存放文档
    - `md` 中存放教程文档。
    - `codes` 中存放每一节的Cpp代码。

### C++标准说明
使用C++20标准，但不会使用C++20的模块功能。

虽然vulkan-hpp提供了模块支持，但是我们还需要使用glfw3库和glm库，将他们模块化是比较繁琐的，
这超出了本教程的目的，因此不使用模块。

*且 C++23 的标准库模块与标准库头文件似乎是冲突的，这很可能导致显著延缓C++模块化的发展*

### 构建与依赖管理
由于C++20，你需要较新的编译器：
- Visual Studio 2022 17.4 或更高版本（提供 19.34 或更高版本）cl.exe
- Clang 16.0.0 或更高版本
- GCC 14.0 或更高版本
- CMake 3.28 或更高版本
- Ninja 1.10.2 或更高版本

我们将使用CMake进行项目构建，跨平台。

我们将使用Vcpkg管理第三方库，简化安装。

## 参考

Vulkan-hpp文档[vulkan-hpp](https://github.com/KhronosGroup/Vulkan-Hpp)。

Vulkan英文教程[Vulkan-Tutorial](https://github.com/Overv/VulkanTutorial)。  
Vulkan中文教程[Vulkan-Tutorial](https://tutorial.vulkan.net.cn/Introduction)。

