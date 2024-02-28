//this will include vulkan/vulkan.h through GLFW
#include <memory>

#include <vulkan/vulkan_core.h>
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
//Optional, removes the need for alignas on UBO's, but doesn't work for nested structures.
//#define GLM_FORCE_DEFAULT_ALIGNED_GENTYPES
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <chrono>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#include <iostream>
#include <stdexcept>
#include <cstdlib>
#include <vector>
#include <cstring>

#include "instance.hpp"
#include "surface.hpp"
#include "device.hpp"
#include "queue.hpp"
#include "swapchain.hpp"
#include "graphics_pipeline.hpp"

const uint32_t WIDTH = 800;
const uint32_t HEIGHT = 600;
const int MAX_FRAMES_IN_FLIGHT = 2;

const std::vector<const char*> validationLayers = {
    "VK_LAYER_KHRONOS_validation"
};

class HelloTriangleApplication {
public:
    void run()
    {
        glfwWindow_ = initWindow();

        //printAvailableExtensions();
        auto myInstance = instance{validationLayers};
        std::shared_ptr<VkInstance> myVkInstance = myInstance.getInstance();
        surface_ = window::getSurface(*myVkInstance, glfwWindow_);
        pMyDevice_ = new device(myVkInstance, surface_, WIDTH, HEIGHT);
        depthFormat_ = pMyDevice_->findDepthFormat();
        
        physicalDevice_ = pMyDevice_->getPhysicalDevice();
        logicalDevice_ = pMyDevice_->getLogicalDevice();
        indices_ = queue::findQueueFamilies(physicalDevice_, surface_);
        graphicsQueue_ = pMyDevice_->getGraphicsDeviceQueue(logicalDevice_, indices_);
        presentQueue_ = pMyDevice_->getPresentDeviceQueue(logicalDevice_, indices_);
        
        pMySwapchain_ = new swapchain(logicalDevice_, physicalDevice_, surface_, WIDTH, HEIGHT);
        swapchainExtent_ = pMySwapchain_->getSwapChainExtent();
        
        createDepthResources();
        depthImageView_ = pMySwapchain_->createImageView(depthImage_, depthFormat_, VK_IMAGE_ASPECT_DEPTH_BIT);
        swapchain_ = pMySwapchain_->getSwapchain();

        //note: image format is just an enum, don't need to recreate this if swapchain dies/changes
        auto colorFormat = pMySwapchain_->getImageFormat();
        //won't change, so we can use in our swapchain recreation


        createDescriptorSetLayout();
        
        pMyGraphicsPipeline_ = new graphics_pipeline(logicalDevice_, colorFormat, depthFormat_, descriptorSetLayout_);
        pipelineLayout_ = pMyGraphicsPipeline_->getPipelineLayout();
        
        renderPass_ = pMyGraphicsPipeline_->getRenderPass();
        commandPool_ = createCommandPool(surface_);

        pMySwapchain_->createFrameBuffers(renderPass_, depthImageView_);
        swapchainFramebuffers_ = pMySwapchain_->getSwapChainFramebuffers();

        
        graphicsPipeline_ = pMyGraphicsPipeline_->getGraphicsPipeline();

        createTextureImage();
        textureImageView_ = pMySwapchain_->createTextureImageView(textureImage_);
        createTextureSampler();
        createVertexBuffer();
        createIndexBuffer();
        createUniformBuffers();
        createDescriptorPool();
        createDescriptorSets();
        
        createCommandBuffers();
        createSyncObjects();
        
        mainLoop(glfwWindow_);

        cleanup(surface_, *myVkInstance);
    }
    bool framebufferResized_ = false;
private:
    uint32_t currentFrame_ = 0;
    
    VkPhysicalDevice physicalDevice_;
    VkDevice logicalDevice_;
    VkQueue graphicsQueue_;
    VkQueue presentQueue_;
    VkCommandPool commandPool_;
    VkRenderPass renderPass_;
    std::vector<VkSemaphore> imageAvailableSemaphores_;
    std::vector<VkSemaphore> renderFinishedSemaphores_;
    std::vector<VkFence> inFlightFences_;
    VkPipeline graphicsPipeline_;
    VkExtent2D swapchainExtent_;
    std::vector<VkCommandBuffer> commandBuffers_;
    VkSwapchainKHR swapchain_;
    VkSurfaceKHR surface_;
    VkPipelineLayout pipelineLayout_;
    
    VkBuffer vertexBuffer_;
    VkDeviceMemory vertexBufferMemory_;
    VkBuffer indexBuffer_;
    VkDeviceMemory indexBufferMemory_;

