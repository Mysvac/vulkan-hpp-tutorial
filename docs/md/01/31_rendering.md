---
title: 同步与渲染呈现
comments: true
---
# **渲染和呈现**

## **基础结构**

本章将把所有内容整合在一起。我们将编写 `drawFrame` 函数，它在主循环中使用以将三角形绘制到屏幕上。
让我们创建函数并在 `mainLoop` 中调用它：

```cpp
void mainLoop() {
    while (!glfwWindowShouldClose( m_window )) {
        glfwPollEvents();
        drawFrame();
    }
}

// ...

void drawFrame() {

}
```

## **渲染概要**

在 Vulkan 中渲染帧包括一组常见的步骤

- 等待上一帧完成
- 从交换链获取图像
- 记录命令缓冲区，将场景绘制到该图像上
- 提交已记录的绘制命令
- 呈现交换链图像

虽然我们将在后面的章节中扩展绘制函数，但目前而言，这是我们渲染循环的核心。

## **同步**

Vulkan 的核心设计理念是 GPU 上的命令执行需要显式同步。
这意味着 Vulkan API 中许多调用 GPU 工作的函数都是异步的，它们将在操作完成之前返回。

在本章中，我们需要显式排序许多事件，因为它们发生在 GPU 上，例如：

- 从交换链获取图像
- 提交绘制命令，从而将内容绘制到图像上
- 将该图像呈现到屏幕以显示，并将其返回到交换链

每个事件都使用单个函数启动，但都是异步执行。
函数调用将在操作实际完成之前返回，且执行顺序未定义。
这很不幸，因为每个操作都依赖于前一个操作的完成，我们需要探索可以使用哪些同步原语来实现所需的排序。

### 1. 信号量

**信号量\(Semaphore\)**用于在队列操作之间添加顺序（比如图形队列和呈现队列）。
队列操作指的是我们提交到队列的工作，无论是在命令缓冲区中还是从函数内部（我们稍后会看到）。
信号量既用于同一队列内部的工作排序，也用于不同队列之间的工作。

Vulkan 中有两种信号量，二进制信号量和时间线信号量，本章节的信号量专指二进制信号量。

信号量要么是未发信号状态、要么是已发信号状态，默认是未发信号状态。
我们使用它的方式是将同一信号量作为一个队列操作中的“发信”信号量，并作为另一个队列操作的“等待”信号量。

假设我们希望按顺序执行操作 A 和 B，可行的方案是：
操作 A 将在完成执行时“发出”信号 S ，而操作 B 需要“等待”信号量 S 。
当操作 A 完成时，信号 S 将被发出，而操作 B 受到信号后才能开始执行。
在操作 B 开始执行后，信号量 S 将自动重置回未发信状态，从而允许再次使用。

刚刚描述的伪代码

```c
VkCommandBuffer A, B = ... // record command buffers
VkSemaphore S = ... // create a semaphore

// enqueue A, signal S when done - starts executing immediately
vkQueueSubmit(work: A, signal: S, wait: None)

// enqueue B, wait on S to start
vkQueueSubmit(work: B, signal: None, wait: S)
```

请注意，此代码片段中对 `vkQueueSubmit()` 的两次调用都会立即返回，等待仅发生在 GPU 上， CPU 继续运行而不阻塞。
要使 CPU 等待，我们需要不同的同步原语。


### 2. 围栏

**围栏\(Fence\)**具有类似的目的，因为它也用于同步命令的执行，但它用于排序 **主机\(CPU\)** 上的命令。简而言之，如果主机需要知道 GPU 何时完成了某项工作，我们就会使用围栏。

与信号量类似，围栏也处于已触发或未触发状态，可以指定初始状态。
每当我们提交要执行的工作时，我们都可以将围栏附加到该工作。当工作完成时，围栏将被触发。然后我们可以使主机等待围栏触发，从而保证工作在主机继续之前已完成。

一个例子是屏幕截图，
假设我们需要将图像从 GPU 传输到主机，然后将主存数据保存到磁盘。
我们有执行传输的命令缓冲 A 和围栏 F 。
我们提交带有围栏 F 的命令缓冲 A，然后立即告诉主机等待 F 触发。
这会导致主机阻塞，直到命令缓冲区 A 完成执行后才能将数据从主存传向磁盘。

所描述内容的伪代码

