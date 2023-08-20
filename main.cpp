//this will include vulkan/vulkan.h through GLFW
#include <vulkan/vulkan_core.h>
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <iostream>
#include <stdexcept>
#include <cstdlib>
#include <vector>

const uint32_t WIDTH = 800;
const uint32_t HEIGHT = 600;

class HelloTriangleApplication {
public:
    void run() {
        auto window = initWindow();
        printAvailableExtensions();
        auto instance = initVulkan();
        mainLoop(window);
        cleanup(window, instance);
    }

private:
    VkInstance* initVulkan() {
        auto instance = createInstance();
        return instance;
    }

    VkInstance* createInstance(){
        VkApplicationInfo appInfo{};
        //most of these are optional
        appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO; //most structs require explicit type names
        appInfo.pApplicationName = "Hello Triangle";
        appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
        appInfo.pEngineName = "The glorious Reesecar engine";
        appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
        appInfo.apiVersion = VK_API_VERSION_1_0;
        appInfo.pNext = nullptr; // pNext is for extensions, usually null

        //glfw can give us the required extensions to be passed through
        uint32_t glfwExtensionCount = 0;
        const char** glfwExtensions;
        glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

        //not optional, defines which global extensions and validation layers to use
        VkInstanceCreateInfo createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
        createInfo.pApplicationInfo = &appInfo;
        createInfo.enabledExtensionCount = glfwExtensionCount;
        createInfo.ppEnabledExtensionNames = glfwExtensions;
        createInfo.enabledLayerCount = 0; //global validation layers to enable

        VkInstance* instance = new VkInstance();
        VkResult result = vkCreateInstance(&createInfo, nullptr, instance);
        
        if (result != VK_SUCCESS) {
            throw std::runtime_error("failed to create instance!");
        }
        
        return instance;
    }

    void mainLoop(GLFWwindow* window) {
        while (!glfwWindowShouldClose(window)) {
            glfwPollEvents();
        }
    }

    void cleanup(GLFWwindow* window, VkInstance* instance) {

        vkDestroyInstance(*instance, nullptr);
        glfwDestroyWindow(window);
        glfwTerminate();
    }

    GLFWwindow* initWindow() {
        glfwInit();
        
        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
        glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE); //disable resizing for now, takes more complexity to handle it
        GLFWwindow* window = glfwCreateWindow(WIDTH, HEIGHT, "Vulkan", nullptr, nullptr);
        
        return window;
    }

    void printAvailableExtensions()
    {
        uint32_t extensionCount = 0;
        vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, nullptr);
        std::vector<VkExtensionProperties> extensions(extensionCount);
        vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, extensions.data());
        std::cout << "available extensions:\n";

        for (const auto& extension : extensions) {
            std::cout << '\t' << extension.extensionName << '\n';
        }
    }
};

int main() {
    HelloTriangleApplication app;

    try {
        app.run();
    } catch (const std::exception& e) {
        std::cerr << e.what() << std::endl;
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