    VkImage textureImage_;
    VkImageView textureImageView_;
    VkDeviceMemory textureImageMemory_;
    VkSampler textureSampler_;
    
    VkImage depthImage_;
    VkDeviceMemory depthImageMemory_;
    VkImageView depthImageView_;
    VkFormat depthFormat_;
    
    VkDescriptorSetLayout descriptorSetLayout_;
    VkDescriptorPool descriptorPool_;

    std::vector<VkBuffer> uniformBuffers_;
    std::vector<VkDeviceMemory> uniformBuffersMemory_;
    std::vector<void*> uniformBuffersMapped_;
    
    std::vector<VkFramebuffer> swapchainFramebuffers_;
    std::vector<VkDescriptorSet> descriptorSets_;    
    std::shared_ptr<GLFWwindow> glfwWindow_;
    
    queue::QueueFamilyIndices indices_;
    
    swapchain* pMySwapchain_;
    graphics_pipeline* pMyGraphicsPipeline_;
    device* pMyDevice_;

    //temporary constant to test shader
    const std::vector<graphics_pipeline::Vertex> vertices = {
        {{-0.5f, -0.5f, 0.0f}, {1.0f, 0.0f, 0.0f}, {0.0f, 0.0f}},
        {{0.5f, -0.5f, 0.0f}, {0.0f, 1.0f, 0.0f}, {1.0f, 0.0f}},
        {{0.5f, 0.5f, 0.0f}, {0.0f, 0.0f, 1.0f}, {1.0f, 1.0f}},
        {{-0.5f, 0.5f, 0.0f}, {1.0f, 1.0f, 1.0f}, {0.0f, 1.0f}},

        {{-0.5f, -0.5f, -0.5f}, {1.0f, 0.0f, 0.0f}, {0.0f, 0.0f}},
        {{0.5f, -0.5f, -0.5f}, {0.0f, 1.0f, 0.0f}, {1.0f, 0.0f}},
        {{0.5f, 0.5f, -0.5f}, {0.0f, 0.0f, 1.0f}, {1.0f, 1.0f}},
        {{-0.5f, 0.5f, -0.5f}, {1.0f, 1.0f, 1.0f}, {0.0f, 1.0f}}
    };

    const std::vector<uint16_t> indices = {
        0, 1, 2, 2, 3, 0,
        4, 5, 6, 6, 7, 4
    };

    struct UniformBufferObject {
        alignas(16) glm::mat4 model;
        alignas(16) glm::mat4 view;
        alignas(16) glm::mat4 projection;
    };

    void createDepthResources() {
        createImage(swapchainExtent_.width, swapchainExtent_.height, depthFormat_, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, depthImage_, depthImageMemory_);
    }
    
    VkCommandBuffer beginSingleTimeCommands() {
        VkCommandBufferAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        allocInfo.commandPool = commandPool_;
        allocInfo.commandBufferCount = 1;

        VkCommandBuffer commandBuffer;
        vkAllocateCommandBuffers(logicalDevice_, &allocInfo, &commandBuffer);

        VkCommandBufferBeginInfo beginInfo{};
        beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

        vkBeginCommandBuffer(commandBuffer, &beginInfo);

        return commandBuffer;
    }

    void endSingleTimeCommands(VkCommandBuffer commandBuffer) {
        vkEndCommandBuffer(commandBuffer);

        VkSubmitInfo submitInfo{};
        submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers = &commandBuffer;

        vkQueueSubmit(graphicsQueue_, 1, &submitInfo, VK_NULL_HANDLE);
        vkQueueWaitIdle(graphicsQueue_);

        vkFreeCommandBuffers(logicalDevice_, commandPool_, 1, &commandBuffer);
    }

    void createTextureSampler() {
        VkPhysicalDeviceProperties properties{};
        vkGetPhysicalDeviceProperties(physicalDevice_, &properties);

        VkSamplerCreateInfo samplerInfo{};
        samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
        samplerInfo.magFilter = VK_FILTER_LINEAR;
        samplerInfo.minFilter = VK_FILTER_LINEAR;
        samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
        samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
        samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
        //If you change this to false, make sure to remove the requirement for it in device.cpp
        samplerInfo.anisotropyEnable = VK_TRUE; 
        samplerInfo.maxAnisotropy = properties.limits.maxSamplerAnisotropy;
        samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
        samplerInfo.unnormalizedCoordinates = VK_FALSE;
        samplerInfo.compareEnable = VK_FALSE;
        samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;
        samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;

        if (vkCreateSampler(logicalDevice_, &samplerInfo, nullptr, &textureSampler_) != VK_SUCCESS) {
            throw std::runtime_error("failed to create texture sampler!");
        }
    }
    
