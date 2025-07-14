---
title: 新版同步
comments: true
---

# **新版同步**

## **前言**

“同步”是 Vulkan 中一个重要且复杂的概念，我们在之前的章节中使用过围栏、二进制信号量和屏障进行同步工作。

Vulkan 1.3 将 `VK_KHR_synchronization2` 纳入了核心规范，它带来了更简单的同步语法。
我们将在本章介绍它，并补充一些重要的相关知识。

> 关于新版同步： [Vulkan-Guide \[VK_KHR_synchronization2\]](https://docs.vulkan.org/guide/latest/extensions/VK_KHR_synchronization2.html#_new_submission_flow)

## **基础**

在介绍 `Synchronization2` 之前，让我们先回顾一下 Vulkan 的同步机制。

除了我们已经使用过的围栏、二进制信号量和屏障外， Vulkan 中还有一些重要的同步机制：

- 事件\(event\)：用于在命令缓冲区中设置和等待事件。
- 时间线信号量\(timeline semaphore\)：用于队列之间以及主机与设备之间的同步操作。

我们不会在本章通过旧语法使用它们，但需要你了解它们的基础概念。

### 1. 事件

事件\(event\)是一种细粒度的同步机制，用于**控制“命令缓冲”中指令的执行顺序**。

![synchronization_overview](../../images/0413/synchronization_overview.png)

它的创建方式非常简单：

```cpp
vk::EventCreateInfo create_info;
vk::raii::Event m_event = m_device.createEvent(create_info);
```

事件只有两种状态：触发和未触发，三种操作：设置（触发）、重置和等待。
默认情况下，它允许 CPU 和 GPU 对其"设置/重置“，但只允许 GPU “等待”。

主机\(CPU\)端可以这样操作它：

```cpp
m_event.set(); // 设置事件为已触发状态
m_event.reset(); // 重置事件为未触发状态
vk::Result res = m_event.getStatus(); // 获取事件状态（非等待）
// 没有 CPU 端的等待函数，你只能轮询 res 进行忙等待
```

由于只能让 GPU 等待事件，因此我们通常不需要让 CPU 操作它。
可以使用 `eDeviceOnly` 标志位禁止主机操作和查询，以实现最佳性能：

```cpp
vk::EventCreateInfo create_info;
create_info.flags = vk::EventCreateFlagBits::eDeviceOnly;
```

设备\(GPU\)端的操作和等待都需要通过命令缓冲区完成。

```cpp
// 重置事件状态为未触发状态
command_buffer.resetEvent(
    m_event,    
    vk::PipelineStageFlagBits::eTopOfPipe  // 立即重置，无需等待任何阶段
);
// 在命令缓冲区中设置事件，与上面的重置同理
command_buffer.setEvent(
    m_event,    // 事件对象
    vk::PipelineStageFlagBits::eTransfer   // 事件将在首次到达 Transfer 阶段时触发
); // 可以设置多个管线阶段，事件将在 GPU 执行到任一指定阶段时立即触发
```

注意，**“等待”完成不会自动重置事件状态**，因此你需要在适当的时候手动重置它。

内存屏障控制的是管线阶段资源访问的同步，而事件是控制命令缓冲区的执行。
在事件等待完成之前，命令缓冲区的后续指令将被阻塞：

```cpp
command_buffer.waitEvents(
    *m_event,   // events，可以设置多个
    vk::PipelineStageFlagBits::eTransfer, // srcStageMask
    vk::PipelineStageFlagBits::eVertexShader,   // dstStageMask
    {},     // memoryBarriers
    {},     // bufferMemoryBarriers
    {}      // imageMemoryBarriers
);
// 后续的命令缓冲指令，比如 draw 命令
```

`waitEvents` 的 `srcStageMask` 必须包含 `setEvent` 函数中的 `srcStageMask` 。
比如事件由主机触发，则 `waitEvents` 的 `srcStageMask` 必须包含 `vk::PipelineStageFlagBits::eHost` 。

这里隐含了两种同步：

1. 在事件等待完成前，命令缓冲的后续指令将被阻塞。
2. 正在执行的管线的 `dstStage` 阶段需要等待 `srcStage` 阶段且事件被触发后才能开始执行。

注意，事件仅控制执行顺序，不自动处理内存一致性。若需要数据同步，必须添加内存屏障，因为具体的访问掩码在屏障中定义。

### 2. 时间线信号量

二进制信号量只有“已发信号”和“未发信号”两种状态，功能有限，只能用于控制一个先后关系。

时间线信号量\(timeline semaphore\)的状态则用一个 64 位无符号整数表示，每次 `wait` 和 `signal` 都需要指定具体的值。
因此你可以让一些任务依次等待 `1、2、3` ，然后在任务完成时依次发出信号 `2、3、4` ，实现更复杂的同步逻辑。

时间线信号量由 Vulkan 1.2 加入核心，因此它的创建方式需要用到上一节介绍的 `pNext` 链。

我们需要让普通的 `vk::SemaphoreCreateInfo` 链接一个 `vk::SemaphoreTypeCreateInfo` 结构体，在后者设置信号量类型与初始状态值：

```cpp
// 创建 pNext 结构体链
vk::StructureChain<vk::SemaphoreCreateInfo,vk::SemaphoreTypeCreateInfo> semaphore_chain;

// 修改信号量类型信息
semaphore_chain.get<vk::SemaphoreTypeCreateInfo>()
    .setSemaphoreType( vk::SemaphoreType::eTimeline ) // 设置类型为时间线信号量
    .setInitialValue( 0 ); // 设置初始值为 0
// 你还可以将 semaphoreType 设置为 `vk::SemaphoreType::eBinary` 来创建二进制信号量。

// 创建时间线信号量，类型依然是 vk::raii::Semaphore
vk::raii::Semaphore semaphore = m_device.createSemaphore( semaphore_chain.get() );
```

如果你尝试创建，会发现它需要启用 GPU 特性，这又用到了上一节介绍的内容：

```cpp
vk::StructureChain<
    vk::DeviceCreateInfo,   // 以 CreateInfo 作为链的起点
    vk::PhysicalDeviceFeatures2, // 注意是 Feature2 才有 pNext 字段
    vk::PhysicalDeviceVulkan12Features
> create_info_chain;

create_info_chain.get() // 空模板默认返回链起点元素
    .setQueueCreateInfos( queue_create_infos )  // set 返回自身引用，可以链式调用
    .setPEnabledExtensionNames( vk::KHRSwapchainExtensionName );
create_info_chain.get<vk::PhysicalDeviceFeatures2>().features
    .setSamplerAnisotropy( true );  // 设置需要启用的 1.0 版本特性
create_info_chain.get<vk::PhysicalDeviceVulkan12Features>()
    .setTimelineSemaphore( true );  // 启用时间线信号量

m_device = m_physical_device.createDevice( create_info_chain.get() );
```

时间线信号量非常强大，除了能像二进制信号量一样用于 GPU 的队列间同步外，还允许 CPU 直接设置信号量状态或阻塞线程：

```cpp
// 在 CPU 端直接发出信号，修改信号量的值
vk::SemaphoreSignalInfo signal_info;
signal_info.semaphore = semaphore;
signal_info.value = 3;

m_device.signalSemaphore( signal_info );

// 在 CPU 端阻塞线程，直到信号量的值达到指定值
std::uint64_t wait_value = 3; // 设置信号量的目标值
vk::SemaphoreWaitInfo wait_info;
wait_info.setSemaphores( semaphore );
wait_info.setValues( wait_value ); // 设置等待的信号量值

// CPU 程序等待信号量的值变为 3 ，这里忽略了 vk::Result
std::ignore = m_device.waitSemaphores( wait_info, std::numeric_limits<std::uint64_t>::max());
```

这是 CPU 端的等待而不是 GPU 等待，此 C++ 线程将被挂起直到信号量变为 3 ，但不会阻塞 GPU 的执行。

注意！！**每次 `signal` 的值必须严格大于信号量现在的值**，即单调递增，否则行为未定义。


GPU 端的使用和二进制信号量类似，需要提交到队列。
`SubmitInfo` 的写法和二进制信号量几乎一样，但是需要通过 `TimelineSemaphoreSubmitInfo` 来指定时间线信号量的值。

```cpp
std::array<vk::PipelineStageFlags,1> waitStages = { vk::PipelineStageFlagBits::eColorAttachmentOutput };

vk::StructureChain<vk::SubmitInfo, vk::TimelineSemaphoreSubmitInfo> submit_chain;
submit_chain.get()
    .setWaitSemaphores( wait_semaphore )    // 设定任务等待的信号量
    .setWaitDstStageMask( waitStages )      // 设置等待信号量的管线阶段
    .setSignalSemaphores( signal_semaphore )    // 设置任务完成后发出信号的信号量
    .setCommandBuffers( m_command_buffers );
std::uint64_t wait_value = 1;   // 设置等待信号量的值
std::uint64_t signal_value = 2; // 设置信号量的值
submit_chain.get<vk::TimelineSemaphoreSubmitInfo>()
    .setWaitSemaphoreValues( wait_value )       //  设置等待信号量的值
    .setSignalSemaphoreValues( signal_value );  // 设置发出信号的信号量的值
    
m_device->graphics_queue().submit( submit_chain.get() ); // 提交到队列
```

上面的任务在提交后，将等待 `wait_semaphore` 的值变为 1 ，然后执行命令缓冲区，完成后将 `signal_semaphore` 的值置为 2。

> 如果你仔细思考，会发现时间线信号量能以一己之力代替二进制信号量、围栏两者！！

## **新版同步语法**

`VK_KHR_synchronization2` 由 Vulkan 1.3 纳入核心规范，它主要做了这些改进：

- 定义了新的管线阶段枚举 `vk::PipelineStageFlagBits2` 。
- 提供了新的屏障类型 `vk::XxxMemoryBarrier2` 。
- 提供了与上述二者相关的一系列新 API ，比如 `SubmitInfo2` 、 `PipelineBarrier2` 等。

> 没错，事件、栅栏和信号量并没有调整，但由于 `SubmitInfo2` 等 API 的出现，它们的使用方式也发生了变化。

![VK_KHR_synchronization2_stage_access](../../images/0413/VK_KHR_synchronization2_stage_access.png)

需要注意一点，你不应该混用新版同步和旧版同步，尤其是操作同一对象时。

### 1. 基础代码

请下载并阅读下面的基础代码，这是“C++模块化”章节的第二部分代码：

**[点击下载](../../codes/04/00_cxxmodule/module_code.zip)**

我们会将此代码中用到的相关同步 API 替换为新版同步 API ，并在此过程中介绍它们的用法。

首先转到 `Device.cppm` ，修改逻辑设备的创建代码，我们需要启用相关 GPU 特性：

```cpp
constexpr std::array<const char*, 1> device_extensions { vk::KHRSwapchainExtensionName };
vk::StructureChain<
    vk::DeviceCreateInfo,
    vk::PhysicalDeviceFeatures2,
    vk::PhysicalDeviceVulkan12Features,
    vk::PhysicalDeviceVulkan13Features
> create_info_chain;
create_info_chain.get()
    .setQueueCreateInfos( queue_create_infos )
    .setPEnabledExtensionNames( device_extensions );
create_info_chain.get<vk::PhysicalDeviceFeatures2>().features
    .setSamplerAnisotropy( true );  // 启用各向异性采样
create_info_chain.get<vk::PhysicalDeviceVulkan12Features>()
    .setTimelineSemaphore( true );  // 启用时间线信号量
create_info_chain.get<vk::PhysicalDeviceVulkan13Features>()
    .setSynchronization2( true );   // 启用同步2

m_device = m_physical_device.createDevice( create_info_chain.get() );
```

### 2. 管线阶段

在 `VK_KHR_synchronization2` 体系中，我们需要使用新的管线阶段枚举和访问掩码。
你可以查看 `vk::PipelineStageFlagBits2` 和 `vk::AccessFlagBits2` 的定义来了解它们的含义。

> 或者查看 [PipelineStage/Access](https://docs.vulkan.org/spec/latest/chapters/synchronization.html#synchronization-pipeline-stages) 和 [Vulkan-Guide \[top and bottom deprecation\]](https://docs.vulkan.org/guide/latest/extensions/VK_KHR_synchronization2.html#_top_of_pipe_and_bottom_of_pipe_deprecation)

大部分枚举值都保持不变，然后对部分阶段提供了更细粒度的划分。
这里只介绍最重要的一点，管线的 `Top` 和 `Bottom` 阶段已经被弃用，现在建议通过 `None` 和 `AllCommands` 阶段控制。

曾经我们简单的使用 `Top` 表示“管线的开始”，使用 `Bottom` 表示“管线的结束”。
现在我们有更语义化的表达， `None` 表示没有命令， `AllCommands` 表示所有命令：

```cpp
// 等待管线的开始 曾经： dp.srcStageMask = vk::PipelineStageFlagBits::eTopOfPipe;
dp.srcStageMask = vk::PipelineStageFlagBits2KHR::eNone; // 现在
// 等待管线的结束 曾经：dp.srcStageMask = vk::PipelineStageFlagBits::eBottomOfPipe;
dp.srcStageMask = vk::PipelineStageFlagBits2KHR::eAllCommands; // 现在，表示所有命令的结束
// 等待完成后才能开始管线 曾经：dp.dstStageMask = vk::PipelineStageFlagBits::eTopOfPipe;
dp.dstStageMask = vk::PipelineStageFlagBits2KHR::eAllCommands; // 现在，表示开始管线的所有任务
// 等待完成后才能结束管线 曾经：dp.dstStageMask = vk::PipelineStageFlagBits::eBottomOfPipe;
dp.dstStageMask = vk::PipelineStageFlagBits2KHR::eNone; // 现在
```

此外，旧版本中 `AccessMask` 字段常使用 `{}` 来表示“无访问”，即只做阶段同步而不涉及资源（如等待所有任务完成）。
现在则推荐使用 `vk::AccessFlagBits2KHR::eNone` 代替它。

### 3. 渲染通道

现在正式开始改写我们的程序，首先是修改渲染通道的子通道依赖。

在新版同步中，子通道依赖中不再依靠 `vk::SubpassDependency` 来描述依赖关系，它仅用于填写前后子通道的索引，具体的同步通过 `pNext` 链交给内存屏障控制。

`SubpassDependency` 结构体没有 `pNext` 字段，
但 Vulkan 1.2 将 `VK_KHR_create_renderpass2` 加入了核心规范，它为渲染通道的所有创建信息提供了 `2` 结尾的版本，此版本附带 `pNext` 字段。

首先，修改 `RenderPass.cppm` 中的 `create_render_pass` 函数，将所有相关内容都加上 `2` 后缀：

```cpp
vk::AttachmentDescription2 ...
vk::AttachmentReference2 ...
vk::SubpassDescription2 ...
vk::SubpassDependency2 ...
vk::RenderPassCreateInfo2 ...
m_render_pass = m_device->device().createRenderPass2( create_info );
```

然后你可以直接运行程序，这些类型只是增加了 `pNext` 字段，因此我们只需修改类型和函数名，无需调整其他内容。

成功运行后，可以修改子通道依赖的代码：

```cpp
vk::StructureChain<vk::SubpassDependency2, vk::MemoryBarrier2> dependency;

dependency.get()    // 设置子通道依赖
    .setDependencyFlags(vk::DependencyFlagBits::eByRegion) // 局部依赖化，优化性能，可选
    .setSrcSubpass( vk::SubpassExternal )   // 只需要设置两个子通道序号
    .setDstSubpass( 0 );
dependency.get<vk::MemoryBarrier2>()    // 具体同步交给内存屏障
    .setSrcStageMask( vk::PipelineStageFlagBits2::eColorAttachmentOutput | vk::PipelineStageFlagBits2::eEarlyFragmentTests )
    .setSrcAccessMask( vk::AccessFlagBits2::eNone )
    .setDstStageMask( vk::PipelineStageFlagBits2::eColorAttachmentOutput | vk::PipelineStageFlagBits2::eEarlyFragmentTests )
    .setDstAccessMask( vk::AccessFlagBits2::eColorAttachmentWrite | vk::AccessFlagBits2::eDepthStencilAttachmentWrite );

...

// 通过 get() 获取结构体链的首元素，然后创建渲染通道
create_info.setDependencies( dependency.get() );
```

现在，同步控制交给内存屏障完成，子通道依赖信息只需要设置子通道索引和特殊标志位即可。

### 4. 图像布局

`VK_KHR_synchronization2` 还引入了两种新的图像布局：`eAttachmentOptimal` 和 `eReadOnlyOptimal` 。

在以前，我们有色彩的 `eColorAttachmentOptimal` 和深度的 `eDepthStencilAttachmentOptimal` ，现在可以统一为 `eAttachmentOptimal` 。
后者同理，有“着色器/深度/模板只读优化”，现在可以统一使用 `eReadOnlyOptimal` 。

> 你可以认为，无论是渲染通道还是管线屏障，转换时会自动根据图形格式将此布局映射为合适的色彩/深度等布局。

因此你可以修改 `RenderPass.cppm` 中的图像布局，将色彩和深度的`eXxxxAttachmentOptimal` 都改为 `eAttachmentOptimal` 。

### 5. 管线屏障

刚刚我们通过子通道依赖使用了管线屏障，它并不需要手动提交命令缓冲。

现在需要调整 `Tools.cppm` 中的 `transition_image_layout` 代码，需要我们配置更多信息。

首先创建一个简单的 `vk::ImageMemoryBarrier2` 结构体，填写熟悉的内容：

```cpp
vk::ImageMemoryBarrier2 barrier2;
barrier2.image = image;
barrier2.oldLayout = oldLayout;
barrier2.newLayout = newLayout;
barrier2.srcQueueFamilyIndex = vk::QueueFamilyIgnored;
barrier2.dstQueueFamilyIndex = vk::QueueFamilyIgnored;
barrier2.subresourceRange = {
    vk::ImageAspectFlagBits::eColor,
    0, 1, 0, 1
};
```

`Barrier2` 与 `Barrier` 的区别在于前者自身就包含了 `srcStageMask` 和 `dstStageMask` 字段，所以我们可以这样设置：

```cpp
if( oldLayout == vk::ImageLayout::eUndefined &&
    newLayout == vk::ImageLayout::eTransferDstOptimal
) {
    barrier2.srcStageMask = vk::PipelineStageFlagBits2::eNone;
    barrier2.srcAccessMask = vk::AccessFlagBits2::eNone;
    barrier2.dstStageMask = vk::PipelineStageFlagBits2::eTransfer;
    barrier2.dstAccessMask = vk::AccessFlagBits2::eTransferWrite;
} else if(
    oldLayout == vk::ImageLayout::eTransferDstOptimal &&
    newLayout == vk::ImageLayout::eReadOnlyOptimal  // 这里调整了布局
) {
    barrier2.srcStageMask = vk::PipelineStageFlagBits2::eTransfer;
    barrier2.srcAccessMask = vk::AccessFlagBits2::eTransferWrite;
    barrier2.dstStageMask = vk::PipelineStageFlagBits2::eFragmentShader;
    barrier2.dstAccessMask = vk::AccessFlagBits2::eShaderRead;
} else {
    throw std::invalid_argument("unsupported layout transition!");
}
```

注意这里我们使用了 `eReadOnlyOptimal` ，你需要前往 `TextureSampler.cppm` 中修改 `create_texture_image` 函数最下方的布局转换：

```cpp
vht::transition_image_layout(
    ...
    vk::ImageLayout::eReadOnlyOptimal
);
```

回到 `Tools.cppm` ，我们还需要创建一个 `vk::DependencyInfo` 结构体来绑定具体的管线对象，然后就可以提交命令了：

```cpp
vk::DependencyInfo dependency_info;
dependency_info.setImageMemoryBarriers( barrier2 );
// dependency_info.setBufferMemoryBarriers()
// dependency_info.setMemoryBarriers()

command_buffer.pipelineBarrier2( dependency_info );
```

`DependencyInfo` 结构体除了 `sType/pNext` 就是只有六个字段，分别是三种屏障的数组指针和数量。

你可以发现，管线阶段的配置都被移动到了屏障本身，而多屏障的绑定被移到了 `DependencyInfo` 中，使 API 更加统一。

记得删除旧的屏障代码，现在可以重新运行程序，与之前的效果相同。

### 6. 事件

回顾前面提到的事件的用法，新版同步提供了 `setEvent2` 、 `resetEvent2` 和 `waitEvents2` 等 API 来替代旧的事件操作。

其中，`setEvent2` 和 `waitEvents2` 的用法与旧版类似，但现在只有 `Event` 和 `vk::DependencyInfo` 两个参数。

现在可以通过 `setEvent2` 命令直接插入屏障了，你可以像这样使用：

```cpp
// ----------- 步骤1: 设置事件 + 触发条件 -----------

// 执行计算着色器 ......

vk::MemoryBarrier2 setBarrier;
setBarrier.srcStageMask = vk::PipelineStageFlagBits2::eTransfer;
setBarrier.srcAccessMask = vk::AccessFlagBits2::eTransferWrite; // 确保传输写入完成
setBarrier.dstStageMask = vk::PipelineStageFlagBits2::eComputeShader;
setBarrier.dstAccessMask = vk::AccessFlagBits2::eShaderRead; // 数据对计算着色器可见

vk::DependencyInfo setDepInfo;
setDepInfo.setMemoryBarriers( setBarrier );

command_buffer.setEvent2(event, setDepInfo); // 传输完成后触发事件

// ----------- 步骤2: 等待事件 + 后续条件 -----------
vk::MemoryBarrier2 waitBarrier;
waitBarrier.srcStageMask = vk::PipelineStageFlagBits2::eComputeShader;
waitBarrier.srcAccessMask = vk::AccessFlagBits2::eShaderWrite; // 确保传输写入完成
waitBarrier.dstStageMask = vk::PipelineStageFlagBits2::eFragmentShader;
waitBarrier.dstAccessMask = vk::AccessFlagBits2::eShaderRead; // 数据对片段着色器可见

vk::DependencyInfo waitDepInfo;
waitDepInfo.setMemoryBarriers( waitBarrier );

command_buffer.waitEvents2(*event, waitDepInfo);
// 绑定图形管线，绘制三角形 ......
```

### 7. 队列提交

`Synchronization2` 的另一项重要改进是提交命令的方式。

以前的 `vk::SubmitInfo` 需要直接绑定信号量和命令缓冲，现在的 `vk::SubmitInfo2` 则将这些信息改成了两种结构体：`vk::SemaphoreSubmitInfo` 和 `vk::CommandBufferSubmitInfo` 。

> `vk::SemaphoreSubmitInfo` 还包含时间线信息量所需的字段，因此无需使用 `pNext` 链。

下面我们将尝试使用时间线信号量改写绘制函数。

转到 `Drawer.cppm` 模块，首先调整成员变量，我们使用时间线信号量，不需要栅栏了。
但我们依然需要使用两个二进制信号量，分别同步呈现命令和图像获取，因为它们不支持时间线信号量：

```cpp
std::vector<vk::raii::Semaphore> m_present_semaphores;
std::vector<vk::raii::Semaphore> m_image_semaphores;
std::vector<vk::raii::Semaphore> m_time_semaphores;
```

然后调整信号量的创建函数：

```cpp
void create_sync_object() {
    vk::SemaphoreCreateInfo image_info;
    // 时间线信号量需要使用 pNext 链
    vk::StructureChain<vk::SemaphoreCreateInfo, vk::SemaphoreTypeCreateInfo> time_info;
    time_info.get<vk::SemaphoreTypeCreateInfo>()
        .setSemaphoreType( vk::SemaphoreType::eTimeline )
        .setInitialValue( 0 );

    m_present_semaphores.reserve( MAX_FRAMES_IN_FLIGHT );
    m_image_semaphores.reserve( MAX_FRAMES_IN_FLIGHT );
    m_time_semaphores.reserve( MAX_FRAMES_IN_FLIGHT );
    for(std::size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i){
        m_present_semaphores.emplace_back( m_device->device().createSemaphore(image_info) );
        m_image_semaphores.emplace_back( m_device->device().createSemaphore(image_info) );
        m_time_semaphores.emplace_back( m_device->device().createSemaphore(time_info.get()) );
    }
}
```

我们需要一个数组用于记录时间线信号量的值，只在 `Drawer` 函数中使用，可以作为函数局部静态变量：

```cpp
void draw() {
    static std::array<std::uint64_t, MAX_FRAMES_IN_FLIGHT> time_counter{};
    
}
```


参考以前的代码，首先要在 CPU 端等待上一轮的本次飞行帧完成：

```cpp
vk::SemaphoreWaitInfo first_wait;
first_wait.setSemaphores( *m_time_semaphores[m_current_frame] ); // 需要 * 转换至少一次类型
first_wait.setValues( time_counter[m_current_frame] );
std::ignore = m_device->device().waitSemaphores( first_wait, std::numeric_limits<std::uint64_t>::max() );
```

我们的计数器数组初始值为 0 ，因此第一次等待不会阻塞 CPU 线程。

> 这里忽略了等待函数的返回值，简化代码。

然后是获取图像，图像获取只能使用二进制信号量，这就是为什么我们需要两个信号量数组。

```cpp
// 获取交换链的下一个图像索引
std::uint32_t image_index;
try{
    auto [res, idx] = m_swapchain->swapchain().acquireNextImage(
        std::numeric_limits<std::uint64_t>::max(), m_image_semaphores[m_current_frame]
    );
    image_index = idx;
} catch (const vk::OutOfDateKHRError&){
    m_render_pass->recreate();
    return;
}
```

如果获取图像索引失败，我们需要重建渲染通道并返回。
如果获取成功，就可以记录命令缓冲区，并增加计数器的值了：

```cpp
// 更新 uniform 缓冲区
m_uniform_buffer->update_uniform_buffer(m_current_frame);
// 重置当前帧的命令缓冲区，并记录新的命令
m_command_buffers[m_current_frame].reset();
record_command_buffer(m_command_buffers[m_current_frame], image_index);

++time_counter[m_current_frame];  // 增加计数器的值，以便在渲染完成时增加时间线信号量
```

> 计数器必须在获取图像索引成功后增加，否则可能因为获取失败的 `return` 导致死锁，即函数开始部分的时间线信号量一直等待。

然后就可以编写提交命令了，这里需要使用 `vk::SubmitInfo2` 来替代 `vk::SubmitInfo` 。

```cpp
// 等待图像准备完成
vk::SemaphoreSubmitInfo wait_image;
wait_image.setSemaphore( m_image_semaphores[m_current_frame] );
wait_image.setStageMask( vk::PipelineStageFlagBits2::eColorAttachmentOutput );
// 二进制信号量，不需要设置值

// 渲染完成时发出信号
std::array<vk::SemaphoreSubmitInfo,2> signal_infos;
signal_infos[0].setSemaphore( m_time_semaphores[m_current_frame] ); // 更新时间线信号量
// 渲染完成后，将时间线信号量的值设置为计数器的值，保证严格递增
signal_infos[0].setValue( time_counter[m_current_frame] );  
// 触发呈现信号量，表示图像已经渲染完成，可用于呈现
signal_infos[1].setSemaphore( m_present_semaphores[m_current_frame] ); 
// 二进制信号量，不需要设置值

// 设置命令缓冲区提交信息
vk::CommandBufferSubmitInfo command_info;
command_info.setCommandBuffer( m_command_buffers[m_current_frame] );

vk::SubmitInfo2 submit_info;
submit_info.setWaitSemaphoreInfos( wait_image );
submit_info.setSignalSemaphoreInfos( signal_infos );
submit_info.setCommandBufferInfos( command_info );
```

注意等待信号量的 `stageMask` 设置的是 `PipelineStageFlag2` 类型。

`vk::SubmitInfo2` 结构体有三个重要内容，分别是等待信号量、发出信号量和命令缓冲区，都可以是数组。
这些类型和字段非常简单，你应该很好理解。

现在可以提交渲染命令了：

```cpp
// 提交命令缓冲区到图形队列
m_device->graphics_queue().submit2( submit_info );
```

当渲染命令完成后，时间线信号量的值将被设置为当前计数器的值，以便下一次 CPU 端的同步；而呈现信号量将被激活，以开始呈现任务。

这部分代码和之前的逻辑几乎一样，只需要调整信号量的标识符：

```cpp
// 设置呈现信息
vk::PresentInfoKHR present_info;
present_info.setWaitSemaphores( *m_present_semaphores[m_current_frame] );
present_info.setSwapchains( *m_swapchain->swapchain() );
present_info.pImageIndices = &image_index;
// 提交呈现命令
try{
    if(  m_device->present_queue().presentKHR(present_info) == vk::Result::eSuboptimalKHR ) {
        m_render_pass->recreate();
    }
} catch (const vk::OutOfDateKHRError&){
    m_render_pass->recreate();
}
// 检查窗口是否被调整大小
if( m_window->framebuffer_resized() ){
    m_render_pass->recreate();
}
// 更新飞行中的帧索引
m_current_frame = (m_current_frame + 1) % MAX_FRAMES_IN_FLIGHT;
```

现在你可以运行程序，应该能看到我们熟悉的房屋模型：

![right_room](../../images/0310/right_room.png)

## **最后**

时间线信号量、事件和新版同步 API 具有非常强大的功能，本章节只是简单介绍了它们的基本用法。

现在你可以自行查看官方示例和文档，了解更多细节。

---

**[基础代码](../../codes/04/00_cxxmodule/module_code.zip)**

**[Device.cppm](../../codes/04/13_sync2/Device.cppm)**

**[Device.diff\(差异文件\)](../../codes/04/13_sync2/Device.diff)**

**[Drawer.cppm](../../codes/04/13_sync2/Drawer.cppm)**

**[Drawer.diff\(差异文件\)](../../codes/04/13_sync2/Drawer.diff)**

**[RenderPass.cppm](../../codes/04/13_sync2/RenderPass.cppm)**

**[RenderPass.diff\(差异文件\)](../../codes/04/13_sync2/RenderPass.diff)**

**[Tools.cppm](../../codes/04/13_sync2/Tools.cppm)**

**[Tools.diff\(差异文件\)](../../codes/04/13_sync2/Tools.diff)**

**[TextureSampler.cppm](../../codes/04/13_sync2/TextureSampler.cppm)**

**[TextureSampler.diff\(差异文件\)](../../codes/04/13_sync2/TextureSampler.diff)**

---
