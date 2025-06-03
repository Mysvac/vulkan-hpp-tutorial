# **纹理**

## **前言**

我们将从这一节开始使用纹理映射让图像看起来更加有趣，这也是为后面的3D模型章节打基础。

添加纹理映射需要这些步骤：

1. 在内存中创建图像对象
2. 用图像文件的像素填充它
3. 创建图像采样器
4. 添加组合图像采样器描述符，以从纹理获取颜色

为了让着色器读取纹理，我们需要一个着色器可读图像\(有ShaderRead标志的VkImage\)。
我们之前已经使用过交换链创建的图像对象`vk::Image`，现在我们需要创建自己的图像对象。

要让着色器读取，资源位于显存，CPU无法直接写入，我们需要通过缓冲\(VkBuffer\)或暂存图像\(有Src的VkImage\)中转数据。

使用暂存缓冲的性能往往不低于暂时图像，所以本教程使用第一种方式。

我们先创建此缓冲区并用像素值填充，然后创建一个图像将像素复制到其中。这和之前的顶点缓冲过程基本一致。

不过我们还需要注意一些事情，图像可以有不同的布局，这些布局会影响图像在内存中的组织方式。
由于显卡的工作模式差异，按行存储图像资源未必带来最佳性能表现，我们需要为图像指定合适的布局方式。

实际上我们在创建渲染通道时已经接触过了一些布局：

|    `vk::ImageLayout`  |      含义      |
|-----------------------|----------------|
| `ePresentSrcKHR` | 优化呈现    |
| `eColorAttachmentOptimal` | 优化色彩的修改 |
| `eTransferSrcOptimal` | 资源传输时作为源优化 |
| `eTransferDstOptimal` | 资源传输时作为目标优化 |
| `eShaderReadOnlyOptimal` | 优化着色器采样 |

一种常见的修改图像布局的方式是使用管线屏障(pipeline barrier)，我们会在本章节中向你展示。

> 另一种常见的方式是交给渲染通道处理，就像我们的颜色附件一样。

## **图像库**

Vulkan不含内置的图像/模型加载工具，你需要使用第三方库，或者自己写一个程序加载简单的图像数据。

你可以选择自己喜欢的库，我们只用它加载图片，所以它只会出现一次且代码量极少，无需担心跟不上本教程的内容。