    void createTextureImage() {
        int texWidth, texHeight, texChannels;
        stbi_uc* pixels = stbi_load("textures/texture.jpg", &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);
        VkDeviceSize imageSize = texWidth * texHeight * 4;

        if (!pixels) {
            throw std::runtime_error("failed to load texture image!");
        }

        VkBuffer stagingBuffer;
        VkDeviceMemory stagingBufferMemory;
        createBuffer(imageSize
                     ,VK_BUFFER_USAGE_TRANSFER_SRC_BIT
                     ,VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
                     ,stagingBuffer
                     ,stagingBufferMemory);

        void* data;
        vkMapMemory(logicalDevice_, stagingBufferMemory, 0, imageSize, 0, &data);
        memcpy(data, pixels, static_cast<size_t>(imageSize));
        vkUnmapMemory(logicalDevice_, stagingBufferMemory);

        stbi_image_free(pixels);

        createImage(texWidth, texHeight, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, textureImage_, textureImageMemory_);

        transitionImageLayout(textureImage_, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
        copyBufferToImage(stagingBuffer, textureImage_, static_cast<uint32_t>(texWidth), static_cast<uint32_t>(texHeight));
        transitionImageLayout(textureImage_, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

        vkDestroyBuffer(logicalDevice_, stagingBuffer, nullptr);
        vkFreeMemory(logicalDevice_, stagingBufferMemory, nullptr);
    }

    void createImage(uint32_t width, uint32_t height, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags properties, VkImage& image, VkDeviceMemory& imageMemory) {
        VkImageCreateInfo imageInfo{};
        imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
        imageInfo.imageType = VK_IMAGE_TYPE_2D;
        imageInfo.extent.width = width;
        imageInfo.extent.height = height;
        imageInfo.extent.depth = 1;
        imageInfo.mipLevels = 1;
        imageInfo.arrayLayers = 1;
        imageInfo.format = format;
        imageInfo.tiling = tiling;
        imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        imageInfo.usage = usage;
        imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
        imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

        if (vkCreateImage(logicalDevice_, &imageInfo, nullptr, &image) != VK_SUCCESS) {
            throw std::runtime_error("failed to create image!");
        }

        VkMemoryRequirements memRequirements;
        vkGetImageMemoryRequirements(logicalDevice_, image, &memRequirements);

        VkMemoryAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        allocInfo.allocationSize = memRequirements.size;
        allocInfo.memoryTypeIndex = findMemoryType(memRequirements.memoryTypeBits, properties);

        if (vkAllocateMemory(logicalDevice_, &allocInfo, nullptr, &imageMemory) != VK_SUCCESS) {
            throw std::runtime_error("failed to allocate image memory!");
        }

        vkBindImageMemory(logicalDevice_, image, imageMemory, 0);
    }
    void createDescriptorSetLayout() {
        VkDescriptorSetLayoutBinding uboLayoutBinding{};
        uboLayoutBinding.binding = 0;
        uboLayoutBinding.descriptorCount = 1;
        uboLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        uboLayoutBinding.pImmutableSamplers = nullptr;
        uboLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

        VkDescriptorSetLayoutBinding samplerLayoutBinding{};
        samplerLayoutBinding.binding = 1;
        samplerLayoutBinding.descriptorCount = 1;
        samplerLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        samplerLayoutBinding.pImmutableSamplers = nullptr;
        samplerLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

        std::array<VkDescriptorSetLayoutBinding, 2> bindings = {uboLayoutBinding, samplerLayoutBinding};        

        VkDescriptorSetLayoutCreateInfo layoutInfo{};
        layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        layoutInfo.bindingCount = bindings.size();
        layoutInfo.pBindings = bindings.data();

        if (vkCreateDescriptorSetLayout(logicalDevice_, &layoutInfo, nullptr, &descriptorSetLayout_) != VK_SUCCESS) {
            throw std::runtime_error("failed to create descriptor set layout!");
        }
    }

    void createDescriptorPool() {
        std::array<VkDescriptorPoolSize, 2> poolSizes{};
        poolSizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        poolSizes[0].descriptorCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);
        poolSizes[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        poolSizes[1].descriptorCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);

        VkDescriptorPoolCreateInfo poolInfo{};
        poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
        poolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
        poolInfo.pPoolSizes = poolSizes.data();
        poolInfo.maxSets = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);

        if (vkCreateDescriptorPool(logicalDevice_, &poolInfo, nullptr, &descriptorPool_) != VK_SUCCESS) {
            throw std::runtime_error("failed to create descriptor pool!");
        }
    }

