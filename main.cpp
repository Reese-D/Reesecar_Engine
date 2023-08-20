//this will include vulkan/vulkan.h through GLFW
#include <vulkan/vulkan_core.h>
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <iostream>
#include <stdexcept>
#include <cstdlib>
#include <vector>
#include <cstring>

const uint32_t WIDTH = 800;
const uint32_t HEIGHT = 600;

const std::vector<const char*> validationLayers = {
    "VK_LAYER_KHRONOS_validation"
};

#ifdef NDEBUG
const bool enableValidationLayers = false;
#else
const bool enableValidationLayers = true;
#endif
    
class HelloTriangleApplication {
public:
    void run()
    {
        auto window = initWindow();
        //printAvailableExtensions();
        auto instance = initVulkan();
        auto debugMessengers = setupDebugMessengers(instance);
        mainLoop(window);
        cleanup(window, instance, debugMessengers);
    }

private:

    std::vector<const char*> getRequiredExtensions()
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
    
    VkInstance* initVulkan()
    {
        auto instance = createInstance();
        return instance;
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
    
    std::vector<VkDebugUtilsMessengerEXT> setupDebugMessengers(VkInstance* instance)
    {
        std::vector<VkDebugUtilsMessengerEXT> result{};
        if (!enableValidationLayers) return result;

        result.push_back(VkDebugUtilsMessengerEXT{});
        auto createError = createErrorDebug();

        result.push_back(VkDebugUtilsMessengerEXT{});
        auto createInfo = createErrorDebug(VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT, VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT);
        createInfo.pfnUserCallback = debugInfoCallback;//change callback

        if (CreateDebugUtilsMessengerEXT(*instance, &createError, nullptr, &result[0]) != VK_SUCCESS) {
            throw std::runtime_error("failed to set up error messenger!");
        }

        if (CreateDebugUtilsMessengerEXT(*instance, &createInfo, nullptr, &result[1]) != VK_SUCCESS) {
            throw std::runtime_error("failed to set up debug messenger!");
        }
        return result;
    }

    VkResult CreateDebugUtilsMessengerEXT(VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkDebugUtilsMessengerEXT* pDebugMessenger) {
        auto func = (PFN_vkCreateDebugUtilsMessengerEXT) vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");
        if (func != nullptr) {
            return func(instance, pCreateInfo, pAllocator, pDebugMessenger);
        } else {
            return VK_ERROR_EXTENSION_NOT_PRESENT;
        }
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
    
    VkInstance* createInstance()
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

        auto initAndDestroyDebugInfo = createErrorDebug();
        initAndDestroyDebugInfo.pfnUserCallback = debugInitDestroyCallback;
        
        if (enableValidationLayers && !checkValidationLayerSupport()) {
            throw std::runtime_error("validation layers requested, but not available!");
        } else {
            if (enableValidationLayers) {
                createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
                createInfo.ppEnabledLayerNames = validationLayers.data();
                createInfo.pNext = (VkDebugUtilsMessengerCreateInfoEXT*)&initAndDestroyDebugInfo;
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

    void mainLoop(GLFWwindow* window)
    {
        while (!glfwWindowShouldClose(window)) {
            glfwPollEvents();
        }
    }

    void cleanup(GLFWwindow* window, VkInstance* instance, std::vector<VkDebugUtilsMessengerEXT> debugMessengers)
    {
        if (enableValidationLayers) {
            for(auto debugMessenger : debugMessengers){
                DestroyDebugUtilsMessengerEXT(*instance, debugMessenger, nullptr);
            }
        }
        
        vkDestroyInstance(*instance, nullptr);
        glfwDestroyWindow(window);
        glfwTerminate();
    }

    GLFWwindow* initWindow()
    {
        glfwInit();
        
        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
        glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE); //disable resizing for now, takes more complexity to handle it
        GLFWwindow* window = glfwCreateWindow(WIDTH, HEIGHT, "Vulkan", nullptr, nullptr);
        
        return window;
    }

    void printAvailableExtensions()
    {
        uint32_t extensionCount = 0;
        vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, nullptr);
        std::vector<VkExtensionProperties> extensions(extensionCount);
        vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, extensions.data());
        std::cout << "available extensions:\n";

        for (const auto& extension : extensions) {
            std::cout << '\t' << extension.extensionName << '\n';
        }
    }

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

};

int main() {
    HelloTriangleApplication app;

    try {
        app.run();
    } catch (const std::exception& e) {
        std::cerr << e.what() << std::endl;
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
