#pragma once
#include "vulkan/vulkan.h"

class render_pass
{
public:
    render_pass() = delete;
    render_pass(VkDevice device, VkFormat swapChainImageFormat);
    VkRenderPass getRenderPass();
    ~render_pass();
private:
    VkDevice device_;
    VkRenderPass renderPass_;
};
