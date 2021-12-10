#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <iostream>
#include <stdexcept>
#include <cstdlib>

#include <vector>
#include <optional>

const uint32_t WIDTH = 800;
const uint32_t HEIGHT = 600;

class HelloTriangleApplication {
public:
    void run() {
        initWindow();
        initVulkan();
        mainLoop();
        cleanup();
    }

private:
    VkQueue graphicsQueue; 
    GLFWwindow* window;
    VkInstance instance;
    VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
    VkDevice device; 

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

    struct QueueFamilyIndices {
        std::optional<uint32_t> graphicsFamily;

        bool isComplete() {
            return graphicsFamily.has_value();
        }
    }; 


    void initVulkan() {
        createInstance();
        pickPhysicalDevice(); 
        createLogicalDevice(); 
    }

    void initWindow() {
        //actually make sure to init glfw
        glfwInit();
        //tell GLFW to create a window but to not include a openGL instance as this is a default behavior
        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
        //disable resizing functionality in glfw as this will not be handled in the first tutorial
        glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

        //create a window, 3rd argument allows selection of monitor, 4th argument only applies to openGL
        window = glfwCreateWindow(WIDTH, HEIGHT, "Vulkan", nullptr, nullptr);
    }

    void mainLoop() {
        while (!glfwWindowShouldClose(window)) {
            glfwPollEvents();
        }
    }

    void cleanup() {
        vkDestroyDevice(device, nullptr); 
        vkDestroyInstance(instance, nullptr);
        glfwDestroyWindow(window);

        glfwTerminate();
    }

