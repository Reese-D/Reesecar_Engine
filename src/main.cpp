// #define VOLK_IMPLEMENTATION
// #include <volk/volk.h>
#include "vulkan/vulkan_core.h"
#include <SDL3/SDL.h>
#include <SDL3/SDL_init.h>
#include <SDL3/SDL_vulkan.h>
#include <cstdint>
#include <iostream>
#include <memory>
#include <vector>
#include <vulkan/vulkan.h>

static inline void temporaryDumbCheck(bool result, std::string &&message) {
    if (!result) {
        std::cerr << "Vulkan failed: " << message << std::endl;
    }
}

static inline void temporaryDumbCheck(VkResult result, std::string &&message) {
    if (result != VK_SUCCESS) {
        std::cerr << "Vulkan failed with code: " << result << ". " << message << std::endl;
    }
}

template <typename T, typename T1> static std::vector<T> EnumerateVulkan(VkResult (*func)(T1, uint32_t *, T *), T1 instance, std::string errorMessage) {
    uint32_t itemCount{0};
    temporaryDumbCheck(func(instance, &itemCount, nullptr), errorMessage + std::string(" (when getting count)"));
    std::vector<T> results{itemCount};
    temporaryDumbCheck(func(instance, &itemCount - 1, results.data()), errorMessage + std::string(" (when getting results)"));
    return results;
}

template <typename T, typename T1> static std::vector<T> EnumerateVulkan(VkResult (*func)(T1, uint32_t *, T *), T1 instance) { return EnumerateVulkan<T, T1>(func, instance, ""); }

template <typename T, typename T1> static std::vector<T> EnumerateVulkan(void (*func)(T1, uint32_t *, T *), T1 instance) {
    uint32_t itemCount{0};
    func(instance, &itemCount, nullptr);
    std::vector<T> results{itemCount};
    func(instance, &itemCount, results.data());
    return results;
}

class gbSDLWrapper {

  private:
    struct Instance_Extension {
        uint32_t count;
        char const *const *extensions;
        Instance_Extension() : count(0), extensions(SDL_Vulkan_GetInstanceExtensions(&count)) {};
    };

  public:
    gbSDLWrapper() {
        std::cout << "Setting up SDL" << std::endl;
        temporaryDumbCheck(SDL_Init(SDL_INIT_VIDEO), "Failed to init SDL with video");
        temporaryDumbCheck(SDL_Vulkan_LoadLibrary(NULL), "SDL failed to load vulkan library");
    };

    ~gbSDLWrapper() {
        SDL_QuitSubSystem(SDL_INIT_VIDEO);
        SDL_Quit();
    };

    bool deviceSupportsPresentation(VkInstance &instance, VkPhysicalDevice &device, uint32_t queueFamily) { return SDL_Vulkan_GetPresentationSupport(instance, device, queueFamily); };

    Instance_Extension getInstanceExtensions() { return Instance_Extension{}; };
};

// Wraps the VkInstance lifetime, keeps a shared pointer to the SDL lifetime to ensure it doesn't go out of scope before this does.
class gbVkInstanceWrapper {
  public:
    VkInstance instance{VK_NULL_HANDLE};
    std::shared_ptr<gbSDLWrapper> sdl_ptr;
    gbVkInstanceWrapper() = delete;
    gbVkInstanceWrapper(gbVkInstanceWrapper &other) = delete;
    gbVkInstanceWrapper(gbVkInstanceWrapper &&other) = delete;

    gbVkInstanceWrapper(VkApplicationInfo appInfo, std::shared_ptr<gbSDLWrapper> sdl) : sdl_ptr(sdl) {
        if (!SDL_WasInit(SDL_INIT_VIDEO)) {
            std::cerr << "Vulkan initialiazation failed. gbVkInstanceWrapper depends on SDL's SDL_INIT_VIDEO being enabled" << std::endl;
        }

        auto instance_extensions = sdl_ptr->getInstanceExtensions();

        VkInstanceCreateInfo instanceInfo{
            .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
            .pNext = nullptr,
            .flags = 0, // VkInstanceCreateFlagBits::VK_INSTANCE_CREATE_ENUMERATE_PORTABILITY_BIT_KHR,
            .pApplicationInfo = &appInfo,
            .enabledLayerCount = 0,
            .ppEnabledLayerNames = nullptr,
            .enabledExtensionCount = instance_extensions.count,
            .ppEnabledExtensionNames = instance_extensions.extensions,
        };

        temporaryDumbCheck(vkCreateInstance(&instanceInfo, nullptr, &instance), "Creating VK instance");
    }

