<div align="center">

<h1>Vulkan-hpp-tutorial</h1>

<p>
    <!-- <a href="#中文"><img src="https://img.shields.io/badge/中文-red?style=for-the-badge" alt="中文" /></a> -->
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

> 如有错误，请提交 Issue 或 PR 。

### 内容简介

本文档是 Vulkan 的入门教程，将系统讲解 Vulkan 图形与计算 API 的基础知识与实际应用。

教程将使用 C++ 编写代码，借助 Vulkan-Hpp 封装，充分利用 RAII 等现代 C++ 特性。

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

项目使用 material-mkdocs 构建，还需要 pymdownx 扩展。请使用 pip 或 conda 安装下面的两个库：

pip:

```shell
pip install mkdocs-material
pip install pygments
```

conda（请先选择并激活合适的虚拟环境）:

```shell
conda install conda-forge::mkdocs-material
conda install conda-forge::pygments
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

</div>

---
<!-- 
<div id="ENGLISH">

// TODO

</div> -->

