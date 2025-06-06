# **开发环境**

## **概述**
本文将指导你完成Vulkan应用程序开发环境的设置，包括安装必要的工具和库。

注意，CMake和Vcpkg自身的安装，以及MSVC/Clang等编译器的安装，不是本教程的内容。

## **Vulkan SDK安装**
Vulkan SDK是开发Vulkan应用程序的核心组件，包含：

- 头文件
- 标准验证层
- 调试工具
- Vulkan函数加载器
- shader编译支持
- ······

可以从 [LunarG官网](https://vulkan.lunarg.com/) 下载SDK，无需注册账户。


### Windows安装
1. 建议使用Visual Studio 2022以获得完整C++20支持
2. 从 [官网](https://vulkan.lunarg.com/) 下载Vulkan SDK并运行安装程序
3. 重要：允许安装程序设置环境变量
4. 验证安装：
    - 进入SDK安装目录的Bin子目录
    - 运行vkcube.exe演示程序
    - 应看到旋转的立方体窗口

![cube](../../images/cube_demo.png)

### Linux
图形界面安装，参考Windows安装方法。

#### 命令行安装：
Ubuntu/Debian系
```shell
sudo apt install vulkan-tools libvulkan-dev vulkan-validationlayers-dev spirv-tools
```

Fedora/RHEL系
```shell 
sudo dnf install vulkan-tools vulkan-loader-devel mesa-vulkan-devel vulkan-validation-layers-devel
```

Arch Linux
```shell
sudo pacman -S vulkan-devel
```

#### 验证安装：
```shell
vkcube
```

确保您看到以下窗口弹出

![cube](../../images/cube_demo_nowindow.png)


### MacOS安装

要求：

- MacOS 10.11或更高版本
- 支持Metal API的硬件

步骤：

1. 从 [LunarG官网](https://vulkan.lunarg.com/) 下载SDK

2. 解压到选定目录

3. 运行`Applications`目录下的`vkcube`演示程序

您应该看到以下内容

![cube](../../images/cube_demo_mac.png)

## **依赖库安装**

我们使用Vcpkg作为跨平台包管理器。

Vcpkg安装参考 [官方文档](https://learn.microsoft.com/zh-cn/vcpkg/get_started/overview)

### GLFW
如前所述，Vulkan 本身是一个平台无关的 API，不包含用于创建窗口以显示渲染结果的工具。  
我们将使用 [GLFW 库](http://www.glfw.org/) 来创建窗口，它支持 Windows、Linux 和 MacOS，且和 Vulkan 有很好的集成。

安装命令：
```shell
vcpkg install glfw3
```

### GLM
与 DirectX 12 不同，Vulkan 不包含用于线性代数运算的库。我们使用 GLM 线性代数库，它专为图形 API 设计，常用于 OpenGL。

安装命令：
```shell
vcpkg install glm
```

## **项目初始化**

### 目录结构

```
项目根目录/
│
├── CMakeLists.txt          # 主CMake配置文件
│
└── src/                    # 源代码目录
    │
    └── main.cpp            # 主程序入口
```

### CMake配置

1. 设置工具链为vcpkg。
2. 设定项目C++标准。
3. 查找Vulkan，glm，glfw3三个库。
4. 添加主程序。
5. 链接库。

**参考代码：**

```cmake
# CMakeLists.txt
cmake_minimum_required(VERSION 3.30)

# 需要设置环境变量VCPKG_ROOT
file(TO_CMAKE_PATH "$ENV{VCPKG_ROOT}" VCPKG_CMAKE_PATH)
set(CMAKE_TOOLCHAIN_FILE "${VCPKG_CMAKE_PATH}/scripts/buildsystems/vcpkg.cmake")

project(HelloVulkan LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)

# 需要设置VULKAN_SDK环境变量，比如 D:\Vulkan\1.4.309.0
# 环境变量默认在Vulkan SDK安装时，自动设置
find_package(Vulkan REQUIRED)

# 通过vcpkg导入第三方库
find_package(glfw3 CONFIG REQUIRED)
find_package(glm CONFIG REQUIRED)

# 添加可执行程序目标
add_executable(${PROJECT_NAME} src/main.cpp)

# 链接库
target_link_libraries(${PROJECT_NAME} PRIVATE Vulkan::Vulkan )
target_link_libraries(${PROJECT_NAME} PRIVATE glm::glm )
target_link_libraries(${PROJECT_NAME} PRIVATE glfw )

```


注意，`CMakeLists.txt`代码将在较长时间内不再变动，因此后面几章只给出C++代码。

### 测试代码

添加测试代码，测试三个库是否正常：

```cpp
// main.cpp
#include <vulkan/vulkan.hpp>
#include <vulkan/vulkan_raii.hpp>
#include <GLFW/glfw3.h>
#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/vec4.hpp>
#include <glm/mat4x4.hpp>

#include <iostream>

int main() {
    glfwInit();

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    GLFWwindow* window = glfwCreateWindow(800, 600, "Vulkan window", nullptr, nullptr);

    uint32_t extensionCount = 0;
    vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, nullptr);

    std::cout << extensionCount << " extensions supported\n";

    glm::mat4 matrix;
    glm::vec4 vec;
    auto test = matrix * vec;

    while(!glfwWindowShouldClose(window)) {
        glfwPollEvents();
    }

    glfwDestroyWindow(window);

    glfwTerminate();

    return 0;
}

```

你无需理解上述C++代码的含义，这只是测试库是否成功导入。

### 构建运行

下面尝试构建和运行程序，在项目根目录执行：

```shell
cmake -B build
cmake --build build
```

运行：
```shell
# Windows
build\Debug\HelloVulkan.exe

# Linux/MacOS
build/HelloVulkan
```

预期结果：

- 弹出空白窗口
- 关闭窗口后，控制台输出支持的Vulkan扩展数量

> 你或许喜欢使用CMakePresets简化配置，可以自行创建使用，但本教程不涉及CMake预设。

---

下面让我们了解一下 `vulkan.h` 与 `vulkan.hpp` 的差异！！

--- 

**[CMake代码](../../codes/00/01_env/CMakeLists.txt)**

**[C++代码](../../codes/00/01_env/main.cpp)**
