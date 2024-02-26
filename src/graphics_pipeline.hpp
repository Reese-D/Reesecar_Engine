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
    graphics_pipeline(VkDevice device, VkFormat format, VkDescriptorSetLayout descriptorSetLayout);
    ~graphics_pipeline();
    VkRenderPass getRenderPass();
    VkPipeline getGraphicsPipeline();
    VkPipelineLayout getPipelineLayout();

    struct Vertex {
        glm::vec2 pos;
        glm::vec3 color;
        glm::vec2 textureCoordinate;
        
        static VkVertexInputBindingDescription getBindingDescription() {
            VkVertexInputBindingDescription bindingDescription{};
            bindingDescription.binding = 0;
            bindingDescription.stride = sizeof(Vertex);
            bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

            return bindingDescription;
        }

        static std::array<VkVertexInputAttributeDescription, 3> getAttributeDescriptions() {
            std::array<VkVertexInputAttributeDescription, 3> attributeDescriptions{};

            attributeDescriptions[0].binding = 0;
            attributeDescriptions[0].location = 0;
            attributeDescriptions[0].format = VK_FORMAT_R32G32_SFLOAT;
            attributeDescriptions[0].offset = offsetof(Vertex, pos);

            attributeDescriptions[1].binding = 0;
            attributeDescriptions[1].location = 1;
            attributeDescriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;
            attributeDescriptions[1].offset = offsetof(Vertex, color);

            attributeDescriptions[2].binding = 0;
            attributeDescriptions[2].location = 2;
            attributeDescriptions[2].format = VK_FORMAT_R32G32_SFLOAT;
            attributeDescriptions[2].offset = offsetof(Vertex, textureCoordinate);
            
            return attributeDescriptions;
        }
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
};

