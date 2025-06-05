# **飞行中的帧**

飞行中的帧"Frames in flight"，指 **同时处于渲染流水线不同阶段的多个帧**。

作者没有找到合适的翻译方式。

## **飞行中的帧**

我们刚才完成的渲染循环其实有个问题：必须等到一帧渲染呈现完成，才能录制命令并开始新的一帧。显然这将导致GPU和CPU的资源浪费。

解决这个问题的方法是允许多个"飞行中的帧"，使得一帧的渲染不影响下一帧的录制。
为达成此目的，我们需要复制所有在渲染期间可能被访问或修改的资源。
所以我们需要多个命令缓冲、信号量和围栏。后面的章节中我们还会添加其他新资源。

首先在类中定义一个常量，用于指定应该同时处理多少个帧。

```cpp
static constexpr int MAX_FRAMES_IN_FLIGHT = 2;
```

我们将最大值设为2可以避免CPU太过领先于GPU。
现在，CPU和GPU将同时处理自己的任务。如果CPU完成的更快，那么在他提交更多任务之前必须先等待GPU完成。
使用三个或更多时，CPU可能领先GPU，导致帧延迟增加。
我们可以让程序指定具体使用几个飞行中的帧，这也是Vulkan显式控制的特点。

### 1. 修改成员变量

每个飞行中的帧都需要自己的命令缓冲、信号量和围栏，所以我们需要修改变量：

```cpp
std::vector<vk::raii::CommandBuffer> m_commandBuffers;
std::vector<vk::raii::Semaphore> m_imageAvailableSemaphores;
std::vector<vk::raii::Semaphore> m_renderFinishedSemaphores;
std::vector<vk::raii::Fence> m_inFlightFences;
```

注意我们还修改了变量名，在末尾加上了 `s` 。

### 2. 修改成员变量的创建

现在将函数 `createCommandBuffer` 改名成 `createCommandBuffers` ，并修改参数，从而获取指定数量的命令缓冲。

```cpp
void createCommandBuffers() {
    vk::CommandBufferAllocateInfo allocInfo;
    allocInfo.commandPool = m_commandPool;
    allocInfo.level = vk::CommandBufferLevel::ePrimary;
    allocInfo.commandBufferCount = MAX_FRAMES_IN_FLIGHT;

    m_commandBuffers = m_device.allocateCommandBuffers(allocInfo);
}
```

同样，还需要修改 `createSyncObjects` 函数：

```cpp
void createSyncObjects() {
    vk::SemaphoreCreateInfo semaphoreInfo;
    vk::FenceCreateInfo fenceInfo(
        vk::FenceCreateFlagBits::eSignaled  // flags
    );

    for(size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i){
        m_imageAvailableSemaphores.emplace_back( m_device, semaphoreInfo );
        m_renderFinishedSemaphores.emplace_back( m_device,  semaphoreInfo );
        m_inFlightFences.emplace_back( m_device , fenceInfo );
    }
}
```

注意到这里创建对象时，调用的是信号量和围栏的构造函数，而不是`m_device`的成员函数。  
你也可以使用成员函数然后移动构造：

```cpp
m_imageAvailableSemaphores.emplace_back( m_device.createSemaphore(semaphoreInfo) );
m_renderFinishedSemaphores.emplace_back( m_device.createSemaphore(semaphoreInfo) );
m_inFlightFences.emplace_back( m_device.createFence(fenceInfo) );
```

### 3. 修改帧的绘制

我们需要定义一个成员变量，来追踪当前程序处理的是哪个帧：

```cpp
uint32_t m_currentFrame = 0;
```

然后我们就可以修改 `drawFrame` 函数，只需要将每个信号量/围栏/命令缓冲都设置上`m_currentFrame`：

```cpp
void drawFrame() {
    if( auto res = m_device.waitForFences( *m_inFlightFences[m_currentFrame], true, UINT64_MAX );
        res != vk::Result::eSuccess ){
        throw std::runtime_error{ "waitForFences in drawFrame was failed" };
    }

    m_device.resetFences( *m_inFlightFences[m_currentFrame] );

    auto [nxtRes, imageIndex] = m_swapChain.acquireNextImage(UINT64_MAX, m_imageAvailableSemaphores[m_currentFrame]);

    m_commandBuffers[m_currentFrame].reset();
    recordCommandBuffer(m_commandBuffers[m_currentFrame], imageIndex);

    vk::SubmitInfo submitInfo;

    submitInfo.setWaitSemaphores( *m_imageAvailableSemaphores[m_currentFrame] );
    std::array<vk::PipelineStageFlags,1> waitStages = { vk::PipelineStageFlagBits::eColorAttachmentOutput };
    submitInfo.setWaitDstStageMask( waitStages );

    submitInfo.setCommandBuffers( *m_commandBuffers[m_currentFrame] );
    submitInfo.setSignalSemaphores( *m_renderFinishedSemaphores[m_currentFrame] );

    m_graphicsQueue.submit(submitInfo, m_inFlightFences[m_currentFrame]);

    vk::PresentInfoKHR presentInfo;
    presentInfo.setWaitSemaphores( *m_renderFinishedSemaphores[m_currentFrame] );
    presentInfo.setSwapchains( *m_swapChain );
    presentInfo.pImageIndices = &imageIndex;

    if( auto res = m_presentQueue.presentKHR( presentInfo );
        res != vk::Result::eSuccess) {
        throw std::runtime_error{ "presentKHR in drawFrame was failed" };
    }
}
```

记得在函数末尾更新我们的 `m_currentFrame` 。

```cpp
void drawFrame() {
    // ...

    m_currentFrame = (m_currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
}
```

> `[]`越界会强行结束程序。虽然我们的逻辑保证了这里不会越界，但你仍可提前检查或使用更安全的`.at()`。

## **最后**

我们现在已经实现了所有必要的同步工作以确保入队的工作帧不超过 `MAX_FRAMES_IN_FLIGHT` 个，且这些帧不会互相覆盖。
请注意，对于代码的其他部分（如最终清理），可以依赖更粗略的同步，例如 `m_device.waitIdle()`。您应该根据性能要求决定使用哪种方法。


要通过示例了解有关同步的更多信息，请查看 Khronos 提供的 [这份全面的概述](https://github.com/KhronosGroup/Vulkan-Docs/wiki/Synchronization-Examples#swapchain-image-acquire-and-present)。

现在构建和运行程序，保证程序正常。

---

在下一章中，我们将处理使 Vulkan 程序良好运行所需的另一件小事。

---

**[C++代码](../../codes/01/33_flightframe/main.cpp)**

**[C++代码差异](../../codes/01/33_flightframe/main.diff)**

**[根项目CMake代码](../../codes/01/21_shader/CMakeLists.txt)**

**[shader-CMake代码](../../codes/01/21_shader/shaders/CMakeLists.txt)**

**[shader-vert代码](../../codes/01/21_shader/shaders/shader.vert)**

**[shader-frag代码](../../codes/01/21_shader/shaders/shader.frag)**
