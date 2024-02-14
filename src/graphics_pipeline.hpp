#pragma once
#include "vulkan/vulkan.h"
#include <vector>
#include <string>

#include "render_pass.hpp"
class graphics_pipeline
{
public:
    graphics_pipeline(VkDevice device, VkFormat format);
    ~graphics_pipeline();
    VkRenderPass getRenderPass();
private:
    VkPipelineLayout pipelineLayout_;
    VkDevice device_;
    VkPipeline graphicsPipeline_;
    render_pass* pCustomRenderPass_;
    
    void createGraphicsPipeline();
    VkShaderModule createShaderModule(const std::vector<char>& code);
    static std::vector<char> readFile(const std::string& filename);
};
