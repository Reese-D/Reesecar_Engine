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

    VkFormat getImageFormat();
    void createFrameBuffers(VkRenderPass renderPass, VkImageView depthImageView);
    VkExtent2D getSwapChainExtent();
    std::vector<VkFramebuffer> getSwapChainFramebuffers();
    VkSwapchainKHR getSwapchain();
    VkImageView createTextureImageView(VkImage textureImage);
    VkImageView createImageView(VkImage image, VkFormat format, VkImageAspectFlags aspectFlags);
private:
    VkDevice device_;
    VkSwapchainKHR swapChain_;
    std::vector<VkImage> swapChainImages_;
    VkFormat swapChainImageFormat_;
    VkExtent2D swapChainExtent_;
    std::vector<VkImageView> swapChainImageViews_;
    std::vector<VkFramebuffer> swapChainFramebuffers_;

    void createImageViews();
    VkSurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats);
    VkPresentModeKHR chooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes);
    VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities, int width, int height);
};
