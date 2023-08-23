#pragma once
#include <vulkan/vulkan.h>
#include <memory>

class device
{
public:
    static void pickPhysicalDevice(std::shared_ptr<VkInstance> instance);
private:
    static bool isDeviceSuitable(VkPhysicalDevice device);
};
