#include <cstdint>
#include <optional>
#include <memory>
#include <vulkan/vulkan.hpp>
#include <vulkan/vulkan_core.h>

class queue{
public:
    //struct QueueFamilyIndices {
    //std::optional<uint32_t> graphicsFamily;
    //};
    using QueueFamilyIndices =  std::optional<uint32_t>;
    static QueueFamilyIndices findQueueFamilies(VkPhysicalDevice device, VkQueueFlags desiredFlags = VK_QUEUE_GRAPHICS_BIT);
};




