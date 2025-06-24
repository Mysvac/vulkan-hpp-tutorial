# **重建交换链**

## **前言**

现在我们已经成功绘制了第一个三角形，但是还有些特殊情况没有处理。
当窗口表面发送变化时，交换链可能不再适配窗口表面。
一种可能的原因是窗口大小被调整，我们需要捕获此事件并重建交换链。

## **重建交换链**

创建一个 `recreateSwapchain` 函数，需要重建的相关内容（依赖交换链和窗口尺寸的）都放入它的内部。

```cpp
void recreateSwapChain() {
    m_device.waitIdle();

    createSwapChain();
    createImageViews();
    createFramebuffers();
}
```

相关内容还会在管线的视口与裁剪、帧绘制和命令录制时使用。
但它们都是一次性的，绘制新的帧时就会自动更新，我们无需修改这些代码。 

使用 `m_device.waitIdle()` 防止重建工作销毁正在使用的资源。

我们需要先清理之前的资源再重建。虽然RAII保证了直接覆盖也能清理资源，但是分离不同的工作可以让代码更加清晰：

```cpp
void recreateSwapChain() {
    m_device.waitIdle();

    m_swapChainFramebuffers.clear();
    m_swapChainImageViews.clear();
    m_swapChainImages.clear(); // optional
    m_swapChain = nullptr;

    createSwapChain();
    createImageViews();
    createFramebuffers();
}
```

之前提到了 `Image` 资源由交换链管理，自身只有句柄。
我们创建时也时直接赋值整个数组而非在末尾追加。
所以 `m_swapChainImages` 可以不手动清理， `createSwapChain` 会处理它。
 `m_swapChainExtent` 和 `m_swapChainImageFormat` 也是这样。

这种方法的缺点是，我们需要在创建新的交换链之前停止所有渲染。
其实我们可以在旧交换链图像上的绘制命令仍在进行时创建新的交换链。
您需要将之前的交换链传递给“交换链创建信息”中的 `oldSwapChain` 字段，并在完成使用旧交换链后立即销毁它。

## **次优或过期的交换链**

现在我们需要知道什么时候应该重建交换链。
`acquireNextImage` 和 `presentKHR` 会返回 `vk::Result` 枚举，我们可以从中得知当前交换链是否合适。

枚举的可能类型有很多，这里只介绍我们需要的三种：

| 返回值 | 含义 |
|-------|------|
| `vk::Result::eSuccess` | 完全成功 |  
| `vk::Result::eSuboptimalKHR` | 还能使用，但已经不是最佳 |  
| `vk::Result::eErrorOutOfDateKHR` | 已经过期且无法再使用 |

过期时需要立刻重建，成功时自不必说。
特殊的是次优时我们认为可以继续使用，因为我们已经获得到了需要的图像。

> 虽然此处返回了`vk::Result`，它不为`eSuccess`和`eSuboptimalKHR`时，依然会抛出异常（除非你全局禁用异常）。

我们依然需要使用异常处理的方式进行操作，修改 `drawFrame` 函数：

```cpp
uint32_t imageIndex;
try{
    // std::pair<vk::Result, uint32_t>
    auto [res, idx] = m_swapChain.acquireNextImage(UINT64_MAX, m_imageAvailableSemaphores[m_currentFrame]);
    imageIndex = idx;
} catch (const vk::OutOfDateKHRError&){
        recreateSwapChain();
        return;
} // Do not catch other exceptions
```

> 如果你禁用了全局异常，那么你应该通过判断`res`决定是否重建交换链。


`presentKHR` 函数也返回一个 `vk::Result` ，我们进行同样的判断。
由于我们已经绘制完毕了，次优时直接重建交换链即可。

```cpp
try{
    auto res = m_presentQueue.presentKHR(presentInfo);
    if( res == vk::Result::eSuboptimalKHR ) {
        recreateSwapChain();
    }
} catch (const vk::OutOfDateKHRError&){
    recreateSwapChain();
} // Do not catch other exceptions

m_currentFrame = (m_currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
```

## **修复死锁**

如果你现在运行代码，可能会遇到死锁问题。
调试或观察后可以发现，这是因为我们先重置了围栏 `fench` ，但是在获取下一个图像时可能重建交换链然后退出了函数。
此时没有执行图形管线的提交，所以 `fench` 不会被补充，下次进入时会无限等待。

一种简单的修复方式是将 `resetFences` 后移，移动到 `acquireNextImage` 的判断之后：

