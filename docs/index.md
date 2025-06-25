---
title: 首页
comments: true
---
# **首页**

## **教程简介**

本文档是 Vulkan 的入门教程，将系统讲解 Vulkan 图形与计算 API 的基础知识与实际应用。

教程将使用 C++ 编写代码，借助 Vulkan-Hpp 封装，充分利用 RAII 等现代 C++ 特性，减少手动资源管理。

> 注：Vulkan-Hpp 是 Vulkan SDK 的官方组成部分，非第三方库。  
> 如果你发现了文档的错误，请点击右上角前往Github仓库，提交 Issue 或 PR 。

## **代码说明**

教程将采用 C++20 标准，主要使用以下工具链：

- [Vulkan SDK](https://lunarg.com/vulkan-sdk/)
- [GLM](http://glm.g-truc.net/) 线性代数库
- [GLFW](http://www.glfw.org/) 窗口库
- [CMake](https://cmake.org/) 构建系统
- [vcpkg](https://vcpkg.io/) 依赖管理

虽然 `vulkan-hpp` 提供了模块接口文件，但模块的编译并不轻松，且 `glfw3` 等库的模块化较为复杂，因此我们只在“进阶”章节使用 C++20 模块功能。

`CMake` 用于项目构建，实现跨平台的项目配置，要求读者了解 CMake 的基础使用。

`Vcpkg` 用于管理第三方库，主要用于安装 `glfw3` 和 `glm` 等依赖，这非常简单。

## **其他说明**

Vulkan SDK 本身由 C 编写，因此具有更好的跨语言兼容性，可通过 C 接口供其他语言调用。

如果你更喜欢 C 风格的底层接口，或希望使用 Rust：

- 基于底层C接口的C++教程：[Vulkan-tutorial](https://vulkan-tutorial.com/)

- 基于Vulkano封装的Rust教程 [Vulkan-tutorial-rs](https://github.com/bwasty/vulkan-tutorial-rs)

## **致谢**

项目文档与知识点参考以下内容：

- [vulkan-hpp](https://github.com/KhronosGroup/Vulkan-Hpp) 

- [Vulkan-Tutorial](https://github.com/Overv/VulkanTutorial) 

- [EasyVulkan](https://easyvulkan.github.io/index.html) 

---
