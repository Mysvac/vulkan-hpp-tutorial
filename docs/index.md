# **首页**

## **教程简介**

本教程是 Vulkan 的入门教程，将系统讲解 Vulkan 图形与计算 API 的基础知识与实际应用。

教程将使用 C++ 编写代码，借助 Vulkan-Hpp 封装，充分利用 RAII 等现代 C++ 特性。

> 注：Vulkan-Hpp 是 Vulkan SDK 的官方组成部分，非第三方库

**教程大致内容如下：**

1. 介绍基础概念
2. 配置开发环境
3. 绘制第一个三角形
4. 扩展基础功能


## **代码说明**

教程将使用 C++ 编写代码，采用C++20标准，并使用以下工具链：

- [Vulkan SDK](https://lunarg.com/vulkan-sdk/)
- [GLM](http://glm.g-truc.net/) 线性代数库
- [GLFW](http://www.glfw.org/) 窗口库
- [CMake](https://cmake.org/) 构建系统
- [vcpkg](https://vcpkg.io/) 依赖管理

正如教程名，我们会使用 Vulkan SDK 为C++提供的 `vulkan-hpp` 和 `raii` 封装，它提供了更现代的C++接口。

虽然 `vulkan-hpp` 提供了模块支持，但是我们还需要使用 `glfw3` 库和 `glm` 库，将它们模块化是比较繁琐的，
这超出了本教程的范围，因此不使用C++20的模块功能。

> C++23 的标准库模块与标准库头文件似乎是冲突的，这很可能显著延缓C++社区模块化的进度

`CMake` 用于项目构建，实现跨平台的项目，但要求读者了解CMake基础使用。

`Vcpkg` 用于管理第三方库，主要用于安装 `glfw3` 和 `glm`，这非常简单。

## **其他说明**

Vulkan SDK本身由C编写，这带来更好的跨语言能力，可以其他语言调用C接口。

你或许更喜欢C风格底层接口，或使用Rust：

- 基于底层C接口的C++教程：[Vulkan-tutorial](https://vulkan-tutorial.com/)

- 基于Vulkano封装的Rust教程 [Vulkan-tutorial-rs](https://github.com/bwasty/vulkan-tutorial-rs)

> 如果你发现了文档的错误，请点击右上角前往Github仓库，提交 Issue 或 PR 。

## **项目参考资料**

Vulkan-hpp文档 [vulkan-hpp](https://github.com/KhronosGroup/Vulkan-Hpp) 。

Vulkan英文教程 [Vulkan-Tutorial](https://github.com/Overv/VulkanTutorial) 。

Vulkan中文教程 [Vulkan-Tutorial](https://tutorial.vulkan.net.cn/Introduction) 。