```c
VkCommandBuffer A = ... // record command buffer with the transfer
VkFence F = ... // create the fence

// enqueue A, start work immediately, signal F when done
vkQueueSubmit(work: A, fence: F)

vkWaitForFence(F) // blocks execution until A has finished executing

save_screenshot_to_disk() // can't run until the transfer has finished
```

与信号量示例不同，此示例确实会阻塞主机执行。

> 一般来说，除非必要，最好不要阻塞主机。我们希望主机和 GPU 有效工作，等待围栏显然不是。因此，我们更喜欢使用信号量或其他尚未涵盖的同步原语来同步任务。

信号量“等待”完成会自动变为“未发信”，但围栏等待完成不会改变状态，需要手动重置才能使其回到“未触发”状态。
这是因为围栏用于控制主机的执行，因此主机可以决定何时重置围栏。

总而言之，信号量常用于指定 GPU 上操作的执行顺序，而围栏用于保持 CPU 和 GPU 彼此同步。

### 3. 如何选择

![synchronization_overview](../../images/0131/synchronization_overview.png)

我们有两个同步原语可以使用，且恰好有两个地方需要同步：交换链操作和等待上一帧完成。

我们希望使用信号量进行交换链操作，因为它们发生在 GPU 上，这样还可以避免让主机等待。
对于等待上一帧完成，我们希望使用围栏，原因恰恰相反，因为我们需要主机等待。

这样我们就不会一次绘制多个帧。
因为每帧都要重新记录命令缓冲区，所以在当前帧完成执行之前，我们无法将下一帧的任务记录到命令缓冲中，因为我们不想在 GPU 正在使用命令缓冲区时覆写它的内容。

## **创建同步对象**

我们将需要一个信号量来指示已从交换链获取图像并准备好进行渲染，另一个信号量来指示渲染已完成并且可以进行呈现，以及一个围栏来确保一次只渲染一个帧。

创建三个类成员来存储这些信号量对象和围栏对象

```cpp
vk::raii::Semaphore m_imageAvailableSemaphore{ nullptr };
vk::raii::Semaphore m_renderFinishedSemaphore{ nullptr };
vk::raii::Fence m_inFlightFence{ nullptr };
```

要创建信号量，我们将为本教程的这一部分添加最后一个 `create` 函数：`createSyncObjects`

```cpp
void initVulkan() {
    createInstance();
    setupDebugMessenger();
    createSurface();
    selectPhysicalDevice();
    createLogicalDevice();
    createSwapChain();
    createImageViews();
    createRenderPass();
    createFramebuffers();
    createGraphicsPipeline();
    createCommandPool();
    createCommandBuffer();
    createSyncObjects();
}

// ...

void createSyncObjects() {

}
```

创建信号量需要 `vk::SemaphoreCreateInfo`，创建围栏需要 `vk::FenceCreateInfo`，但目前都没什么需要填写的字段。

```cpp
constexpr vk::SemaphoreCreateInfo semaphoreInfo;
constexpr vk::FenceCreateInfo fenceInfo;
```

然后直接创建对象即可：

```cpp
m_imageAvailableSemaphore = m_device.createSemaphore( semaphoreInfo );
m_renderFinishedSemaphore = m_device.createSemaphore( semaphoreInfo );
m_inFlightFence = m_device.createFence( fenceInfo );
```

## **等待上一帧完成**

### 1. 等待围栏

在帧开始时，我们希望等待上一帧完成，以便命令缓冲区和信号量可供使用。为此，我们调用 `waitForFences`:

```cpp
m_device.waitForFences( *m_inFlightFence, true, UINT64_MAX );
```

第一个参数类型是数组代理，意味着我们可以一次性等待多个围栏。

第二个参数 `true` 表示我们想要等待\(第一个参数中的\)所有围栏，但在单个围栏的情况下，这无关紧要。
此函数还具有超时参数，我们将其设置为 64 位无符号整数的最大值 `UINT64_MAX` ，这实际上禁用了超时。

### 2. 处理错误码

如果你观察函数签名，会发现它返回一个 `vk::Result` 变量，且使用 `[[nodiscard]]` 标记，建议我们处理它。
如果你仔细阅读过“接口介绍”那一章，可能会注意到 C 风格接口就使用 `VkResult` 返回值处理错误。

事实上在 Vulkan-hpp 中， `waitForFences` 操作失败依然会抛出异常，且不同类型的异常表示了不同类型的错误。
此处 `vk::Result` 的最大作用是区别“成功”时的状况。

