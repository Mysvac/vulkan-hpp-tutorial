export module Tools;


import std;
import vulkan_hpp;

export namespace vht {

    // 读取 shader 代码
    [[nodiscard]]
    std::vector<char> read_shader(const std::string& path) {
        std::ifstream file(path, std::ios::ate | std::ios::binary);
        if (!file.is_open()) throw std::runtime_error("failed to open file!");
        const long long fileSize = file.tellg();
        std::vector<char> buffer(fileSize);
        file.seekg(0);
        file.read(buffer.data(), fileSize);
        file.close();
        return buffer;
    }

    // 创建 shader 模块
    [[nodiscard]]
    vk::raii::ShaderModule create_shader_module(const vk::raii::Device& device, const std::vector<char>& code) {
        vk::ShaderModuleCreateInfo create_info;
        create_info.codeSize = code.size();
        create_info.pCode = reinterpret_cast<const std::uint32_t*>(code.data());
        return device.createShaderModule(create_info);
    }

    // 创建单个命令缓冲区并开始记录命令
    [[nodiscard]]
    vk::raii::CommandBuffer begin_command(const vk::raii::CommandPool& command_pool, const vk::raii::Device& device) {
        vk::CommandBufferAllocateInfo alloc_info;
        alloc_info.level = vk::CommandBufferLevel::ePrimary;
        alloc_info.commandPool = command_pool;
        alloc_info.commandBufferCount = 1;

        auto commandBuffers = device.allocateCommandBuffers(alloc_info);
        vk::raii::CommandBuffer commandBuffer = std::move(commandBuffers.at(0));

        commandBuffer.begin( { vk::CommandBufferUsageFlagBits::eOneTimeSubmit } );
        return commandBuffer;
    }

    // 结束命令缓冲区并提交到队列
    void end_command(const vk::raii::CommandBuffer& command_buffer, const vk::raii::Queue& queue) {
        command_buffer.end();
        vk::SubmitInfo submit_info;
        submit_info.setCommandBuffers( *command_buffer );
        queue.submit(submit_info);
        queue.waitIdle();
    }

    // 根据物理设备和内存类型过滤器查找合适的内存类型
    [[nodiscard]]
    std::uint32_t findMemoryType(
        const vk::raii::PhysicalDevice& physical_device,
        const std::uint32_t type_filter,
        const vk::MemoryPropertyFlags properties
    ) {
        const auto memProperties = physical_device.getMemoryProperties();
        for(std::uint32_t i = 0; const auto& type : memProperties.memoryTypes){
            if( (type_filter & (1 << i)) &&
                (type.propertyFlags & properties ) == properties
            ) return i;
            ++i;
        }
        throw std::runtime_error("failed to find suitable memory type!");
    }

    // 创建图像
    void create_image(
        vk::raii::Image& image,
        vk::raii::DeviceMemory& image_memory,
        const vk::raii::Device& device,
        const vk::raii::PhysicalDevice& physical_device,
        const std::uint32_t width,
        const std::uint32_t height,
        const vk::Format format,
        const vk::ImageTiling tiling,
        const vk::ImageUsageFlags usage,
        const vk::MemoryPropertyFlags properties
    ) {
        vk::ImageCreateInfo create_info;
        create_info.imageType = vk::ImageType::e2D;
        create_info.extent.width = width;
        create_info.extent.height = height;
        create_info.extent.depth = 1;
        create_info.mipLevels = 1;
        create_info.arrayLayers = 1;
        create_info.format = format;
        create_info.tiling = tiling;
        create_info.initialLayout = vk::ImageLayout::eUndefined;
        create_info.usage = usage;
        create_info.samples = vk::SampleCountFlagBits::e1;
        create_info.sharingMode = vk::SharingMode::eExclusive;
        image = device.createImage(create_info);

        const vk::MemoryRequirements requirements = image.getMemoryRequirements();
        vk::MemoryAllocateInfo allocInfo;
        allocInfo.allocationSize = requirements.size;
        allocInfo.memoryTypeIndex = findMemoryType(physical_device, requirements.memoryTypeBits, properties);
        image_memory = device.allocateMemory(allocInfo);

        image.bindMemory(image_memory, 0);
    }

    // 创建缓冲区
    void create_buffer(
        vk::raii::Buffer& buffer,
        vk::raii::DeviceMemory& buffer_memory,
        const vk::raii::Device& device,
        const vk::raii::PhysicalDevice& physical_device,
        const vk::DeviceSize size,
        const vk::BufferUsageFlags usage,
        const vk::MemoryPropertyFlags properties
    ) {
        vk::BufferCreateInfo create_info;
        create_info.size = size;
        create_info.usage = usage;
        create_info.sharingMode = vk::SharingMode::eExclusive;

        buffer = device.createBuffer(create_info);

        const vk::MemoryRequirements requirements = buffer.getMemoryRequirements();
        vk::MemoryAllocateInfo alloc_info;
        alloc_info.allocationSize = requirements.size;
        alloc_info.memoryTypeIndex = findMemoryType(physical_device, requirements.memoryTypeBits, properties);
        buffer_memory = device.allocateMemory( alloc_info );

        buffer.bindMemory(buffer_memory, 0);
    }

