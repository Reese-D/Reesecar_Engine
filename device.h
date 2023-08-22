#pragma once
#include <vulkan/vulkan.h>

class device
{
public:
    static void pickPhysicalDevice(VkInstance* instance);
private:
    static bool isDeviceSuitable(VkPhysicalDevice device);
};