    void createDescriptorSets() {
        std::vector<VkDescriptorSetLayout> layouts(MAX_FRAMES_IN_FLIGHT, descriptorSetLayout_);
        VkDescriptorSetAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        allocInfo.descriptorPool = descriptorPool_;
        allocInfo.descriptorSetCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);
        allocInfo.pSetLayouts = layouts.data();

        descriptorSets_.resize(MAX_FRAMES_IN_FLIGHT);
        if (vkAllocateDescriptorSets(logicalDevice_, &allocInfo, descriptorSets_.data()) != VK_SUCCESS) {
            throw std::runtime_error("failed to allocate descriptor sets!");
        }

        for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
            VkDescriptorBufferInfo bufferInfo{};
            bufferInfo.buffer = uniformBuffers_[i];
            bufferInfo.offset = 0;
            bufferInfo.range = sizeof(UniformBufferObject);

            VkDescriptorImageInfo imageInfo{};
            imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            imageInfo.imageView = textureImageView_;
            imageInfo.sampler = textureSampler_;

            std::array<VkWriteDescriptorSet, 2> descriptorWrites{};

            descriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            descriptorWrites[0].dstSet = descriptorSets_[i];
            descriptorWrites[0].dstBinding = 0;
            descriptorWrites[0].dstArrayElement = 0;
            descriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
            descriptorWrites[0].descriptorCount = 1;
            descriptorWrites[0].pBufferInfo = &bufferInfo;

            descriptorWrites[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            descriptorWrites[1].dstSet = descriptorSets_[i];
            descriptorWrites[1].dstBinding = 1;
            descriptorWrites[1].dstArrayElement = 0;
            descriptorWrites[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
            descriptorWrites[1].descriptorCount = 1;
            descriptorWrites[1].pImageInfo = &imageInfo;

            vkUpdateDescriptorSets(logicalDevice_, static_cast<uint32_t>(descriptorWrites.size()), descriptorWrites.data(), 0, nullptr);
        }
    }
    
