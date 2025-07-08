---
title: 暂存缓冲区
comments: true
---
# **暂存缓冲**

## **前言**

我们现在的顶点缓冲已经可以正常工作了，但它的内存类型还不是最佳选择。

对于显卡而言，最佳内存类型应该有`vk::MemoryPropertyFlagBits::eDeviceLocal`标志。
也就是设备本地的内存，实际是指显卡上的内存（显存），通常无法被CPU直接访问。

容易理解， CPU 访问主存更快、而 GPU 访问显存更快，交叉访问却比较麻烦，所以我们最好需要两个缓冲区：

1. 位于主存，像上一节一样，我们称其为暂存缓冲\(Staging Buffer\)。
2. 位于显存，是我们最终需要的顶点缓冲，类型是设备本地缓冲\(Device Local\)。

我们的 CPU 向暂存缓冲写数据，然后通过某种方式将数据从暂存缓冲复制到最终缓冲，然后 GPU 从最终缓冲中读取（使用最终缓冲绑定顶点输入）。

## **转移队列**

在缓冲之间复制数据需要支持相关操作的队列族，附带 `vk::QueueFlagBits::eTransfer` 标志位。
好消息是满足 `eGraphics` 或 `eCompute` 的队列族已经隐式支持了 `eTransfer` 操作。
所以我们可以不创建新队列族。

如果您喜欢挑战，那么仍可以尝试使用专用于传输操作的队列族。这将需要您对程序进行以下修改

- 修改 `QueueFamilyIndices` 和 `findQueueFamilies` 以显式查找具有 `eTransfer` 但没有 `eGraphics` 标志位的队列族。

- 修改 `createLogicalDevice` 以获取传输队列的句柄

- 为传输队列族需要的命令缓冲创建第二个命令池

- 将资源的 `sharingMode` 更改为 `vk::SharingMode::eConcurrent` ，并指定图形和传输队列族

- 将任何传输命令（例如 `copyBuffer`，我们将在本章中使用它）提交到传输队列，而不是图形队列

这有点工作量，但它会教您很多关于资源如何在队列族之间共享的知识。

## **修改缓冲创建**

因为我们需要创建多个缓冲，最好将单个缓冲创建的代码移入一个辅助函数。
现在创建一个新函数 `createBuffer` ，并将 `createVertexBuffer` 的部分代码写入此函数：

```cpp
void createBuffer(
    const vk::DeviceSize size,
    const vk::BufferUsageFlags usage,
    const vk::MemoryPropertyFlags properties,
    vk::raii::Buffer& buffer,
    vk::raii::DeviceMemory& bufferMemory
) {
    vk::BufferCreateInfo bufferInfo;
    bufferInfo.size = size;
    bufferInfo.usage = usage;
    bufferInfo.sharingMode = vk::SharingMode::eExclusive;

    buffer = m_device.createBuffer(bufferInfo);

    const vk::MemoryRequirements memRequirements = buffer.getMemoryRequirements();

    vk::MemoryAllocateInfo allocInfo;
    allocInfo.allocationSize = memRequirements.size;
    allocInfo.memoryTypeIndex = findMemoryType(memRequirements.memoryTypeBits, properties);

    bufferMemory = m_device.allocateMemory( allocInfo );

    buffer.bindMemory(bufferMemory, 0);
}
```

我们增加了一些函数参数，使这个函数更加通用，可以创建不同的缓冲和资源。

现在可以修改 `createVertexBuffer`， 调用我们刚刚写的 `createBuffer` 创建资源：

```cpp
void createVertexBuffer() {
    const vk::DeviceSize bufferSize = sizeof(Vertex) * vertices.size();

    createBuffer(bufferSize,
        vk::BufferUsageFlagBits::eVertexBuffer,
        vk::MemoryPropertyFlagBits::eHostVisible |
        vk::MemoryPropertyFlagBits::eHostCoherent,
        m_vertexBuffer,
        m_vertexBufferMemory
    );

    void* data = m_vertexBufferMemory.mapMemory(0, bufferSize);
    memcpy(data, vertices.data(), bufferSize);
    m_vertexBufferMemory.unmapMemory();
}
```

现在运行程序，保证程序依然正常工作。

## **使用暂存缓冲**

现在我们修改 `createVertexBuffer`，主机可见缓冲将只作为临时缓冲，然后创建设备本地缓冲作为实际的顶点缓冲。

```cpp
void createVertexBuffer() {
    const vk::DeviceSize bufferSize = sizeof(Vertex) * vertices.size();

    vk::raii::DeviceMemory stagingBufferMemory{ nullptr };
    vk::raii::Buffer stagingBuffer{ nullptr };
    createBuffer(bufferSize, 
        vk::BufferUsageFlagBits::eTransferSrc, 
        vk::MemoryPropertyFlagBits::eHostVisible | 
        vk::MemoryPropertyFlagBits::eHostCoherent,
        stagingBuffer, 
        stagingBufferMemory
    );

    void* data = stagingBufferMemory.mapMemory(0, bufferSize);
    memcpy(data, vertices.data(), static_cast<size_t>(bufferSize));
    stagingBufferMemory.unmapMemory();

    createBuffer(bufferSize, 
        vk::BufferUsageFlagBits::eTransferDst |
        vk::BufferUsageFlagBits::eVertexBuffer, 
        vk::MemoryPropertyFlagBits::eDeviceLocal,
        m_vertexBuffer, 
        m_vertexBufferMemory
    );
}
```

