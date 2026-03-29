// #define VOLK_IMPLEMENTATION
// #include <volk/volk.h>
#include "vulkan/vulkan_core.h"
#include <SDL3/SDL.h>
#include <SDL3/SDL_init.h>
#include <SDL3/SDL_video.h>
#include <SDL3/SDL_vulkan.h>
#include <cstdint>
#include <iostream>
#include <memory>
#include <vector>
#include <vulkan/vulkan.h>
#define VMA_IMPLEMENTATION
#include <glm/glm.hpp>
#include <vk_mem_alloc.h>

#define PRAGMA_STR(x) _Pragma(#x)
#define START_IGNORE_FIELD(fieldName)                                                                                                                                                   \
    _Pragma("clang diagnostic push");                                                                                                                                                   \
    PRAGMA_STR(clang diagnostic ignored fieldName);
#define END_IGNORE_FIELD _Pragma("clang diagnostic pop")

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

template <typename T, typename... T1> static std::vector<T> EnumerateVulkan(std::string errorMessage, VkResult (*func)(T1..., uint32_t *, T *), T1... instance) {
    uint32_t itemCount{0};
    temporaryDumbCheck(func(instance..., &itemCount, nullptr), errorMessage + std::string(" (when getting count)"));
    std::vector<T> results{itemCount};
    temporaryDumbCheck(func(instance..., &itemCount - 1, results.data()), errorMessage + std::string(" (when getting results)"));
    return results;
}

template <typename T, typename... T1> static std::vector<T> EnumerateVulkan(VkResult (*func)(T1..., uint32_t *, T *), T1... instance) {
    return EnumerateVulkan<T, T1...>("", func, instance...);
}

template <typename T, typename... T1> static std::vector<T> EnumerateVulkan(void (*func)(T1..., uint32_t *, T *), T1... instance) {
    uint32_t itemCount{0};
    func(instance..., &itemCount, nullptr);
    std::vector<T> results{itemCount};
    func(instance..., &itemCount, results.data());
    return results;
}

class gbSDLWrapper {

  private:
    struct Instance_Extension {
        uint32_t count;
        char const *const *extensions;
        Instance_Extension() : count(0), extensions(SDL_Vulkan_GetInstanceExtensions(&count)) {};
    };

    // Theoretically you can have multiple windows for a single sdl instance, so it will keep track of its own SDL_DestroyWindow call instead of the sdl wrapper
    struct Window_Wrapper {
        SDL_Window *window;
        std::shared_ptr<VkSurfaceKHR> surface;
        glm::ivec2 windowSize;
        Window_Wrapper(std::shared_ptr<VkInstance> instance, std::string &&windowName, int width, int height, SDL_WindowFlags flags)
            : surface(new VkSurfaceKHR{VK_NULL_HANDLE}, [instance](VkSurfaceKHR *ptr) { vkDestroySurfaceKHR(*instance, *ptr, nullptr); }) {
            window = SDL_CreateWindow(windowName.c_str(), width, height, flags);
            temporaryDumbCheck(SDL_Vulkan_CreateSurface(window, *instance, nullptr, surface.get()), "Creating vulkan surface");
            temporaryDumbCheck(SDL_GetWindowSize(window, &windowSize.x, &windowSize.y), "Getting window size");
        }
        ~Window_Wrapper() { SDL_DestroyWindow(window); }
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

    Window_Wrapper GetWindowAndExtensions(std::shared_ptr<VkInstance> instance) {
        Window_Wrapper result{instance, std::string("Gobline Horde!"), 1280u, 720u, SDL_WINDOW_VULKAN | SDL_WINDOW_RESIZABLE};
        return result;
    };
};

// Wraps the VkInstance lifetime, keeps a shared pointer to the SDL lifetime to ensure it doesn't go out of scope before this does.
class gbVkInstanceWrapper {
  public:
    std::shared_ptr<VkInstance> instance{VK_NULL_HANDLE};
    std::shared_ptr<gbSDLWrapper> sdl_ptr;
    gbVkInstanceWrapper() = delete;
    gbVkInstanceWrapper(gbVkInstanceWrapper &other) = delete;
    gbVkInstanceWrapper(gbVkInstanceWrapper &&other) = delete;

    gbVkInstanceWrapper(VkApplicationInfo appInfo, std::shared_ptr<gbSDLWrapper> sdl)
        : instance(
              new VkInstance{VK_NULL_HANDLE},
              [](VkInstance *ptr) {
                  std::cout << "Destroying vulkan instance" << std::endl;
                  vkDestroyInstance(*ptr, nullptr);
              }
          ),
          sdl_ptr(sdl) {
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

        temporaryDumbCheck(vkCreateInstance(&instanceInfo, nullptr, instance.get()), "Creating VK instance");
    }

