import std;
import glfw;
import vulkan_hpp;

int main() {
    const vk::raii::Context context{};

    // Initialize GLFW and create a window
    glfw::init();
    glfw::window_hint(glfw::CLIENT_API, glfw::NO_API);
    glfw::window_hint(glfw::RESIZABLE, glfw::FALSE);
    glfw::Window* window = glfw::create_window(800, 600, "Vulkan", nullptr, nullptr);

    // get the required Vulkan extensions for GLFW
    unsigned int count = 0;
    const char ** extensions = glfw::get_required_instance_extensions(&count);
    std::vector<const char*> extension_list(extensions, extensions + count);

    // create a Vulkan instance
    constexpr vk::ApplicationInfo appInfo{
        "Vulkan GLFW Example", 1,
        "No Engine",1 ,
        vk::makeApiVersion(1,4,0,0)
    };
    vk::InstanceCreateInfo create_info{{}, &appInfo, };
    create_info.setPEnabledExtensionNames(extension_list);

    const vk::raii::Instance instance{context, create_info};

    // create a Vulkan surface from the GLFW window
    glfw::VkSurfaceKHR c_surface{};
    if (static_cast<vk::Result>(glfw::create_window_surface(*instance, window, nullptr, &c_surface)) != vk::Result::eSuccess) {
        std::println(std::cerr << "Failed to create window surface!");
        return -1;
    }
    vk::raii::SurfaceKHR surface{ instance, c_surface };
    std::println("Vulkan window surface created successfully!");

    // wait for the window to close
    while (!glfw::window_should_close(window)) {
        glfw::poll_events();
    }

    // clean up
    glfw::destroy_window(window);
    glfw::terminate();
}
