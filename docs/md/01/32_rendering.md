# **渲染和呈现**

## **基础结构**

本章将把所有内容整合在一起。我们将编写 `drawFrame` 函数，该函数将在主循环中调用，将三角形绘制到屏幕上。
让我们创建函数并在 `mainLoop` 调用它

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
- 提交已记录的命令缓冲
- 呈现交换链图像

虽然我们将在后面的章节中扩展绘制函数，但目前而言，这是我们渲染循环的核心。

## **同步**

Vulkan 的核心设计理念是 GPU 上命令执行的需要显式同步。
这意味着 Vulkan API 中许多调用 GPU 工作的函数都是异步的，它们将在操作完成之前返回。

在本章中，我们需要显式排序许多事件，因为它们发生在 GPU 上，例如

- 从交换链获取图像
- 执行命令从而将内容绘制到图像上
- 将该图像呈现到屏幕以进行显示，并将其返回到交换链

每个事件都使用单个函数启动，但都是异步执行。
函数调用将在操作实际完成之前返回，并且执行顺序未定义。
这很不幸，因为每个操作都依赖于前一个操作的完成，我们需要探索可以使用哪些同步原语来实现所需的排序。

### 1. 信号量

**信号量\(Semaphore\)**用于在队列操作之间添加顺序（比如图形队列和呈现队列）。队列操作指的是我们提交到队列的工作，无论是在命令缓冲区中还是从函数内部（我们稍后会看到）。信号量既用于同一队列内部的工作排序，也用于不同队列之间的工作。

Vulkan 中恰好有两种信号量，二进制信号量和时间线信号量。本教程中仅使用二进制信号量，因此我们不会讨论时间线信号量。后面提及信号量术语专门指二进制信号量。

信号量要么是未发信号状态、要么是已发信号状态，默认是未发信号状态。
我们使用它的方式是将同一信号量作为一个队列操作中的“发信”信号量，并作为另一个队列操作的“等待”信号量。

例如，假设我们有信号量 S 和队列操作 A 和 B，我们希望按顺序执行它们。
我们告诉 Vulkan 的是，操作 A 将在完成执行时“发出信号”信号量 S，而操作 B 将在信号量 S 发出信号之前“等待”信号量 S。
当操作 A 完成时，信号量 S 将被发出信号，而操作 B 将在 S 被发出信号之前不会启动。
在操作 B 开始执行后，信号量 S 将自动重置回未发信号状态，从而允许再次使用它。

刚刚描述的伪代码

```c
VkCommandBuffer A, B = ... // record command buffers
VkSemaphore S = ... // create a semaphore

// enqueue A, signal S when done - starts executing immediately
vkQueueSubmit(work: A, signal: S, wait: None)

// enqueue B, wait on S to start
vkQueueSubmit(work: B, signal: None, wait: S)
```

请注意，在此代码片段中，对 `vkQueueSubmit()` 的两次调用都会立即返回 - 等待仅发生在 GPU 上。CPU 继续运行而不阻塞。要使 CPU 等待，我们需要不同的同步原语。


### 2. 围栏

**围栏\(Fence\)**具有类似的目的，因为它也用于同步命令的执行，但它用于排序 CPU（主机） 上的命令。简而言之，如果主机需要知道 GPU 何时完成了某项工作，我们就会使用围栏。

与信号量类似，围栏也处于已发信号或未发信号状态，可以指定初始状态。
每当我们提交要执行的工作时，我们都可以将围栏附加到该工作。当工作完成时，围栏将被发信号。然后我们可以使主机等待围栏被发信号，从而保证工作在主机继续之前已完成。

一个具体的例子是截取屏幕截图。假设我们已经在 GPU 上完成了必要的工作。
现在需要将图像从 GPU 传输到主机，然后将内存保存到文件中。
我们有执行传输的命令缓冲区 A 和围栏 F。
我们提交带有围栏 F 的命令缓冲区 A，然后立即告诉主机等待 F 发出信号。
这会导致主机阻塞，直到命令缓冲区 A 完成执行。
因此，我们可以安全地让主机将文件保存到磁盘，因为内存传输已完成。

所描述内容的伪代码

```c
VkCommandBuffer A = ... // record command buffer with the transfer
VkFence F = ... // create the fence

// enqueue A, start work immediately, signal F when done
vkQueueSubmit(work: A, fence: F)

vkWaitForFence(F) // blocks execution until A has finished executing

save_screenshot_to_disk() // can't run until the transfer has finished
```

与信号量示例不同，此示例确实会阻塞主机执行。这意味着主机除了等待执行完成之外，什么也不会做。对于这种情况，我们必须确保传输已完成，然后才能将屏幕截图保存到磁盘。

