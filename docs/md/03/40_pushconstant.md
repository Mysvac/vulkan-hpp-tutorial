# **推送常量**

不同几何体的模型矩阵常常不同，而我们的 uniform 缓冲区\(UBO\)中只有一个模型矩阵。
我们当然可以在 UBO 中载入全部需要的模型矩阵，但在这一节内容，我们将介绍一种更简单的方式 -- **推送常量（Push Constants）**。

推送常量是一种在 GPU 着色器中传递少量数据的高效机制，一种可快速访问的小型内存区域，适合传递频繁变化但体积小的数据（例如变换矩阵、颜色、标志位等）。

| 特性             | 推送常量（Push Constants）         | Uniform 缓冲区（UBO）         |
|------------------|-----------------------------------|-------------------------------|
| 适用场景         | 小数据量、频繁变化                 | 大数据量、变化不频繁           |
| 典型用途         | 每物体模型矩阵、标志位等           | 全局参数、灯光、投影矩阵等     |
| 最大容量         | 通常 128 字节（硬件相关）           | 通常几 KB 甚至更大（硬件相关） |
| 更新方式         | `vkCmdPushConstants`，无需缓冲区    | 需创建/更新缓冲区              |
| 访问速度         | 更快                               | 快                            |
| 资源管理         | 无需显式分配内存                   | 需管理缓冲区                   |
| 适合实例渲染     | 不适合大批量实例数据                | 适合存储大量实例数据           |

## **添加新模型**

还记得“移动摄像机”章节提到的内容吗？现在使用的模型OBJ文件采用Z轴向上的方式，导致我们不得不使用模型矩阵将其旋转。

而本章我们会为场景再添加一个模型，它是Y轴向上的，不需要旋转。
我们将通过推送常量的方式为着色器提供模型矩阵，而视口与投影矩阵依然使用之前 UBO 的方式。

### 1. 下载模型

可以点击 **[这里](../../res/bunny.obj)** 下载模型，将它也放入“models”文件夹中。

[![bunny](../../images/0340/bunny.png)](../../res/bunny.obj)

### 2. 读入模型

现在添加新的静态常量成员，存放模型的路径：

```cpp
inline static const std::string BUNNY_PATH = "models/bunny.obj";
```

我们可以复用使用之前的 `loadModel` 函数，但需要略微调整函数签名和内容：

```cpp

void loadModel(const std::string& model_path) {
    ......
    if (!tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, model_path.c_str())) {
        throw std::runtime_error(warn + err);
    }
    ......
    static std::unordered_map<
        ......
    > uniqueVertices;
    ......
}
```

我们将模型路径作为参数名，并且将 `uniqueVertices` 设为了 `static` ，保证多次模型重用同一个顶点数据哈希表。

现在可以修改 `initVulkan` 函数，加载两个模型：

```cpp
loadModel(MODEL_PATH);
loadModel(BUNNY_PATH);
```

### 3. 纹理映射与顶点色彩

如果你打开兔子模型的obj文件，会注意到它没有法线也没有纹理坐标，只有最普通的顶点集和三角形集，所以我需要修改代码防止数组越界：

```cpp
// 检查是否有纹理坐标
if (!attrib.texcoords.empty() && index.texcoord_index >= 0) {
    vertex.texCoord = {
        attrib.texcoords[2 * index.texcoord_index],
        1.0f - attrib.texcoords[2 * index.texcoord_index + 1]
    };
} else {
    vertex.texCoord = {0.61f, 0.17f}; // 暂时选用一个灰色的纹理点
}
```

### 4. 运行

现在启动程序，仔细观察，兔子出现在中央锅炉旁，半身嵌入地下且身体翻转：

![half_bunny](../../images/0340/half_bunny.png)

> 使用 `WSAD` 进行水平移动摄像头， `Space` 和 `LShift` 升降高度，`↑↓←→` 转动视角。

## **使用推送常量**

### 1. 推送常量数据

首先添加一个新类型，存放推送常量的数据：

```cpp
struct PushConstantData {
    glm::mat4 model;
    uint enableTexture;
};
```

我们需要使用两个数据，分别是模型矩阵和纹理控制标志位。
因为兔子模型没有纹理，我们希望为之前的场景模型启用纹理采样，而兔子模型直接使用顶点色彩（我们在 `loadModel` 中将顶点色彩设置为了 `(1.0f,1.0f,1.0f)`）。

### 2. 管线布局

现在需要修改管线布局，加入推送常量的信息。修改 `createGraphicsPipeline` 函数：

```cpp
vk::PushConstantRange pushConstantRange;
pushConstantRange.stageFlags = vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment;
pushConstantRange.offset = 0;
pushConstantRange.size = sizeof(PushConstantData);

vk::PipelineLayoutCreateInfo pipelineLayoutInfo;
pipelineLayoutInfo.setPushConstantRanges( pushConstantRange );
```

