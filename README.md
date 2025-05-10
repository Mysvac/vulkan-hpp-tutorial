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

## 项目文档说明

这是Vulkan的C++入门教程文档。  
我们将基于Vulkan-Hpp封装提供的raii相关内容进行介绍。

**文档网站：<https://mysvac.github.io/vulkan-hpp-tutorial>**

> 本文档使用AI优化语言组织，请注意甄别错误!

## 项目说明

### 项目结构
- `src` 中存放Cpp完整代码。
- `shaders`中存放shader代码。
- `docs` 中存放文档
    - `md` 中存放教程文档。
    - `images` 相关图片资源
    - `codes` 中存放每一节的代码和差异文件。

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


</div>

---
<!-- 
<div id="ENGLISH">

// TODO

</div> -->