    // 创建图像视图
    [[nodiscard]]
    vk::raii::ImageView create_image_view(
        const vk::raii::Device& device,
        const vk::Image image,
        const vk::Format format,
        const vk::ImageAspectFlags aspect_flags
    ) {
        vk::ImageViewCreateInfo viewInfo;
        viewInfo.image = image;
        viewInfo.viewType = vk::ImageViewType::e2D;
        viewInfo.format = format;
        viewInfo.subresourceRange.aspectMask = aspect_flags;
        viewInfo.subresourceRange.baseMipLevel = 0;
        viewInfo.subresourceRange.levelCount = 1;
        viewInfo.subresourceRange.baseArrayLayer = 0;
        viewInfo.subresourceRange.layerCount = 1;
        return device.createImageView(viewInfo);
    }

    // 复制缓冲区内容到另一个缓冲区
    void copy_buffer(
        const vk::raii::CommandPool& command_pool,
        const vk::raii::Device& device,
        const vk::raii::Queue& queue,
        const vk::Buffer src_buffer,
        const vk::Buffer dst_buffer,
        const vk::DeviceSize size
    )  {
        const auto command_buffer = begin_command(command_pool, device);
        const vk::BufferCopy region{ 0, 0, size };
        command_buffer.copyBuffer(src_buffer, dst_buffer, region);
        end_command( command_buffer , queue );
    }

    // 将缓冲区内容复制到图像
    void copy_buffer_to_image(
        const vk::raii::CommandPool& command_pool,
        const vk::raii::Device& device,
        const vk::raii::Queue& queue,
        const vk::Buffer buffer,
        const vk::Image image,
        const std::uint32_t width,
        const std::uint32_t height
    ) {
        const auto commandBuffer = begin_command( command_pool, device) ;
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
        commandBuffer.copyBufferToImage(
                buffer,
                image,
                vk::ImageLayout::eTransferDstOptimal,
                region
        );
        end_command( commandBuffer , queue );
    }

    // 使用屏障转换图像布局
    void transition_image_layout(
        const vk::raii::CommandPool& command_pool,
        const vk::raii::Device& device,
        const vk::raii::Queue& queue,
        const vk::Image image,
        const vk::ImageLayout oldLayout,
        const vk::ImageLayout newLayout
    ) {
        const auto command_buffer = begin_command(command_pool, device);

        vk::ImageMemoryBarrier barrier;
        barrier.oldLayout = oldLayout;
        barrier.newLayout = newLayout;
        barrier.srcQueueFamilyIndex = vk::QueueFamilyIgnored;
        barrier.dstQueueFamilyIndex = vk::QueueFamilyIgnored;
        barrier.image = image;
        barrier.subresourceRange.baseMipLevel = 0;
        barrier.subresourceRange.levelCount = 1;
        barrier.subresourceRange.baseArrayLayer = 0;
        barrier.subresourceRange.layerCount = 1;
        barrier.subresourceRange.aspectMask = vk::ImageAspectFlagBits::eColor;

        vk::PipelineStageFlagBits src_stage;
        vk::PipelineStageFlagBits dst_stage;

        if( oldLayout == vk::ImageLayout::eUndefined &&
            newLayout == vk::ImageLayout::eTransferDstOptimal
        ) {
            barrier.srcAccessMask = {};
            barrier.dstAccessMask = vk::AccessFlagBits::eTransferWrite;
            src_stage = vk::PipelineStageFlagBits::eTopOfPipe;
            dst_stage = vk::PipelineStageFlagBits::eTransfer;
        } else if(
            oldLayout == vk::ImageLayout::eTransferDstOptimal &&
            newLayout == vk::ImageLayout::eShaderReadOnlyOptimal
        ) {
            barrier.srcAccessMask = vk::AccessFlagBits::eTransferWrite;
            barrier.dstAccessMask = vk::AccessFlagBits::eShaderRead;
            src_stage = vk::PipelineStageFlagBits::eTransfer;
            dst_stage = vk::PipelineStageFlagBits::eFragmentShader;
        } else {
            throw std::invalid_argument("unsupported layout transition!");
        }

        command_buffer.pipelineBarrier(
                src_stage,        // srcStageMask
                dst_stage,   // dstStageMask
                {},         // dependencyFlags
                nullptr,    // memoryBarriers
                nullptr,    // bufferMemoryBarriers
                barrier     // imageMemoryBarriers
        );

        end_command( command_buffer, queue );
    }

}