`pushConstantRange` 是 Vulkan 中用于描述推送常量在着色器中的可用范围的结构体。它告诉 Vulkan：

- 这些推送常量会被哪些着色器阶段访问
- 推送常量数据的在数据区的起始偏移
- 推送常量数据的大小

注意到我们可以指定多个推送常量范围，但实际上所有推送常量都在同一个数据区中，需要指定的是每个小范围的大小和偏移量。
我们的模型矩阵只需要在顶点着色器访问，纹理控制矩阵只需要在片段着色器访问，所以你还可以这样写：

```cpp
std::array<vk::PushConstantRange, 2> pushConstantRanges;
pushConstantRanges[0].stageFlags = vk::ShaderStageFlagBits::eVertex;
pushConstantRanges[0].offset = 0;
pushConstantRanges[0].size = sizeof(glm::mat4);
pushConstantRanges[1].stageFlags = vk::ShaderStageFlagBits::eFragment;
pushConstantRanges[1].offset = sizeof(glm::mat4);
pushConstantRanges[1].size = sizeof(uint32_t);

vk::PipelineLayoutCreateInfo pipelineLayoutInfo;
pipelineLayoutInfo.setPushConstantRanges( pushConstantRanges );
```

后一种写法只允许顶点着色器访问模型矩阵，只允许片段着色器访问纹理控制矩阵，因此它的封装性更好。
但需要注意，二者的性能其实差不多，因为推送常量本就只用于处理小量数据，且两种方法并没有改变实际的数据量。

作为教程，我们会使用第二种写法，因为它包含了第一种的内容，还要求我们在着色器中指定偏移量才能正确访问数据。

### 3. 着色器代码

上面提到所有推送常量位于同一个数据区，所以我们不需要像UBO那样设置绑定点，只需要声明 `push_constant` 和 `PushConstants` 即可。

首先是顶点着色器，我们需要用到模型矩阵，它的偏移量是 0 ，所以不需要显式指定：

```glsl
layout(push_constant) uniform PushConstants {
    mat4 model;
} pc;

......

void main() {
    gl_Position = ubo.proj * ubo.view * pc.model * vec4(inPosition, 1.0);
    ......
}
```

然后是片段着色器，它的偏移量是一个4*4矩阵的大小，每个元素是32为浮点数，共64字节，所以我们应该这样写：

```glsl
layout(push_constant) uniform PushConstants {
    layout(offset = 64) uint enableTexture;
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

可以发现显式指定偏移量还是挺麻烦的。如果你不那么在意封装性，使用上面的第一种推送常量布局写法，那么两个着色器都可以访问模型矩阵和纹理控制字段，则都可以直接写成下面这样：

```glsl
layout(push_constant) uniform PushConstants {
    mat4 model;
    uint enableTexture;
} pc;
```

### 4. 模型分离

推送常量将在命令缓冲记录时填写，但在此之前还有一个问题。
我们有两个模型，需要两个不同的推送常量内容，要指定哪些顶点使用第一个内容、哪些顶点使用第二个。

我们现在是根据索引绘制图像的，因此可以修改 `loadModel` 函数，并用一个成员变量记录每个模型对应的的索引偏移量：

```cpp
std::vector<Vertex> m_vertices;
std::vector<uint32_t> m_indices;
std::vector<uint32_t> m_indicesOffsets; // 索引偏移量

......