一般来说，除非必要，最好不要阻塞主机。我们希望为主机和 GPU 提供有用的工作，而等待围栏发信号不是有用的工作。因此，我们更喜欢信号量或其他尚未涵盖的同步原语来同步我们的工作。

围栏必须手动重置，才能使其返回到未发信号状态。这是因为围栏用于控制主机的执行，因此主机可以决定何时重置围栏。将此与信号量进行对比，信号量用于在不涉及主机的情况下排序 GPU 上的工作。

总而言之，信号量用于指定 GPU 上操作的执行顺序，而围栏用于保持 CPU 和 GPU 彼此同步。

### 3. 如何选择

![synchronization_overview](../../images/0132/synchronization_overview.png)

我们有两个同步原语可以使用，并且方便地有两个地方可以应用同步：交换链操作和等待上一帧完成。

我们希望使用信号量进行交换链操作，因为它们发生在 GPU 上，因此如果可以避免，我们不希望让主机等待。
对于等待上一帧完成，我们希望使用围栏，原因恰恰相反，因为我们需要主机等待。

这样我们就不会一次绘制多个帧。
因为每帧都重新记录命令缓冲区，所以在当前帧完成执行之前，我们无法将下一帧的工作记录到命令缓冲区中，因为我们不想在 GPU 使用命令缓冲区时覆盖命令缓冲区的当前内容。

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
    pickPhysicalDevice();
    createLogicalDevice();
    createSwapChain();
    createImageViews();
    createRenderPass();
    createGraphicsPipeline();
    createFramebuffers();
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
vk::SemaphoreCreateInfo semaphoreInfo;
vk::FenceCreateInfo fenceInfo;
```

然后直接创建对象即可：

```cpp
m_imageAvailableSemaphore = m_device.createSemaphore( semaphoreInfo );
m_renderFinishedSemaphore = m_device.createSemaphore( semaphoreInfo );
m_inFlightFence = m_device.createFence( fenceInfo );
```

## **等待上一帧完成**

### 1. 等待围栏

在帧开始时，我们希望等到上一帧完成，以便命令缓冲区和信号量可供使用。为此，我们调用 `waitForFences`:

```cpp
m_device.waitForFences( *m_inFlightFence, true, UINT64_MAX );
```

> 第一个参数类型是数组代理，意味着我们可以一次性等待多个围栏。

第二个参数表示我们想要等待所有围栏，但在单个围栏的情况下，这无关紧要。
此函数还具有超时参数，我们将其设置为 64 位无符号整数的最大值 `UINT64_MAX`，这实际上禁用了超时。

### 2. 处理错误码

如果你观察函数签名，会发现它返回一个`vk::Result`变量，且使用`[[nodiscard]]`标记，建议我们处理它。

如果你仔细观察过“接口介绍”那一章，可能会注意到C风格接口就使用`VkResult`返回值处理错误。

事实上在Vulkan-hpp中，`waitForFences`操作失败依然会抛出异常，且不同类型的异常表示了不同类型的错误。
此处`vk::Result`的最大作用是区别成功时的状况（当然失败时也会对应的错误码）。

`vk::Result::eSuccess`是完全成功的状态，还有部分状态表示成功，但是不是最佳情况（建议程序调整），我们后面会介绍。
此处我们只需要保证它完全成功，可以这样写：

```cpp
if( auto res = m_device.waitForFences( *m_inFlightFence, true, UINT64_MAX );
    res != vk::Result::eSuccess ){
    throw std::runtime_error{ "waitForFences in drawFrame was failed" };
}
```

### 3. 重置围栏状态

等待后，我们需要使用 `resetFences` 手动将围栏重置为未发信号状态：

```cpp
m_device.resetFences( *m_inFlightFence );
```

> 注意到`s`，这里同样是数组代理。

### 4. 设置围栏初始状态

我们的设计中有一个小问题：

在第一帧中，我们调用 `drawFrame()`，它立即等待 `m_inFlightFence` 发出信号。
`m_inFlightFence` 仅在帧完成渲染后才发出信号，但由于这是第一帧，因此没有之前的帧可以发出围栏信号！
所以 `waitForFences` 会无限期地阻塞，等待永远不会发生的事情。

API 中内置了一个巧妙的解决方法。
在已发信号状态下创建围栏，以便首次调用 `waitForFences()` 时立即返回，因为围栏已发出信号。

为此，我们将 `eSignaled` 标志添加到 `vk::FenceCreateInfo` 的`falgs`字段：

```cpp
vk::FenceCreateInfo fenceInfo(
    vk::FenceCreateFlagBits::eSignaled  // flags
);
```

## **从交换链获取图像**

我们在 `drawFrame` 函数中需要做的下一件事是从交换链获取图像。

需要使用交换链的 `acquireNextImage` 成员函数，它返回 `std::pair<vk::Result, uint32_t>` 类型的值，我们可以使用结构化绑定接收：

```cpp
auto [nxtRes, imageIndex] = m_swapChain.acquireNextImage(UINT64_MAX, m_imageAvailableSemaphore);
```

第一个参数指定图像变为可用的超时时间（以纳秒为单位）。使用 64 位无符号整数的最大值意味着我们有效地禁用了超时。

接下来的两个参数指定在呈现引擎完成使用图像后要**发出信号**的同步对象。可以指定信号量和围栏，
我们在这里将使用我们的 `imageAvailableSemaphore` ，忽略了第三个参数围栏。

> 我们暂时不处理此错误码，后两节会回到这里进行处理。

## **记录命令缓冲**

有了指定要使用的交换链图像的 `imageIndex`，我们现在可以记录命令缓冲区了。

我们先清空命令缓冲区的内容，然后使用上一节的函数录制命令。注意我们的成员变量是一个数组，但只有一个元素，我们使用第一个元素即可。

```cpp
m_commandBuffers[0].reset();
recordCommandBuffer(m_commandBuffers[0], imageIndex);
```

## **提交命令缓冲**

队列提交和同步通过 `vk::SubmitInfo` 结构中的参数进行配置。

```cpp
vk::SubmitInfo submitInfo;

