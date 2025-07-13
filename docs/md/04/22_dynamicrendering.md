---
title: 动态渲染
comments: true
---
# **动态渲染**

## **前言**

动态渲染（Dynamic Rendering）是 Vulkan 1.3 引入的新特性，允许开发者在命令缓冲录制阶段直接配置渲染目标和相关参数，无需提前创建渲染通道（Render Pass）和帧缓冲（Framebuffer）对象。
这大大简化了渲染流程，提高了开发灵活性。

但需要注意，动态渲染目前不支持子通道（Subpass），因此无法在一次渲染过程中使用多个子通道。
此外，动态渲染可能带来一定的性能开销，具体影响取决于驱动实现和使用场景。

> 关于动态渲染：[Vulkan-Guide \[dynamic rendering\]](https://docs.vulkan.org/samples/latest/samples/extensions/dynamic_rendering/README.html)

## **基础代码**

请下载并阅读下面的基础代码，这是“C++模块化”章节的第二部分代码：

**[点击下载](../../codes/04/00_cxxmodule/module_code.zip)**

## **添加扩展**

动态渲染在 Vulkan 1.2 中由扩展引入，在 1.3 中成为核心特性。