`vk::Result::eSuccess` 是完全成功的状态，但还有部分状态表示成功但不是最佳情况（建议程序调整，但不抛出异常），我们会在后续章节介绍。
此处只需保证它完全成功，可以这样写：

```cpp
if (const auto res = m_device.waitForFences( *m_inFlightFence, true, std::numeric_limits<uint64_t>::max() );
    res != vk::Result::eSuccess
) throw std::runtime_error{ "waitForFences in drawFrame was failed" };
```

### 3. 重置围栏状态

等待后，我们需要使用 `resetFences` 手动将围栏重置为“未触发”状态：

```cpp
m_device.resetFences( *m_inFlightFence );
```

> 注意到 `s` ，这里同样是数组代理，可以一次性重置多个围栏。

### 4. 设置围栏初始状态

我们的设计中有一个小问题：

我们在第一帧也调用 `drawFrame()`，它立即等待 `m_inFlightFence` 触发。
此围栏仅在完成渲染后触发，可这是第一帧的开始，我们还没有渲染任务，所以 `waitForFences` 会无限期地阻塞。

API 内置了一个巧妙的解决方法：可以创建“已触发”状态的围栏，以便首次调用 `waitForFences` 时立即返回。
为此，我们将 `eSignaled` 标志添加到 `vk::FenceCreateInfo` 的 `falgs` 字段：

```cpp
constexpr vk::FenceCreateInfo fenceInfo( 
    vk::FenceCreateFlagBits::eSignaled  // flags
);
```

## **从交换链获取图像**

已经通过围栏等待上一帧绘制完成，下一件事是从交换链获取图像。

需要使用交换链的 `acquireNextImage` 成员函数，它返回 `std::pair<vk::Result, uint32_t>` 类型的值，我们可以使用结构化绑定接收：

```cpp
auto [nxtRes, imageIndex] = m_swapChain.acquireNextImage(std::numeric_limits<uint64_t>::max(), m_imageAvailableSemaphore);
```

第一个参数指定图像变为可用的超时时间（以纳秒为单位）。使用 64 位无符号整数的最大值意味着我们有效地禁用了超时。

接下来的两个参数指定了图像变为可用状态后 **发出信号/触发** 的同步对象，可以指定信号量和围栏。
我们使用 `imageAvailableSemaphore` 记录图像是否可用，忽略了第三个参数围栏。

> 我们暂时不处理此错误码，后两节会回到这里进行处理。

## **命令缓冲**

### 1. 录制绘制命令

有了指定要使用的交换链图像的 `imageIndex` ，我们现在可以记录命令缓冲区了。

我们先清空命令缓冲区的内容，然后使用上一节的函数录制命令。注意我们的成员变量是一个数组，但只有一个元素，我们使用第一个元素即可。

```cpp
m_commandBuffers[0].reset();
recordCommandBuffer(m_commandBuffers[0], imageIndex);
```

> 注意录制只是“录制”，没有真正用到图像，无需等待 `imageAvailableSemaphore` 发信。

### 2. 提交信息

队列提交和同步通过 `vk::SubmitInfo` 结构体进行配置。

```cpp
vk::SubmitInfo submitInfo;

submitInfo.setWaitSemaphores( *m_imageAvailableSemaphore );
std::array<vk::PipelineStageFlags,1> waitStages = { vk::PipelineStageFlagBits::eColorAttachmentOutput };
submitInfo.setWaitDstStageMask( waitStages );
```

这里设置了两个参数，“等待哪些信号量”和“在哪些管线阶段等待”，二者根据数组索引一一对应。

> “一一对应”是指索引为 `x` 的阶段只等待索引为 `x` 的信号量，不会交错等待。

我们的图像用作颜色附件，会在管线的 `eColorAttachmentOutput` 阶段被使用，所以要在此阶段等待图像可用。


接下来要设置需提交的命令缓冲。我们只需提交我们拥有的单个命令缓冲区。

```cpp
submitInfo.setCommandBuffers( *m_commandBuffers[0] );
```

我们设置了需要“等待”的信号量和阶段，还可以设置任务完成后“发信”的信号量。
我们需要保证 GPU 在“渲染任务”完成后再执行“呈现任务”，所以需要设置 `m_renderFinishedSemaphore` ：

```cpp
submitInfo.setSignalSemaphores( *m_renderFinishedSemaphore );
```