submitInfo.setWaitSemaphores( *m_imageAvailableSemaphore );
std::array<vk::PipelineStageFlags,1> waitStages = { vk::PipelineStageFlagBits::eColorAttachmentOutput };
submitInfo.setWaitDstStageMask( waitStages );
```

> 因为我们会频繁调用此函数，这里使用`std::array`从而使用栈内存而非堆，
> 可以略微提高效率。你甚至可以使用普通的数组。

这两个参数指定在执行开始之前要等待哪些信号量以及在管线的哪些阶段等待。
我们希望在图像可用之前等待向图像写入颜色，因此我们指定写入颜色附件的图形管线的阶段。
这意味着从理论上讲，可以已经在图像尚不可用时开始执行我们的顶点着色器等。
`waitStages` 数组中的每个条目都对应于 `WaitSemaphores` 中具有相同索引的信号量。

接下来的两个参数指定要实际提交执行的命令缓冲区。我们只需提交我们拥有的单个命令缓冲区。

```cpp
submitInfo.setCommandBuffers( *m_commandBuffers[0] );
```

`SignalSemaphores` 相关参数指定命令缓冲区（或多个缓冲区）完成执行时要发出信号的信号量。
在我们的例子中，我们为此目的使用了 `renderFinishedSemaphore`。

```cpp
submitInfo.setSignalSemaphores( *m_renderFinishedSemaphore );
```

我们现在可以使用 `submit` 将命令缓冲区提交到图形队列。

```cpp
m_graphicsQueue.submit(submitInfo, m_inFlightFence);
```

该函数将 `vk::SubmitInfo` 结构的数组作为参数，以便在工作负载大得多时提高效率。
最后一个参数引用一个可选的围栏，该围栏将在命令缓冲区完成执行时发出信号。
这使我们知道何时可以安全地重用命令缓冲区，因此我们希望为其提供 `m_inFlightFence`。

现在在下一帧中，CPU 将等待此命令缓冲区完成执行，然后再将新命令记录到其中。

## **子通道依赖**

渲染通道的子通道会自动处理图像布局转换。
这些转换由子通道依赖控制，这些依赖指定子通道之间的内存关系和执行顺序。

我们现在只有一个子通道，但是紧接在此子通道之前和之后的运算也算作隐式“子通道”。
有两个内置依赖关系负责它们的同步，前者假定转换发生在管线开始时，但此时我们尚未获取到图像！！

有两种方法可以解决此问题：

1. 将 `imageAvailableSemaphore` 的 `waitStages` 更改为 `vk::PipelineStageFlagBits::eTopOfPipe`，以确保渲染通道在图像可用之前不会开始。

2. 使渲染通道等待 `vk::PipelineStageFlagBits::eColorAttachmentOutput` 阶段。

我们在此处选择第二种选择，因为这是一个很好的借口用于了解子通道依赖关系及其工作原理。

子通道依赖关系在 `vk::SubpassDependency` 结构中指定。转到 `createRenderPass` 函数并添加：

```cpp
vk::SubpassDependency dependency;
dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
dependency.dstSubpass = 0;
```

这里填写的是子通道索引，`0`是我们唯一的子通道。 
`VK_SUBPASS_EXTERNAL`是特殊值，在`srcSubpass`时表示渲染通道之前的隐式子通道，在`dstSubpass`表示之后的。

`dstSubpass` 必须始终高于 `srcSubpass`，以防止依赖关系图中出现循环（除非其中一个是`VK_SUBPASS_EXTERNAL`）。

下面需指定源阶段src（需要等待完成的）和目标阶段dst（等待完可执行的）。

我们需要等待交换链完成从图像读取完成才能访问它，这可以通过等待颜色附件输出阶段来完成。

```cpp
dependency.srcStageMask = vk::PipelineStageFlagBits::eColorAttachmentOutput;
dependency.srcAccessMask = {};
```

我们在颜色附件阶段等待颜色附件阶段完成，并涉及颜色附件的写入。

```cpp
dependency.dstStageMask = vk::PipelineStageFlagBits::eColorAttachmentOutput;
dependency.dstAccessMask = vk::AccessFlagBits::eColorAttachmentWrite;
```

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
由于我们希望等待命令缓冲区完成执行（因此我们的三角形被绘制），因此我们采用将发出信号的信号量并等待它们，因此我们使用 `m_renderFinishedSemaphore`。

```cpp
presentInfo.setSwapchains( *m_swapChain );
presentInfo.pImageIndices = &imageIndex;
```

接下来的两个参数指定要将图像呈现到的交换链以及每个交换链的图像索引。这几乎总是单个的。

最后提交将图像呈现到交换链的请求。

```cpp
m_presentQueue.presentKHR( presentInfo );
```

注意到此函数也返回了`vk::Result`，我们暂时要求完美成功，从而避免编译器警告：

```cpp
if( auto res = m_presentQueue.presentKHR( presentInfo );
    res != vk::Result::eSuccess) {
    throw std::runtime_error{ "presentKHR in drawFrame was failed" };
}
```

## **运行与修改**

如果您到目前为止一切都正确，那么当您运行程序时，您现在应该会看到类似以下内容

![triangle](../../images/0132/triangle.png)

> 这种彩色三角形可能看起来与您在图形教程中看到的三角形略有不同。
> 这是因为本教程让着色器在线性颜色空间中插值，然后在之后转换为 sRGB 颜色空间。

嘿！不幸的是，您可能会看到程序在您关闭它后立即崩溃。当启用验证层时，打印到终端的消息告诉了我们原因

![error](../../images/0132/semaphore_in_use.png)

请记住，`drawFrame` 中的所有操作都是异步的。
这意味着当我们退出 `mainLoop` 中的循环时，绘制和呈现操作可能仍在进行中。
在这种情况下清理资源不是一个好主意。

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


您还可以等待特定命令队列中的操作完成，方法是使用 `queue.waitIdle`。这些函数可以用作执行同步的一种非常基本的方法。
您会看到，现在关闭窗口时，程序退出时不会出现问题。

## **最小化崩溃**

虽然关闭窗口时能正常退出，但你将窗口最小化时可能直接导致程序崩溃。

这是因为最小化改变了窗口的大小，导致我们的交换链尺寸不匹配，`presentKHR`提交呈现命令后将抛出异常。

我们将在后面的“重建交换链”章节解决这个问题。

> 注意此问题不是必然发生的，部分设备改变窗口大小时不会自动触发`ErrorOutOfDateKHR`。  
> 但无论如何，在窗口尺寸改变后我们都应该主动重建交换链。

## **结论**

经过 700 多行代码，我们终于看到屏幕上弹出了一些东西！

引导 Vulkan 程序绝对是一项艰巨的工作，但重要的信息是 Vulkan 通过其显式性为您提供了大量的控制权。

我建议您现在花一些时间重新阅读代码，并构建程序中所有 Vulkan 对象的用途以及它们之间如何相互关联的心理模型。
从现在开始，我们将在此知识的基础上扩展程序的功能。

---

下一章将扩展渲染循环以处理飞行中的多个帧。

---

**[C++代码](../../codes/01/32_rendering/main.cpp)**

**[C++代码差异](../../codes/01/32_rendering/main.diff)**

**[根项目CMake代码](../../codes/01/21_shader/CMakeLists.txt)**

**[shader-CMake代码](../../codes/01/21_shader/shaders/CMakeLists.txt)**

**[shader-vert代码](../../codes/01/21_shader/shaders/shader.vert)**

**[shader-frag代码](../../codes/01/21_shader/shaders/shader.frag)**
