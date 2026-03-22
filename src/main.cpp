// #define VOLK_IMPLEMENTATION
// #include <volk/volk.h>
#include "vulkan/vulkan_core.h"
#include <SDL3/SDL.h>
#include <SDL3/SDL_init.h>
#include <SDL3/SDL_vulkan.h>
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
    temporaryDumbCheck(func(instance, &itemCount, nullptr), errorMessage + std::string(" (when getting count)") );
    std::vector<T> results{itemCount};
    temporaryDumbCheck(func(instance, &itemCount-1, results.data()), errorMessage + std::string(" (when getting results)"));
    return results;
}

template <typename T, typename T1> static std::vector<T> EnumerateVulkan(VkResult (*func)(T1, uint32_t *, T *), T1 instance) {
    return EnumerateVulkan<T, T1>(func, instance, "");
}    
    

template <typename T, typename T1> static std::vector<T> EnumerateVulkan(void (*func)(T1, uint32_t *, T *), T1 instance) {
    uint32_t itemCount{0};
    func(instance, &itemCount, nullptr);
    std::vector<T> results{itemCount};
    func(instance, &itemCount, results.data());
    return results;
}

class gbSDLWrapper {
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
        uint32_t instanceExtensionsCount{0};
        char const *const *instanceExtensions{SDL_Vulkan_GetInstanceExtensions(&instanceExtensionsCount)};

        VkInstanceCreateInfo instanceInfo{
            .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
            .pNext = nullptr,
            .flags = 0, // VkInstanceCreateFlagBits::VK_INSTANCE_CREATE_ENUMERATE_PORTABILITY_BIT_KHR,
            .pApplicationInfo = &appInfo,
            .enabledLayerCount = 0,
            .ppEnabledLayerNames = nullptr,
            .enabledExtensionCount = instanceExtensionsCount,
            .ppEnabledExtensionNames = instanceExtensions,
        };

        temporaryDumbCheck(vkCreateInstance(&instanceInfo, nullptr, &instance), "Creating VK instance");
    }

    ~gbVkInstanceWrapper() { vkDestroyInstance(instance, nullptr); }
};

int main(int argc, char *argv[]) {
    if (argc > 1) {
        std::cout << argv[0] << " doesn't take any parameters." << std::endl;
    }

    auto sdl = std::make_shared<gbSDLWrapper>();
    // volkInitialize();
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

    std::vector<VkPhysicalDevice> devices = EnumerateVulkan<VkPhysicalDevice>(vkEnumeratePhysicalDevices, instanceWrapper.instance, std::string("Failed creating physical device"));

    VkPhysicalDeviceProperties2 deviceProperties{.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2, .pNext = nullptr, .properties = {}};
    vkGetPhysicalDeviceProperties2(devices[0], &deviceProperties);

    auto queueFamilies = EnumerateVulkan(vkGetPhysicalDeviceQueueFamilyProperties2, devices[0]);
    uint32_t queueFamily{ 0 };
    for (size_t i = 0; i < queueFamilies.size(); i++) {
	if (queueFamilies[i].queueFamilyProperties.queueFlags & VK_QUEUE_GRAPHICS_BIT) {
	    queueFamily = i;
	    break;
	}
    }    
    std::cout << "Chosen device: " << deviceProperties.properties.deviceName << "\n";

    return 0;
}
