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
    physicalDeviceFeatures_.samplerAnisotropy = VK_TRUE;
    physicalDeviceProperties_ = getDeviceProperties(physicalDevice_);
    indices_ = queue::findQueueFamilies(physicalDevice_, surface);

    logicalDevice_ = getLogicalDevice(physicalDevice_, physicalDeviceFeatures_, indices_, deviceExtensions_);
}

VkPhysicalDevice device::getPhysicalDevice()
{
    return physicalDevice_;
}

VkDevice device::getLogicalDevice()
{
    return logicalDevice_;
}

device::~device()
{
    std::cout << "device destructor was called" << std::endl;
    std::cout << "destroying logical device" << std::endl;
    vkDestroyDevice(logicalDevice_, nullptr);
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

    VkPhysicalDeviceFeatures supportedFeatures;
    vkGetPhysicalDeviceFeatures(physicalDevice, &supportedFeatures);
    
    return deviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU &&
        deviceFeatures.geometryShader &&
        indices.isComplete() &&
        swapChainAdequate &&
        supportedFeatures.samplerAnisotropy;
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