    ~gbVkInstanceWrapper() = default;
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
    std::vector<VkPhysicalDevice> devices = EnumerateVulkan<VkPhysicalDevice>(std::string("Failed creating physical device"), vkEnumeratePhysicalDevices, *instanceWrapper.instance);
    VkPhysicalDeviceProperties2 deviceProperties{.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2, .pNext = nullptr, .properties = {}};
    auto physicalDevice = devices[0];
    vkGetPhysicalDeviceProperties2(physicalDevice, &deviceProperties);
    std::cout << "Chosen device: " << deviceProperties.properties.deviceName << "\n";

    // ----- Setup Queue Families -----
    auto queueFamilies = EnumerateVulkan<VkQueueFamilyProperties2>(vkGetPhysicalDeviceQueueFamilyProperties2, physicalDevice);
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

    temporaryDumbCheck(sdl->deviceSupportsPresentation(*instanceWrapper.instance, physicalDevice, queueFamily), std::string("Physical device does not support presentation"));

    // ----- Setup logical device with desired extensions -----

    // There are so many different fields in these structs it's just too verbose to keep this error here and makes it hard to tell which were enabled.
    // These structs won't change, vulkan just makes a new one in newer versions for backwards compatability.
    START_IGNORE_FIELD("-Wmissing-designated-field-initializers")
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
    END_IGNORE_FIELD

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

    std::shared_ptr<VkDevice> device{new VkDevice{VK_NULL_HANDLE}, [](VkDevice *ptr) {
                                         std::cout << "Destroying vulkan device" << std::endl;
                                         vkDestroyDevice(*ptr, nullptr);
                                     }};
    temporaryDumbCheck(vkCreateDevice(physicalDevice, &deviceCreateInfo, nullptr, device.get()), "Unable to create logical device");

    //----- Get logical device queue -----
    VkQueue queue{VK_NULL_HANDLE};
    vkGetDeviceQueue(*device, queueFamily, 0, &queue); // 0 is queue index

    //----- VMA -----
    START_IGNORE_FIELD("-Wmissing-designated-field-initializers")
    VmaVulkanFunctions vmaVkFunctions{.vkGetInstanceProcAddr = vkGetInstanceProcAddr, .vkGetDeviceProcAddr = vkGetDeviceProcAddr, .vkCreateImage = vkCreateImage};
    VmaAllocatorCreateInfo vmaAllocatorCreateInfo{
        .flags = VMA_ALLOCATOR_CREATE_BUFFER_DEVICE_ADDRESS_BIT,
        .physicalDevice = physicalDevice,
        .device = *device,
        .pVulkanFunctions = &vmaVkFunctions,
        .instance = *instanceWrapper.instance
    };
    END_IGNORE_FIELD
    std::shared_ptr<VmaAllocator> allocator{new VmaAllocator{VK_NULL_HANDLE}, [](VmaAllocator *ptr) { vmaDestroyAllocator(*ptr); }};
    temporaryDumbCheck(vmaCreateAllocator(&vmaAllocatorCreateInfo, allocator.get()), "VMA couldn't allocate space for the create info and functions");

    //----- Window and Surface -----
    auto window_wrapper = sdl->GetWindowAndExtensions(instanceWrapper.instance);
    VkSurfaceCapabilitiesKHR surfaceCapabilities{};
    temporaryDumbCheck(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physicalDevice, *window_wrapper.surface, &surfaceCapabilities), "Getting physical device surface capabilities");

    //----- Swapchain ------
    const VkFormat imageFormat{VK_FORMAT_B8G8R8A8_SRGB};
    START_IGNORE_FIELD("-Wmissing-designated-field-initializers")
    VkSwapchainCreateInfoKHR swapchainCreateInfo{
        .sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
        .pNext = nullptr,
        .surface = *window_wrapper.surface,
        .minImageCount = surfaceCapabilities.minImageCount,
        .imageFormat = imageFormat,
        .imageColorSpace = VK_COLORSPACE_SRGB_NONLINEAR_KHR, // Guaranteed to be available with VK_FORMAT_B8G8R8A8_SRGB on all systems
        .imageExtent{.width = surfaceCapabilities.currentExtent.width, .height = surfaceCapabilities.currentExtent.height},
        .imageArrayLayers = 1,
        .imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
        .preTransform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR,
        .compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
        .presentMode = VK_PRESENT_MODE_FIFO_KHR
    };
    END_IGNORE_FIELD

    std::shared_ptr<VkSwapchainKHR> swapchain{new VkSwapchainKHR{VK_NULL_HANDLE}, [device](VkSwapchainKHR *ptr) {
                                                  std::cout << "Destroying vulkan swapchainKHR" << std::endl;
                                                  vkDeviceWaitIdle(*device);
                                                  vkDestroySwapchainKHR(*device, *ptr, nullptr);
                                              }};
    temporaryDumbCheck(vkCreateSwapchainKHR(*device, &swapchainCreateInfo, nullptr, swapchain.get()), "Create Swapchain");