```cpp
if( auto res = m_device.waitForFences( *m_inFlightFences[m_currentFrame], true, UINT64_MAX );
    res != vk::Result::eSuccess ){
    throw std::runtime_error{ "waitForFences in drawFrame was failed" };
}

uint32_t imageIndex;
try{
    // std::pair<vk::Result, uint32_t>
    auto [res, idx] = m_swapChain.acquireNextImage(UINT64_MAX, m_imageAvailableSemaphores[m_currentFrame]);
    imageIndex = idx;
} catch (const vk::OutOfDateKHRError&){
        recreateSwapChain();
        return;
} // Do not catch other exceptions

// Only reset the fence if we are submitting work
m_device.resetFences( *m_inFlightFences[m_currentFrame] );
```

## **显式处理尺寸变化**

尽管大多数的驱动和平台都可以在窗口大小变化后自动触发 `Result::eErrorOutOfDateKHR` ，但只是大多数而非全部。
我们需要一些额外操作，显式处理窗口尺寸的变化。

> 作者的台式机正常，但笔记本就没触发。

首先我们添加一个新的成员变量，用于记录尺寸是否发生了变化：

```cpp
bool m_framebufferResized = false;
```

然后我们修改 `recreateSwapChain` ，在末尾重置 `m_framebufferResized` 状态：

```cpp
void recreateSwapChain() {
    // ......
    createSwapChain();
    createImageViews();
    createFramebuffers();

    m_framebufferResized = false;
}
```

然后再`drawFrame`末尾添加一个判断，用于显式重建交换链：

```cpp
// ......
try{
    auto res = m_presentQueue.presentKHR(presentInfo);
    if( res == vk::Result::eSuboptimalKHR ) {
        recreateSwapChain();
    }
} catch (const vk::OutOfDateKHRError&){
    recreateSwapChain();
} // Do not catch other exceptions

if( m_framebufferResized ){
    recreateSwapChain();
}

m_currentFrame = (m_currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
```

我们还需要在窗口尺寸变化时将 `m_framebufferResized` 修改为 `true `，
可以使用GLFW框架提供的回调函数 `glfwSetFramebufferSizeCallback` 实现。
修改 `initWindow` ，注册回调函数：

```cpp
void initWindow() {
    glfwInit();
    
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

    m_window = glfwCreateWindow(WIDTH, HEIGHT, "Vulkan", nullptr, nullptr);

    glfwSetFramebufferSizeCallback(m_window, framebufferResizeCallback);
}

static void framebufferResizeCallback(GLFWwindow* window, int width, int height) {

}
```

我们还删除了`glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE)`，现在可以调整窗口大小。

我们的函数是静态的，但是我们需要修改成员变量，所以需要给 GLFW 传递当前类对象的指针：

```cpp
m_window = glfwCreateWindow(WIDTH, HEIGHT, "Vulkan", nullptr, nullptr);
glfwSetWindowUserPointer(m_window, this);
glfwSetFramebufferSizeCallback(m_window, framebufferResizeCallback);
```

现在可以使用 `glfwGetWindowUserPointer` 从回调中检索此值，以正确设置标志：

```cpp
static void framebufferResizeCallback(GLFWwindow* window, int width, int height) {
    auto app = reinterpret_cast<HelloTriangleApplication*>(glfwGetWindowUserPointer(window));
    app->m_framebufferResized = true;
}
```

现在尝试运行程序并调整窗口大小，看看帧缓冲是否确实随窗口正确调整大小。

## **处理最小化**

还有另一种情况下交换链可能会过期，那是一种特殊的窗口大小调整：窗口最小化。
这种情况很特殊，因为它会导致帧缓冲区大小为 `0`。
在本教程中，我们将通过暂停程序（直到窗口再次位于前台）来处理这种情况。现在扩展 `recreateSwapChain` 函数：

```cpp
void recreateSwapChain() {
    int width = 0, height = 0;
    glfwGetFramebufferSize(m_window, &width, &height);
    while (width == 0 || height == 0) {
        glfwGetFramebufferSize(m_window, &width, &height);
        glfwWaitEvents();
    }

    m_device.waitIdle();

    // ......
}
```

我们循环等待直到窗口大小不为 0 ，也就是最小化状态结束。

## **最后**

恭喜，您现在已经完成了您的第一个行为良好的 Vulkan 程序！在下一章中，我们将摆脱顶点着色器中的硬编码顶点，并使用顶点缓冲区作为管线输入。

---

**[C++代码](../../codes/01/40_recreateswapchain/main.cpp)**

**[C++代码差异](../../codes/01/40_recreateswapchain/main.diff)**

**[根项目CMake代码](../../codes/01/21_shader/CMakeLists.txt)**

**[shader-CMake代码](../../codes/01/21_shader/shaders/CMakeLists.txt)**

**[shader-vert代码](../../codes/01/21_shader/shaders/shader.vert)**

**[shader-frag代码](../../codes/01/21_shader/shaders/shader.frag)**