    void createUniformBuffers() {
        VkDeviceSize bufferSize = sizeof(UniformBufferObject);

        uniformBuffers_.resize(MAX_FRAMES_IN_FLIGHT);
        uniformBuffersMemory_.resize(MAX_FRAMES_IN_FLIGHT);
        uniformBuffersMapped_.resize(MAX_FRAMES_IN_FLIGHT);

        for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
            createBuffer(bufferSize
                         ,VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT
                         ,VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
                         ,uniformBuffers_[i]
                         ,uniformBuffersMemory_[i]);
            
            vkMapMemory(logicalDevice_, uniformBuffersMemory_[i], 0, bufferSize, 0, &uniformBuffersMapped_[i]);
        }
    }
    void updateUniformBuffer(uint32_t currentImage) {
        static auto startTime = std::chrono::high_resolution_clock::now();

        auto currentTime = std::chrono::high_resolution_clock::now();
        float time = std::chrono::duration<float, std::chrono::seconds::period>(currentTime - startTime).count();

        UniformBufferObject ubo{};
        ubo.model = glm::rotate(glm::mat4(1.0f), time * glm::radians(90.0f), glm::vec3(0.0f, 0.0f, 1.0f));
        ubo.view = glm::lookAt(glm::vec3(2.0f, 2.0f, 2.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f));
        ubo.projection = glm::perspective(glm::radians(45.0f), swapchainExtent_.width / (float) swapchainExtent_.height, 0.1f, 10.0f);
        ubo.projection[1][1] *= -1; //GLM was originally for openGL where the Y coordinate was inversed for clip

        //TODO: not the most efficient way, consider using push constants
        memcpy(uniformBuffersMapped_[currentImage], &ubo, sizeof(ubo));
    }
    
    void createBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer& buffer, VkDeviceMemory& bufferMemory)
    {
        VkBufferCreateInfo bufferInfo{};
        bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        bufferInfo.size = size;
        bufferInfo.usage = usage;
        bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

        if (vkCreateBuffer(logicalDevice_, &bufferInfo, nullptr, &buffer) != VK_SUCCESS) {
            throw std::runtime_error("failed to create vertex buffer!");
        }
        
        VkMemoryRequirements memRequirements;
        vkGetBufferMemoryRequirements(logicalDevice_, buffer, &memRequirements);

        VkMemoryAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        allocInfo.allocationSize = memRequirements.size;
        allocInfo.memoryTypeIndex = findMemoryType(memRequirements.memoryTypeBits, properties);

        //TODO: don't callf or every individual buffer. see comments at the end of this chapter: https://vulkan-tutorial.com/Vertex_buffers/Staging_buffer
        //consider using https://github.com/GPUOpen-LibrariesAndSDKs/VulkanMemoryAllocator
        if (vkAllocateMemory(logicalDevice_, &allocInfo, nullptr, &bufferMemory) != VK_SUCCESS) {
            throw std::runtime_error("failed to allocate vertex buffer memory!");
        }

        vkBindBufferMemory(logicalDevice_, buffer, bufferMemory, 0);

    }

    void createVertexBuffer()
    {

        VkDeviceSize bufferSize = sizeof(vertices[0]) * vertices.size();

        VkBuffer stagingBuffer;
        VkDeviceMemory stagingBufferMemory;

        //setup the staging buffer 
        createBuffer(bufferSize
                     ,VK_BUFFER_USAGE_TRANSFER_SRC_BIT
                     ,VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
                     ,stagingBuffer
                     ,stagingBufferMemory);

        //put it in CPU accessible memory
        void* data;
        vkMapMemory(logicalDevice_, stagingBufferMemory, 0, bufferSize, 0, &data);
        memcpy(data, vertices.data(), (size_t) bufferSize);
        vkUnmapMemory(logicalDevice_, stagingBufferMemory);

        //now create another buffer, this time for the GPU only (not cpu accessible)
        createBuffer(bufferSize
                     ,VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT
                     ,VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT
                     ,vertexBuffer_
                     ,vertexBufferMemory_);

        //copy the data from staging to vertex and cleanup the staging buffer (vertex cleaned up later)
        copyBuffer(stagingBuffer, vertexBuffer_, bufferSize);
        
        vkDestroyBuffer(logicalDevice_, stagingBuffer, nullptr);
        vkFreeMemory(logicalDevice_, stagingBufferMemory, nullptr);
    }

    void createIndexBuffer()
    {

        //TODO refactor this and vertex buffer to something more common, they're basically identical with the exception of the notes listed below
        //EDIT: Alternative, use a single VkBuffer to make things more efficient. See here: https://developer.nvidia.com/vulkan-memory-management
        //Also see here at the bottom: https://vulkan-tutorial.com/en/Vertex_buffers/Index_buffer
        VkDeviceSize bufferSize = sizeof(indices[0]) * indices.size();//NOTE: Different than vertex buffer, need to pass in the indices/vertices

        VkBuffer stagingBuffer;
        VkDeviceMemory stagingBufferMemory;

        createBuffer(bufferSize
                     ,VK_BUFFER_USAGE_TRANSFER_SRC_BIT
                     ,VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
                     ,stagingBuffer
                     ,stagingBufferMemory);

        void* data;
        vkMapMemory(logicalDevice_, stagingBufferMemory, 0, bufferSize, 0, &data);
        memcpy(data, indices.data(), (size_t) bufferSize);
        vkUnmapMemory(logicalDevice_, stagingBufferMemory);

        createBuffer(bufferSize
                     ,VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT //NOTE: Different than vertex buffer (for refactoring), would need to pass in this bit
                     ,VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT
                     ,indexBuffer_ //NOTE: Different buffer
                     ,indexBufferMemory_); //NOTE: Different memory

        
        copyBuffer(stagingBuffer, indexBuffer_, bufferSize);//NOTE: Different buffer

        vkDestroyBuffer(logicalDevice_, stagingBuffer, nullptr);
        vkFreeMemory(logicalDevice_, stagingBufferMemory, nullptr);
    }

    void copyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size) {
        //TODO: Look into using another command pool for short lived command buffers like this
        VkCommandBuffer commandBuffer = beginSingleTimeCommands();

        VkBufferCopy copyRegion{};
        copyRegion.size = size;
        vkCmdCopyBuffer(commandBuffer, srcBuffer, dstBuffer, 1, &copyRegion);

        endSingleTimeCommands(commandBuffer);
        //NOTE: could use a fence here to wait if multiple transfers are done simultaneously. We're only doing 1 right now so we'll just execute immediately
    }

    void transitionImageLayout(VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout) {
        VkCommandBuffer commandBuffer = beginSingleTimeCommands();

        VkImageMemoryBarrier barrier{};
        barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        barrier.oldLayout = oldLayout;
        barrier.newLayout = newLayout;
        barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.image = image;
        barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        barrier.subresourceRange.baseMipLevel = 0;
        barrier.subresourceRange.levelCount = 1;
        barrier.subresourceRange.baseArrayLayer = 0;
        barrier.subresourceRange.layerCount = 1;

        VkPipelineStageFlags sourceStage;
        VkPipelineStageFlags destinationStage;

        if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) {
            barrier.srcAccessMask = 0;
            barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

            sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
            destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
        } else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
            barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
            barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

            sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
            destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
        } else {
            throw std::invalid_argument("unsupported layout transition!");
        }

        vkCmdPipelineBarrier(
                             commandBuffer,
                             sourceStage, destinationStage,
                             0,
                             0, nullptr,
                             0, nullptr,
                             1, &barrier
                             );

        endSingleTimeCommands(commandBuffer);
    }

    
    void copyBufferToImage(VkBuffer buffer, VkImage image, uint32_t width, uint32_t height) {
        VkCommandBuffer commandBuffer = beginSingleTimeCommands();

        VkBufferImageCopy region{};
        region.bufferOffset = 0;
        region.bufferRowLength = 0;
        region.bufferImageHeight = 0;
        region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        region.imageSubresource.mipLevel = 0;
        region.imageSubresource.baseArrayLayer = 0;
        region.imageSubresource.layerCount = 1;
        region.imageOffset = {0, 0, 0};
        region.imageExtent = {
            width,
            height,
            1
        };

        vkCmdCopyBufferToImage(commandBuffer, buffer, image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);

        endSingleTimeCommands(commandBuffer);
    }
    
    //Note only checks for the memory TYPE not the HEAP it comes from which can make
    //a big difference. For example, one heap might be swap space and another from RAM.
    uint32_t findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties) {
        VkPhysicalDeviceMemoryProperties memProperties;
        vkGetPhysicalDeviceMemoryProperties(physicalDevice_, &memProperties);

        for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++) {
            if ((typeFilter & (1 << i)) && (memProperties.memoryTypes[i].propertyFlags & properties) == properties) {
                return i;
            }
        }

        throw std::runtime_error("failed to find suitable memory type!");
    }
    void mainLoop(std::shared_ptr<GLFWwindow> window)
    {
        while (!glfwWindowShouldClose(window.get())) {
            glfwPollEvents();
            //glfwSetWindowSize(window.get(), WIDTH, HEIGHT);
            drawFrame();
        }
        vkDeviceWaitIdle(logicalDevice_);
    }

    void recreateSwapChain() {
        int width = 0, height = 0;
        glfwGetFramebufferSize(glfwWindow_.get(), &width, &height);
        while (width == 0 || height == 0) {
            glfwGetFramebufferSize(glfwWindow_.get(), &width, &height);
            glfwWaitEvents();
        }
        
        vkDeviceWaitIdle(logicalDevice_);

        delete pMySwapchain_;
        pMySwapchain_ = new swapchain(logicalDevice_, physicalDevice_, surface_, width, height);
        swapchainExtent_ = pMySwapchain_->getSwapChainExtent();
        createDepthResources();        
        pMySwapchain_->createFrameBuffers(renderPass_, depthImageView_);

        swapchainFramebuffers_ = pMySwapchain_->getSwapChainFramebuffers();
        swapchain_ = pMySwapchain_->getSwapchain();

    }

    void createSyncObjects() {
        imageAvailableSemaphores_.resize(MAX_FRAMES_IN_FLIGHT);
        renderFinishedSemaphores_.resize(MAX_FRAMES_IN_FLIGHT);
        inFlightFences_.resize(MAX_FRAMES_IN_FLIGHT);
    
        VkSemaphoreCreateInfo semaphoreInfo{};
        semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
        
        VkFenceCreateInfo fenceInfo{};
        fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
        fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;//first call to vkWaitForFences will immediately return

        for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
            if (vkCreateSemaphore(logicalDevice_, &semaphoreInfo, nullptr, &imageAvailableSemaphores_[i]) != VK_SUCCESS ||
                vkCreateSemaphore(logicalDevice_, &semaphoreInfo, nullptr, &renderFinishedSemaphores_[i]) != VK_SUCCESS ||
                vkCreateFence(logicalDevice_, &fenceInfo, nullptr, &inFlightFences_[i]) != VK_SUCCESS) {
                throw std::runtime_error("failed to create semaphores!");
            }
        }
    }

    void drawFrame()
    {
        vkWaitForFences(logicalDevice_, 1, &inFlightFences_[currentFrame_], VK_TRUE, UINT64_MAX);

        uint32_t imageIndex;
        VkResult result = vkAcquireNextImageKHR(logicalDevice_, swapchain_, UINT64_MAX, imageAvailableSemaphores_[currentFrame_], VK_NULL_HANDLE, &imageIndex);

        if (result == VK_ERROR_OUT_OF_DATE_KHR) {
            recreateSwapChain();
            return;
        } else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
            throw std::runtime_error("failed to acquire swap chain image!");
        }
        
        updateUniformBuffer(currentFrame_);
        vkResetFences(logicalDevice_, 1, &inFlightFences_[currentFrame_]);
        
        vkResetCommandBuffer(commandBuffers_[currentFrame_], /*VkCommandBufferResetFlagBits*/ 0);
        recordCommandBuffer(commandBuffers_[currentFrame_]
                            ,imageIndex
                            ,renderPass_
                            ,swapchainFramebuffers_);

        VkSubmitInfo submitInfo{};
        submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

        VkSemaphore waitSemaphores[] = {imageAvailableSemaphores_[currentFrame_]};
        VkPipelineStageFlags waitStages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
        submitInfo.waitSemaphoreCount = 1;
        submitInfo.pWaitSemaphores = waitSemaphores;
        submitInfo.pWaitDstStageMask = waitStages;

        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers = &commandBuffers_[currentFrame_];

        VkSemaphore signalSemaphores[] = {renderFinishedSemaphores_[currentFrame_]};
        submitInfo.signalSemaphoreCount = 1;
        submitInfo.pSignalSemaphores = signalSemaphores;

        if (vkQueueSubmit(graphicsQueue_, 1, &submitInfo, inFlightFences_[currentFrame_]) != VK_SUCCESS) {
            throw std::runtime_error("failed to submit draw command buffer!");
        }

        VkPresentInfoKHR presentInfo{};
        presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;

        presentInfo.waitSemaphoreCount = 1;
        presentInfo.pWaitSemaphores = signalSemaphores;

        VkSwapchainKHR swapChains[] = {swapchain_};
        presentInfo.swapchainCount = 1;
        presentInfo.pSwapchains = swapChains;

        presentInfo.pImageIndices = &imageIndex;

        result = vkQueuePresentKHR(presentQueue_, &presentInfo);
        if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || framebufferResized_) {
            framebufferResized_ = false;
            recreateSwapChain();
        } else if (result != VK_SUCCESS) {
            throw std::runtime_error("failed to present swap chain image!");
        }

        currentFrame_ = (currentFrame_ + 1) % MAX_FRAMES_IN_FLIGHT;
    }
    void createCommandBuffers()
    {
        std::cout << "creating command buffer" << std::endl;
        commandBuffers_.resize(MAX_FRAMES_IN_FLIGHT);
        
        VkCommandBufferAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        allocInfo.commandPool = commandPool_;
        allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        allocInfo.commandBufferCount = (uint32_t) commandBuffers_.size();

        if (vkAllocateCommandBuffers(logicalDevice_, &allocInfo, commandBuffers_.data()) != VK_SUCCESS) {
            throw std::runtime_error("failed to allocate command buffers!");
        }
    }
    
    VkCommandPool createCommandPool(VkSurfaceKHR surface)
    {
        std::cout << "creating command pool" << std::endl;
        queue::QueueFamilyIndices queueFamilyIndices = queue::findQueueFamilies(physicalDevice_, surface);
 
        VkCommandPoolCreateInfo poolInfo{};
        poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
        poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
        poolInfo.queueFamilyIndex = queueFamilyIndices.graphicsFamily.value();
        VkCommandPool commandPool;
        if (vkCreateCommandPool(logicalDevice_, &poolInfo, nullptr, &commandPool) != VK_SUCCESS) {
            throw std::runtime_error("failed to create command pool!");
        }

        return commandPool;
    }


    void cleanup(VkSurfaceKHR surface, VkInstance instance)
    {

        delete pMySwapchain_;
        vkDestroyImageView(logicalDevice_, depthImageView_, nullptr);
        vkDestroyImage(logicalDevice_, depthImage_, nullptr);
        vkFreeMemory(logicalDevice_, depthImageMemory_, nullptr);
        
        vkDestroySampler(logicalDevice_, textureSampler_, nullptr);
        vkDestroyImageView(logicalDevice_, textureImageView_, nullptr);
        vkDestroyImage(logicalDevice_, textureImage_, nullptr);
        vkFreeMemory(logicalDevice_, textureImageMemory_, nullptr);
        
        for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
            vkDestroyBuffer(logicalDevice_, uniformBuffers_[i], nullptr);
            vkFreeMemory(logicalDevice_, uniformBuffersMemory_[i], nullptr);
        }

        
        vkDestroyBuffer(logicalDevice_, indexBuffer_, nullptr);
        vkFreeMemory(logicalDevice_, indexBufferMemory_, nullptr);
        
        vkDestroyBuffer(logicalDevice_, vertexBuffer_, nullptr);
        vkFreeMemory(logicalDevice_, vertexBufferMemory_, nullptr);
        
        delete pMyGraphicsPipeline_;
        std::cout << "destroying command pool" << std::endl;
        vkDestroyCommandPool(logicalDevice_, commandPool_, nullptr);
        for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
            vkDestroySemaphore(logicalDevice_, imageAvailableSemaphores_[i], nullptr);
            vkDestroySemaphore(logicalDevice_, renderFinishedSemaphores_[i], nullptr);
            vkDestroyFence(logicalDevice_, inFlightFences_[i], nullptr);
        }
        
        vkDestroyDescriptorPool(logicalDevice_, descriptorPool_, nullptr);
        vkDestroyDescriptorSetLayout(logicalDevice_, descriptorSetLayout_, nullptr);
        
        delete pMyDevice_;
        vkDestroySurfaceKHR(instance, surface, nullptr);
        glfwTerminate();

    }

    std::shared_ptr<GLFWwindow> initWindow()
    {
        glfwInit();
        
        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
        glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE); //disable resizing for now, takes more complexity to handle it
        std::shared_ptr<GLFWwindow> window(glfwCreateWindow(WIDTH, HEIGHT, "Vulkan", nullptr, nullptr), &glfwDestroyWindow);
        
        glfwSetWindowUserPointer(window.get(), this);
        glfwSetFramebufferSizeCallback(window.get(), framebufferResizeCallback);
        
        return window;
    }

    static void framebufferResizeCallback(GLFWwindow* window, int width, int height) {
        auto app = reinterpret_cast<HelloTriangleApplication*>(glfwGetWindowUserPointer(window));
        app->framebufferResized_ = true;
        std::cout << "resized window" << std::endl;
    }

    void printAvailableExtensions()
    {
        uint32_t extensionCount = 0;
        vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, nullptr);
        std::vector<VkExtensionProperties> extensions(extensionCount);
        vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, extensions.data());
        std::cout << "available extensions:\n";

        for (const auto& extension : extensions) {
            std::cout << '\t' << extension.extensionName << '\n';
        }
    }

    void recordCommandBuffer(VkCommandBuffer commandBuffer
                             ,uint32_t imageIndex
                             ,VkRenderPass renderPass
                             ,std::vector<VkFramebuffer> swapChainFramebuffers) {
        VkCommandBufferBeginInfo beginInfo{};
        beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        beginInfo.flags = 0; // Optional
        beginInfo.pInheritanceInfo = nullptr; // Optional

        if (vkBeginCommandBuffer(commandBuffer, &beginInfo) != VK_SUCCESS) {
            throw std::runtime_error("failed to begin recording command buffer!");
        }

        
        VkRenderPassBeginInfo renderPassInfo{};
        renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        renderPassInfo.renderPass = renderPass;
        renderPassInfo.framebuffer = swapChainFramebuffers[imageIndex];
        renderPassInfo.renderArea.offset = {0, 0};
        renderPassInfo.renderArea.extent = swapchainExtent_; //TODO get swapchain extent

        std::array<VkClearValue, 2> clearValues{};
        clearValues[0].color = {{0.0f, 0.0f, 0.0f, 1.0f}};
        clearValues[1].depthStencil = {1.0f, 0};

        renderPassInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
        renderPassInfo.pClearValues = clearValues.data();

        vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
        vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, graphicsPipeline_);//TODO get graphics pipeline

        VkViewport viewport{};
        viewport.x = 0.0f;
        viewport.y = 0.0f;
        viewport.width = static_cast<float>(swapchainExtent_.width);
        viewport.height = static_cast<float>(swapchainExtent_.height);
        viewport.minDepth = 0.0f;
        viewport.maxDepth = 1.0f;
        vkCmdSetViewport(commandBuffer, 0, 1, &viewport);

        VkRect2D scissor{};
        scissor.offset = {0, 0};
        scissor.extent = swapchainExtent_;
        vkCmdSetScissor(commandBuffer, 0, 1, &scissor);

        VkBuffer vertexBuffers[] = {vertexBuffer_};
        VkDeviceSize offsets[] = {0};
        vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers, offsets);
        
        vkCmdBindIndexBuffer(commandBuffer, indexBuffer_, 0, VK_INDEX_TYPE_UINT16);
        
        vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout_, 0, 1, &descriptorSets_[currentFrame_], 0, nullptr);
        
        vkCmdDrawIndexed(commandBuffer, static_cast<uint32_t>(indices.size()), 1, 0, 0, 0);
        vkCmdEndRenderPass(commandBuffer);
        if (vkEndCommandBuffer(commandBuffer) != VK_SUCCESS) {
            throw std::runtime_error("failed to record command buffer!");
        }
    }

};

int main() {
    HelloTriangleApplication app;

    try {
        app.run();
    } catch (const std::exception& e) {
        std::cerr << e.what() << std::endl;
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
