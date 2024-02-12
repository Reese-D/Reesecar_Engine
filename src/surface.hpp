#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <memory>

class window
{
public:
    static VkSurfaceKHR getSurface(VkInstance instance, std::shared_ptr<GLFWwindow> window); 
};
