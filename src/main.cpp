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

const std::vector<const char*> validationLayers = {
    "VK_LAYER_KHRONOS_validation"
};

class HelloTriangleApplication {
public:
    void run()
    {
        std::shared_ptr<GLFWwindow> glfwWindow = initWindow();

        //printAvailableExtensions();
        auto myInstance = instance{validationLayers};
        std::shared_ptr<VkInstance> myVkInstance = myInstance.getInstance();
        auto surface = window::getSurface(*myVkInstance, glfwWindow);
        pMyDevice_ = new device(myVkInstance, surface, WIDTH, HEIGHT);

        physicalDevice_ = pMyDevice_->getPhysicalDevice();
        logicalDevice_ = pMyDevice_->getLogicalDevice();
        indices_ = queue::findQueueFamilies(physicalDevice_, surface);
        graphicsQueue_ = pMyDevice_->getGraphicsDeviceQueue(logicalDevice_, indices_);
        presentQueue_ = pMyDevice_->getPresentDeviceQueue(logicalDevice_, indices_);
        
        pMySwapchain_ = new swapchain(logicalDevice_, physicalDevice_, surface, WIDTH, HEIGHT);
        auto format = pMySwapchain_->getImageFormat();

        pMyGraphicsPipeline_ = new graphics_pipeline(logicalDevice_, format);
        renderPass_ = pMyGraphicsPipeline_->getRenderPass();
        pMySwapchain_->createFrameBuffers(renderPass_);
        commandPool_ = createCommandPool(surface);

        graphicsPipeline_ = pMyGraphicsPipeline_->getGraphicsPipeline();
        swapchainExtent_ = pMySwapchain_->getSwapChainExtent();
        swapchainFramebuffers_ = pMySwapchain_->getSwapChainFramebuffers();
        swapchain_ = pMySwapchain_->getSwapchain();
        
        commandBuffer_ = createCommandBuffer();
        createSyncObjects();
        
        mainLoop(glfwWindow);

        cleanup(surface, *myVkInstance);
    }

private:
    VkPhysicalDevice physicalDevice_;
    VkDevice logicalDevice_;
    VkQueue graphicsQueue_;
    VkQueue presentQueue_;
    VkCommandPool commandPool_;
    VkRenderPass renderPass_;
    VkSemaphore imageAvailableSemaphore_;
    VkSemaphore renderFinishedSemaphore_;
    VkFence inFlightFence_;
    VkPipeline graphicsPipeline_;
    VkExtent2D swapchainExtent_;
    VkCommandBuffer commandBuffer_;
    VkSwapchainKHR swapchain_;
    
    std::vector<VkFramebuffer> swapchainFramebuffers_;
    
    queue::QueueFamilyIndices indices_;
    
    swapchain* pMySwapchain_;
    graphics_pipeline* pMyGraphicsPipeline_;
    device* pMyDevice_;

    
    
    void mainLoop(std::shared_ptr<GLFWwindow> window)
    {
        while (!glfwWindowShouldClose(window.get())) {
            glfwPollEvents();
            drawFrame();
        }
        vkDeviceWaitIdle(logicalDevice_);
    }

    void createSyncObjects() {
        VkSemaphoreCreateInfo semaphoreInfo{};
        semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
        VkFenceCreateInfo fenceInfo{};
        fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
        fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;//first call to vkWaitForFences will immediately return
        
        if (vkCreateSemaphore(logicalDevice_, &semaphoreInfo, nullptr, &imageAvailableSemaphore_) != VK_SUCCESS ||
            vkCreateSemaphore(logicalDevice_, &semaphoreInfo, nullptr, &renderFinishedSemaphore_) != VK_SUCCESS ||
            vkCreateFence(logicalDevice_, &fenceInfo, nullptr, &inFlightFence_) != VK_SUCCESS) {
            throw std::runtime_error("failed to create semaphores!");
        }
    }

    void drawFrame()
    {
        vkWaitForFences(logicalDevice_, 1, &inFlightFence_, VK_TRUE, UINT64_MAX);
        vkResetFences(logicalDevice_, 1, &inFlightFence_);
        uint32_t imageIndex;
        vkAcquireNextImageKHR(logicalDevice_, swapchain_, UINT64_MAX, imageAvailableSemaphore_, VK_NULL_HANDLE, &imageIndex);

        vkResetCommandBuffer(commandBuffer_, /*VkCommandBufferResetFlagBits*/ 0);
        
        
        recordCommandBuffer(commandBuffer_
                            ,imageIndex
                            ,renderPass_
                            ,swapchainExtent_
                            ,graphicsPipeline_
                            ,swapchainFramebuffers_);

        VkSubmitInfo submitInfo{};
        submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

        VkSemaphore waitSemaphores[] = {imageAvailableSemaphore_};
        VkPipelineStageFlags waitStages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
        submitInfo.waitSemaphoreCount = 1;
        submitInfo.pWaitSemaphores = waitSemaphores;
        submitInfo.pWaitDstStageMask = waitStages;

        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers = &commandBuffer_;

        VkSemaphore signalSemaphores[] = {renderFinishedSemaphore_};
        submitInfo.signalSemaphoreCount = 1;
        submitInfo.pSignalSemaphores = signalSemaphores;

        if (vkQueueSubmit(graphicsQueue_, 1, &submitInfo, inFlightFence_) != VK_SUCCESS) {
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

        vkQueuePresentKHR(presentQueue_, &presentInfo);
    }
    VkCommandBuffer createCommandBuffer()
    {
        std::cout << "creating command buffer" << std::endl;
        VkCommandBuffer commandBuffer{};
        VkCommandBufferAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        allocInfo.commandPool = commandPool_;
        allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        allocInfo.commandBufferCount = 1;

        if (vkAllocateCommandBuffers(logicalDevice_, &allocInfo, &commandBuffer) != VK_SUCCESS) {
            throw std::runtime_error("failed to allocate command buffers!");
        }

        return commandBuffer;
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
        delete pMyGraphicsPipeline_;
        std::cout << "destroying command pool" << std::endl;
        vkDestroyCommandPool(logicalDevice_, commandPool_, nullptr);

        vkDestroySemaphore(logicalDevice_, imageAvailableSemaphore_, nullptr);
        vkDestroySemaphore(logicalDevice_, renderFinishedSemaphore_, nullptr);
        vkDestroyFence(logicalDevice_, inFlightFence_, nullptr);

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
        
        return window;
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
                             ,VkExtent2D swapChainExtent
                             ,VkPipeline graphicsPipeline
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
        renderPassInfo.renderArea.extent = swapChainExtent; //TODO get swapchain extent

        VkClearValue clearColor = {{{0.0f, 0.0f, 0.0f, 1.0f}}};
        renderPassInfo.clearValueCount = 1;
        renderPassInfo.pClearValues = &clearColor;

        vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
        vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, graphicsPipeline);//TODO get graphics pipeline

        VkViewport viewport{};
        viewport.x = 0.0f;
        viewport.y = 0.0f;
        viewport.width = static_cast<float>(swapChainExtent.width);
        viewport.height = static_cast<float>(swapChainExtent.height);
        viewport.minDepth = 0.0f;
        viewport.maxDepth = 1.0f;
        vkCmdSetViewport(commandBuffer, 0, 1, &viewport);

        VkRect2D scissor{};
        scissor.offset = {0, 0};
        scissor.extent = swapChainExtent;
        vkCmdSetScissor(commandBuffer, 0, 1, &scissor);

        vkCmdDraw(commandBuffer, 3, 1, 0, 0);

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
