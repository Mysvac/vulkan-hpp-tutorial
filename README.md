<div align="center">

<h1>Vulkan-hpp-tutorial</h1>

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

## 项目说明

[Vulkan Tutorial](https://docs.vulkan.org/tutorial/latest/index.html) 英文文档已经发布了现代 C++ 的版本，
本文档完成时间略早于官方的新版文档，因此现在正在进行对齐和补充工作，但作者近期繁忙，更新可能较慢。
建议读者直接阅读 [Vulkan Tutorial](https://docs.vulkan.org/tutorial/latest/index.html)  。

### 内容简介

本文档是 Vulkan 的入门教程，将介绍 Vulkan 图形与计算 API 的基础知识与实际应用。

文档以代码演示为主，你可以在 **[Vulkan Guide](https://docs.vulkan.org/guide/latest/index.html)** 等网站找到更详细的 Vulkan 规范和概念介绍。

教程将采用 C++20/23 标准，使用 SDK 内置的 Vulkan-Hpp 封装，充分利用 RAII 等现代 C++ 特性，这也是 Vulkan 官方推荐的方式。

**文档网站：<https://mysvac.github.io/vulkan-hpp-tutorial>**

> 如果你发现了文档的错误，请提交 Issue 或 PR 。

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

## 静态站点构建

### 安装构建依赖

项目使用 material-mkdocs 构建，请使用 pip 或 conda 安装下面的两个库：

pip:

```shell
pip install mkdocs-material
```

conda（请先选择并激活合适的虚拟环境）:

```shell
conda install conda-forge::mkdocs-material
```

### 生成静态网页资源

首先将仓库内容克隆到本地：

```shell
git clone https://github.com/Mysvac/vulkan-hpp-tutorial.git
cd ./vulkan-hpp-tutorial
```

可以用下面的命令在本地部署临时站点，用于调试或预览：（使用 conda 时记得激活虚拟环境）

```shell
mkdocs serve
```

或者使用下面的命令生成静态文件：

```shell
mkdocs build
```