`m_vertexBuffer` 的内存类型现在变成了 `eDeviceLocal` ，无法直接被CPU访问，所以我们不能简单地使用 `mapMemory` 。
但是我们可以将数据从 `stagingBuffer` 拷贝到 `m_vertexBuffer` 。所以我们修改了 `BufferUsageFlags`标志位:

- `eTransferSrc` 表示缓冲可以作为内存传输操作的源
- `eTransferDst` 表示缓冲可以作为内存传输操作的目的地

## **缓冲拷贝函数**

我们现在创建一个新函数 `copyBuffer` ，用于缓冲之间的数据拷贝，

```cpp
void copyBuffer(const vk::raii::Buffer& srcBuffer,const vk::raii::Buffer& dstBuffer,const vk::DeviceSize size) const {

}
```

内存传输操作需要使用命令缓冲，我们必须分配一个临时的命令缓冲用于命令的录制和提交。
同时最好为这些临时命令缓冲创建一个独立的命令池，程序可以更好进行资源分配的优化，此时需要使用 `vk::CommandBufferLevel::ePrimary` 标记。

```cpp
void copyBuffer(const vk::raii::Buffer& srcBuffer,const vk::raii::Buffer& dstBuffer,const vk::DeviceSize size) const {
    vk::CommandBufferAllocateInfo allocInfo;
    allocInfo.level = vk::CommandBufferLevel::ePrimary;
    allocInfo.commandPool = m_commandPool;
    allocInfo.commandBufferCount = 1;

    // std::vector<vk::raii::CommandBuffer>
    auto commandBuffers = m_device.allocateCommandBuffers(allocInfo);
    const vk::raii::CommandBuffer commandBuffer = std::move(commandBuffers.at(0));
}
```

然后开始录制命令缓冲

```cpp
vk::CommandBufferBeginInfo beginInfo;
beginInfo.flags = vk::CommandBufferUsageFlagBits::eOneTimeSubmit;

commandBuffer.begin(beginInfo);
```

我们只使用此命令缓冲一次，所以使用 `eOneTimeSubmit` 标志位。

使用 `copyBuffer` 成员函数录制拷贝命令，输入源缓冲和目标缓冲，以及需要拷贝的区域。

```cpp
vk::BufferCopy copyRegion;
copyRegion.srcOffset = 0; // optional
copyRegion.dstOffset = 0; // optional
copyRegion.size = size;
commandBuffer.copyBuffer(srcBuffer, dstBuffer, copyRegion);
```

然后就可以结束录制了，我们只需要这一个操作。

```cpp
commandBuffer.end();
```

与绘制命令不同，我们只希望它立刻执行内存传输命令。
我们至少有两种方式等待内存传输完成，使用围栏 `Fence` 进行同步或直接 `waitIdle` ，这里使用后者：

```cpp
vk::SubmitInfo submitInfo;
submitInfo.setCommandBuffers( *commandBuffer );

m_graphicsQueue.submit(submitInfo);
m_graphicsQueue.waitIdle();
```

> 使用 `Fence` 更精准，允许你同时同步地进行多条传输命令，可以带来更好的性能优化。

## **最后**

现在在 `createVertexBuffer` 函数末尾调用 `copyBuffer` ，将暂存缓冲的数据拷贝到设备本地缓冲。

```cpp
copyBuffer(stagingBuffer, m_vertexBuffer, bufferSize);
```

现在尝试运行程序，保证程序可以正常执行。

---

## **注意**

在实际应用中，你不应该给每个缓冲都调用一次 `allocateMemory` 。
同时为大量对象分配内存的正确方法是创建一个自定义分配器，该分配器通过使用我们在许多函数中看到的 `offset` 参数将单个资源拆分到许多不同的对象中。

你可以自己实现这样的分配器，也可以使用 [VulkanMemoryAllocator-Hpp](https://github.com/YaaZ/VulkanMemoryAllocator-Hpp) 这样的第三方库。

但是对于本教程，为每个资源使用单独的分配是可以的，因为我们的数据量很小。

> 我们将在进阶章节详细讨论内存分配器内容。

---

**[C++代码](../../codes/02/02_stagingbuffer/main.cpp)**

**[C++代码差异](../../codes/02/02_stagingbuffer/main.diff)**

**[根项目CMake代码](../../codes/02/00_vertexinput/CMakeLists.txt)**

**[shader-CMake代码](../../codes/02/00_vertexinput/shaders/CMakeLists.txt)**

**[shader-vert代码](../../codes/02/00_vertexinput/shaders/graphics.vert.glsl)**

**[shader-frag代码](../../codes/02/00_vertexinput/shaders/graphics.frag.glsl)**

---
