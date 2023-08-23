#include "validation.h"

#include <iostream>
#include <cstring>


VKAPI_ATTR VkBool32 VKAPI_CALL validation::debugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
                                                         VkDebugUtilsMessageTypeFlagsEXT messageType,
                                                         const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
                                                         void* pUserData)
{

    std::cerr << "validation layer: " << pCallbackData->pMessage << std::endl;

    return VK_FALSE;
}

VKAPI_ATTR VkBool32 VKAPI_CALL validation::debugInitDestroyCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
                                                                    VkDebugUtilsMessageTypeFlagsEXT messageType,
                                                                    const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
                                                                    void* pUserData)
{

    std::cerr << "Create/Destroy validation layer: " << pCallbackData->pMessage << std::endl;

    return VK_FALSE;
}

VKAPI_ATTR VkBool32 VKAPI_CALL validation::debugInfoCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
                                                             VkDebugUtilsMessageTypeFlagsEXT messageType,
                                                             const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
                                                             void* pUserData)
{
    std::cout << "validation layer info: " << pCallbackData->pMessage << std::endl;

    return VK_TRUE;
}


VkDebugUtilsMessengerCreateInfoEXT validation::createErrorDebug( VkStructureType structureType
                                                                 ,VkDebugUtilsMessageSeverityFlagsEXT messageSeverity
                                                                 ,VkDebugUtilsMessageTypeFlagsEXT messageType
                                                                 ,PFN_vkDebugUtilsMessengerCallbackEXT callback)
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

const std::vector<VkDebugUtilsMessengerCreateInfoEXT> validation::getDebugMessengers() noexcept
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

VkResult validation::CreateDebugUtilsMessengerEXT(VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkDebugUtilsMessengerEXT* pDebugMessenger) {
    auto func = (PFN_vkCreateDebugUtilsMessengerEXT) vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");
    if (func != nullptr) {
        return func(instance, pCreateInfo, pAllocator, pDebugMessenger);
    } else {
        return VK_ERROR_EXTENSION_NOT_PRESENT;
    }
}

std::vector<VkDebugUtilsMessengerEXT> validation::setupDebugMessengers(std::shared_ptr<VkInstance> instance)
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

void validation::DestroyDebugUtilsMessengerEXT(VkInstance instance, VkDebugUtilsMessengerEXT debugMessenger, const VkAllocationCallbacks* pAllocator) {
    auto func = (PFN_vkDestroyDebugUtilsMessengerEXT) vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");
    if (func != nullptr) {
        func(instance, debugMessenger, pAllocator);
    }
}

bool validation::checkValidationLayerSupport(std::vector<const char*> validationLayers)
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

validation::~validation()
{
    if (enableValidationLayers) {
        for(auto debugMessenger : debugMessengers_){
            DestroyDebugUtilsMessengerEXT(*initializedInstance_, debugMessenger, nullptr);
        }
    }
}

validation::validation(std::shared_ptr<VkInstance> instance, const std::vector<const char*> validationLayers)
    :initializedInstance_(instance)
    ,validationLayers_(validationLayers)
{
    if(enableValidationLayers){
        debugMessengers_ = setupDebugMessengers(instance);
    }
}
