#pragma once

#include <vulkan/vulkan_core.h>
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <vector>
#include <memory>

#include "validation.h"
#include "device.h"

class instance
{
 public:
    instance() = delete;
    instance(const std::vector<const char*> validationLayers);
    ~instance();
    std::shared_ptr<VkInstance> getInstance();
 private:
    std::shared_ptr<VkInstance> initVulkan();
    std::shared_ptr<VkInstance> createInstance(const std::vector<const char*> validationLayers);
    std::vector<const char*> getRequiredExtensions();
    void pickPhysicalDevice(std::shared_ptr<VkInstance> instance);
    bool isDeviceSuitable(VkPhysicalDevice device);
    std::shared_ptr<VkInstance> instance_;
    std::unique_ptr<validation> validation_;

};

