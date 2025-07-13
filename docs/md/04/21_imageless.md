---
title: 无图像帧缓冲
comments: true
---
# **无图像帧缓冲**

## **前言**

**无图像帧缓冲\(Imageless Framebuffer\)**是 Vulkan 1.2 引入的一项新特性，允许开发者在创建帧缓冲时不指定具体的图像对象（但需要指定图像格式和其他参数），而具体图像视图将在命令缓冲录制阶段动态绑定。

这使得帧缓冲的创建更加灵活，还可以减少帧缓冲的创建和销毁开销，尤其是在需要频繁切换渲染目标的场景中。

> 关于无图像帧缓冲：[Vulkan-Guide \[imageless framebuffer\]](https://docs.vulkan.org/guide/latest/extensions/VK_KHR_imageless_framebuffer.html)

