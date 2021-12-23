#pragma once
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>

#include <iostream>
#include <fstream>
#include <stdexcept>
#include <algorithm>
#include <vector>
#include <cstring>
#include <cstdlib>
#include <cstdint>
#include <array>
#include <optional>
#include <set>

const uint32_t WIDTH = 800;
const uint32_t HEIGHT = 600;


class HelloTriangleApplication
{

public:

    void run(); 

private:
    struct Vertex {
        glm::vec2 pos; 
        glm::vec3 color; 

        /// <summary>
        /// Generates VkVertexInputBindingDescription from vertex object. This describes at which rate to load data from memory throughout the verticies. 
        /// Such as: number of bytes between data entries or if should move the next data entry after each vertex or after each instance
        /// </summary>
        /// <returns></returns>
        static VkVertexInputBindingDescription getBindingDescription() {
            VkVertexInputBindingDescription bindingDescription{}; 

            //all vertex data is in one array, so only using one binding 
            bindingDescription.binding = 0; //specifies index of the binding in the array of bindings

            //number of bytes from one entry to the next
            bindingDescription.stride = sizeof(Vertex); 

            //can have one of the following: 
                //1. VK_VERTEX_INPUT_RATE_VERTEX: move to the next data entry after each vertex
                //2. VK_VERTEX_INPUT_RATE_INSTANCE: move to the next data entry after each instance
            //not using instanced rendering so per-vertex data
            bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX; 

            return bindingDescription; 
        }

        /// <summary>
        /// Generates attribute data for the verticies. VkVertexInputAttributeDescriptions describes to vulkan how to extract a vertex attribute froma chunk of 
        /// vertex data originating from a binding descritpion. For this program, there are 2: position and color. 
        /// </summary>
        /// <returns>Array containing attribute descriptions</returns>
        static std::array<VkVertexInputAttributeDescription, 2> getAttributeDescriptions() {
            std::array<VkVertexInputAttributeDescription, 2> attributeDescriptions{}; 

            /* Struct */
                //1. binding - which binding the per-vertex data comes in 
                //2. location - references the location directive of the input in the vertex shader 
                //3. format - describes type of data 
                    // common shader and formats used 
                    // float : VK_FORMAT_R32_SFLOAT 
                    // vec2  : VK_FORMAT_R32G32_SFLOAT
                    // vec3  : VK_FORMAT_R32G32B32_SFLOAT
                    // vec4  : VK_FORMAT_R32G32B32A32_SFLOAT
                        //more odd examples
                            // ivec2 : VK_FORMAT_R32G32_SINT -- 2 component vector of 32-bit signed integers
                            // uvec4 : VK_FORMAT_R32G32B32A32_UINT -- 4 component vector of 32-bit unsigned integers 
                            // double: VK_FORMAT_R64_SFLOAT -- double precision 64-bit float 
                //4. offset - specifies the number of bytes since the start of the per-vertex data to read from
            attributeDescriptions[0].binding = 0;
            attributeDescriptions[0].location = 0;
            attributeDescriptions[0].format = VK_FORMAT_R32G32_SFLOAT;

            //the binding is loading over vertex at a time and the position attribute is at an offset of 0 bytes from the beginning of the struct.
            //offset macro calculates this distance for us
            attributeDescriptions[0].offset = offsetof(Vertex, pos);

            attributeDescriptions[1].binding = 0;
            attributeDescriptions[1].location = 1;
            attributeDescriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;
            attributeDescriptions[1].offset = offsetof(Vertex, color);

            return attributeDescriptions;
        }
    };

    //both vertex and vert attribute data is contained in one array of verticies == 'interleaving vertex' attributes
    //const std::vector<Vertex> vertices = {
    //    {{0.0f, -0.5f}, {1.0f, 0.0f, 0.0f}},
    //    {{0.5f, 0.5f}, {0.0f, 1.0f, 0.0f}},
    //    {{-0.5f, 0.5f}, {0.0f, 0.0f, 1.0f}}
    //}; 

    const std::vector<Vertex> vertices = {
    {{0.0f, -0.5f}, {1.0f, 1.0f, 1.0f}},
    {{0.5f, 0.5f}, {0.0f, 1.0f, 0.0f}},
    {{-0.5f, 0.5f}, {0.0f, 0.0f, 1.0f}}
    };

