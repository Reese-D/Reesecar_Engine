//this will include vulkan/vulkan.h through GLFW
#include <memory>

#include <vulkan/vulkan_core.h>
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

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

        physicalDevice_ = pMyDevice_->getPhysicalDevice();
        logicalDevice_ = pMyDevice_->getLogicalDevice();
        indices_ = queue::findQueueFamilies(physicalDevice_, surface_);
        graphicsQueue_ = pMyDevice_->getGraphicsDeviceQueue(logicalDevice_, indices_);
        presentQueue_ = pMyDevice_->getPresentDeviceQueue(logicalDevice_, indices_);
        
        pMySwapchain_ = new swapchain(logicalDevice_, physicalDevice_, surface_, WIDTH, HEIGHT);

        //note: image format is just an enum, don't need to recreate this if swapchain dies/changes
        auto format = pMySwapchain_->getImageFormat();

        pMyGraphicsPipeline_ = new graphics_pipeline(logicalDevice_, format);
        renderPass_ = pMyGraphicsPipeline_->getRenderPass();
        pMySwapchain_->createFrameBuffers(renderPass_);
        commandPool_ = createCommandPool(surface_);

        graphicsPipeline_ = pMyGraphicsPipeline_->getGraphicsPipeline();
        swapchainExtent_ = pMySwapchain_->getSwapChainExtent();
        swapchainFramebuffers_ = pMySwapchain_->getSwapChainFramebuffers();
        swapchain_ = pMySwapchain_->getSwapchain();
        
        createVertexBuffer();
                    
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
    VkBuffer vertexBuffer_;
    VkDeviceMemory vertexBufferMemory_;
    std::vector<VkFramebuffer> swapchainFramebuffers_;
    
    std::shared_ptr<GLFWwindow> glfwWindow_;
    
    queue::QueueFamilyIndices indices_;
    
    swapchain* pMySwapchain_;
    graphics_pipeline* pMyGraphicsPipeline_;
    device* pMyDevice_;

    //temporary constant to test shader
    const std::vector<graphics_pipeline::Vertex> vertices = {
        {{0.0f, -0.5f}, {1.0f, 1.0f, 1.0f}},
        {{0.5f, 0.5f}, {0.0f, 1.0f, 0.0f}},
        {{-0.5f, 0.5f}, {0.0f, 0.0f, 1.0f}}
    };

    void createVertexBuffer() {
        VkBufferCreateInfo bufferInfo{};
        bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        bufferInfo.size = sizeof(vertices[0]) * vertices.size();
        bufferInfo.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
        bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

        if (vkCreateBuffer(logicalDevice_, &bufferInfo, nullptr, &vertexBuffer_) != VK_SUCCESS) {
            throw std::runtime_error("failed to create vertex buffer!");
        }
        
        VkMemoryRequirements memRequirements;
        vkGetBufferMemoryRequirements(logicalDevice_, vertexBuffer_, &memRequirements);

        VkMemoryAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        allocInfo.allocationSize = memRequirements.size;
        allocInfo.memoryTypeIndex = findMemoryType(memRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

        if (vkAllocateMemory(logicalDevice_, &allocInfo, nullptr, &vertexBufferMemory_) != VK_SUCCESS) {
            throw std::runtime_error("failed to allocate vertex buffer memory!");
        }

        vkBindBufferMemory(logicalDevice_, vertexBuffer_, vertexBufferMemory_, 0);

        void* data;
        vkMapMemory(logicalDevice_, vertexBufferMemory_, 0, bufferInfo.size, 0, &data);
        memcpy(data, vertices.data(), (size_t) bufferInfo.size);
        vkUnmapMemory(logicalDevice_, vertexBufferMemory_);
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
        pMySwapchain_->createFrameBuffers(renderPass_);
        swapchainExtent_ = pMySwapchain_->getSwapChainExtent();
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

        VkClearValue clearColor = {{{0.0f, 0.0f, 0.0f, 1.0f}}};
        renderPassInfo.clearValueCount = 1;
        renderPassInfo.pClearValues = &clearColor;

        vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
        vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, graphicsPipeline_);//TODO get graphics pipeline

        VkBuffer vertexBuffers[] = {vertexBuffer_};
        VkDeviceSize offsets[] = {0};
        vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers, offsets);
        
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

        vkCmdDraw(commandBuffer, static_cast<uint32_t>(vertices.size()), 1, 0, 0);

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