### 3. 提交命令

现在可以使用 `submit` 将命令缓冲区提交到图形队列。

```cpp
m_graphicsQueue.submit(submitInfo, m_inFlightFence);
```

我们还将 `m_inFlightFence` 作为第二个参数传入，此围栏将在“渲染命令”完成后触发。

我们在 `drawFrame` 的开始部分等待此围栏并将其重置，这保证了命令缓冲区不会在 GPU 使用时（执行渲染命令时）被 CPU（我们的录制函数） 修改。

## **子通道依赖**

### 1. 内存屏障

我们在“渲染通道”章节粗略介绍过子通道依赖。**子通道依赖的本质是内存屏障**，在较新的 Vulkan API 中已经使用内存屏障代替子通道的写法，我们会在进阶章节介绍相关内容。

Vulkan 的内存屏障实际是“管线阶段屏障+内存可见性屏障”。

**管线阶段屏障**用于同步管线阶段，比如多子通道并发时要求第二个子通道的顶点着色器晚于第一个子通道的片段着色器，而部分阶段可以并发执行。

只有管线阶段屏障是不够的，现代 GPU 也存在多级缓存。即使有阶段屏障保证了操作的先写后读，也可能出现前者写入缓存、后者从内存中读取，但缓存数据并未即时写回内存的情况。

**内存可见性屏障**做的就是这件事，它指定了 GPU 对某些资源进行某些内存操作时，会将缓存修改强制写回高层级（如L2缓存或显存）或令缓存失效（下次访问必须从高层级加载），保证数据可见性。

### 2. 布局转换

回顾“渲染通道”章节，我们设置了图像的初始布局、子通道布局和最终布局，这些布局转换的发生时机是由子通道依赖定义的。

现在我们没有子通道依赖，因此使用默认的依赖，我们的子通道的开始（管线的 `Top` 阶段）将等待渲染通道开始前的“外部子通道”的结束（管线的 `Bottom` 阶段）。
因此，图像布局转换可能发生在管线开始时，但此时我们可能尚未获取到图像！

有两种方法可以解决此问题：

1. 将 `imageAvailableSemaphore` 的 `waitStages` 更改为 `eTopOfPipe`，保证图像可用之前不会进入图形管线 `Top` 阶段。
2. 修改子通道依赖，推迟布局转换到 `eColorAttachmentOutput` 阶段。

我们将使用第二种，它通常带来更好的性能（因为更高的并行度），且有助于读者了解子通道依赖关系及其工作原理。

### 3. 同步控制

子通道依赖关系在 `vk::SubpassDependency` 结构中指定。转到 `createRenderPass` 函数并添加：

```cpp
vk::SubpassDependency dependency;
dependency.srcSubpass = vk::SubpassExternal;
dependency.dstSubpass = 0;
```

这里填写的是子通道索引，`0`是我们唯一的子通道。 
`vk::SubpassExternal`是特殊值，在`srcSubpass`时表示渲染通道之前的外部子通道，在`dstSubpass`表示之后的。

`dstSubpass` 必须始终高于 `srcSubpass`，以防止依赖关系图中出现循环（除非其中一个是`vk::SubpassExternal`）。

还需指定源阶段和目标阶段，这对应上面提到的“管线阶段屏障”，内存访问则对应上面的“内存可见性屏障”。

我们在“颜色附件输出”阶段使用图像并涉及附件的写入：

```cpp
dependency.dstStageMask = vk::PipelineStageFlagBits::eColorAttachmentOutput;
dependency.dstAccessMask = vk::AccessFlagBits::eColorAttachmentWrite;
```

恰好我们的“图像可用”信号量也绑定在此阶段，它保证了上一次呈现任务已完成且可使用此图像。

同样的，交换链读取图像也在这一阶段，我们需要等待上一次的此阶段处理完成：

```cpp
dependency.srcStageMask = vk::PipelineStageFlagBits::eColorAttachmentOutput;
dependency.srcAccessMask = {};
```

Vulkan 保证目标阶段在源阶段之后开始，源阶段的操作对目标阶段可见。

最后将依赖绑定到渲染管线的创建信息中：

```cpp
renderPassInfo.setDependencies( dependency );
```


## **呈现**

绘制帧的最后一步是将结果提交回交换链，以便最终在屏幕上显示。呈现通过 `drawFrame` 函数末尾的 `vk::PresentInfoKHR` 结构进行配置。

