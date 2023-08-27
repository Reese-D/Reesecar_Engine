//this will include vulkan/vulkan.h through GLFW
#include <memory>
#include <vulkan/vulkan_core.h>

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <iostream>
#include <stdexcept>
#include <cstdlib>
#include <vector>
#include <cstring>

#include "instance.h"
#include "surface.hpp"
#include "device.h"

const uint32_t WIDTH = 800;
const uint32_t HEIGHT = 600;

const std::vector<const char*> validationLayers = {
    "VK_LAYER_KHRONOS_validation"
};

class HelloTriangleApplication {
public:
    void run()
    {
        std::shared_ptr<GLFWwindow> glfwWindow = initWindow();


        //printAvailableExtensions();
        auto myInstance = instance{validationLayers};
        std::shared_ptr<VkInstance> myVkInstance = myInstance.getInstance();
        auto surface = window::getSurface(*myVkInstance, glfwWindow);
        auto myDevice = std::make_unique<device>(myVkInstance, surface);
                
        mainLoop(glfwWindow);
        cleanup(surface, *myVkInstance);
    }

private:
    void mainLoop(std::shared_ptr<GLFWwindow> window)
    {
        while (!glfwWindowShouldClose(window.get())) {
            glfwPollEvents();
        }
    }

    void cleanup(VkSurfaceKHR surface, VkInstance instance)
    {
        vkDestroySurfaceKHR(instance, surface, nullptr);
        glfwTerminate();
    }

    std::shared_ptr<GLFWwindow> initWindow()
    {
        glfwInit();
        
        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
        glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE); //disable resizing for now, takes more complexity to handle it
        std::shared_ptr<GLFWwindow> window(glfwCreateWindow(WIDTH, HEIGHT, "Vulkan", nullptr, nullptr), &glfwDestroyWindow);
        
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
