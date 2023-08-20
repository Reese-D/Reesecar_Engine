#include <vulkan/vulkan_core.h>
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <iostream>
#include <vector>
#include <cstring>

#ifdef NDEBUG
const bool enableValidationLayers = false;
#else
const bool enableValidationLayers = true;
#endif

namespace validation
{
    const std::vector<const char*> validationLayers = {
        "VK_LAYER_KHRONOS_validation"
    };

    static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
                                                        VkDebugUtilsMessageTypeFlagsEXT messageType,
                                                        const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
                                                        void* pUserData)
        {

            std::cerr << "validation layer: " << pCallbackData->pMessage << std::endl;

            return VK_FALSE;
        }

    static VKAPI_ATTR VkBool32 VKAPI_CALL debugInitDestroyCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
                                                                   VkDebugUtilsMessageTypeFlagsEXT messageType,
                                                                   const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
                                                                   void* pUserData)
    {

        std::cerr << "Create/Destroy validation layer: " << pCallbackData->pMessage << std::endl;

        return VK_FALSE;
    }

    static VKAPI_ATTR VkBool32 VKAPI_CALL debugInfoCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
                                                            VkDebugUtilsMessageTypeFlagsEXT messageType,
                                                            const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
                                                            void* pUserData)
    {
        std::cout << "validation layer info: " << pCallbackData->pMessage << std::endl;

        return VK_TRUE;
    }


    VkDebugUtilsMessengerCreateInfoEXT createErrorDebug( VkStructureType structureType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT
                                                         ,VkDebugUtilsMessageSeverityFlagsEXT messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT
                                                         ,VkDebugUtilsMessageTypeFlagsEXT messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT
                                                         ,PFN_vkDebugUtilsMessengerCallbackEXT callback = debugCallback)
    {
        VkDebugUtilsMessengerCreateInfoEXT createError{};
        createError.sType = structureType;
        createError.messageSeverity = messageSeverity;
        createError.messageType = messageType;
        createError.pfnUserCallback = callback;
        createError.pUserData = nullptr; // Optional
        createError.pNext = nullptr;
        return createError;
    }

    static const int DEBUG_MESSENGER_INDEX_DEBUG = 0;
    static const int DEBUG_MESSENGER_INDEX_ERROR = 1;
    static const int DEBUG_MESSENGER_INDEX_CREATE_DESTROY = 2;
    const std::vector<VkDebugUtilsMessengerCreateInfoEXT> getDebugMessengers() noexcept
    {
        std::vector<VkDebugUtilsMessengerCreateInfoEXT> debugMessengers{};
        debugMessengers.push_back(createErrorDebug());
        auto createInfo = createErrorDebug(VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT, VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT);
        debugMessengers.push_back(createInfo);
        auto initAndDestroyDebugInfo = createErrorDebug();
        initAndDestroyDebugInfo.pfnUserCallback = debugInitDestroyCallback;

        debugMessengers.push_back(initAndDestroyDebugInfo);

        return debugMessengers;
    }

    VkResult CreateDebugUtilsMessengerEXT(VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkDebugUtilsMessengerEXT* pDebugMessenger) {
        auto func = (PFN_vkCreateDebugUtilsMessengerEXT) vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");
        if (func != nullptr) {
            return func(instance, pCreateInfo, pAllocator, pDebugMessenger);
        } else {
            return VK_ERROR_EXTENSION_NOT_PRESENT;
        }
    }


    std::vector<VkDebugUtilsMessengerEXT> setupDebugMessengers(VkInstance* instance)
    {
        std::vector<VkDebugUtilsMessengerEXT> result{};
        if (!enableValidationLayers) return result;

        result.push_back(VkDebugUtilsMessengerEXT{});
        result.push_back(VkDebugUtilsMessengerEXT{});

        auto messengers = getDebugMessengers();

        if (CreateDebugUtilsMessengerEXT(*instance, &messengers[DEBUG_MESSENGER_INDEX_DEBUG], nullptr, &result[0]) != VK_SUCCESS) {
            throw std::runtime_error("failed to set up error messenger!");
        }

        if (CreateDebugUtilsMessengerEXT(*instance, &messengers[DEBUG_MESSENGER_INDEX_ERROR], nullptr, &result[1]) != VK_SUCCESS) {
            throw std::runtime_error("failed to set up debug messenger!");
        }
        return result;

    }

    void DestroyDebugUtilsMessengerEXT(VkInstance instance, VkDebugUtilsMessengerEXT debugMessenger, const VkAllocationCallbacks* pAllocator) {
        auto func = (PFN_vkDestroyDebugUtilsMessengerEXT) vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");
        if (func != nullptr) {
            func(instance, debugMessenger, pAllocator);
        }
    }

        bool checkValidationLayerSupport()
    {
        uint32_t layerCount;
        vkEnumerateInstanceLayerProperties(&layerCount, nullptr);

        std::vector<VkLayerProperties> availableLayers(layerCount);
        vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());

        for (const char* layerName : validationLayers) {
            bool layerFound = false;

            for (const auto& layerProperties : availableLayers) {
                if (strcmp(layerName, layerProperties.layerName) == 0) {
                    layerFound = true;
                    break;
                }
            }

            if (!layerFound) {
                return false;
            }
        }

        return true;
    }

}