void loadModel(const std::string& model_path) {
    ......

    if (!tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, model_path.c_str())) {
        throw std::runtime_error(warn + err);
    }

    m_indicesOffsets.push_back(m_indices.size());
    ......
}
```

### 4. 命令记录

然后在命令缓冲记录时\(`recordCommandBuffer`函数\)，为不同的模型使用不同的推送常量：

```cpp
for(size_t counter = 1; const uint32_t firstIndex : m_indicesOffsets) {
    PushConstantData pcData;
    if(counter == 1) {
        pcData.model = glm::rotate(
            glm::mat4(1.0f), 
            glm::radians(-90.0f), 
            glm::vec3(1.0f, 0.0f, 0.0f)
        )  * glm::rotate(
            glm::mat4(1.0f), 
            glm::radians(-90.0f), 
            glm::vec3(0.0f, 0.0f, 1.0f)
        );
        pcData.enableTexture = 1;
    } else {
        pcData.model = glm::mat4(1.0f);
        pcData.enableTexture = 0;
    }
    commandBuffer.pushConstants<glm::mat4>(
        m_pipelineLayout,
        vk::ShaderStageFlagBits::eVertex,
        0, // offset
        pcData.model
    );
    commandBuffer.pushConstants<uint32_t>(
        m_pipelineLayout,
        vk::ShaderStageFlagBits::eFragment,
        sizeof(glm::mat4), // offset
        pcData.enableTexture
    );
    commandBuffer.drawIndexed(
        counter == m_indicesOffsets.size() ? m_indices.size() - firstIndex : m_indicesOffsets[counter] - firstIndex,
        1,
        firstIndex,
        0,
        0
    );
    ++counter;
}
```

我们使用一个循环，遍历所有的模型。第一个模型是场景，需要旋转和纹理采样；第二个模型是兔子，暂时不移动，不需要纹理采样。

然后使用 `pushConstants` 函数传入推送常量的数据，可以使用模版函数简化代码。

最后根据索引绘制图像，第一个参数是索引数，第二个参数是实例数，第三个参数是索引开始位置。
索引数量可以用后一个开始位置减去当前开始位置得到，注意最后一个模型没有后一个开始位置，需要使用总数减去当前开始位置。。

如果你使用上面的第一种推送常量的管线布局写法，可以将两条 `pushConstants` 语句合并为一条：

```cpp
commandBuffer.pushConstants<PushConstantData>(
    m_pipelineLayout,
    vk::ShaderStageFlagBits::eFragment,
    0,
    pcData
);
```

### 5. 移动兔子

现在运行程序，你会发现兔子已经摆正，但位置依然不太好，半个模型位于锅炉内。
可以调整上面的模型矩阵从而移动兔子，下面给出一个例子：

```cpp
pcData.model = glm::translate(glm::mat4(1.0f), glm::vec3(0.5f, 0.12f, 0.0f));
```

你应该能看到兔子移动到了石桌上：

![move_bunny](../../images/0340/move_bunny.png)

## **删除冗余内容**

现在，UBO中的模型矩阵已经用不到了，我们可以将其删除。

首先修改 `UniformBufferObject` ，删除第一个成员变量：

```cpp
struct alignas(16) UniformBufferObject {
    glm::mat4 view;
    glm::mat4 proj;
};
```

然后调整 `updateUniformBuffer` ，删除模型矩阵的代码：

```cpp
void updateUniformBuffer(uint32_t currentImage) {
    updateCamera();

    glm::vec3 front;
    front.x = std::cos(glm::radians(m_yaw)) * std::cos(glm::radians(m_pitch));
    front.y = std::sin(glm::radians(m_pitch));
    front.z = std::sin(glm::radians(m_yaw)) * std::cos(glm::radians(m_pitch));
    front = glm::normalize(front);

    UniformBufferObject ubo{};
    ubo.view = glm::lookAt(
        m_cameraPos, 
        m_cameraPos + front, 
        m_cameraUp
    );
    ubo.proj = glm::perspective(
        glm::radians(45.0f), 
        static_cast<float>(m_swapChainExtent.width) / m_swapChainExtent.height, 
        0.1f, 
        10.0f
    );

    ubo.proj[1][1] *= -1;

    memcpy(m_uniformBuffersMapped[currentImage], &ubo, sizeof(ubo));
}
```

别忘了顶点着色器中也需要删除UBO的模型矩阵：

```glsl
layout(binding = 0) uniform UniformBufferObject {
    mat4 view;
    mat4 proj;
} ubo;
```

`loadModel` 中我们检查了模型纹理坐标是否为空，如果为空则指定了默认值。
现在此默认值的设置也可以省略：

```cpp
// 检查是否有纹理坐标
if (!attrib.texcoords.empty() && index.texcoord_index >= 0) {
    vertex.texCoord = {
        attrib.texcoords[2 * index.texcoord_index],
        1.0f - attrib.texcoords[2 * index.texcoord_index + 1]
    };
} 
// else {
//     vertex.texCoord = {0.61f, 0.17f}; // 暂时选用一个灰色的点
// }
```

然后重新运行程序，与之前没有差别。

---

**[C++代码](../../codes/03/40_pushconstant/main.cpp)**

**[C++代码差异](../../codes/03/40_pushconstant/main.diff)**

**[根项目CMake代码](../../codes/03/40_pushconstant/CMakeLists.txt)**

**[shader-CMake代码](../../codes/03/40_pushconstant/shaders/CMakeLists.txt)**

**[shader-vert代码](../../codes/03/40_pushconstant/shaders/shader.vert)**

**[shader-vert代码差异](../../codes/03/40_pushconstant/shaders/vert.diff)**

**[shader-frag代码](../../codes/03/40_pushconstant/shaders/shader.frag)**

**[shader-frag代码差异](../../codes/03/40_pushconstant/shaders/frag.diff)**

