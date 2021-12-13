#pragma once
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <iostream>
#include <stdexcept>
#include <cstdlib>

#include <vector>
#include <optional>
#include <set>
#include <cstdint>
#include <algorithm>

const uint32_t WIDTH = 800;
const uint32_t HEIGHT = 600;


class HelloTriangleApplication
{

public:

    void run(); 

private:
    struct QueueFamilyIndices {
        std::optional<uint32_t> graphicsFamily;
        std::optional<uint32_t> presentFamily;

        bool isComplete() {
            return graphicsFamily.has_value() && presentFamily.has_value();
        }
    };

    const uint32_t WIDTH = 800;
    const uint32_t HEIGHT = 600;

    VkQueue graphicsQueue;
    VkQueue presentQueue;
    GLFWwindow* window;
    VkInstance instance;
    VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
    VkDevice device;
    VkSurfaceKHR surface;

    //more swapchain info 
    VkSwapchainKHR swapChain;
    std::vector<VkImage> swapChainImages;
    VkFormat swapChainImageFormat;
    VkExtent2D swapChainExtent;

    std::vector<VkImageView> swapChainImageViews; 

    const std::vector<const char*> validationLayers = {
        "VK_LAYER_KHRONOS_validation"
    };

    const std::vector<const char*> deviceExtensions = {
        VK_KHR_SWAPCHAIN_EXTENSION_NAME //image presentation is not built into the vulkan core...need to enable it through an extension 
    };

#ifdef NDEBUG 
    const bool enableValidationLayers = false;
#else
    const bool enableValidationLayers = true;
#endif

    /*
        Details needed to create swap chain:
        1. Surface capabilities (# swap chain images, resolution of images)
        2. Surface formats (pixel format, color space)
        3. Presentation modes
    */
    struct SwapChainSupportDetails {
        VkSurfaceCapabilitiesKHR capabilities;
        std::vector<VkSurfaceFormatKHR> formats;
        std::vector<VkPresentModeKHR> presentModes;
    };


    void initVulkan(); 
    void createSwapChain(); 
    void initWindow();
    void mainLoop(); 
    void cleanup(); 
    void createSurface(); 
    bool checkValidationLayerSupport();
    void createInstance(); 
    void pickPhysicalDevice(); 
    bool isDeviceSuitable(VkPhysicalDevice device); 

    /*
            Check if the given device supports required extensions.
    */
    bool checkDeviceExtensionSupport(VkPhysicalDevice device); 

    /*
        Find what queues are available for the device
        Queues support different types of commands such as: processing compute commands or memory transfer commands
    */
    QueueFamilyIndices findQueueFamilies(VkPhysicalDevice device);

    //Create a logical device to communicate with the physical device 
    void createLogicalDevice(); 

    /*
        Request specific details about swap chain support for a given device
    */
    SwapChainSupportDetails querySwapChainSupport(VkPhysicalDevice device);

    VkSurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats); 

    //Look through givent present modes and pick the "best" one
    VkPresentModeKHR chooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes); 

    VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities); 

    /// <summary>
    /// Create an image view object for use in the rendering pipeline
    /// 'Image View' -> describes how to access an image and what part of an image to access
    /// </summary>
    void createImageViews(); 
};

