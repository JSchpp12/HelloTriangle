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
#include <fstream>

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

    VkPipeline graphicsPipeline; 
    VkRenderPass renderPass; 
    VkPipelineLayout pipelineLayout;
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
    std::vector<VkFramebuffer> swapChainFramebuffers; 

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

    /// <summary>
    /// Definition of vulkan main loop
    /// </summary>
    void mainLoop();

    /// <summary>
    /// Vulkan requires that explicitly created objects be destroyed as these will not be destroyed automatically. This handles that step. 
    /// </summary>
    void cleanup();

    /// <summary>
    /// Set up vulkan
    /// </summary>
    void initVulkan(); 

    /// <summary>
    /// Create a swap chain that will be used in rendering images
    /// </summary>
    void createSwapChain(); 

    /// <summary>
    /// Create the window structure that will be used to contain images from the swapchain
    /// </summary>
    void initWindow();

    /// <summary>
    /// Create a vulkan surface that will be placed onto the window object.
    /// </summary>
    void createSurface(); 

    /// <summary>
    /// Check if validation layers are supported and create the layers if needed. Will create layers for debugging builds only.
    /// </summary>
    /// <returns></returns>
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
    /// 'Image View': describes how to access an image and what part of an image to access
    /// </summary>
    void createImageViews(); 

    /// <summary>
    /// Create a graphics pipeline to handle the needs for the application with the vertex and fragment shaders. The pipeline is immutable so it must be created if any changes are needed.
    /// </summary>
    void createGraphicsPipeline(); 

    /// <summary>
    /// Create a shader module from bytecode. The shader module is a wrapper around the shader code with function definitions. 
    /// </summary>
    /// <param name="code">bytecode for the shader program</param>
    /// <returns></returns>
    VkShaderModule createShaderModule(const std::vector<char>& code); 

    /// <summary>
    /// Create a rendering pass object which will tell vulkan information about framebuffer attachments:
    /// number of color and depth buffers, how many samples to use for each, how to handle contents
    /// </summary>
    void createRenderPass(); 
    
    /// <summary>
    /// Create framebuffers that will hold representations of the images in the swapchain
    /// </summary>
    void createFramebuffers(); 

    static std::vector<char> readFile(const std::string& filename) {
        std::ifstream file(filename, std::ios::ate | std::ios::binary);

        if (!file.is_open()) {
            throw std::runtime_error("failed to open file"); 
        }

        //get the size of the file
        size_t fileSize = (size_t)file.tellg(); 
        std::vector<char> buffer(fileSize); 

        //move back to beginning of file to actually begin reading data
        file.seekg(0);
        file.read(buffer.data(), fileSize); 

        //cleanup 
        file.close(); 
        return buffer; 
    }
#pragma region Unused Functions
    //VkPipelineColorBlendAttachmentState createAlphaColorBlending();
#pragma endregion
};

