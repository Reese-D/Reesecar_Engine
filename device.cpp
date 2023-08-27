#include "device.h"

#include <vector>
#include <stdexcept>
#include <iostream>
#include <vulkan/vulkan_core.h>

device::device(std::shared_ptr<VkInstance> instance, VkSurfaceKHR surface)
{
    std::cout << "device constructor was called" << std::endl;
    physicalDevice_ = getPhysicalDevice(instance, surface);

    //we end up doing these twice (see getPhysicalDevice)
    //they're fast operations however, so we'll take the speed hit for cleaniness
    physicalDeviceFeatures_ = getDeviceFeatures(physicalDevice_);
    physicalDeviceProperties_ = getDeviceProperties(physicalDevice_);
    indices_ = queue::findQueueFamilies(physicalDevice_);

    logicalDevice_ = getLogicalDevice(physicalDevice_, physicalDeviceFeatures_, indices_);
    
    graphicsQueue_ = getDeviceQueue(logicalDevice_, indices_);
}

device::~device()
{
    std::cout << "device destructor was called" << std::endl;
    vkDestroyDevice(logicalDevice_, nullptr);
}

VkQueue device::getDeviceQueue(VkDevice logicalDevice
                               , queue::QueueFamilyIndices indices)
{
    VkQueue graphicsQueue;
    vkGetDeviceQueue(logicalDevice, indices.value(), 0, &graphicsQueue);
    return graphicsQueue;
}

bool device::hasSupportForSurface(VkPhysicalDevice device, VkSurfaceKHR surface, queue::QueueFamilyIndices indices)
{
    VkBool32 presentSupport = false;
    vkGetPhysicalDeviceSurfaceSupportKHR(device, indices.value(), surface, &presentSupport);
    return presentSupport;
}


VkPhysicalDevice device::getPhysicalDevice(std::shared_ptr<VkInstance> instance, VkSurfaceKHR surface)
{
    uint32_t deviceCount = 0;
    vkEnumeratePhysicalDevices(*instance, &deviceCount, nullptr);
    if (deviceCount == 0) {
        throw std::runtime_error("failed to find GPUs with Vulkan support!");
    }
    std::vector<VkPhysicalDevice> devices(deviceCount);
    vkEnumeratePhysicalDevices(*instance, &deviceCount, devices.data());
    
    VkPhysicalDevice physicalDevice;
    for (auto& device : devices) {
        auto physicalDeviceFeatures = getDeviceFeatures(device);
        auto physicalDeviceProperties = getDeviceProperties(device);
        auto indices = queue::findQueueFamilies(device);
        bool supportForSurface = hasSupportForSurface(device, surface, indices);
        bool deviceSuitable = isDeviceSuitable(device, physicalDeviceProperties, physicalDeviceFeatures, indices);
        if (auto result = supportForSurface && deviceSuitable) {
            std::cout << "here" << std::endl;
            physicalDevice = device;
            break;
        }
    }

    if (physicalDevice == VK_NULL_HANDLE) {
        throw std::runtime_error("failed to find a suitable GPU!");
    }

    return physicalDevice;
}

bool device::isDeviceSuitable(VkPhysicalDevice physicalDevice
                              ,VkPhysicalDeviceProperties deviceProperties
                              ,VkPhysicalDeviceFeatures deviceFeatures
                              ,queue::QueueFamilyIndices indices)
{
    std::cout << "found a graphics device...: " << deviceProperties.deviceName << std::endl;

    return deviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU &&
        deviceFeatures.geometryShader && indices.has_value();
}

VkDevice device::getLogicalDevice(VkPhysicalDevice physicalDevice
                                  ,VkPhysicalDeviceFeatures deviceFeatures
                                  ,std::optional<uint32_t> indices)
{
    VkDeviceQueueCreateInfo queueCreateInfo{};
    queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    queueCreateInfo.queueFamilyIndex = indices.value();
    queueCreateInfo.queueCount = 1;

    float queuePriority = 1.0f;
    queueCreateInfo.pQueuePriorities = &queuePriority;

    VkDeviceCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    createInfo.pQueueCreateInfos = &queueCreateInfo;
    createInfo.queueCreateInfoCount = 1;
    createInfo.pEnabledFeatures = &deviceFeatures;
    createInfo.enabledExtensionCount = 0; //newer versions of vulkan don't need to support these

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