    ~gbVkInstanceWrapper() { vkDestroyInstance(instance, nullptr); }
};

int main(int argc, char *argv[]) {
    if (argc > 1) {
        std::cout << argv[0] << " doesn't take any parameters." << std::endl;
    }
    // ----- Setup SDL -----
    auto sdl = std::make_shared<gbSDLWrapper>();
    // volkInitialize();

    // ----- Acquire vulkan instance -----    
    VkApplicationInfo appInfo{
        .sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
        .pNext = nullptr,
        .pApplicationName = "Goblin Horde",
        .applicationVersion = 0,
        .pEngineName = "Reesecar Engine",
        .engineVersion = 0,
        .apiVersion = VK_API_VERSION_1_3,
    };
    gbVkInstanceWrapper instanceWrapper{std::move(appInfo), sdl};

    // ----- Get physical device & its properties -----
    std::vector<VkPhysicalDevice> devices = EnumerateVulkan<VkPhysicalDevice>(vkEnumeratePhysicalDevices, instanceWrapper.instance, std::string("Failed creating physical device"));
    VkPhysicalDeviceProperties2 deviceProperties{.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2, .pNext = nullptr, .properties = {}};
    auto physicalDevice = devices[0];
    vkGetPhysicalDeviceProperties2(physicalDevice, &deviceProperties);
    std::cout << "Chosen device: " << deviceProperties.properties.deviceName << "\n";

    // ----- Setup Queue Families -----
    auto queueFamilies = EnumerateVulkan(vkGetPhysicalDeviceQueueFamilyProperties2, physicalDevice);
    uint32_t queueFamily{0};
    for (size_t i = 0; i < queueFamilies.size(); i++) {
        if (queueFamilies[i].queueFamilyProperties.queueFlags & VK_QUEUE_GRAPHICS_BIT) {
            queueFamily = i;
            break;
        }
    }
    const float queuePriorities{1.0f};
    VkDeviceQueueCreateInfo deviceQueueInfo{
        .sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0, // bitmask
        .queueFamilyIndex = queueFamily,
        .queueCount = 1,
        .pQueuePriorities = &queuePriorities
    };

    temporaryDumbCheck(sdl->deviceSupportsPresentation(instanceWrapper.instance, physicalDevice, queueFamily), std::string("Physical device does not support presentation"));

    // ----- Setup logical device with desired extensions -----
     
// There are so many different fields in these structs it's just too verbose to keep this error here and makes it hard to tell which were enabled.
// These structs won't change, vulkan just makes a new one in newer versions for backwards compatability.
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wmissing-designated-field-initializers"

    const std::vector<const char *> deviceExtensions{VK_KHR_SWAPCHAIN_EXTENSION_NAME};
    VkPhysicalDeviceVulkan12Features enabledVk12Features{
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES,
        .pNext = nullptr,
        .descriptorIndexing = true,
        .shaderSampledImageArrayNonUniformIndexing = true,
        .descriptorBindingVariableDescriptorCount = true,
        .bufferDeviceAddress = true,
    };
    const VkPhysicalDeviceVulkan13Features enabledVk13Features{
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES,
        .pNext = &enabledVk12Features,
        .synchronization2 = true,
        .dynamicRendering = true,
    };
    const VkPhysicalDeviceFeatures enabledVk10Features{.samplerAnisotropy = VK_TRUE};
#pragma clang diagnostic pop

    VkDeviceCreateInfo deviceCreateInfo{
        .sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
        .pNext = &enabledVk13Features,
        .flags = 0, // bitmask
        .queueCreateInfoCount = 1,
        .pQueueCreateInfos = &deviceQueueInfo,
        .enabledLayerCount = 0,         // LEGACY NOT USED, must be 0
        .ppEnabledLayerNames = nullptr, // LEGACY NOT USED, must be NULL
        .enabledExtensionCount = static_cast<uint32_t>(deviceExtensions.size()),
        .ppEnabledExtensionNames = deviceExtensions.data(),
        .pEnabledFeatures = &enabledVk10Features
    };
    VkDevice device{VK_NULL_HANDLE};
    temporaryDumbCheck(vkCreateDevice(physicalDevice, &deviceCreateInfo, nullptr, &device), "Unable to create logical device");

    //----- Get logical device queue -----
    VkQueue queue{VK_NULL_HANDLE};        
    vkGetDeviceQueue(device, queueFamily, 0, &queue); //0 is queue index
    return 0;

    //----- VMA -----
}
