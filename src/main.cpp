#include "vulkan/vulkan_core.h"
#include <SDL3/SDL.h>
#include <SDL3/SDL_init.h>
#include <SDL3/SDL_vulkan.h>
#include <iostream>
#include <memory>
#include <vulkan/vulkan.h>



static inline void temporaryDumbCheck(bool result, std::string &&message) {
    if (!result) {
	std::cerr << "Vulkan failed: " <<  message << std::endl;
    }        
}

class gbSDLWrapper {
  public:
    gbSDLWrapper() { SDL_Init(SDL_INIT_VIDEO); };

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

    gbVkInstanceWrapper(VkApplicationInfo &&appInfo, std::shared_ptr<gbSDLWrapper> sdl) : sdl_ptr(sdl) {
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

    return 0;
}
