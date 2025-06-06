<div align="center">

<h1>Vulkan-raii-hpp-tutorial</h1>

<p>
    <a href="#中文"><img src="https://img.shields.io/badge/中文-red?style=for-the-badge" alt="中文" /></a>
    <!-- &nbsp;&nbsp;
    <a href="#ENGLISH"><img src="https://img.shields.io/badge/English-blue?style=for-the-badge" alt="English" /></a> -->
</p>
<p>
    <a href="https://www.vulkan.org/">
        <img src="https://img.shields.io/badge/Vulkan-SDK-green?style=for-the-badge" alt="Vulkan-SDK" />
    </a>
    &nbsp;&nbsp;
    <a href="https://github.com/KhronosGroup/Vulkan-Hpp">
        <img src="https://img.shields.io/badge/Vulkan-Hpp-yellow?style=for-the-badge" alt="Vulkan-Hpp" />
    </a>
    &nbsp;&nbsp;
    <a href="https://github.com/Overv/VulkanTutorial">
        <img src="https://img.shields.io/badge/Vulkan-Tutorial-yellow?style=for-the-badge" alt="Vulkan-Tutorial" />
    </a>
</p>

</div>

---

<div id="中文">

## 项目说明

**文档网站：<https://mysvac.github.io/vulkan-hpp-tutorial>**

备用站点：<https://vulkan.mysvac.com>

> 如有错误，请提交Issue或PR。

### 教程简介

本教程是 Vulkan 的入门教程，将系统讲解 Vulkan 图形与计算 API 的基础知识与实际应用。

教程将使用 C++ 编写代码，借助 Vulkan-Hpp 封装，充分利用 RAII 等现代 C++ 特性。

> 注：Vulkan-Hpp 是 Vulkan SDK 的官方组成部分，非第三方库。

**教程大致内容如下：**

1. 介绍基础概念
2. 配置开发环境
3. 绘制第一个三角形
4. 扩展基础功能

### 项目结构

- `src` 存放C++完整代码
- `shaders` 存放shader代码
- `texture` 存放纹理图片
- `models` 存放模型文件
- `docs` 存放静态站点相关资源
    - `md` 存放教程文档
    - `images` 图片资源
    - `res` 其他资源
    - `codes` 每一节的代码和差异文件。


### 代码说明

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

### 其他说明

Vulkan SDK本身由C编写，这带来更好的跨语言能力，可以其他语言调用C接口。

你或许更喜欢C风格底层接口，或使用Rust：

- 基于底层C接口的C++教程：[Vulkan-tutorial](https://vulkan-tutorial.com/)

- 基于Vulkano封装的Rust教程 [Vulkan-tutorial-rs](https://github.com/bwasty/vulkan-tutorial-rs)



### 项目参考资料

Vulkan-hpp文档 [vulkan-hpp](https://github.com/KhronosGroup/Vulkan-Hpp) 。

Vulkan英文教程 [Vulkan-Tutorial](https://github.com/Overv/VulkanTutorial) 。

Vulkan中文教程 [Vulkan-Tutorial](https://tutorial.vulkan.net.cn/Introduction) 。



</div>

---
<!-- 
<div id="ENGLISH">

// TODO

</div> -->

