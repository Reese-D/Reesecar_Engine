#include "device.hpp"

#include <vector>
#include <stdexcept>
#include <iostream>
#include <vulkan/vulkan_core.h>
#include <set>
device::device(std::shared_ptr<VkInstance> instance, VkSurfaceKHR surface, int width, int height)
{
    std::cout << "device constructor was called" << std::endl;
    physicalDevice_ = getPhysicalDevice(instance, surface, deviceExtensions_);

    //we end up doing these twice (see getPhysicalDevice)
    //they're fast operations however, so we'll take the speed hit for cleaniness
    physicalDeviceFeatures_ = getDeviceFeatures(physicalDevice_);
    physicalDeviceProperties_ = getDeviceProperties(physicalDevice_);
    indices_ = queue::findQueueFamilies(physicalDevice_, surface);

    logicalDevice_ = getLogicalDevice(physicalDevice_, physicalDeviceFeatures_, indices_, deviceExtensions_);

    graphicsQueue_ = getGraphicsDeviceQueue(logicalDevice_, indices_);
    presentQueue_ = getPresentDeviceQueue(logicalDevice_, indices_);

    swapchain_ = new swapchain(logicalDevice_, physicalDevice_, surface, width, height);
    auto format = swapchain_->getImageFormat();

    graphicsPipeline_ = new graphics_pipeline(logicalDevice_, format);
    VkRenderPass renderPassVk = graphicsPipeline_->getRenderPass();
    swapchain_->createFrameBuffers(renderPassVk);
    createCommandPool(surface);

    auto graphicsPipelineVk = graphicsPipeline_->getGraphicsPipeline();
    auto swapChainExtentVk = swapchain_->getSwapChainExtent();
    auto swapChainFrameBuffersVk = swapchain_->getSwapChainFramebuffers();
    // recordCommandBuffer(createCommandBuffer()
    //                     ,0
    //                     ,renderPassVk
    //                     ,swapChainExtentVk
    //                     ,graphicsPipelineVk
    //                     ,swapChainFrameBuffersVk);
}

device::~device()
{
    std::cout << "device destructor was called" << std::endl;
    delete swapchain_;
    delete graphicsPipeline_;
    std::cout << "destroying command pool" << std::endl;
    vkDestroyCommandPool(logicalDevice_, commandPool_, nullptr);
    std::cout << "destroying logical device" << std::endl;
    vkDestroyDevice(logicalDevice_, nullptr);
}

VkCommandBuffer device::createCommandBuffer()
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

void device::recordCommandBuffer(VkCommandBuffer commandBuffer
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

void device::createCommandPool(VkSurfaceKHR surface)
{
    std::cout << "creating command pool" << std::endl;
    queue::QueueFamilyIndices queueFamilyIndices = queue::findQueueFamilies(physicalDevice_, surface);
 
    VkCommandPoolCreateInfo poolInfo{};
    poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    poolInfo.queueFamilyIndex = queueFamilyIndices.graphicsFamily.value();

    if (vkCreateCommandPool(logicalDevice_, &poolInfo, nullptr, &commandPool_) != VK_SUCCESS) {
        throw std::runtime_error("failed to create command pool!");
    }
}


VkQueue device::getGraphicsDeviceQueue(VkDevice logicalDevice
                                       , queue::QueueFamilyIndices indices)
{
    VkQueue graphicsQueue;
    vkGetDeviceQueue(logicalDevice, indices.graphicsFamily.value(), 0, &graphicsQueue);
    return graphicsQueue;
}

VkQueue device::getPresentDeviceQueue(VkDevice logicalDevice
                                      , queue::QueueFamilyIndices indices)
{
    VkQueue presentQueue;
    vkGetDeviceQueue(logicalDevice, indices.presentFamily.value(), 0, &presentQueue);
    return presentQueue;
}

bool device::hasSupportForSurface(VkPhysicalDevice device, VkSurfaceKHR surface, queue::QueueFamilyIndices indices)
{
    VkBool32 presentSupport = false;
    vkGetPhysicalDeviceSurfaceSupportKHR(device, indices.presentFamily.value(), surface, &presentSupport);
    return presentSupport;
}


VkPhysicalDevice device::getPhysicalDevice(std::shared_ptr<VkInstance> instance, VkSurfaceKHR surface, const std::vector<const char*> deviceExtensions)
{
    uint32_t deviceCount = 0;
    vkEnumeratePhysicalDevices(*instance, &deviceCount, nullptr);
    if (deviceCount == 0) {
        throw std::runtime_error("failed to find GPUs with Vulkan support!");
    }
    std::vector<VkPhysicalDevice> devices(deviceCount);
    vkEnumeratePhysicalDevices(*instance, &deviceCount, devices.data());
    bool foundSuitableDevice = false;
    VkPhysicalDevice physicalDevice;
    for (auto& device : devices) {
        auto physicalDeviceFeatures = getDeviceFeatures(device);
        auto physicalDeviceProperties = getDeviceProperties(device);
        auto indices = queue::findQueueFamilies(device, surface);
        bool supportForSurface = hasSupportForSurface(device, surface, indices);
        bool supportsExtensions = doesDeviceSupportExtensions(device, deviceExtensions);
        bool deviceSuitable = isDeviceSuitable(device, physicalDeviceProperties, physicalDeviceFeatures, indices, surface);
        if (auto result = supportForSurface && deviceSuitable && supportsExtensions) {
            foundSuitableDevice = true;
            physicalDevice = device;
            break;
        }
    }

    if(!foundSuitableDevice){
        std::cout << "No suitable vulkan device was found... aborting" << std::endl;
    }
    if (physicalDevice == VK_NULL_HANDLE) {
        throw std::runtime_error("failed to find a suitable GPU!");
    }

    return physicalDevice;
}

bool device::doesDeviceSupportExtensions(VkPhysicalDevice device, const std::vector<const char*> deviceExtensions) {
    uint32_t extensionCount;
    vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, nullptr);

    std::vector<VkExtensionProperties> availableExtensions(extensionCount);
    vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, availableExtensions.data());

    std::set<std::string> requiredExtensions(deviceExtensions.begin(), deviceExtensions.end());

    for (const auto& extension : availableExtensions) {
        requiredExtensions.erase(extension.extensionName);
    }
    std::cout << "device supports all extensions?: " << requiredExtensions.empty() << std::endl;
    return requiredExtensions.empty();
}