    bool checkValidationLayerSupport() {
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

    /*
        Create an instance of a vulkan object
    */
    void createInstance() {
        uint32_t glfwExtensionCount = 0;
        uint32_t extensionCount = 0;
        uint32_t glfwRequiredExtensionEnum = 0;

        if (enableValidationLayers && !checkValidationLayerSupport()) {
            throw std::runtime_error("validation layers requested, but not available");
        }

        //get required vulkan extensions from glfw
        const char** glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);
        std::vector<VkExtensionProperties> requiredExtensions(**glfwExtensions);
        //enumerate required extensions
        vkEnumerateInstanceExtensionProperties(nullptr, &glfwExtensionCount, requiredExtensions.data());

        //get a count of the number of supported extensions on the system
        //first argument is a filter for type -- leaving null to get all 
        vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, nullptr);
        std::vector<VkExtensionProperties> extensions(extensionCount);
        //query the extension details
        vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, extensions.data());

        int foundExtensions = 0;
        for (const auto& extension : requiredExtensions) {
            bool found = false;
            for (const auto& availableExtension : extensions) {
                if (found) {
                    foundExtensions++;
                    break;
                }
                found = ((*extension.extensionName == *availableExtension.extensionName) && (extension.specVersion == availableExtension.specVersion));
            }
        }
        if (foundExtensions != glfwExtensionCount) {
            throw std::runtime_error("Not all required extensions found for glfw");
        }

        VkApplicationInfo appInfo{};
        appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
        appInfo.pApplicationName = "Hello Triangle";
        appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
        appInfo.pEngineName = "No Engine";
        appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
        appInfo.apiVersion = VK_API_VERSION_1_0;

        VkInstanceCreateInfo createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
        createInfo.pApplicationInfo = &appInfo;
        createInfo.enabledExtensionCount = glfwExtensionCount;
        createInfo.ppEnabledExtensionNames = glfwExtensions;
        createInfo.enabledLayerCount = 0;
        if (enableValidationLayers) {
            createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
            createInfo.ppEnabledLayerNames = validationLayers.data();
        }
        else {
            createInfo.enabledLayerCount = 0;
        }

        /*
        All vulkan objects follow this pattern of creation :
        1.pointer to a struct with creation info
            2.pointer to custom allocator callbacks, (nullptr) here
            3.pointer to the variable that stores the handle to the new object
        */
        VkResult result = vkCreateInstance(&createInfo, nullptr, &instance);
        if (result != VK_SUCCESS) {
            throw std::runtime_error("failed to create instance!");
        }
    }

    void pickPhysicalDevice() {
        uint32_t deviceCount = 0; 
        vkEnumeratePhysicalDevices(instance, &deviceCount, nullptr); 
        if (deviceCount == 0) {
            throw std::runtime_error("failed to find GPUs with Vulkan support!"); 
        }

        std::vector<VkPhysicalDevice> devices(deviceCount); 
        vkEnumeratePhysicalDevices(instance, &deviceCount, devices.data()); 
        
        //check devices and see if they are suitable for use
        for (const auto& device : devices) {
            if (isDeviceSuitable(device)) {
                physicalDevice = device; 
                break; 
            }
        }
        
        if (physicalDevice == VK_NULL_HANDLE) {
            throw std::runtime_error("failed to find suitable GPU!"); 
        }
    }


    //Check if the given physical device is suitable for vulkan use
    bool isDeviceSuitable(VkPhysicalDevice device) {
        /*
            Method of querying specific information about a device and checking if that device features support for a geometryShader
            VkPhysicalDeviceProperties deviceProperties; 
            VkPhysicalDeviceFeatures deviceFeatures; 

            vkGetPhysicalDeviceProperties(device, &deviceProperties); 
            vkGetPhysicalDeviceFeatures(device, &deviceFeatures); 

            return deviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU && deviceFeatures.geometryShader;
        */

        QueueFamilyIndices indicies = findQueueFamilies(device); 
        return indicies.isComplete(); 

    }

    /*
        Find what queues are available for the device 
        Queues support different types of commands such as: processing compute commands or memory transfer commands
    */
    QueueFamilyIndices findQueueFamilies(VkPhysicalDevice device) {
        uint32_t queueFamilyCount = 0; 
        QueueFamilyIndices indicies; 
        vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr); 

        std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount); 
        vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilies.data()); 

        //need to find a graphicsQueue that supports VK_QUEUE_GRAPHICS_BIT 
        int i = 0; 
        for (const auto& queueFamily : queueFamilies) {
            if (queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT) {
                indicies.graphicsFamily = i; 
            }
            i++; 
        }

        return indicies; 
    }

    //Create a logical device to communicate with the physical device 
    void createLogicalDevice() {
        float queuePrioriy = 1.0f;
        QueueFamilyIndices indicies = findQueueFamilies(physicalDevice); 

        //create a struct to contain the information required 
        //create a queue with graphics capabilities
        VkDeviceQueueCreateInfo  queueCreateInfo{}; 
        queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO; 
        queueCreateInfo.queueFamilyIndex = indicies.graphicsFamily.value(); 

        //most drivers support only a few queue per queueFamily 
        queueCreateInfo.queueCount = 1; 
        queueCreateInfo.pQueuePriorities = &queuePrioriy; 

        //specifying device features that we want to use -- can pull any of the device features that was queried before...for now use nothing
        VkPhysicalDeviceFeatures deviceFeatures{};

        //Create actual logical device
        VkDeviceCreateInfo createInfo{}; 
        createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO; 
        createInfo.pQueueCreateInfos = &queueCreateInfo; 
        createInfo.queueCreateInfoCount = 1; 
        createInfo.pEnabledFeatures = &deviceFeatures; 

        //specify specific instance info but it is device specific this time
        createInfo.enabledExtensionCount = 0; 

        if (enableValidationLayers) {
            createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size()); 
            createInfo.ppEnabledLayerNames = validationLayers.data(); 
        }
        else {
            createInfo.enabledLayerCount = 0; 
        }

        //call to create the logical device 
        if (vkCreateDevice(physicalDevice, &createInfo, nullptr, &device) != VK_SUCCESS) {
            throw std::runtime_error("failed to create logical device"); 
        }

        vkGetDeviceQueue(device, indicies.graphicsFamily.value(), 0, &graphicsQueue); 
    }
};

int main() {
    HelloTriangleApplication app;

    try {
        app.run();
    }
    catch (const std::exception& e) {
        std::cerr << e.what() << std::endl;
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}