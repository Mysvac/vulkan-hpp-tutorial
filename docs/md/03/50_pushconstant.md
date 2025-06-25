---
title: 推送常量
comments: true
---
# **推送常量**

## **前言**

在上一节中，我们使用实例化渲染成功绘制了多个兔子模型。

由于实例数据位于顶点输入缓冲中（实例缓冲和顶点缓冲作为顶点着色器的输入），我们必须在顶点着色器获取 `enableTexture` 字段的输入然后输出至片段着色器。
显然此字段只在片段着色器中使用，有没有办法让它直接输入到片段着色器而不经过顶点着色器呢？

我们曾学过 Uniform 缓冲，可以将此字段放在UBO中，将UBO直接绑定到片段着色器，然后在绘制房屋或兔子之前修改标志位的值。

在这一节内容，我们将介绍一种更简单的方法 -- **推送常量（Push Constants）**。

推送常量是一种在 GPU 着色器中传递少量数据的高效机制，直接将数据嵌入命令缓冲区，适合传递频繁变化但体积小的数据（例如少量变换矩阵、颜色、标志位等）。

| 特性             | 推送常量        | Uniform 缓冲区         |  顶点输入缓冲 |
|-----------------|------------- ---|-----------------------------------------|----------------------------|
| 适用场景         | 极小数据量、频繁变化        | 中小数据量、变化不频繁         | 静态或半静态的顶点/实例数据   |
| 典型用途         | 每物体模型矩阵、标志位等    | 全局参数、灯光、投影矩阵等      | 顶点位置、法线、UV、实例参数 |
| 最大容量         | 通常 128-256 字节          | 通常 64KB-16MB               | 仅受 GPU 内存限制            |
| 更新方式         | 直接直接写入命令缓冲区      | 常用过指针映射修改内存         | 常用暂存缓冲更新             |
| 访问速度         | 最快（直接嵌入命令缓冲）     | 中等                         | 快                         |
| 资源管理         | 无需显式分配               | 需管理缓冲区与描述符           | 需管理缓冲区                |


## **使用推送常量**

### 1. 管线布局

现在需要修改管线布局，加入推送常量的信息。修改 `createGraphicsPipeline` 函数：

```cpp
vk::PushConstantRange pushConstantRange;
pushConstantRange.stageFlags = vk::ShaderStageFlagBits::eFragment;
pushConstantRange.offset = 0;
pushConstantRange.size = sizeof(uint32_t);

vk::PipelineLayoutCreateInfo pipelineLayoutInfo;
pipelineLayoutInfo.setPushConstantRanges( pushConstantRange );
```

推送常量的数据直接嵌入命令缓冲，所有数据共享一块数据区，但可以通过 `PushConstantRange` 分割出小的数据段。
可以为每个数据段指定大小，起始位置的偏移量和着色器阶段。

我们需要在片段着色器中使用它控制模型是否进行纹理采样，所以设置为 `eFragment` 。

### 2. 命令记录

现在回到 `recordCommandBuffer` 函数，在两次绘制之前进行推送常量命令即可：

```cpp
uint32_t enableTexture = 1; // 启用纹理映射
commandBuffer.pushConstants<uint32_t>(
    m_pipelineLayout,
    vk::ShaderStageFlagBits::eFragment,
    0,              // offset
    enableTexture   // value
);
commandBuffer.drawIndexed( // 绘制房屋
    m_firstIndices[1],
    1,
    0,
    0,
    0
);

enableTexture = 0; // 关闭纹理映射
commandBuffer.pushConstants<uint32_t>(
    m_pipelineLayout,
    vk::ShaderStageFlagBits::eFragment,
    0,              // offset
    enableTexture   // value
);
commandBuffer.drawIndexed( // 绘制兔子
    static_cast<uint32_t>(m_indices.size() - m_firstIndices[1]),
    BUNNY_NUMBER,
    m_firstIndices[1],
    0, 
    1
);
```

使用 `pushConstants` 函数向命令缓冲中写入推送常量数据，模版函数可以简化代码。
由于最后一个参数是代理数组，无法输入字面量，需要创建一个变量 `enableTexture` 。

在绘制房屋时推送常量的内容为 1 ，绘制兔子时为 0 。

### 3. 着色器代码

我们可以在片段着色中直接读取推送常量的值：

```glsl
layout(push_constant) uniform PushConstants {
    uint enableTexture;
} pc;

......

void main() {
    if (pc.enableTexture > 0) {
        outColor = texture(texSampler, fragTexCoord);
    } else {
        outColor = vec4(fragColor, 1);
    }
}
```

因为推送常量只有一个数据区，直接使用 `push_constant` 标记即可。

> `PushConstants` 是自定义的类型名。

之前提到可以使用 `Range` 和 `offset` 划分数据段，假设某个数据段的起始偏移是 64 字节，那么可以这样读取：

```glsl
layout(push_constant) uniform PushConstants {
    layout(offset = 64) typeName varName;
} pc;
```

### 4. 测试

推送常量的使用就这么简单，设置管线布局 + 推送命令 + 着色器读取。
现在可以重新构建和运行程序，你看到的画面和上一章没有什么区别：

![random_bunny](../../images/0340/random_bunny.png)

## **删除冗余代码**

我们使用推送常量设置纹理标志位，实例缓冲中就不需要设置了。

首先修改 `InstanceData` 结构体，删除 `enableTexture` 成员变量：

```cpp
struct alignas(16) InstanceData {
    glm::mat4 model;
    // uint32_t enableTexture;
    ......
};
```

然后修改 `getAttributeDescriptions` 函数，现在只需要四个 `location` 了：

```cpp
static std::array<vk::VertexInputAttributeDescription, 4>  getAttributeDescriptions() {
    std::array<vk::VertexInputAttributeDescription, 4> attributeDescriptions;
    for(uint32_t i = 0; i < 4; ++i) {
        attributeDescriptions[i].binding = 1;
        attributeDescriptions[i].location = 3 + i;
        attributeDescriptions[i].format = vk::Format::eR32G32B32A32Sfloat;
        attributeDescriptions[i].offset = sizeof(glm::vec4) * i;
    }
    return attributeDescriptions;
}
```

还有 `initInstanceDatas` 函数，不需要再初始化纹理控制字段：

```cpp
void initInstanceDatas() {
    ......

    // instanceData.enableTexture = 1;

    ......

    // instanceData.enableTexture = 0;

    ......
}
```

最后一件事，修改两个顶点着色器代码，删除输入输出的纹理控制字段内容：

顶点着色器：
```glsl
...
// layout(location = 7) in uint inEnableTexture;
...
// layout(location = 2) flat out uint enableTexture;
...
// enableTexture = inEnableTexture;
```

片段着色器：
```glsl
// layout(location = 2) flat in uint enableTexture;
```

然后重新构建和运行，依然能看到之前的内容，且没有报错。

---

**[C++代码](../../codes/03/50_pushconstant/main.cpp)**

**[C++代码差异](../../codes/03/50_pushconstant/main.diff)**

**[根项目CMake代码](../../codes/03/50_pushconstant/CMakeLists.txt)**

**[shader-CMake代码](../../codes/03/50_pushconstant/shaders/CMakeLists.txt)**

**[shader-vert代码](../../codes/03/50_pushconstant/shaders/shader.vert)**

**[shader-vert代码差异](../../codes/03/50_pushconstant/shaders/vert.diff)**

**[shader-frag代码](../../codes/03/50_pushconstant/shaders/shader.frag)**

**[shader-frag代码差异](../../codes/03/50_pushconstant/shaders/frag.diff)**

---