bool device::isDeviceSuitable(VkPhysicalDevice physicalDevice
                              ,VkPhysicalDeviceProperties deviceProperties
                              ,VkPhysicalDeviceFeatures deviceFeatures
                              ,queue::QueueFamilyIndices indices
                              ,VkSurfaceKHR surface)
{
    bool swapChainAdequate = false;
    swapchain::SwapChainSupportDetails swapChainSupport = querySwapChainSupport(physicalDevice, surface);
    swapChainAdequate = !swapChainSupport.formats.empty() && !swapChainSupport.presentModes.empty();
    
    return deviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU &&
        deviceFeatures.geometryShader &&
        indices.isComplete() &&
        swapChainAdequate;
}

swapchain::SwapChainSupportDetails device::querySwapChainSupport(VkPhysicalDevice device, VkSurfaceKHR surface)
{
    swapchain::SwapChainSupportDetails details;
    
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, surface, &details.capabilities);
    
    uint32_t formatCount;
    vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, nullptr);

    if (formatCount != 0) {
        details.formats.resize(formatCount);
        vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, details.formats.data());
    }

    uint32_t presentModeCount;
    vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentModeCount, nullptr);

    if (presentModeCount != 0) {
        details.presentModes.resize(presentModeCount);
        vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentModeCount, details.presentModes.data());
    }
    return details;
}
VkDevice device::getLogicalDevice(VkPhysicalDevice physicalDevice
                                  ,VkPhysicalDeviceFeatures deviceFeatures
                                  ,queue::QueueFamilyIndices indices
                                  ,std::vector<const char*> deviceExtensions)
{

    std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
    std::set<uint32_t> uniqueQueueFamilies = {indices.graphicsFamily.value(), indices.presentFamily.value()};

    float queuePriority = 1.0f;
    for (uint32_t queueFamily : uniqueQueueFamilies) {
        VkDeviceQueueCreateInfo queueCreateInfo{};
        queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        queueCreateInfo.queueFamilyIndex = queueFamily;
        queueCreateInfo.queueCount = 1;
        queueCreateInfo.pQueuePriorities = &queuePriority;
        queueCreateInfos.push_back(queueCreateInfo);
    }
  
    VkDeviceCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    createInfo.pQueueCreateInfos = queueCreateInfos.data();
    createInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());;
    createInfo.pEnabledFeatures = &deviceFeatures;
    //newer versions of vulkan don't need to support these
    createInfo.enabledExtensionCount = static_cast<uint32_t>(deviceExtensions.size()); 
    createInfo.ppEnabledExtensionNames = deviceExtensions.data();
    
    VkDevice logicalDevice{};
    if (vkCreateDevice(physicalDevice, &createInfo, nullptr, &logicalDevice) != VK_SUCCESS) {
        throw std::runtime_error("failed to create logical device!");
    }

    return logicalDevice;
}

VkPhysicalDeviceProperties device::getDeviceProperties(VkPhysicalDevice physicalDevice)
{
    VkPhysicalDeviceProperties deviceProperties;
    vkGetPhysicalDeviceProperties(physicalDevice, &deviceProperties);
    return deviceProperties;
}

VkPhysicalDeviceFeatures device::getDeviceFeatures(VkPhysicalDevice physicalDevice)
{
    VkPhysicalDeviceFeatures deviceFeatures;
    vkGetPhysicalDeviceFeatures(physicalDevice, &deviceFeatures);
    return deviceFeatures;
}







