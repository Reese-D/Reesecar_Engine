#pragma once
#include "vulkan/vulkan.h"
#include <vector>
#include <string>
class graphics_pipeline
{
public:
    VkPipelineLayout pipelineLayout_;
    std::vector<VkPipelineShaderStageCreateInfo> shaderStages_;

    graphics_pipeline(VkDevice device);
    ~graphics_pipeline();
private:
    VkDevice device_;
    void createGraphicsPipeline();
    VkShaderModule createShaderModule(const std::vector<char>& code);
    static std::vector<char> readFile(const std::string& filename);
};