    auto swapchainImages = EnumerateVulkan<VkImage>("Get swapchain images", vkGetSwapchainImagesKHR, *device, *swapchain);
    std::vector<std::shared_ptr<VkImageView>> swapchainImageViews = {};
    for (size_t i = 0; i < swapchainImages.size(); i++) {
        swapchainImageViews.emplace_back(std::shared_ptr<VkImageView>{new VkImageView{VK_NULL_HANDLE}, [device](VkImageView *ptr) {
                                                                          std::cout << "Destroying Image view" << std::endl;
                                                                          vkDestroyImageView(*device, *ptr, nullptr);
                                                                      }});
        auto currentImageView = swapchainImageViews.back();
        VkImageViewCreateInfo viewCreateInfo{
            .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
            .pNext = nullptr,
            .flags = 0,
            .image = swapchainImages[i],
            .viewType = VK_IMAGE_VIEW_TYPE_2D,
            .format = imageFormat,
            .components = {},
            .subresourceRange{.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT, .baseMipLevel = 0, .levelCount = 1, .baseArrayLayer = 0, .layerCount = 1}
        };
        temporaryDumbCheck(vkCreateImageView(*device, &viewCreateInfo, nullptr, currentImageView.get()), "Create image view");
    }

    //----- Depth Attachment -----
    std::vector<VkFormat> depthFormatList{VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT};
    VkFormat depthFormat{VK_FORMAT_UNDEFINED};
    for (VkFormat &format : depthFormatList) {
        VkFormatProperties2 formatProperties{.sType = VK_STRUCTURE_TYPE_FORMAT_PROPERTIES_2, .pNext = nullptr, .formatProperties = {}};
        vkGetPhysicalDeviceFormatProperties2(physicalDevice, format, &formatProperties);
        if (formatProperties.formatProperties.optimalTilingFeatures & VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT) {
            depthFormat = format;
            break;
        }
    }
    temporaryDumbCheck(depthFormat != VK_FORMAT_UNDEFINED, "Depth format undefined");
    VkImageCreateInfo depthImageCreateInfo{
        .sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .imageType = VK_IMAGE_TYPE_2D,
        .format = depthFormat,
        .extent{.width = static_cast<uint32_t>(window_wrapper.windowSize.x), .height = static_cast<uint32_t>(window_wrapper.windowSize.y), .depth = 1},
        .mipLevels = 1,
        .arrayLayers = 1,
        .samples = VK_SAMPLE_COUNT_1_BIT,
        .tiling = VK_IMAGE_TILING_OPTIMAL,
        .usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
        .sharingMode = {},
        .queueFamilyIndexCount = 0,
        .pQueueFamilyIndices = nullptr,
        .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
    };
    VmaAllocationCreateInfo allocCreateInfo{
        .flags = VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT,
        .usage = VMA_MEMORY_USAGE_AUTO,
        .requiredFlags = 0,  // flags that must be set for chosen allocation. AFAIK there aren't any for this setup?
        .preferredFlags = 0, // 0 if no additional flags are preferred
        .memoryTypeBits = 0, // a mask, 0 means any memory type is acceptable
        .pool = VK_NULL_HANDLE,
        .pUserData = nullptr, // must be nullptr if VMA_ALLOCATION_CREATE_USER_DATA_COPY_STRING_BIT is used, we're just not using it though so nullptr
        .priority = 0,        // Ignored if VMA_ALLOCATOR_CREATE_EXT_MEMORY_PRIORITY_BIT was not set, so we just use 0
        .minAlignment = 0     // default
    };

    auto depthImageAllocation = std::make_shared<VmaAllocation>();
    std::shared_ptr<VkImage> depthImage{new VkImage{VK_NULL_HANDLE}, [depthImageAllocation, allocator](VkImage *ptr) {
                                            std::cout << "Destroying depth image" << std::endl;
                                            vmaDestroyImage(*allocator, *ptr, *depthImageAllocation);
                                        }};
    temporaryDumbCheck(vmaCreateImage(*allocator, &depthImageCreateInfo, &allocCreateInfo, depthImage.get(), depthImageAllocation.get(), nullptr), "VMA create image");
    VkImageViewCreateInfo depthViewCI{
        .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .image = *depthImage,
        .viewType = VK_IMAGE_VIEW_TYPE_2D,
        .format = depthFormat,
        .components = {},
        .subresourceRange{.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT, .baseMipLevel = 0, .levelCount = 1, .baseArrayLayer = 0, .layerCount = 1}
    };
    std::shared_ptr<VkImageView> depthImageView{new VkImageView{VK_NULL_HANDLE}, [device](VkImageView *ptr) {
                                                    std::cout << "Destroying depth image view" << std::endl;
                                                    vkDestroyImageView(*device, *ptr, nullptr);
                                                }};
    temporaryDumbCheck(vkCreateImageView(*device, &depthViewCI, nullptr, depthImageView.get()), "Create image view");

    //----- Program exit -----
    std::cout << "Program finished, Terminating..." << std::endl;
    return 0;
}
