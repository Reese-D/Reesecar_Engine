#pragma once
#include <vulkan/vulkan.h>
#include <vulkan/vulkan_core.h>

#include <memory>
#include <optional>

#include "queue.hpp"
#include "swapchain.hpp"
#include "graphics_pipeline.hpp"

class device
{
public:
    device() = delete;
    ~device();
    device(std::shared_ptr<VkInstance> instance, VkSurfaceKHR surface, int width, int height);
    static swapchain::SwapChainSupportDetails querySwapChainSupport(VkPhysicalDevice device, VkSurfaceKHR surface);
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
    static VkQueue getGraphicsDeviceQueue(VkDevice logicalDevice
                                  ,queue::QueueFamilyIndices indices);
    static VkQueue getPresentDeviceQueue(VkDevice logicalDevice
                                  ,queue::QueueFamilyIndices indices);
    static bool hasSupportForSurface(VkPhysicalDevice device, VkSurfaceKHR surface, queue::QueueFamilyIndices indices);
    static bool doesDeviceSupportExtensions(VkPhysicalDevice device, const std::vector<const char*> deviceExtensions);
    void createCommandPool(VkSurfaceKHR surface);

    VkCommandPool commandPool_;
    VkDevice logicalDevice_;
    VkPhysicalDevice physicalDevice_;
    VkQueue graphicsQueue_;
    VkQueue presentQueue_;
    queue::QueueFamilyIndices indices_;
    VkPhysicalDeviceProperties physicalDeviceProperties_;
    VkPhysicalDeviceFeatures physicalDeviceFeatures_;
    swapchain* swapchain_;
    graphics_pipeline* graphicsPipeline_;
    
    const std::vector<const char*> deviceExtensions_ = {
        VK_KHR_SWAPCHAIN_EXTENSION_NAME
    };
};








