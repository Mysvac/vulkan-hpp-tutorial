# 教程简介

本教程将系统讲解 Vulkan 图形与计算 API 的基础知识与实际应用。

教程基于现代 C++20 标准和 Vulkan-Hpp 绑定，充分利用 RAII 等现代 C++ 特性。

*注：Vulkan-Hpp 是 Vulkan SDK 的官方组成部分，非第三方库*

## Vulkan 技术概览

Vulkan 是由 [Khronos group](https://www.khronos.org/)（OpenGL 的制定者）推出的新一代图形与计算 API，它提供了对现代显卡硬件更精细的抽象控制。与传统的 [OpenGL](https://en.wikipedia.org/wiki/OpenGL) 和 [Direct3D](https://en.wikipedia.org/wiki/Direct3D) 相比，Vulkan 的主要优势包括：

- 更明确的应用程序行为描述
- 显著的性能提升
- 更可预测的驱动程序行为
- 真正的跨平台支持（Windows/Linux/Android）

*注：macOS 需要通过 MoltenVK 进行转译*

这些优势的代价是 API 复杂度显著提高。开发者需要手动管理包括帧缓冲创建、内存分配等底层细节，驱动程序提供的自动化帮助大幅减少。


**基于上述原因，Vulkan 适合：**

- 追求极致性能的图形程序员
- 需要精细硬件控制的开发者
- 跨平台图形应用开发者

**若您更关注游戏开发而非底层图形编程，建议考虑：**

- 继续使用 OpenGL/Direct3D
- 采用 Unity/Unreal 等游戏引擎



## 学习前提

### 硬件要求
- 支持 Vulkan 的显卡（NVIDIA/AMD/Intel/Apple Silicon）
- 较新的显卡驱动

### 软件技能

- 熟练的现代 C++ 编程能力（RAII、初始化列表等）
- CMake 和 vcpkg 基础使用经验

### 专业知识
- 线性代数，微积分等数学基础
- 3D 图形学基础知识

本教程不会教你 OpenGL 或 Direct3D 的概念，但它确实要求您了解 计算机图形学 的基础知识。例如，它不会解释透视投影背后的数学原理。  

强烈推荐先修课程： [GAMES101-现代计算机图形学入门](https://www.bilibili.com/video/BV1X7411F744)。  


## 开发环境
我们将使用以下工具链：

- [Vulkan SDK](https://lunarg.com/vulkan-sdk/)
- [GLM](http://glm.g-truc.net/) 线性代数库
- [GLFW](http://www.glfw.org/) 窗口库
- [CMake](https://cmake.org/) 构建系统
- [vcpkg](https://vcpkg.io/) 依赖管理

后面的章节会详细介绍环境的搭建。

## 教程结构
1. 核心概念解析
2. 开发环境配置
3. 第一个三角形绘制
4. 高级功能实现

每个章节包含：

- 概念讲解
- API 代码演示
- 辅助函数封装
- 完整代码链接

*注意：虽然渲染第一个三角形需要约 1000 行代码，但后续扩展纹理和 3D 模型不再需要那么多重复工作，相对简单。*

### 其他说明

Vulkan SDK本身由C编写，这带来更好的跨语言能力，你可以其他语言调用C接口。

基于底层C接口的C++教程：[Vulkan-tutorial](https://vulkan-tutorial.com/)  
Rust教程 [Vulkan-tutorial-rs](https://github.com/bwasty/vulkan-tutorial-rs) 


准备好投入高性能图形 API 的未来了吗？ 让我们开始吧！
