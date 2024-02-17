#pragma once
#include <vulkan/vulkan.h>
#include <vulkan/vulkan_core.h>

#include <memory>
#include <optional>

#include "queue.hpp"
#include "swapchain.hpp"

class device
{
public:
    device() = delete;
    ~device();
    device(std::shared_ptr<VkInstance> instance, VkSurfaceKHR surface, int width, int height);
    static swapchain::SwapChainSupportDetails querySwapChainSupport(VkPhysicalDevice device, VkSurfaceKHR surface);
    VkDevice getLogicalDevice();
    VkPhysicalDevice getPhysicalDevice();
    VkQueue getGraphicsDeviceQueue(VkDevice logicalDevice
                                           ,queue::QueueFamilyIndices indices);
    VkQueue getPresentDeviceQueue(VkDevice logicalDevice
                                          ,queue::QueueFamilyIndices indices);

private:
    static VkPhysicalDevice getPhysicalDevice(std::shared_ptr<VkInstance> instance, VkSurfaceKHR surface, const std::vector<const char*> deviceExtensions);
    static VkPhysicalDeviceFeatures getDeviceFeatures(VkPhysicalDevice device);
    static VkPhysicalDeviceProperties getDeviceProperties(VkPhysicalDevice device);
    static VkDevice getLogicalDevice(VkPhysicalDevice physicalDevice
                                     ,VkPhysicalDeviceFeatures deviceFeatures
                                     ,queue::QueueFamilyIndices indices
                                     ,std::vector<const char*> deviceExtensions);
    static bool isDeviceSuitable(VkPhysicalDevice physicalDevice
                                 ,VkPhysicalDeviceProperties deviceProperties
                                 ,VkPhysicalDeviceFeatures deviceFeatures
                                 ,queue::QueueFamilyIndices indices
                                 ,VkSurfaceKHR surface);
    static bool hasSupportForSurface(VkPhysicalDevice device, VkSurfaceKHR surface, queue::QueueFamilyIndices indices);
    static bool doesDeviceSupportExtensions(VkPhysicalDevice device, const std::vector<const char*> deviceExtensions);
    
    VkDevice logicalDevice_;
    VkPhysicalDevice physicalDevice_;
    queue::QueueFamilyIndices indices_;
    VkPhysicalDeviceProperties physicalDeviceProperties_;
    VkPhysicalDeviceFeatures physicalDeviceFeatures_;

    
    const std::vector<const char*> deviceExtensions_ = {
        VK_KHR_SWAPCHAIN_EXTENSION_NAME
    };
};