    struct QueueFamilyIndices {
        std::optional<uint32_t> graphicsFamily;
        std::optional<uint32_t> presentFamily;

        bool isComplete() {
            return graphicsFamily.has_value() && presentFamily.has_value();
        }
    };

    const uint32_t WIDTH = 800;
    const uint32_t HEIGHT = 600;

    bool frameBufferResized = false; //explicit declaration of resize, used if driver does not trigger VK_ERROR_OUT_OF_DATE

    //how many frames will be sent through the pipeline
    const int MAX_FRAMES_IN_FLIGHT = 2; 

    //tracker for which frame is being processed of the available permitted frames
    size_t currentFrame = 0; 

    //Sync obj storage 
    std::vector<VkSemaphore> imageAvailableSemaphores; 
    std::vector<VkSemaphore> renderFinishedSemaphores; 

    //vulkan command storage
    VkCommandPool commandPool;
    std::vector<VkCommandBuffer> commandBuffers; 

    //buffer and memory information storage
    VkBuffer vertexBuffer;
    VkDeviceMemory vertexBufferMemory; 

    //pipeline and dependency storage
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
    std::vector<VkFence> inFlightFences; 
    std::vector<VkFence> imagesInFlight; 

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
    /// Make the calls to draw a frame 
    /// </summary>
    void drawFrame();

    /// <summary>
    /// Vulkan requires that explicitly created objects be destroyed as these will not be destroyed automatically. This handles that step. 
    /// </summary>
    void cleanup();

    /// <summary>
    /// Clean up objects used by the swap chain.
    /// </summary>
    void cleanupSwapChain(); 

    /// <summary>
    /// Set up vulkan
    /// </summary>
    void initVulkan(); 

    /// <summary>
    /// Create a swap chain that will be used in rendering images
    /// </summary>
    void createSwapChain(); 

    /// <summary>
    /// If the swapchain is no longer compatible, it must be recreated.
    /// </summary>
    void recreateSwapChain(); 

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
    
    /// <summary>
    /// Check if the given device supports required extensions.
    /// </summary>
    bool checkDeviceExtensionSupport(VkPhysicalDevice device); 

    /// <summary>
    /// Find what queues are available for the device
    /// Queues support different types of commands such as : processing compute commands or memory transfer commands
    /// </summary>  
    QueueFamilyIndices findQueueFamilies(VkPhysicalDevice device);

    //Create a logical device to communicate with the physical device 
    void createLogicalDevice(); 

    /// <summary>
    /// Query the GPU for the proper memory type that matches properties defined in passed arguments. 
    /// </summary>
    /// <param name="typeFilter">Which bit field of memory types that are suitable</param>
    /// <param name="properties"></param>
    /// <returns></returns>
    uint32_t findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties); 

    /// <summary>
    /// Request specific details about swap chain support for a given device
    /// </summary>
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

    /// <summary>
    /// Create command pools which will contain all predefined draw commands for later use in vulkan main loop
    /// </summary>
    void createCommandPool(); 

    /// <summary>
    /// Allocate and record the commands for each swapchain image
    /// </summary>
    void createCommandBuffers(); 

    /// <summary>
    /// Create semaphores that are going to be used to sync rendering and presentation queues
    /// </summary>
    void createSemaphores(); 

    /// <summary>
    /// Fences are needed for CPU-GPU sync. Creates these required objects
    /// </summary>
    void createFences(); 

    /// <summary>
    /// Create tracking information in order to link fences with the swap chain images using 
    /// </summary>
    void createFenceImageTracking();

    /// <summary>
    /// Create a vertex buffer to hold the vertex information that will be passed to the GPU. 
    /// </summary>
    void createVertexBuffer(); 

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

    /// <summary>
    /// Callback function that is called by GLFW when the window is resized
    /// </summary>
    static void framebufferResizeCallback(GLFWwindow* window, int width, int height) {
        //retreive the pointer to the instance of the class that was created in initWindow() -> glfwSetWindowUserPointer
        auto app = reinterpret_cast<HelloTriangleApplication*>(glfwGetWindowUserPointer(window)); 
        app->frameBufferResized = true; 
    }

#pragma region Unused Functions
    //VkPipelineColorBlendAttachmentState createAlphaColorBlending();
#pragma endregion
};