本教程将使用`stb_image`库，它属于 [stb库](https://github.com/nothings/stb) ，单头文件且足够轻量，支持 PNG、JPG、BMP 等常见格式。

你可以直接去它的 [Github仓库](https://github.com/nothings/stb) 下载`stb_image.h` 头文件，然后直接放在项目中使用，记得修改CMake。
但本教程依然使用VCPkg安装它，使用命令：

```shell
vcpkg install stb
```

然后修改`CMakeLists.txt`，添加以下内容：

```cmake
find_package(Stb REQUIRED)

······

target_include_directories(${PROJECT_NAME} PRIVATE ${Stb_INCLUDE_DIR})
```

## **加载图像**

现在在程序中添加头文件从而导入库：

```cpp
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>
```

它是仅头文件库，但默认只有函数签名，我们需要使用 `STB_IMAGE_IMPLEMENTATION` 宏让他包含函数主体。

现在添加新函数 `createTextureImage` 用于创建纹理图像对象。
我们需要创建临时命令缓冲，所以需要把它放在`createCommandPool`之后：

```cpp
void initVulkan() {
    ...
    createCommandPool();
    createTextureImage();
    createVertexBuffer();
    ...
}

...

void createTextureImage() {

}
```

现在在项目根目录创建一个新文件夹 `textures` 用于存放图像资源，文件夹与 `shaders` 平级。
本教程将使用 [CC0 licensed image](https://pixabay.com/en/statue-sculpture-fig-historically-1275469/)，你可以使用自己喜欢的图像。

原教程已经将此图像修改成了 512*512 像素，并改名为 `texture.jpg`，你可以直接点击下方的图像并保存：

![texture.jpg](../images/texture.jpg)

现在可以像下面的代码一样加载图像：

```cpp
void createTextureImage() {
    int texWidth, texHeight, texChannels;
    stbi_uc* pixels = stbi_load("textures/texture.jpg", &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);
    if (!pixels) {
        throw std::runtime_error("failed to load texture image!");
    }
    vk::DeviceSize imageSize = texWidth * texHeight * 4;
}
```

`stbi_load` 函数用于加载图像，输入图像位置和需要加载的通道数，返回数据指针以及宽高和实际色彩通道数。
我们使用 `STBI_rgb_alpha` 让他强制加载4通道，缺少的通道会自动补齐。
一个像素一个通道是1Byte，可以轻松算出图片的总大小。

## **暂存缓冲**

我们需要创建一个主机可见缓冲用于暂存数据，请在 `createTextureImage` **函数中**添加两个临时变量：

```cpp
vk::raii::DeviceMemory stagingBufferMemory{ nullptr };
vk::raii::Buffer stagingBuffer{ nullptr };
```

然后使用之前创建的辅助函数 `createBuffer` 分配资源，这和之前顶点缓冲章节的内容基本一致：

```cpp
createBuffer(
    imageSize,
    vk::BufferUsageFlagBits::eTransferSrc,
    vk::MemoryPropertyFlagBits::eHostVisible |
    vk::MemoryPropertyFlagBits::eHostCoherent,
    stagingBuffer,
    stagingBufferMemory
);
```

然后把资源拷贝到暂存缓冲：

```cpp
void* data = stagingBufferMemory.mapMemory(0, imageSize);
memcpy(data, pixels, static_cast<size_t>(imageSize));
stagingBufferMemory.unmapMemory();
```

最后还需要清理 `stb_image` 库的图像资源：

```cpp
stbi_image_free(pixels);
```

## **纹理图像**

我们还要让着色器能够访问缓冲中的像素值，最好的方式是使用 Vulkan 的图像对象。
图像对象允许我们简单且快速地使用2D坐标检索对应位置的颜色。
图像对象中的像素们被称为纹素(texels)，我们后面会使用此名称。

现在添加两个新的类成员，放在`m_swapChain`的上方：

```cpp
vk::raii::DeviceMemory m_textureImageMemory{ nullptr };
vk::raii::Image m_textureImage{ nullptr };
vk::raii::SwapchainKHR m_swapChain{ nullptr };
```

> 如果你对成员变量的声明顺序不清晰，请参考最下方的C++代码样例。

然后需要添加图像的 `CreateInfo` 结构体：

```cpp
vk::ImageCreateInfo imageInfo;
imageInfo.imageType = vk::ImageType::e2D;
imageInfo.extent.width = static_cast<uint32_t>(texWidth);
imageInfo.extent.height = static_cast<uint32_t>(texHeight);
imageInfo.extent.depth = 1;
imageInfo.mipLevels = 1;
imageInfo.arrayLayers = 1;
```

我们使用 `imageType` 指定图像类型，可以是1D、2D和3D图像，它们在Vulkan中有不同的坐标系统。
一维图像是一个数组，二维常用于存放纹理，三维图形则常用于存放立体元素(voxel volumes)。

`extent`自动指定了图像每个轴包含的纹素数，所以 `depth` 是1。
我们暂时不使用mipmapping，所以`mipLevels`设为了1。
我们只有一副图像，所以`arrayLayers`也为1。

然后指定图像格式，我们强制读取成了RGBA，所以应该这样写：

```cpp
imageInfo.format = vk::Format::eR8G8B8A8Srgb;
```

然后我们需要指定图像中纹素的排列顺序：

```cpp
imageInfo.tiling = vk::ImageTiling::eOptimal;
```

`tilling` 字段至少可以有两个选择：

| `vk::ImageTiling` | 意义 |
|---------|------|
| `eLinear` | 以行优先的顺序排列，就像之前的`pixels`数组一样 |
| `eOptimal` | 以实现定义的顺序排列，以获取最佳访问性能 |

tilling模式在图像创建后之后不能再更改。
如果你希望能够直接访问图像内存中的纹数，应该使用 `eLinear` 。
我们将使用暂存缓冲区而不是暂存图像，所以没这必要，可以使用 `eOptimal` 以便着色器高效访问。

然后设置图像的初始布局：

```cpp
imageInfo.initialLayout = vk::ImageLayout::eUndefined;
```

它也有至少两种选择：

| `vk::ImageLayout` | 意义 |
|---------|------|
| `eUndefined` | GPU 不可用，并且第一次转换期间丢弃纹素 |
| `ePreinitialized` | GPU 不可用，但第一次转换期间保留纹素 |

少数情况下需要在第一次转换期间保留纹素。
比如您想将图像用作暂存图像，并结合 `vk::ImageTiling::eLinear` 布局使用。
此时你需要上传纹素数据，然后将图像转换为传输源，此时不能丢失数据。

但在我们的例子中，我们先将图像转换为传输目的地，之后才从缓冲区复制纹素数据，所以可以使用 `eUndefined`。

然后填写`usage`字段，我们的图像作为缓冲区的复制目标，并且允许着色器进行采样：

```cpp
imageInfo.usage = vk::ImageUsageFlagBits::eTransferDst | vk::ImageUsageFlagBits::eSampled;
```

通过`shaderMode`字段表示图像只由一个队列族使用，通过`samples`字段设置多重采样相关。

```cpp
imageInfo.sharingMode = vk::SharingMode::eExclusive;
imageInfo.samples = vk::SampleCountFlagBits::e1;
```

`flags`字段可以控制一些特点内容，比如一些稀疏图像需要的功能，我们此处不需要指定。

现在可以创建图像了：

```cpp
m_textureImage = m_device.createImage(imageInfo);
```

需要说明的是小部分显卡不支持`vk::Format::eR8G8B8A8Srgb`，使用不同格式需要烦人的转换，所以我们暂时不处理此问题。
我们会在深度缓冲章节回到这里。

现在需要为图像分配内存资源：

```cpp
vk::MemoryRequirements memRequirements = m_textureImage.getMemoryRequirements();
vk::MemoryAllocateInfo allocInfo;
allocInfo.allocationSize = memRequirements.size;
allocInfo.memoryTypeIndex = findMemoryType(memRequirements.memoryTypeBits, vk::MemoryPropertyFlagBits::eDeviceLocal);

m_textureImageMemory = m_device.allocateMemory(allocInfo);

m_textureImage.bindMemory(m_textureImageMemory, 0);
```

上面的内存分配方式和缓冲区的分配几乎完全一致。

现在函数已经变得很大了，我们应该像前面缓冲区的做法一样，独立出一个辅助函数 `createImage` 以便后续重用代码：

```cpp
void createImage(
    uint32_t width,
    uint32_t height,
    vk::Format format,
    vk::ImageTiling tilling,
    vk::ImageUsageFlags usage,
    vk::MemoryPropertyFlags properties,
    vk::raii::Image& image,
    vk::raii::DeviceMemory& imageMemory
) {
    vk::ImageCreateInfo imageInfo;
    imageInfo.imageType = vk::ImageType::e2D;
    imageInfo.extent.width = width;
    imageInfo.extent.height = height;
    imageInfo.extent.depth = 1;
    imageInfo.mipLevels = 1;
    imageInfo.arrayLayers = 1;
    imageInfo.format = format;
    imageInfo.tiling = tilling;
    imageInfo.initialLayout = vk::ImageLayout::eUndefined;
    imageInfo.usage = usage;
    imageInfo.samples = vk::SampleCountFlagBits::e1;
    imageInfo.sharingMode = vk::SharingMode::eExclusive;

    image = m_device.createImage(imageInfo);

    vk::MemoryRequirements memRequirements = image.getMemoryRequirements();
    vk::MemoryAllocateInfo allocInfo;
    allocInfo.allocationSize = memRequirements.size;
    allocInfo.memoryTypeIndex = findMemoryType(memRequirements.memoryTypeBits, properties);

    imageMemory = m_device.allocateMemory(allocInfo);

    image.bindMemory(imageMemory, 0);
}
```

现在将宽度、高度、格式、tiling 模式、usage 和内存属性作为参数，因为这些参数在本教程中创建的图像之间都会有所不同。

现在可以简化 `createTextureImage` :

```cpp
void createTextureImage() {
    int texWidth, texHeight, texChannels;
    stbi_uc* pixels = stbi_load("textures/texture.jpg", &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);
    if (!pixels) {
        throw std::runtime_error("failed to load texture image!");
    }
    vk::DeviceSize imageSize = texWidth * texHeight * 4;

    vk::raii::DeviceMemory stagingBufferMemory{ nullptr };
    vk::raii::Buffer stagingBuffer{ nullptr };

    createBuffer(
        imageSize,
        vk::BufferUsageFlagBits::eTransferSrc,
        vk::MemoryPropertyFlagBits::eHostVisible |
        vk::MemoryPropertyFlagBits::eHostCoherent,
        stagingBuffer,
        stagingBufferMemory
    );

    void* data = stagingBufferMemory.mapMemory(0, imageSize);
    memcpy(data, pixels, static_cast<size_t>(imageSize));
    stagingBufferMemory.unmapMemory();

    stbi_image_free(pixels);

    createImage( 
        texWidth, 
        texHeight,
        vk::Format::eR8G8B8A8Srgb,
        vk::ImageTiling::eOptimal,
        vk::ImageUsageFlagBits::eTransferDst | 
        vk::ImageUsageFlagBits::eSampled,
        vk::MemoryPropertyFlagBits::eDeviceLocal,
        m_textureImage,
        m_textureImageMemory
    );
}
```

## **布局转换**

我们现在要编写的函数再次涉及命令缓冲的记录和执行，现在是时候将此逻辑分离成两个独立函数了。

下面的代码实际上来自于`copyBuffer`函数：

```cpp
vk::raii::CommandBuffer beginSingleTimeCommands() {
    vk::CommandBufferAllocateInfo allocInfo;
    allocInfo.level = vk::CommandBufferLevel::ePrimary;
    allocInfo.commandPool = m_commandPool;
    allocInfo.commandBufferCount = 1;

    auto commandBuffers = m_device.allocateCommandBuffers(allocInfo);
    vk::raii::CommandBuffer commandBuffer = std::move(commandBuffers.at(0));

    vk::CommandBufferBeginInfo beginInfo;
    beginInfo.flags = vk::CommandBufferUsageFlagBits::eOneTimeSubmit;

    commandBuffer.begin(beginInfo);

    return commandBuffer;
}
void endSingleTimeCommands(vk::raii::CommandBuffer commandBuffer) {
    commandBuffer.end();

    vk::SubmitInfo submitInfo;
    submitInfo.setCommandBuffers( *commandBuffer );

    m_graphicsQueue.submit(submitInfo);
    m_graphicsQueue.waitIdle();
}
```

> 使用`endSingleTimeCommands`时需要通过移动语义将命令缓冲移入，函数结束时自动销毁。

现在可以优化 `copyBuffer` 函数：

```cpp
void copyBuffer(vk::raii::Buffer& srcBuffer, vk::raii::Buffer& dstBuffer, vk::DeviceSize size) {
    vk::raii::CommandBuffer commandBuffer = beginSingleTimeCommands();

    vk::BufferCopy copyRegion;
    copyRegion.size = size;
    commandBuffer.copyBuffer(srcBuffer, dstBuffer, copyRegion);

    endSingleTimeCommands( std::move(commandBuffer) );
}
```

我们需要使用 `commandBuffer.copyBufferToImage` 将缓冲区中的数据拷贝到图像，但这需要图像有正确的布局方式。

现在创建一个新函数用于处理布局转换：

```cpp
void transitionImageLayout(
    vk::raii::Image& image,
    vk::Format format,
    vk::ImageLayout oldLayout,
    vk::ImageLayout newLayout
) {
    vk::raii::CommandBuffer commandBuffer = beginSingleTimeCommands();

    endSingleTimeCommands( std::move(commandBuffer) );
}
```

一种最常用的修改图像布局的方式是使用图像内存屏障(image memory barrier)。
管线屏障常用于同步资源访问，比如先写后读。
但它也可以在`vk::SharingMode::eExclusive`时用来变化图像布局和转移队列族所有权。
有一个等效的缓冲区内存屏障可以为缓冲区执行类似此操作。

现在指定图像内存屏障信息：

```cpp
vk::ImageMemoryBarrier barrier;
barrier.oldLayout = oldLayout;
barrier.newLayout = newLayout;
barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
```

我们首先设置了新旧图像布局，然后设置了忽视队列族所有权的传输。
注意后两个字段不是默认值，你必须显式设置`VK_QUEUE_FAMILY_IGNORED`。

然后设置受影响的图像和特定区域：

```cpp
barrier.image = image;
barrier.subresourceRange.aspectMask = vk::ImageAspectFlagBits::eColor;
barrier.subresourceRange.baseMipLevel = 0;
barrier.subresourceRange.levelCount = 1;
barrier.subresourceRange.baseArrayLayer = 0;
barrier.subresourceRange.layerCount = 1;
```

我们的图像不是数组，也没有 mipmapping 级别，因此只有一个级别和层级，

屏障主要用于同步，所以你必须指定屏障开始和等待的操作类型。
及时我们通过其他方式进行了同步，也必须填写这两个参数。
但具体的值取决于新旧布局，我们后面再回来填写它：

```cpp
barrier.srcAccessMask = {}; // TODO
barrier.dstAccessMask = {}; // TODO
```

> `src`表示之前必须完成的操作，`dst`之后才能开始的操作。

现在填写屏障提交函数，所有类型的管线屏障都使用相同的提交函数：

```cpp
commandBuffer.pipelineBarrier(
    {},     // TODO: srcStageMask
    {},     // TODO: dstStageMask
    {},     // dependencyFlags
    nullptr,    // memoryBarriers
    nullptr,    // bufferMemoryBarriers
    barrier     // imageMemoryBarriers
);
```

前两个参数分别指定了在屏障触发之前发生的阶段以及需要等待屏障的阶段。
这两个参数指定想要在屏障前后使用的资源，它们的可选值列在了 [规范表](https://www.khronos.org/registry/vulkan/specs/1.3-extensions/html/chap7.html#synchronization-access-types-supported) 上。

举个例子，如果你要让片段着色器读取uniform数据，且要求在屏障之后读取。
那么你应该将资源的`dstAccessMask`设置为 `vk::AccessFlagBits::eUniformRead`，将`dstStageMask`设置为`vk::PipelineStageFlagBits::eFragmentShader`。
当你的`AccessMask`和`StageMask`不匹配时，验证层会发出警告。

第三个参数可以是 `{}` 或 `vk::DependencyFlagBits::eByRegion` 。
后者会将屏障转换成 按区域条件(per-region condition)，允许GPU在资源的部分区域仍被写入时，提前读取那些已经完成写入的部分。

最后三个参数是三种管线屏障的代理数组，即内存屏障、缓冲内存屏障和图像内存屏障，我们只有图像内存屏障。

## **复制缓冲区内容到图像**

现在创建一个`copyBufferToImage`辅助函数，用于复制数据：

```cpp
void copyBufferToImage(
    vk::raii::Buffer& buffer, 
    vk::raii::Image& image,
    uint32_t width,
    uint32_t height
) {
    vk::raii::CommandBuffer commandBuffer = beginSingleTimeCommands();

    endSingleTimeCommands( std::move(commandBuffer) );
}
```

和缓冲区的复制类似，你需要指定哪部分需要被拷贝。此外还需指定图像自身的属性：

```cpp
vk::BufferImageCopy region;
region.bufferOffset = 0;
region.bufferRowLength = 0;
region.bufferImageHeight = 0;

region.imageSubresource.aspectMask = vk::ImageAspectFlagBits::eColor;
region.imageSubresource.mipLevel = 0;
region.imageSubresource.baseArrayLayer = 0;
region.imageSubresource.layerCount = 1;

region.imageOffset = vk::Offset3D{0, 0, 0};
region.imageExtent = vk::Extent3D{width, height, 1};
```

大多数参数都已经在图像创建或缓冲复制时见过了。

`bufferRowLength` 和 `bufferImageHeight` 字段指定像素在内存中的布局方式。
比如图像每行之间可能有一些填充字节。
将两者都指定为 `0` 表示像素紧密堆积，就像我们在本例中的情况一样。

然后使用 `copyBufferToImage` 函数录制拷贝命令:

```cpp
commandBuffer.copyBufferToImage(
    buffer,
    image,
    vk::ImageLayout::eTransferDstOptimal,
    region
);
```

第四个参数接受代理数组，我们现在只需要复制一块区域，但你可以通过多个 `vk::BufferImageCopy` 信息在一次操作中实现多种不同的复制。

## **准备纹理图像**

现在我们回到`createTextureImage`函数，先调用刚才编写的布局转换函数，再调用数据复制函数：

```cpp
transitionImageLayout(
    m_textureImage,
    vk::Format::eR8G8B8A8Srgb,
    vk::ImageLayout::eUndefined,
    vk::ImageLayout::eTransferDstOptimal
);

copyBufferToImage(
    stagingBuffer,
    m_textureImage,
    static_cast<uint32_t>(texWidth),
    static_cast<uint32_t>(texHeight)
);
```

我们的图像在创建时使用`vk::ImageLayout::eUndefined`，所以第一个函数的`oldLayout`参数也是它。
记住，我们不关心它在拷贝操作之前的内容。

为了允许着色器采样，我们还再将布局转换成着色器可读类型：

```cpp
transitionImageLayout(
    m_textureImage,
    vk::Format::eR8G8B8A8Srgb,
    vk::ImageLayout::eTransferDstOptimal,
    vk::ImageLayout::eShaderReadOnlyOptimal
);
```

## **修改屏障掩码**

如果你现在运行程序，会发现验证层提示StageMask的设置不合法。
我们现在需要设置 `transitionImageLayout` 中没有填写的`StageMask`和`AccessMask`。

我们需要处理两个变换：

1. undefined -> tranfer destination
2. transfer destination -> shader read only

第一个变换时，我们不需要任何同步。第二个变换时，我们需要保证着色器在数据写入之后才能进行读取。


那么我们可以这样设置：

```cpp
vk::PipelineStageFlagBits sourceStage;
vk::PipelineStageFlagBits destinationStage;

if( oldLayout == vk::ImageLayout::eUndefined &&
    newLayout == vk::ImageLayout::eTransferDstOptimal
) {
    barrier.srcAccessMask = {};
    barrier.dstAccessMask = vk::AccessFlagBits::eTransferWrite;
    sourceStage = vk::PipelineStageFlagBits::eTopOfPipe;
    destinationStage = vk::PipelineStageFlagBits::eTransfer;
} else if(
    oldLayout == vk::ImageLayout::eTransferDstOptimal &&
    newLayout == vk::ImageLayout::eShaderReadOnlyOptimal
) {
    barrier.srcAccessMask = vk::AccessFlagBits::eTransferWrite;
    barrier.dstAccessMask = vk::AccessFlagBits::eShaderRead;

    sourceStage = vk::PipelineStageFlagBits::eTransfer;
    destinationStage = vk::PipelineStageFlagBits::eFragmentShader;
} else {
    throw std::invalid_argument("unsupported layout transition!");
}

commandBuffer.pipelineBarrier(
    sourceStage,        // srcStageMask
    destinationStage,   // dstStageMask
    {},         // dependencyFlags
    nullptr,    // memoryBarriers
    nullptr,    // bufferMemoryBarriers
    barrier     // imageMemoryBarriers
);
```

如果你仔细看了上面提到的 [规范表](https://www.khronos.org/registry/vulkan/specs/1.3-extensions/html/chap7.html#synchronization-access-types-supported) ，那么你会发现数据转写(transfer write)只发生在管线的transfer阶段。
我们转写图像数据不需要任何前置管线操作，所以指定了空的`srcAccessMask`，同时设置管线阶段为最早的`vk::PipelineStageFlagBits::eTopOfPipe`。 
注意`eTopOfPipe`不是一个真实的阶段，它更像是发生传输的伪阶段，具体请参考 [文档](https://www.khronos.org/registry/vulkan/specs/1.3-extensions/html/chap7.html#VkPipelineStageFlagBits)。

图像将在同一管线阶段写入，随后由片段着色器读取，这就是为什么我们在片段着色器管线阶段指定着色器读取访问。

## **延伸**

注意的一件事是，命令缓冲区提交会导致开始时隐式的 `vk::AccessFlagBits::eHostWrite` 同步，用于同步 CPU 和 GPU 之间的内存访问，保证了GPU图像管线转写发生在CPU命令提交之后。

事实上还存在一个特殊的图像类型 `vk::ImageLayout::eGeneral` 支持所有操作，但不保证性能最优。
有的时候一张图像既需要写又需要读，则不得不用它。

到目前为止，我们所有提交命令的函数通过等待队列变为空闲来同步执行。
对于实际应用，建议将这些操作组合在单个命令缓冲区中并异步执行它们以获得更高的吞吐量，特别是 `createTextureImage` 函数中的转换和复制。
尝试创建一个 `setupCommandBuffer` 进行实验，通过辅助函数将命令记录到其中，并添加一个 `flushSetupCommands` 来执行到目前为止已记录的命令。
最好在纹理映射工作后执行此操作，以检查纹理资源是否仍然正确设置。

---

现在你可以运行程序，虽然显式的内容没变，但不应存在报错。
图像现在包含纹理，但我们仍然需要一种从图形管线访问它的方法。我们将在下一章中对此进行研究。

---

**[C++代码](../codes/0400_texture/main.cpp)**

**[C++代码差异](../codes/0400_texture/main.diff)**

**[根项目CMake代码](../codes/0300_descriptor1/CMakeLists.txt)**

**[shader-CMake代码](../codes/0300_descriptor1/shaders/CMakeLists.txt)**

**[shader-vert代码](../codes/0300_descriptor1/shaders/shader.vert)**

**[shader-frag代码](../codes/0300_descriptor1/shaders/shader.frag)**
