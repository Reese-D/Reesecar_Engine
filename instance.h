#pragma once
#include <vulkan/vulkan_core.h>
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include "validation.h"
#include <vector>
class instance
{
 public:
    instance() = delete;
    instance(const std::vector<const char*> validationLayers);
    ~instance();
    VkInstance* getInstance();
 private:
    VkInstance* initVulkan();
    VkInstance* createInstance(const std::vector<const char*> validationLayers);
    std::vector<const char*> getRequiredExtensions();
    
    VkInstance* instance_;
    validation* validation_;
};

