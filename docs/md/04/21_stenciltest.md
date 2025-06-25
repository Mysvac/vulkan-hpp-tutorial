---
title: 模板测试
comments: true
---

# **模板测试**

// TODO 内容未检查

## **前言**

我们在“深度缓冲”章节中介绍了 **深度测试\(Depth Test\)** ，它注意用于判断每个像素的“远近”，从而决定哪些像素可以被写入帧缓冲。

当时我们还提到了 **模板测试\(Stencil Test\)** ，它允许用户自定义一套简单的规则，决定像素是否可被写入帧缓冲，本章将为您演示它的基础使用。

需要注意的是，模板测试与图像内容无关，它仅依赖于用户设置的模板缓冲区的值。
而模板缓冲区的值通常需要通过一次渲染过程来设置，因此模板测试常用于多次渲染。
比如第一次渲染基础场景，第二次通过深度/模板测试标记特殊区域（如镜子、阴影等），第三次渲染时仅在标记区域内绘制内容。

## **基础代码**

请下载并阅读下面的基础代码，这个是上一节“多子通道”的修改版代码：

// TODO

**点击下载**

模板测试和深度测试共用一个缓冲区，而此代码中已经创建了深度缓冲，且创建方式和“深度缓冲”章节相同，因此在此基础上添加模板测试十分简单。

## **模板测试介绍**

### 1. 字段介绍

每个像素在片段着色器后需通过以下测试： `裁剪测试 → 模板测试 → 深度测试 → 混合` ，只有通过测试的像素才会被写入帧缓冲。

模板测试的测试规则直接在图形管线创建时设置，且字段非常简单，只有下面这些：

```cpp
vk::StencilOpState stencil_op; // 模板测试操作配置
stencil_op.failOp = vk::StencilOp::eKeep; // 模板测试失败时不改变模板值
stencil_op.passOp = vk::StencilOp::eReplace; // 模板测试通过时替换模板值
stencil_op.depthFailOp = vk::StencilOp::eKeep; // 深度测试失败时不改变模板值
stencil_op.compareOp = vk::CompareOp::eEqual; // 始终通过模板测试
stencil_op.compareMask = 0xFF; // 模板比较掩码
stencil_op.writeMask = 0xFF; // 模板写掩码
stencil_op.reference = 1; // 模板参考值

vk::PipelineDepthStencilStateCreateInfo depth_stencil;
depth_stencil.depthTestEnable = true; // 启用深度测试
// ... 深度测试配置
depth_stencil.stencilTestEnable = true; // 启用模板测试
depth_stencil.back = stencil_op; // 设置背面的模板测试操作
depth_stencil.front = stencil_op; // 设置正面的模板测试操作
```

“前言”部分提到使用模板测试时通常会进行多次渲染，每次的模板测试规则可以不同，因此我们需要多个图形管线，这就是为什么我们将“模板测试”内容放在“多子通道”章节。

模板测试的工作原理是：每个像素的模板值与参考值进行比较，比较决定是否通过测试：

```cpp
if compareOp == vk::CompareOp::eNever: "永远不通过测试"
if compareOp == vk::CompareOp::eGreater && 
  (Reference & CompareMask) > (Stencil & CompareMask): "参考值大于模板值时通过测试"
```

`Stencil` 是模板缓冲区中当前像素的模板值，`Reference` 是我们在上面设置的参考值字段。

可用的比较操作有：

| `vk::CompareOp`       | 描述          | `vk::CompareOp`       | 描述          |
|-----------------------|-------------|-----------------------|-------------|
| `eNever`              | 永远不通过测试     | `eLess`               | 参考值小于模板值    |
| `eAlways`             | 始终通过测试      | `eLessOrEqual`        | 参考值小于或等于模板值 |
| `eEqual`              | 等于参考值       | `eGreater`            | 参考值大于模板值    |
| `eNotEqual`           | 不等于参考值      | `eGreaterOrEqual`     | 参考值大于等于模板值  |

然后根据比较结果，模板测试会 `failOp` 或 `passOp` 中指定的操作来处理模板值。

| `vk::StencilOp` | 描述         | `vk::StencilOp`      | 描述              |
|-----------------|------------|----------------------|-----------------|
| `eKeep`         | 保持当前模板值    | `eIncrementAndClamp` | 将模板值加 1，但不超过最大值 |
| `eZero`         | 将模板值设置为 0  | `eDecrementAndClamp` | 将模板值减 1，但不低于 0  |
| `eReplace`      | 将模板值替换为参考值 | `eInvert`            | 将当前模板值按位取反      |

当模板测试通过但深度测试失败 时，执行 `depthFailOp` 指定的操作。

而 `writeMask` 字段用于控制哪些值可被修改，最终写入值等于：   
 `(NewValue & writeMask) | (OldValue & ~writeMask)` 。

几何体被光栅化时分为正反面，视为正面的顶点绘制顺序由光栅化器的 `frontFace` 字段决定。
这里的重点是图元的两个面可以有不同的模板测试规则，分别由 `front` 和 `back` 字段指定。

### 2. 使用介绍

通过上面的介绍你会发现一个问题，模板测试的比较仅依赖于参考值和模板缓冲区的值，与像素颜色等图像内容无关。

实际模板依赖用户提前设置模板缓冲部分的值，然后可以在单次渲染中只处理部分像素，这在多次渲染中非常有用。

比如实现镜面反射，第一次渲染时将镜子所在平面的模板值设为 1 ，第二次仅在被标记区域（模板值=1）内绘制。
还可以用于 UI 、阴影与光照内容绘制时的遮罩，或者在多重渲染目标中选择性地更新某些缓冲区。



