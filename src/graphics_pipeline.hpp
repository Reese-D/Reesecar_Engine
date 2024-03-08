#pragma once
#include "vulkan/vulkan.h"
#include <glm/glm.hpp>
#include <vector>
#include <string>
#include <array>

#include "render_pass.hpp"
class graphics_pipeline
{
public:
    graphics_pipeline(VkDevice device, VkFormat colorFormat, VkFormat depthFormat, VkDescriptorSetLayout descriptorSetLayout);
    ~graphics_pipeline();
    VkRenderPass getRenderPass();
    VkPipeline getGraphicsPipeline();
    VkPipelineLayout getPipelineLayout();

    struct Vertex {
        glm::vec3 pos;
        glm::vec3 color;
        glm::vec2 textureCoordinate;        
    };

private:
    VkPipelineLayout pipelineLayout_;
    VkDevice device_;
    VkPipeline graphicsPipeline_;
    VkDescriptorSetLayout descriptorSetLayout_;
    render_pass* pCustomRenderPass_;

    void createGraphicsPipeline();
    VkShaderModule createShaderModule(const std::vector<char>& code);
    static std::vector<char> readFile(const std::string& filename);
    static VkVertexInputAttributeDescription generateVertexInputAttributeDescription(uint32_t binding,
                                                                                     uint32_t location,
                                                                                     VkFormat format,
                                                                                     uint32_t offset);
    static VkVertexInputBindingDescription generateVertexInputBindingDescription(uint32_t binding,
                                                                         uint32_t stride,
                                                                         VkVertexInputRate inputRate);
    static std::vector<VkVertexInputBindingDescription> getBindingDescriptions();
    static std::vector<VkVertexInputAttributeDescription> getAttributeDescriptions();


};

