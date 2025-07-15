---
title: 首页
comments: true
---
# **首页**

## **教程简介**

本文档是 Vulkan 的入门教程，将介绍 Vulkan 图形与计算 API 的基础知识与实际应用。

文档以代码演示为主，你可以在 **[Vulkan Guide](https://docs.vulkan.org/guide/latest/index.html)** 等网站找到更详细的 Vulkan 规范和概念介绍。

教程将使用 C++ 编写代码，C++23 标准，使用 Vulkan-SDK 内含的 Vulkan-Hpp 封装，充分利用 RAII 等现代 C++ 特性。

如果需要，你可以降低至 C++17 或 C++20 标准进行学习。因为我们不会用到那些复杂的高级特性，且仅在“进阶”章节使用 C++20 模块功能。

> 如果你发现了文档的错误，请点击右上角前往Github仓库，提交 Issue 或 PR 。

## **代码说明**

教程不限操作系统、代码编辑器与 C++ 编译器，主要使用以下工具链：

- [Vulkan SDK](https://lunarg.com/vulkan-sdk/)
- [GLM](http://glm.g-truc.net/) 线性代数库
- [GLFW](http://www.glfw.org/) 窗口库
- [CMake](https://cmake.org/) 构建系统
- [vcpkg](https://vcpkg.io/) 依赖管理

`CMake` 用于项目构建，实现跨平台的项目配置，要求读者了解 CMake 的基础使用。

`vcpkg` 用于管理第三方库，主要用于安装 `glfw3` 和 `glm` 等依赖，这非常简单。

> 教程预期在 C++23 标准库模块完全支持后再次重构内容。

## **其他说明**

Vulkan SDK 本身由 C 编写，因此具有更好的跨语言兼容性，可通过 C 接口供其他语言调用。

如果你更喜欢 C 风格的底层接口，或希望使用 Rust：

- 基于底层 C 接口的 C++ 教程： [Vulkan-tutorial](https://vulkan-tutorial.com/)

- 基于 Vulkano 封装的 Rust 教程： [Vulkan-tutorial-rs](https://github.com/bwasty/vulkan-tutorial-rs)

## **致谢**

本文档内容参考了许多公开的资料以及课程，在此感谢：

- [Vulkan-Hpp](https://github.com/KhronosGroup/Vulkan-Hpp) 

- [Vulkan-Guide](https://docs.vulkan.org/guide/latest/index.html)

- [Vulkan-Tutorial](https://github.com/Overv/VulkanTutorial) 

- [EasyVulkan](https://easyvulkan.github.io/index.html) 

- [GAMES101-现代计算机图形学入门\(闫令琪\)](https://www.bilibili.com/video/BV1X7411F744)

- [GAMES202-高质量实时渲染\(闫令琪\)](https://www.bilibili.com/video/BV1YK4y1T7yY)

- [GAMES104-现代游戏引擎：从入门到实践\(王希\)](https://games104.boomingtech.com/sc/)

---
