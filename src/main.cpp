import std;
import vulkan_hpp;
import App;

int main() {
    try {
        vht::App app{};
        app.run();
    } catch (const vk::SystemError& e) {
        std::cerr << e.code().message() << std::endl;
        std::cerr << e.what() << std::endl;
    } catch (const std::exception& e) {
        std::cerr << e.what() << std::endl;
    }
}
