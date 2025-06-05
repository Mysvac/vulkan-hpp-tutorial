# **加载模型**

## **前言**

经过前面的内容，程序已经可以渲染三维网格体了。这一章节我们将从模型文件中加载顶点和索引，而不再硬编码于C++代码中。

许多图形API教程都会让读者在本章中编写自己的OBJ文件加载器。
但任何稍微有趣些的3D程序就会需要此文件格式不支持的功能，比如骨骼动画。
我们将在本章中从OBJ模型加载网格数据，但我们更关注如何使用这些数据，而不是如何从文件读取。

## **库**

我们将使用 [tinyobjloader](https://github.com/syoyo/tinyobjloader) 库用于加载 OBJ 文件的数据。
它像stb_image一样是单头文件库，你可以直接去仓库下载此文件，但我们依然使用VCPkg安装。

```shell
vcpkg install tinyobjloader
```

然后在CMakeLists.txt中导入库：

```cmake
find_package(tinyobjloader CONFIG REQUIRED)

...

target_link_libraries(${PROJECT_NAME} PRIVATE tinyobjloader::tinyobjloader)
```

## **示例网格**

本章中我们依然不启用光照系统，所以我们将使用把光照烘焙到纹理上的模型。
查找此类模型的简便方法是在 [Sketchfab](https://sketchfab.com/) 上查找。该网站上的许多模型都以 OBJ 格式提供，并具有宽松的许可证。

本教程使用 [Viking room](https://sketchfab.com/3d-models/viking-room-a49f1b8e4f5c4ecf9e1fe7d81915ad38) 模型，作者是 [nigelgoh](https://sketchfab.com/nigelgoh) \([CC BY 4.0](https://web.archive.org/web/20200428202538/https://sketchfab.com/3d-models/viking-room-a49f1b8e4f5c4ecf9e1fe7d81915ad38)\)。
原教程文档作者调整了模型的大小和方向，可以直接点击下方的链接下载：

- [viking_room.obj](../../res/viking_room.obj)
- [viking_room.png](../../res/viking_room.png)

欢迎使用你自己的模型，但请确保它仅包含一种材质，且尺寸约 1.5*1.5*1.5 单位。
如果它大于此尺寸，你必须更改视口矩阵。

现在在 `shaders` 和 `textures` 旁创建新文件夹 `models`，将 OBJ 文件放入此文件夹，将纹理图像放入 `textures`文件夹。

在您的程序中放置两个新的配置变量，以定义模型和纹理路径

```cpp
static constexpr uint32_t WIDTH = 800;
static constexpr uint32_t HEIGHT = 600;

inline static const std::string MODEL_PATH = "models/viking_room.obj";
inline static const std::string TEXTURE_PATH = "textures/viking_room.png";
```

并更新 `createTextureImage` 以使用此路径变量

```cpp
stbi_uc* pixels = stbi_load(TEXTURE_PATH.c_str(), &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);
```

## **前置准备**

### 1. 修改顶点和索引变量

我们现在需要从模型中加载数据，顶点和索引不能再作为静态成员常量了。
现在将他们修改为成员变量，记得去除const修饰：

```cpp
std::vector<Vertex> m_vertices;
std::vector<uint32_t> m_indices;
vk::raii::DeviceMemory m_vertexBufferMemory{ nullptr };
vk::raii::Buffer m_vertexBuffer{ nullptr };
```

> 如果提示找不到 `Vertex` ，可以提供前向声明或者把类定义前移动。

注意我们把索引的单个数据改成了 `uint32_t` ，因为模型顶点数超过了 65535 。
记得修改 `recordCommandBuffer` 中的绑定语句：

```cpp
commandBuffer.bindIndexBuffer( m_indexBuffer, 0, vk::IndexType::eUint32 );
```

注意我们修改了变量名，加了 `m_`前缀用于区分是不是成员变量，你可以不这么做。现在需要修改几处地方：

```cpp
void recordCommandBuffer(const vk::raii::CommandBuffer& commandBuffer, uint32_t imageIndex) {
    ...
    commandBuffer.drawIndexed(static_cast<uint32_t>(m_indices.size()), 1, 0, 0, 0);
    ...
}
void createVertexBuffer() {
    vk::DeviceSize bufferSize = sizeof(m_vertices[0]) * m_vertices.size();
    ...
    memcpy(data, m_vertices.data(), static_cast<size_t>(bufferSize));
    ...
}
void createIndexBuffer() {
    vk::DeviceSize bufferSize = sizeof(m_indices[0]) * m_indices.size();
    ...
    memcpy(data, m_indices.data(), static_cast<size_t>(bufferSize));
    ...
}
```

### 2. 导入库

tinyobjloader 库的头文件导入和STB基本一致，导入 `tiny_obj_loader.h` 头文件并在前面加上 `TINYOBJLOADER_IMPLEMENTATION` 保证头文件包含函数主体，避免链接错误、

```cpp
#define TINYOBJLOADER_IMPLEMENTATION
#include <tiny_obj_loader.h>
```

## **加载模型数据**

### 1. 辅助函数

现在创建一个 `loadModel` 函数用于加载顶点和索引数据。它应该在顶点缓冲创建之前执行：

```cpp
void initVulkan() {
    ...
    loadModel();
    createVertexBuffer();
    createIndexBuffer();
    ...
}

...

void loadModel() {

}
```

### 2. 加载模型

使用 `tinyobj::LoadObj` 函数加载模型：

```cpp
void loadModel() {
    tinyobj::attrib_t attrib;
    std::vector<tinyobj::shape_t> shapes;
    std::vector<tinyobj::material_t> materials;
    std::string warn, err;

    if (!tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, MODEL_PATH.c_str())) {
        throw std::runtime_error(warn + err);
    }
}
```

OBJ 文件由位置、法线、纹理坐标和面组成。
面由任意数量的顶点组成，其中每个顶点通过索引引用位置、法线和纹理坐标。
这使得不仅可以重用整个顶点，还可以重用单个属性。

`attrib` 容器在其 `attrib.vertices`、`attrib.normals` 和 `attrib.texcoords` 向量中保存所有位置、法线和纹理坐标。

`shapes` 容器包含所有单独的对象及其面。每个面都由一个顶点数组组成，并且每个顶点都包含位置、法线和纹理坐标属性的索引。

OBJ 模型还可以为每个面定义材质和纹理，但我们将忽略这些。

`err` 字符串包含错误，`warn` 字符串包含加载文件时发生的警告，例如缺少材质定义。
仅当 `LoadObj` 函数返回 `fals`e 时，加载才真正失败。

如上所述，OBJ 文件中的面实际上可以包含任意数量的顶点，而我们的应用程序只能渲染三角形。
幸运的是，`LoadObj` 有一个可选参数可以自动三角化此类面，默认情况下启用该参数。

我们将文件中的所有面组合成一个模型，因此只需遍历所有 `shape`

```cpp
for (const auto& shape : shapes) {

}
```

三角化功能已经确保每个面有三个顶点，因此我们现在可以直接迭代顶点并将它们直接转储到我们的 `vertices` 向量中

```cpp
for (const auto& shape : shapes) {
    for (const auto& index : shape.mesh.indices) {
        Vertex vertex;

        m_vertices.push_back(vertex);
        m_indices.push_back(m_indices.size());
    }
}
```

为了简单起见，我们现在假设每个顶点都是唯一的，因此 `m_indices` 使用简单的自增索引。

`index` 变量的类型为 `tinyobj::index_t`，其中包含 `vertex_index`、`normal_index` 和 `texcoord_index `成员。
我们需要使用这些索引在 `attrib` 数组中查找实际的顶点属性

```cpp
vertex.pos = {
    attrib.vertices[3 * index.vertex_index + 0],
    attrib.vertices[3 * index.vertex_index + 1],
    attrib.vertices[3 * index.vertex_index + 2]
};

vertex.texCoord = {
    attrib.texcoords[2 * index.texcoord_index + 0],
    attrib.texcoords[2 * index.texcoord_index + 1]
};

vertex.color = {1.0f, 1.0f, 1.0f};
```

### 3. 测试与调整

如果您的设备配置不高，建议在 Release 模式或者 -O3 优化下启动程序，否则加载模型可能较慢。

您应该看到类似以下内容

![inverted_texture_coordinates](../../images/inverted_texture_coordinates.png)

太棒了，几何体看起来是正确的，但是纹理似乎有些问题。
OBJ 格式假定一个坐标系，其中垂直坐标 0 表示图像的底部，但是我们使用 Vulkan 坐标系 0 表示图像的顶部。
通过翻转纹理坐标的垂直分量来解决此问题

```cpp
vertex.texCoord = {
    attrib.texcoords[2 * index.texcoord_index + 0],
    1.0f - attrib.texcoords[2 * index.texcoord_index + 1]
};
```

再次运行程序时，您现在应该看到正确的结果

![drawing_model](../../images/drawing_model.png)

> 当模型旋转时，您可能会注意到后部（墙壁的背面）看起来有点奇怪。这是正常的，仅仅是因为该模型设计时就不支持从后侧观看。

## **顶点去重**

上面的代码中，我们使用自增索引，给每个面的每个点都记录了相关信息，并没有真正利用到索引缓冲区。
此时 `vertices` 向量包含大量重复的顶点数据，而我们应该去除重复顶点，并通过索引重用它们。

一种直接的方式是使用`map`或者`unordered_map`来跟踪唯一的顶点和相应的索引。

### 1. unordered_map

我们将使用 `unordered_map` ，这类数据的分布足够随机，通常可以比 `map` 更快。

```cpp
#include <unordered_map>

std::unordered_map<Vertex, uint32_t> uniqueVertices;

for (const auto& shape : shapes) {
    for (const auto& index : shape.mesh.indices) {
        Vertex vertex;

        vertex.pos = {
            attrib.vertices[3 * index.vertex_index + 0],
            attrib.vertices[3 * index.vertex_index + 1],
            attrib.vertices[3 * index.vertex_index + 2]
        };

        vertex.texCoord = {
            attrib.texcoords[2 * index.texcoord_index + 0],
            1.0f - attrib.texcoords[2 * index.texcoord_index + 1]
        };

        vertex.color = {1.0f, 1.0f, 1.0f};

        if (!uniqueVertices.contains(vertex)) {
            uniqueVertices[vertex] = static_cast<uint32_t>(m_vertices.size());
            m_vertices.push_back(vertex);
        }
        m_indices.push_back(uniqueVertices[vertex]);
    }
}
```

每次我们从 OBJ 文件读取顶点时，都会检查之前是否已经有完全一样的顶点。
如果是新顶点，就加入 `uniqueVertices` 中。最后再从 `uniqueVertices` 读取顶点索引。

> 注意`uniqueVertices`是函数局部变量。
> 此处`map`的使用可以优化，减少一次查找次数，但我们使用最简单的写法。

### 2. 自定义比较与哈希

现在程序无法通过编译，因为自定义类型没有对应的哈希函数和等于函数。

> `map` 只需要提供 `<` 运算符重载即可。`unordered_map` 则需要 `==` 和 `std::hash<>` 特化。

自定义比较和哈希的方式有很多，最常见的就是提供哈希模板的特化以及`==`运算符重载。
不过我决定使用一种不太常见的方式：直接向 `std::unordered_map<>` 模板填写第三和第四个模板形参，第三个参数是哈希，第四个是等于。

> 示例代码将`Vertex`放在了`HelloTriangleApplication`的私有区域，导致模板特化和`==`重载较为麻烦。

首先导入 GLM 的哈希库，它封装了 内置向量的哈希函数，可以简化代码。因为是实验性的，我们需要加上 `GLM_ENABLE_EXPERIMENTAL` 宏。

```cpp
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/hash.hpp>
```

模板只接受类型，而 lambda 表达式得到的是类对象，所以我们通过 `decltype()` 获取原类型。
代码像这样：

```cpp
std::unordered_map<
    Vertex, 
    uint32_t,
    decltype( [](const Vertex& vertex) -> size_t {
        return (((std::hash<glm::vec3>()(vertex.pos) << 1) ^ std::hash<glm::vec3>()(vertex.color) ) >> 1) ^
            (std::hash<glm::vec2>()(vertex.texCoord) << 1);
    } ),
    decltype( [](const Vertex& vtx_1, const Vertex& vtx_2){
        return vtx_1.pos == vtx_2.pos && vtx_1.color == vtx_2.color && vtx_1.texCoord == vtx_2.texCoord;
    } )
> uniqueVertices;
```

- 第三个模板形参是哈希算法，看起来比较复杂，但实际只是几个位运算的杂乱组合。
- 第四个模板形参是键的等于算法，我们要求三个内容完全相等。

## **最后**

您现在应该能够成功编译并运行您的程序。
如果您检查 `vertices` 的大小，您将看到它从 `11484` 缩小到 `3566`！
这意味着每个顶点在平均约 3 个三角形中被重用。
这绝对为我们节省了大量 GPU 内存。

---

**[C++代码](../../codes/03/00_loadmodel/main.cpp)**

**[C++代码差异](../../codes/03/00_loadmodel/main.diff)**

**[根项目CMake代码](../../codes/03/00_loadmodel/CMakeLists.txt)**

**[shader-CMake代码](../../codes/02/40_depthbuffer/shaders/CMakeLists.txt)**

**[shader-vert代码](../../codes/02/40_depthbuffer/shaders/shader.vert)**

**[shader-frag代码](../../codes/02/40_depthbuffer/shaders/shader.frag)**