```cpp
vk::PresentInfoKHR presentInfo;
presentInfo.setWaitSemaphores( *m_renderFinishedSemaphore );
```

前两个参数指定在可以呈现之前要等待哪些信号量，就像 `vk::SubmitInfo` 一样。
我们希望等待图像绘制完毕，即渲染命令执行完成，因此等待 `m_renderFinishedSemaphore`。

```cpp
presentInfo.setSwapchains( *m_swapChain );
presentInfo.pImageIndices = &imageIndex;
```

接下来的两个参数指定要将图像呈现到的交换链以及交换链中的图像索引，这通常是单个值。

最后提交将图像呈现到交换链的请求。

```cpp
m_presentQueue.presentKHR( presentInfo );
```

注意到此函数也返回了`vk::Result`，我们暂时要求完美成功，从而避免编译器警告：

```cpp
if (const auto res = m_presentQueue.presentKHR( presentInfo );
    res != vk::Result::eSuccess
) throw std::runtime_error{ "presentKHR in drawFrame was failed" };
```

## **运行与修改**

如果您到目前为止一切都正确，那么当您运行程序时，您现在应该会看到类似以下内容

![triangle](../../images/0131/triangle.png)

> 这种彩色三角形可能看起来与您在图形教程中看到的三角形略有不同。
> 这是因为本教程让着色器在线性颜色空间中插值，然后在之后转换为 sRGB 颜色空间。

### 信号量重用警告

如果你启用了验证层，终端可能会输出一堆警告，表示信号量非预期的重复使用：

```
... pSignalSemaphores[0] ... is being signaled by VkQueue ..., but it may still be in use by VkSwapchainKHR.
```

我们会在下一章详细解释这一警告并将其解决，此时一个临时的修复措施是在 `drawFrame` 函数开头加上 `m_device.waitIdle();` ：

```cpp
void drawFrame() {
    m_device.waitIdle();
    // 其他内容不变
}
```

这会强制上一轮操作完全结束，才能开始本轮内容。

### 关闭时崩溃

此外，如果你尝试关闭程序，可能会看到它在关闭时崩溃，验证层会告诉我们原因：

![error](../../images/0131/semaphore_in_use.png)

请记住，`drawFrame` 中的所有操作都是异步的。
这意味着当我们退出 `mainLoop` 中的循环时，绘制和呈现操作可能仍在进行中，这种情况下清理资源不是一个好主意。

要解决此问题，我们应该在退出 `mainLoop` 并销毁窗口之前，等待逻辑设备完成操作

```cpp
void mainLoop() {
    while (!glfwWindowShouldClose( m_window )) {
        glfwPollEvents();
        drawFrame();
    }

    m_device.waitIdle();
}
```


您还可以等待特定命令队列中的操作完成，方法是使用 `queue.waitIdle`。这些函数是执行同步的一种非常基础的方法。
现在关闭窗口时，程序退出时应该不会出现问题。

### **最小化崩溃**

虽然关闭窗口时能正常退出，但你将窗口最小化时可能直接导致程序崩溃。

这是因为最小化改变了窗口的大小，导致我们的交换链尺寸不匹配，`presentKHR`提交呈现命令后将抛出异常。

我们将在后面的“重建交换链”章节解决这个问题。

> 注意此问题不是必然发生的，部分设备改变窗口大小时不会自动触发`ErrorOutOfDateKHR`。  
> 但无论如何，在窗口尺寸改变后我们都应该主动重建交换链。

## **总结**

经过接近 700 行代码，我们终于看到屏幕上出现了一些东西！

引导 Vulkan 程序绝对是一项艰巨的工作，但重要的信息是 Vulkan 通过其显式性为您提供了大量的控制权。
我建议您现在花一些时间重新阅读代码，并构建一个心理模型用于记忆这些 Vulkan 组件的作用与相互关系。 

---

**[C++代码](../../codes/01/31_rendering/main.cpp)**

**[C++代码差异](../../codes/01/31_rendering/main.diff)**

**[根项目CMake代码](../../codes/01/21_shader/CMakeLists.txt)**

**[shader-CMake代码](../../codes/01/21_shader/shaders/CMakeLists.txt)**

**[shader-vert代码](../../codes/01/21_shader/shaders/graphics.vert.glsl)**

**[shader-frag代码](../../codes/01/21_shader/shaders/graphics.frag.glsl)**

---
