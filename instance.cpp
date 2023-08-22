#include "instance.h"
#include <stdexcept>
#include <iostream>

instance::instance(const std::vector<const char*> validationLayers)
{
    instance_ = createInstance(validationLayers);
    pickPhysicalDevice(instance_);
    validation_  = new validation{instance_, validationLayers};
}

void instance::pickPhysicalDevice(VkInstance* instance) {
    VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
    uint32_t deviceCount = 0;
    vkEnumeratePhysicalDevices(*instance, &deviceCount, nullptr);
    if (deviceCount == 0) {
        throw std::runtime_error("failed to find GPUs with Vulkan support!");
    }
    std::vector<VkPhysicalDevice> devices(deviceCount);
    vkEnumeratePhysicalDevices(*instance, &deviceCount, devices.data());

    for (const auto& device : devices) {
        if (isDeviceSuitable(device)) {
            physicalDevice = device;
            break;
        }
    }

    if (physicalDevice == VK_NULL_HANDLE) {
        throw std::runtime_error("failed to find a suitable GPU!");
    }
}

bool instance::isDeviceSuitable(VkPhysicalDevice device) {
    VkPhysicalDeviceProperties deviceProperties;
    VkPhysicalDeviceFeatures deviceFeatures;
    vkGetPhysicalDeviceProperties(device, &deviceProperties);
    vkGetPhysicalDeviceFeatures(device, &deviceFeatures);
    std::cout << "found a graphics device...: " << deviceProperties.deviceName << std::endl;

    return deviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU &&
        deviceFeatures.geometryShader;
}

instance::~instance()
{
    
        delete validation_;
        vkDestroyInstance(*instance_, nullptr);
}
        
VkInstance* instance::createInstance(const std::vector<const char*> validationLayers)
{
    VkApplicationInfo appInfo{};
    //most of these are optional
    appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO; //most structs require explicit type names
    appInfo.pApplicationName = "Hello Triangle";
    appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.pEngineName = "The glorious Reesecar engine";
    appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.apiVersion = VK_API_VERSION_1_0;
    appInfo.pNext = nullptr; // pNext is for extensions, usually null

    //glfw can give us the required extensions to be passed through
    uint32_t glfwExtensionCount = 0;
    const char** glfwExtensions;
    auto extensions = getRequiredExtensions();
        
    //not optional, defines which global extensions and validation layers to use
    VkInstanceCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    createInfo.pApplicationInfo = &appInfo;
    createInfo.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
    createInfo.ppEnabledExtensionNames = extensions.data();
    createInfo.enabledLayerCount = 0; //global validation layers to enable

    auto validationCreateDestroy = validation::getDebugMessengers()[validation::DEBUG_MESSENGER_INDEX_CREATE_DESTROY];
                        
    if (enableValidationLayers && !validation::checkValidationLayerSupport(validationLayers)) {
        throw std::runtime_error("validation layers requested, but not available!");
    } else {
        if (enableValidationLayers) {
            createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
            createInfo.ppEnabledLayerNames = validationLayers.data();
            createInfo.pNext = &validationCreateDestroy;
        } else {
            createInfo.enabledLayerCount = 0;
            createInfo.pNext = nullptr;
        }
    }

    VkInstance* instance = new VkInstance();
    VkResult result = vkCreateInstance(&createInfo, nullptr, instance);
        
    if (result != VK_SUCCESS) {
        throw std::runtime_error("failed to create instance!");
    }
        
        
    return instance;
}
std::vector<const char*> instance::getRequiredExtensions()
{
    uint32_t glfwExtensionCount = 0;
    const char** glfwExtensions;
    glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

    std::vector<const char*> extensions(glfwExtensions, glfwExtensions + glfwExtensionCount);

    if (enableValidationLayers) {
        extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
    }

    return extensions;
}


VkInstance* instance::getInstance()
{
    return instance_;
}

