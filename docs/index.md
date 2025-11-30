---
title: 首页
comments: true
---
# **首页**

## **教程简介**

本文档是 Vulkan 的入门教程，将介绍 Vulkan 图形与计算 API 的基础知识与实际应用。

文档以代码演示为主，你可以在 **[Vulkan Guide](https://docs.vulkan.org/guide/latest/index.html)** 等网站找到更详细的 Vulkan 规范和概念介绍。

教程将采用 C++20 标准，使用 SDK 内含的 Vulkan-Hpp 封装，充分利用 RAII 等现代 C++ 特性，这也是 Vulkan 官方推荐的方式。

你可以将此文档的前半部分看做是 [Vulkan Tutorial](https://docs.vulkan.org/tutorial/latest/index.html) 的中文翻译，它是官方教程，且在近期发布了现代 C++ 的新版本。

本文档完成时间略早于官方的新版文档，因此现在正在进行对齐和补充工作，但注意二者存在许多内容差异与编程风格差异。

> 如果你发现了文档的错误，请点击右上角前往Github仓库，提交 Issue 或 PR 。

## **代码说明**

教程不限操作系统、代码编辑器与 C++ 编译器，主要使用以下工具链：

- [Vulkan SDK](https://lunarg.com/vulkan-sdk/)
- [GLM](http://glm.g-truc.net/) 线性代数库
- [GLFW](http://www.glfw.org/) 窗口库
- [CMake](https://cmake.org/) 构建系统
- [vcpkg](https://vcpkg.io/) 依赖管理

特殊的是着色器语言的选择，Vulkan 官方教程在旧版选用 GLSL，新版选用Slang，而本文档将采用基于 WebGPU 规范的 WGSL 语言。

> WebGPU 是一个更新的图形与计算 API 规范，它不仅仅可用于 Web 平台，也可作为渲染引擎抽象层的实现规范。
> Bevy 游戏引擎就采用了基于 WebGPU 规范的 [wgpu](https://wgpu.rs/) 库作为抽象层，底层可选用 Vulkan 等渲染引擎。

好消息是着色器语言没有那么多复杂的特性，学会任何一种就能轻松上手其他语言。

## **其他说明**

Vulkan SDK 本身由 C 编写，因此具有更好的跨语言兼容性，可通过 C 接口供其他语言调用。

如果你更喜欢 C 风格的底层接口，或希望使用 Rust：

- 基于底层 C 接口的 C++ 教程： [旧版 Vulkan-Tutorial](https://vulkan-tutorial.com/)

> 官方更推荐使用现代 C++ 封装。

- 基于 Vulkano 封装的 Rust 教程： [Vulkan-tutorial-rs](https://github.com/bwasty/vulkan-tutorial-rs)

> Rust 更推荐直接学习基于 WebGPU 规范的 [wgpu](https://wgpu.rs/)。

## **致谢**

本文档内容参考了许多公开的资料以及课程，在此感谢：

- [Vulkan-Hpp](https://github.com/KhronosGroup/Vulkan-Hpp) 

- [Vulkan-Guide](https://docs.vulkan.org/guide/latest/index.html)

- [Vulkan-Tutorial旧版](https://vulkan-tutorial.com/) 

- [Vulkan-Tutorial新版](https://docs.vulkan.org/tutorial/latest/index.html)

- [EasyVulkan](https://easyvulkan.github.io/index.html) 

- [GAMES101-现代计算机图形学入门\(闫令琪\)](https://www.bilibili.com/video/BV1X7411F744)

- [GAMES202-高质量实时渲染\(闫令琪\)](https://www.bilibili.com/video/BV1YK4y1T7yY)

- [GAMES104-现代游戏引擎：从入门到实践\(王希\)](https://games104.boomingtech.com/sc/)

---
