#include "queue.hpp"
#include <iostream>

queue::QueueFamilyIndices queue::findQueueFamilies(VkPhysicalDevice device, VkQueueFlags desiredFlags)
{
    uint32_t queueFamilyCount = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);
    
    std::vector<VkQueueFamilyProperties> queueFamilies{queueFamilyCount};
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilies.data());
        
    queue::QueueFamilyIndices indices;
    int i = 0;
    for (const auto& queueFamily : queueFamilies) {
        std::cout << "queue family found with flags: 0x" << std::hex << queueFamily.queueFlags << std::endl;
        if (queueFamily.queueFlags & desiredFlags) {
            std::cout << "queue familiy was a match" << std::endl;
            indices = i;
            break;
        }
    }
    return indices;
}
