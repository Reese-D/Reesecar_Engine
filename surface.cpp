#include "surface.hpp"

VkSurfaceKHR window::getSurface(VkInstance instance, std::shared_ptr<GLFWwindow> window)
{
    VkSurfaceKHR surface{};
    if (glfwCreateWindowSurface(instance, window.get(), nullptr, &surface) != VK_SUCCESS) {
        throw std::runtime_error("failed to create window surface!");
    }

    return surface;
}
