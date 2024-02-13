#pragma once
#include <vulkan/vulkan.h>
#include <vector>
class swapchain
{
public:
    swapchain(VkDevice device, VkPhysicalDevice physicalDevice, VkSurfaceKHR surface, int windowWidth, int windowHeight);
    ~swapchain();
    struct SwapChainSupportDetails {
        VkSurfaceCapabilitiesKHR capabilities;
        std::vector<VkSurfaceFormatKHR> formats;
        std::vector<VkPresentModeKHR> presentModes;
    };
private:
    VkDevice device_;
    VkSwapchainKHR swapChain_;
    VkSurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats);
    VkPresentModeKHR chooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes);
    VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities, int width, int height);
};
