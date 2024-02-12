#pragma once
#include <memory>
#include <vulkan/vulkan_core.h>
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <vector>
#ifdef NDEBUG
const bool enableValidationLayers = false;
#else
const bool enableValidationLayers = true;
#endif

static const int DEBUG_MESSENGER_INDEX_DEBUG = 0;
static const int DEBUG_MESSENGER_INDEX_ERROR = 1;
static const int DEBUG_MESSENGER_INDEX_CREATE_DESTROY = 2;
 
class validation
{
 public:
    ~validation();
    validation(std::shared_ptr<VkInstance> instance, const std::vector<const char*> validationLayers);
    validation() = delete;
    
    
    static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
                                                        VkDebugUtilsMessageTypeFlagsEXT messageType,
                                                        const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
                                                        void* pUserData);
    
    static VKAPI_ATTR VkBool32 VKAPI_CALL debugInitDestroyCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
                                                                   VkDebugUtilsMessageTypeFlagsEXT messageType,
                                                                   const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
                                                                   void* pUserData);

    static VKAPI_ATTR VkBool32 VKAPI_CALL debugInfoCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
                                                            VkDebugUtilsMessageTypeFlagsEXT messageType,
                                                            const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
                                                            void* pUserData);

    static VkDebugUtilsMessengerCreateInfoEXT createErrorDebug( VkStructureType structureType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT
                                                                ,VkDebugUtilsMessageSeverityFlagsEXT messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT
                                                                ,VkDebugUtilsMessageTypeFlagsEXT messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT
                                                                ,PFN_vkDebugUtilsMessengerCallbackEXT callback = debugCallback);

    static const std::vector<VkDebugUtilsMessengerCreateInfoEXT> getDebugMessengers() noexcept;



    std::vector<VkDebugUtilsMessengerEXT> setupDebugMessengers(std::shared_ptr<VkInstance> instance);

    static bool checkValidationLayerSupport(std::vector<const char*> validationLayers);
    
 private:
    void DestroyDebugUtilsMessengerEXT(VkInstance instance, VkDebugUtilsMessengerEXT debugMessenger, const VkAllocationCallbacks* pAllocator);
    VkResult CreateDebugUtilsMessengerEXT(VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkDebugUtilsMessengerEXT* pDebugMessenger);
    
    std::shared_ptr<VkInstance> initializedInstance_;
    std::vector<const char*> validationLayers_;
    std::vector<VkDebugUtilsMessengerEXT> debugMessengers_;
};
