#include <iostream>
#include <format>
#include <vulkan/vulkan_hpp_macros.hpp>
import vulkan_hpp;



int main() {
    vk::raii::Context ctx; // 初始化上下文

    vk::ApplicationInfo app_info = {
        "My App", 1,
        "My Engine", 1,
        vk::makeApiVersion(1, 0, 0, 0)
    };
    vk::InstanceCreateInfo create_info{ {}, &app_info };
    vk::raii::Instance instance = ctx.createInstance(create_info);

    auto physicalDevices = instance.enumeratePhysicalDevices();
    for (const auto& physicalDevice : physicalDevices) {
        std::cout << physicalDevice.getProperties().deviceName << std::endl;
    }

}
