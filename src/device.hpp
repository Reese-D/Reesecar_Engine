#pragma once
#include <vulkan/vulkan.h>
#include <vulkan/vulkan_core.h>

#include <memory>
#include <optional>

#include "queue.hpp"
class device
{
public:
    device() = delete;
    ~device();
    device(std::shared_ptr<VkInstance> instance, VkSurfaceKHR surface);
    
private:
    static VkPhysicalDevice getPhysicalDevice(std::shared_ptr<VkInstance> instance, VkSurfaceKHR surface);
    static VkPhysicalDeviceFeatures getDeviceFeatures(VkPhysicalDevice device);
    static VkPhysicalDeviceProperties getDeviceProperties(VkPhysicalDevice device);
    static VkDevice getLogicalDevice(VkPhysicalDevice physicalDevice
                                     ,VkPhysicalDeviceFeatures deviceFeatures
                                     ,queue::QueueFamilyIndices indices);
    static bool isDeviceSuitable(VkPhysicalDevice physicalDevice
                                 ,VkPhysicalDeviceProperties deviceProperties
                                 ,VkPhysicalDeviceFeatures deviceFeatures
                                 ,std::optional<uint32_t> indices);
    static VkQueue getDeviceQueue(VkDevice logicalDevice
                                  ,queue::QueueFamilyIndices indices);
    static bool hasSupportForSurface(VkPhysicalDevice device, VkSurfaceKHR surface, queue::QueueFamilyIndices indices);

    VkDevice logicalDevice_;
    VkPhysicalDevice physicalDevice_;
    VkQueue graphicsQueue_;
    queue::QueueFamilyIndices indices_;
    VkPhysicalDeviceProperties physicalDeviceProperties_;
    VkPhysicalDeviceFeatures physicalDeviceFeatures_;
    
};








