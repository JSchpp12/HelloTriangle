#include "HelloTriangleApplication.h"

void HelloTriangleApplication::mainLoop() {
    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();
        drawFrame(); 
    }

    //wait until the device is complete with all drawing operations before cleanup
    vkDeviceWaitIdle(device); 
}

void HelloTriangleApplication::drawFrame() {
    /* Goals of each call to drawFrame: 
    *   get an image from the swap chain
    *   execute command buffer with that image as attachment in the framebuffer
    *   return the image to swapchain for presentation 
    * by default each of these steps would be executed asynchronously so need method of synchronizing calls with rendering
    * two ways of doing this: 
    *   1. fences
    *       accessed through calls to vkWaitForFences
    *       designed to synchronize appliecation itself with rendering ops 
    *   2. semaphores 
    *       designed to synchronize opertaions within or across command queues 
    * need to sync queu operations of draw and presentation commmands -> using semaphores
    */ 

    //wait for fence to be ready 
    // 3. 'VK_TRUE' -> waiting for all fences
    // 4. timeout 
    vkWaitForFences(device, 1, &inFlightFences[currentFrame], VK_TRUE, UINT64_MAX);

    VkResult result; //swapchain status

    /* Get Image From Swapchain */
    uint32_t imageIndex;

    //as is extension we must use vk*KHR naming convention
    //UINT64_MAX -> 3rd argument: used to specify timeout in nanoseconds for image to become available
    /* Suboptimal SwapChain notes */
        //vulkan can return two different flags 
        // 1. VK_ERROR_OUT_OF_DATE_KHR: swap chain has become incompatible with the surface and cant be used for rendering. (Window resize)
        // 2. VK_SUBOPTIMAL_KHR: swap chain can still be used to present to the surface, but the surface properties no longer match
    result = vkAcquireNextImageKHR(device, swapChain, UINT64_MAX, imageAvailableSemaphores[currentFrame], VK_NULL_HANDLE, &imageIndex);
    if (result == VK_ERROR_OUT_OF_DATE_KHR) {
        //the swapchain is no longer optimal according to vulkan. Must recreate a more efficient swap chain
        recreateSwapChain(); 
        return; 
    }
    else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
        //for VK_SUBOPTIMAL_KHR can also recreate swap chain. However, chose to continue to presentation stage
        throw std::runtime_error("failed to aquire swap chain image");
    }

    //check if a previous frame is using the current image
    if (imagesInFlight[imageIndex] != VK_NULL_HANDLE) {
        vkWaitForFences(device, 1, &imagesInFlight[imageIndex], VK_TRUE, UINT64_MAX); 
    }
    //mark image as now being in use by this frame
    imagesInFlight[imageIndex] = inFlightFences[currentFrame]; 

    /* Command Buffer */
    VkSubmitInfo submitInfo{}; 
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO; 

    VkSemaphore waitSemaphores[] = { imageAvailableSemaphores[currentFrame]};

    //where in the pipeline should the wait happen, want to wait until image becomes available
    //wait at stage of color attachment -> theoretically allows for shader execution before wait 
    VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT }; //each entry corresponds through index to waitSemaphores[]
    submitInfo.waitSemaphoreCount = 1; 
    submitInfo.pWaitSemaphores = waitSemaphores; 
    submitInfo.pWaitDstStageMask = waitStages; 

    //which command buffers to submit for execution -- should submit command buffer that binds the swap chain image that was just acquired as color attachment
    submitInfo.commandBufferCount = 1; 
    submitInfo.pCommandBuffers = &commandBuffers[imageIndex]; 

    //what semaphores to signal when command buffers have finished
    VkSemaphore signalSemaphores[] = { renderFinishedSemaphores[currentFrame]};

    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores = signalSemaphores;

    //set fence to unsignaled state
    vkResetFences(device, 1, &inFlightFences[currentFrame]);

    if (vkQueueSubmit(graphicsQueue, 1, &submitInfo, inFlightFences[currentFrame]) != VK_SUCCESS) {
        throw std::runtime_error("failed to submit draw command buffer"); 
    }

    /* Presentation */
    VkPresentInfoKHR presentInfo{};
    presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;

    //what to wait for 
    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pWaitSemaphores = signalSemaphores;

    //what swapchains to present images to 
    VkSwapchainKHR swapChains[] = { swapChain };
    presentInfo.swapchainCount = 1;
    presentInfo.pSwapchains = swapChains;
    presentInfo.pImageIndices = &imageIndex;

    //can use this to get results from swap chain to check if presentation was successful
    presentInfo.pResults = nullptr; // Optional

    //make call to present image
    result = vkQueuePresentKHR(presentQueue, &presentInfo);

    if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || frameBufferResized) {
        frameBufferResized = false; 
        recreateSwapChain(); 
    }
    else if (result != VK_SUCCESS) {
        throw std::runtime_error("failed to present swap chain image");
    }

    //advance to next frame
    currentFrame = (currentFrame + 1) % MAX_FRAMES_IN_FLIGHT; 
}

void HelloTriangleApplication::cleanup() {
    cleanupSwapChain(); 

    vkDestroyBuffer(device, vertexBuffer, nullptr); 
    vkFreeMemory(device, vertexBufferMemory, nullptr); 

    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        vkDestroySemaphore(device, renderFinishedSemaphores[i], nullptr);
        vkDestroySemaphore(device, imageAvailableSemaphores[i], nullptr);
        vkDestroyFence(device, inFlightFences[i], nullptr);
    }

    vkDestroyCommandPool(device, commandPool, nullptr); 
    
    vkDestroyDevice(device, nullptr);

    vkDestroySurfaceKHR(instance, surface, nullptr);
    vkDestroyInstance(instance, nullptr);
    glfwDestroyWindow(window);

    glfwTerminate();
}

void HelloTriangleApplication::cleanupSwapChain() {
    //delete framebuffers 
    for (auto framebuffer : swapChainFramebuffers) {
        vkDestroyFramebuffer(device, framebuffer, nullptr);
    }

    vkFreeCommandBuffers(device, commandPool, static_cast<uint32_t>(commandBuffers.size()), commandBuffers.data()); 

    vkDestroyPipeline(device, graphicsPipeline, nullptr);
    vkDestroyPipelineLayout(device, pipelineLayout, nullptr);
    vkDestroyRenderPass(device, renderPass, nullptr);

    //destroy image views 
    for (auto imageView : swapChainImageViews) {
        vkDestroyImageView(device, imageView, nullptr);
    }

    vkDestroySwapchainKHR(device, swapChain, nullptr);
}

void HelloTriangleApplication::run() {
    initWindow();
    initVulkan();
    mainLoop();
    cleanup();
}

void HelloTriangleApplication::initVulkan() {
    createInstance();
    createSurface();
    pickPhysicalDevice();
    createLogicalDevice();
    createSwapChain();
    createImageViews(); 
    createRenderPass(); 
    createGraphicsPipeline(); 
    createFramebuffers(); 
    createCommandPool(); 
    createVertexBuffer();
    createCommandBuffers(); 
    createSemaphores(); 
    createFences(); 
    createFenceImageTracking();
    std::cout << "Finished Vulcan Init \n";
}

void HelloTriangleApplication::createSurface() {
    if (glfwCreateWindowSurface(instance, window, nullptr, &surface) != VK_SUCCESS) {
        throw std::runtime_error("failed to create window surface");
    }
}

void HelloTriangleApplication::initWindow() {
    //actually make sure to init glfw
    glfwInit();
    //tell GLFW to create a window but to not include a openGL instance as this is a default behavior
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    //disable resizing functionality in glfw as this will not be handled in the first tutorial
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

    //create a window, 3rd argument allows selection of monitor, 4th argument only applies to openGL
    window = glfwCreateWindow(WIDTH, HEIGHT, "Vulkan", nullptr, nullptr);

    //need to give GLFW a pointer to current instance of this class
    glfwSetWindowUserPointer(window, this); 

    glfwSetFramebufferSizeCallback(window, framebufferResizeCallback); 
}

void HelloTriangleApplication::createSwapChain() {
    //TODO: current implementation requires halting to all rendering when recreating swapchain. Can place old swap chain in oldSwapChain field 
    //  in order to prevent this and allow rendering to continue
    SwapChainSupportDetails swapChainSupport = querySwapChainSupport(physicalDevice);

    VkSurfaceFormatKHR surfaceFormat = chooseSwapSurfaceFormat(swapChainSupport.formats);
    VkPresentModeKHR presentMode = chooseSwapPresentMode(swapChainSupport.presentModes);
    VkExtent2D extent = chooseSwapExtent(swapChainSupport.capabilities);

    //how many images should be in the swap chain 
    //in order to avoid extra waiting for driver overhead, author of tutorial recommends +1 of the minimum
    uint32_t imageCount = swapChainSupport.capabilities.minImageCount + 1;

    //with this additional +1 make sure not to go over maximum permitted 
    if (swapChainSupport.capabilities.maxImageCount > 0 && imageCount > swapChainSupport.capabilities.maxImageCount) {
        imageCount = swapChainSupport.capabilities.maxImageCount;
    }

    VkSwapchainCreateInfoKHR createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    createInfo.surface = surface;

    //specify image information for the surface 
    createInfo.minImageCount = imageCount;
    createInfo.imageFormat = surfaceFormat.format;
    createInfo.imageColorSpace = surfaceFormat.colorSpace;
    createInfo.imageExtent = extent;
    createInfo.imageArrayLayers = 1; //1 unless using 3D display 
    createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT; //how are these images going to be used? Color attachment since we are rendering to them (can change for postprocessing effects)

    QueueFamilyIndices indicies = findQueueFamilies(physicalDevice);
    uint32_t queueFamilyIndicies[] = { indicies.graphicsFamily.value(), indicies.presentFamily.value() };

    if (indicies.graphicsFamily != indicies.presentFamily) {
        /*need to handle how images will be transferred between different queues
        * so we need to draw images on the graphics queue and then submitting them to the presentation queue
        * Two ways of handling this:
        * 1. VK_SHARING_MODE_EXCLUSIVE: an image is owned by one queue family at a time and can be transferred between groups
        * 2. VK_SHARING_MODE_CONCURRENT: images can be used across queue families without explicit ownership
        */
        createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
        createInfo.queueFamilyIndexCount = 2;
        createInfo.pQueueFamilyIndices = queueFamilyIndicies;
    }
    else {
        //same family is used for graphics and presenting
        createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
        createInfo.queueFamilyIndexCount = 0; //optional
        createInfo.pQueueFamilyIndices = nullptr; //optional
    }

    //can specify certain transforms if they are supported (like 90 degree clockwise rotation)
    createInfo.preTransform = swapChainSupport.capabilities.currentTransform;
    createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;

    //what present mode is going to be used
    createInfo.presentMode = presentMode;
    //if clipped is set to true, we dont care about color of pixes that arent in sight -- best performance to enable this
    createInfo.clipped = VK_TRUE;

    //for now, only assume we are making one swapchain
    createInfo.oldSwapchain = VK_NULL_HANDLE;

    if (vkCreateSwapchainKHR(device, &createInfo, nullptr, &swapChain) != VK_SUCCESS) {
        throw std::runtime_error("failed to create swap chain");
    }

    //get images in the newly created swapchain 
    vkGetSwapchainImagesKHR(device, swapChain, &imageCount, nullptr);
    swapChainImages.resize(imageCount);
    vkGetSwapchainImagesKHR(device, swapChain, &imageCount, swapChainImages.data());

    //save swapChain information for later use
    swapChainImageFormat = surfaceFormat.format;
    swapChainExtent = extent;
}

void HelloTriangleApplication::recreateSwapChain() {
    int width = 0, height = 0; 
    //check for window minimization and wait for window size to no longer be 0
    glfwGetFramebufferSize(window, &width, &height); 
    while (width == 0 || height == 0) {
        glfwGetFramebufferSize(window, &width, &height); 
        glfwWaitEvents(); 
    }
    //wait for device to finish any current actions
    vkDeviceWaitIdle(device); 

    cleanupSwapChain(); 

    //create swap chain itself 
    createSwapChain(); 

    //image views depend directly on swap chain images so these need to be recreated
    createImageViews(); 

    //render pass depends on the format of swap chain images
    createRenderPass(); 

    //viewport and scissor rectangle size are declared during pipeline creation, so the pipeline must be recreated
    //can use dynamic states for viewport and scissor to avoid this 
    createGraphicsPipeline(); 

    createFramebuffers(); 
    createCommandBuffers(); 
}

bool HelloTriangleApplication::checkValidationLayerSupport() {
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

void HelloTriangleApplication::createInstance() {
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

void HelloTriangleApplication::pickPhysicalDevice() {
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
bool HelloTriangleApplication::isDeviceSuitable(VkPhysicalDevice device) {
    /*
    Method of querying specific information about a device and checking if that device features support for a geometryShader
    VkPhysicalDeviceProperties deviceProperties;
    VkPhysicalDeviceFeatures deviceFeatures;

    vkGetPhysicalDeviceProperties(device, &deviceProperties);
    vkGetPhysicalDeviceFeatures(device, &deviceFeatures);

    return deviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU && deviceFeatures.geometryShader;
*/
    bool swapChainAdequate = false;
    QueueFamilyIndices indicies = findQueueFamilies(device);
    bool extensionsSupported = checkDeviceExtensionSupport(device);
    if (extensionsSupported) {
        SwapChainSupportDetails swapChainSupport = querySwapChainSupport(device);
        swapChainAdequate = !swapChainSupport.formats.empty() && !swapChainSupport.presentModes.empty();
    }
    return indicies.isComplete() && extensionsSupported && swapChainAdequate;
}

bool HelloTriangleApplication::checkDeviceExtensionSupport(VkPhysicalDevice device) {
    uint32_t extensionCount;
    vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, nullptr);
    std::vector<VkExtensionProperties> availableExtensions(extensionCount);
    vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, availableExtensions.data());

    std::set<std::string> requiredExtensions(deviceExtensions.begin(), deviceExtensions.end());

    //iterate through extensions looking for those that are required
    for (const auto& extension : availableExtensions) {
        requiredExtensions.erase(extension.extensionName);
    }

    return requiredExtensions.empty();
}

HelloTriangleApplication::QueueFamilyIndices HelloTriangleApplication::findQueueFamilies(VkPhysicalDevice device) {
    uint32_t queueFamilyCount = 0;
    QueueFamilyIndices indicies;
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);

    std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilies.data());

    //need to find a graphicsQueue that supports VK_QUEUE_GRAPHICS_BIT 
    int i = 0;
    for (const auto& queueFamily : queueFamilies) {
        VkBool32 presentSupport = false;
        vkGetPhysicalDeviceSurfaceSupportKHR(device, i, surface, &presentSupport);

        //pick the family that supports presenting to the display 
        if (presentSupport) {
            indicies.presentFamily = i;
        }
        //pick family that has graphics support
        if (queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT) {
            indicies.graphicsFamily = i;
        }

        //--COULD DO :: pick a device that supports both of these in the same queue for increased performance--
        i++;
    }

    return indicies;
}

void HelloTriangleApplication::createLogicalDevice() {
    float queuePrioriy = 1.0f;
    QueueFamilyIndices indicies = findQueueFamilies(physicalDevice);

    //need multiple structs since we now have a seperate family for presenting and graphics 
    std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
    std::set<uint32_t> uniqueQueueFamilies = { indicies.graphicsFamily.value(), indicies.presentFamily.value() };

    for (uint32_t queueFamily : uniqueQueueFamilies) {
        //create a struct to contain the information required 
        //create a queue with graphics capabilities
        VkDeviceQueueCreateInfo  queueCreateInfo{};
        queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        queueCreateInfo.queueFamilyIndex = queueFamily;
        //most drivers support only a few queue per queueFamily 
        queueCreateInfo.queueCount = 1;
        queueCreateInfo.pQueuePriorities = &queuePrioriy;
        queueCreateInfos.push_back(queueCreateInfo);
    }

    //specifying device features that we want to use -- can pull any of the device features that was queried before...for now use nothing
    VkPhysicalDeviceFeatures deviceFeatures{};

    //Create actual logical device
    VkDeviceCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    createInfo.pQueueCreateInfos = queueCreateInfos.data();
    createInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());
    createInfo.pEnabledFeatures = &deviceFeatures;

    //specify specific instance info but it is device specific this time
    createInfo.enabledExtensionCount = static_cast<uint32_t>(deviceExtensions.size());
    createInfo.ppEnabledExtensionNames = deviceExtensions.data();

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
    vkGetDeviceQueue(device, indicies.presentFamily.value(), 0, &presentQueue);
}

uint32_t HelloTriangleApplication::findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties)
{
    //query available memory -- right now only concerned with memory type, not the heap that it comes from
    /*VkPhysicalDeviceMemoryProperties contains: 
        1. memoryTypes 
        2. memoryHeaps - distinct memory resources (dedicated VRAM or swap space)
    */
    VkPhysicalDeviceMemoryProperties memProperties;
    vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memProperties); 

    for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++) {
        //use binary AND to test each bit (Left Shift)
        //check memory types array for more detailed information on memory capabilities
            //we need to be able to write to memory, so speficially looking to be able to MAP to the memory to write to it from the CPU -- VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT
        //also need VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
                
        if ((typeFilter & (1 << i)) && (memProperties.memoryTypes[i].propertyFlags & properties) == properties) {
            return i; 
        }
    }

    throw std::runtime_error("failed to find suitable memory type"); 
}

HelloTriangleApplication::SwapChainSupportDetails HelloTriangleApplication::querySwapChainSupport(VkPhysicalDevice device) {
    SwapChainSupportDetails details;
    uint32_t formatCount, presentModeCount;

    //get surface capabilities
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, surface, &details.capabilities);

    vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, nullptr);
    vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &presentModeCount, nullptr);

    if (formatCount != 0) {
        //resize vector in order to hold all available formats
        details.formats.resize(formatCount);
        vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, details.formats.data());
    }

    if (presentModeCount != 0) {
        //resize for same reasons as format 
        details.presentModes.resize(presentModeCount);
        vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentModeCount, details.presentModes.data());
    }

    return details;
}

VkSurfaceFormatKHR HelloTriangleApplication::chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats) {
    for (const auto& availableFormat : availableFormats) {
        //check if a format allows 8 bits for R,G,B, and alpha channel
        //use SRGB color space
        if (availableFormat.format == VK_FORMAT_B8G8R8A8_SRGB && availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
            return availableFormat;
        }
    }

    //if nothing matches what we are looking for, just take what is available
    return availableFormats[0];
}

VkPresentModeKHR HelloTriangleApplication::chooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes) {
    /*
    * There are a number of swap modes that are in vulkan
    * 1. VK_PRESENT_MODE_IMMEDIATE_KHR: images submitted by application are sent to the screen right away -- can cause tearing
    * 2. VK_PRESENT_MODE_FIFO_RELAXED_KHR: images are placed in a queue and images are sent to the display in time with display refresh (VSYNC like). If queue is full, application has to wait
    *   "Vertical blank" -> time when the display is refreshed
    * 3. VK_PRESENT_MODE_FIFO_RELAXED_KHR: same as above. Except if the application is late, and the queue is empty: the next image submitted is sent to display right away instead of waiting for next blank.
    * 4. VK_PRESENT_MODE_MAILBOX_KHR: similar to #2 option. Instead of blocking applicaiton when the queue is full, the images in the queue are replaced with newer images.
    *   This mode can be used to render frames as fast as possible while still avoiding tearing. Kind of like "tripple buffering". Does not mean that framerate is unlocked however.
    *   Author of tutorial statement: this mode [4] is a good tradeoff if energy use is not a concern. On mobile devices it might be better to go with [2]
    */

    for (const auto& availablePresentMode : availablePresentModes) {
        if (availablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR) {
            return availablePresentMode;
        }
    }

    //only VK_PRESENT_MODE_FIFO_KHR is guaranteed to be available
    return VK_PRESENT_MODE_FIFO_KHR;
}

VkExtent2D HelloTriangleApplication::chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities) {
    /*
    * "swap extent" -> resolution of the swap chain images (usually the same as window resultion
    * Range of available resolutions are defined in VkSurfaceCapabilitiesKHR
    * Resultion can be changed by setting value in currentExtent to the maximum value of a uint32_t
    *   then: the resolution can be picked by matching window size in minImageExtent and maxImageExtent
    */
    if (capabilities.currentExtent.width != UINT32_MAX) {
        return capabilities.currentExtent;
    }
    else {
        //vulkan requires that resultion be defined in pixels -- if a high DPI display is used, screen coordinates do not match with pixels
        int width, height;
        glfwGetFramebufferSize(window, &width, &height);

        VkExtent2D actualExtent = {
            static_cast<uint32_t>(width),
            static_cast<uint32_t>(height)
        };

        //(clamp) -- keep the width and height bounded by the permitted resolutions 
        actualExtent.width = std::clamp(actualExtent.width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width);
        actualExtent.height = std::clamp(actualExtent.height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height);

        return actualExtent;
    }
}

void HelloTriangleApplication::createImageViews() {
    swapChainImageViews.resize(swapChainImages.size()); 

    //need to create an imageView for each of the images available
    for (size_t i = 0; i < swapChainImages.size(); i++) {
        VkImageViewCreateInfo createInfo{}; 
        createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO; 
        createInfo.image = swapChainImages[i]; 

        //specify how the image will be interpreted
        createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        createInfo.format = swapChainImageFormat; 

        //the next fields allows to swizzle RGB values -- leaving as defaults for now 
        createInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY; 
        createInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
        createInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
        createInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;

        //subresourceRange describes image purpose -- this use is color targets without any mipmapping levels or multiple layers
        createInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT; 
        createInfo.subresourceRange.baseMipLevel = 0; 
        createInfo.subresourceRange.levelCount = 1; 
        createInfo.subresourceRange.baseArrayLayer = 0; 
        createInfo.subresourceRange.layerCount = 1; 

        if (vkCreateImageView(device, &createInfo, nullptr, &swapChainImageViews[i]) != VK_SUCCESS) {
            throw std::runtime_error("failed to create image views"); 
        }

    }
}

VkShaderModule HelloTriangleApplication::createShaderModule(const std::vector<char>& code) {
    VkShaderModuleCreateInfo createInfo{}; 
    createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO; 
    createInfo.codeSize = code.size(); 
    createInfo.pCode = reinterpret_cast<const uint32_t*>(code.data()); 

    VkShaderModule shaderModule; 
    if (vkCreateShaderModule(device, &createInfo, nullptr, &shaderModule) != VK_SUCCESS) {
        throw std::runtime_error("failed to create shader module"); 
    }

    return shaderModule; 
}

void HelloTriangleApplication::createGraphicsPipeline() {
    auto fragShaderCode = readFile("fragShader.spv");
    auto vertShaderCode = readFile("vertShader.spv");

    auto bindingDescriptions = Vertex::getBindingDescription(); 
    auto attributeDescriptions = Vertex::getAttributeDescriptions(); 

    VkShaderModule vertShaderModule = createShaderModule(vertShaderCode); 
    VkShaderModule fragShaderModule = createShaderModule(fragShaderCode); 

    //assign each shader module to a specific stage of the graphics pipeline
    //vert shader first
    VkPipelineShaderStageCreateInfo vertShaderStageInfo{}; 
    vertShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO; 
    vertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT; 
    vertShaderStageInfo.module = vertShaderModule; 
    vertShaderStageInfo.pName = "main"; //the function to invoke in the shader module
    //optional member -> pSpecializationInfo: 
    //  allows specification for values to shader constants. Use a single single shader module whos function could be customized through this optional value. 
    //  if not useing, set to nullptr which is done automatically in this case with the constructor of the struct. 
    //  Additionally: it is a good choice to use this value instead of variables so that graphics driver can remove if statements if needed for optimization

    //create pipeline info for fragment shader 
    VkPipelineShaderStageCreateInfo fragShaderStageInfo{}; 
    fragShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO; 
    fragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT; 
    fragShaderStageInfo.module = fragShaderModule; 
    fragShaderStageInfo.pName = "main"; 

    //store these creation infos for later use 
    VkPipelineShaderStageCreateInfo shaderStages[] = { vertShaderStageInfo, fragShaderStageInfo }; 

    VkPipelineVertexInputStateCreateInfo vertexInputInfo{}; 
    vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO; 
    
    //pVertexBindingDescriptions and pVertexAttributeDescription -> point to arrays of structs to load vertex data
    //for now: leaving blank as the verticies are hard coded in the shaders
    vertexInputInfo.vertexBindingDescriptionCount = 1; 
    vertexInputInfo.pVertexBindingDescriptions = &bindingDescriptions; 
    vertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(attributeDescriptions.size()); 
    vertexInputInfo.pVertexAttributeDescriptions = attributeDescriptions.data(); 

    /*
    VkPipelineInputAssemblyStateCreateInfo -> Describes 2 things: 
        1.what kind of geometry will be drawn 
            described in topology member: 
                -VK_PRIMITIVE_TOPOLOGY_POINT_LIST: points from verticies
                -VK_PRIMITIVE_TOPOLOGY_LINE_LIST: line from every 2 verticies without reuse
                -VK_PRIMITIVE_TOPOLOGY_LINE_STRIP: line strip 
                -VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST: triangle from every 3 verticies without reuse
                -VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP: 2nd and 3rd vertex of every triangle are used as first two verticies of the next triangle
        2.if primitive restart should be enabled
    */
    //Verticies are normally loaded from vertex buffer in sequential order 
    //element buffer can be used to specify this information manually
    //this allows the reuse of verticies!
    VkPipelineInputAssemblyStateCreateInfo inputAssembly{}; 
    inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO; 
    inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST; 
    inputAssembly.primitiveRestartEnable = VK_FALSE; 

    /* Viewport */
    //Viewport describes the region of the framebuffer where the output will be rendered to
    VkViewport viewport{}; 
    viewport.x = 0.0f; 
    viewport.y = 0.0f; 

    viewport.width = (float)swapChainExtent.width; 
    viewport.height = (float)swapChainExtent.height;
    //Specify values range of depth values to use for the framebuffer. If not doing anything special, leave at default
    viewport.minDepth = 0.0f; 
    viewport.maxDepth = 1.0f; 

    /* Scissor */
    //this defines in which regions pixels will actually be stored. 
    //any pixels outside will be discarded 
    
    //we just want to draw the whole framebuffer for now
    VkRect2D scissor{}; 
    scissor.offset = { 0, 0 }; 
    scissor.extent = swapChainExtent; 

    //put scissor and viewport together into struct for creation 
    VkPipelineViewportStateCreateInfo viewportState{}; 
    viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO; 
    viewportState.viewportCount = 1; 
    viewportState.pViewports = &viewport; 
    viewportState.scissorCount = 1; 
    viewportState.pScissors = &scissor; 

    /* Rasterizer */
    //takes the geometry and creates fragments which are then passed onto the fragment shader 
    //also does: depth testing, face culling, and the scissor test
    VkPipelineRasterizationStateCreateInfo rasterizer{}; 
    rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    //if set to true -> fragments that are beyond near and far planes are set to those distances rather than being removed
    rasterizer.depthClampEnable = VK_FALSE; 
    
    //polygonMode determines how frags are generated. Different options: 
    //1. VK_POLYGON_MODE_FILL: fill the area of the polygon with fragments
    //2. VK_POLYGON_MODE_LINE: polygon edges are drawn as lines 
    //3. VK_POLYGON_MODE_POINT: polygon verticies are drawn as points
    //NOTE: using any other than fill, requires GPU feature
    rasterizer.polygonMode = VK_POLYGON_MODE_FILL;

    //available line widths, depend on GPU. If above 1.0f, required wideLines GPU feature
    rasterizer.lineWidth = 1.0f; //measured in fragment widths

    //cullMode : type of face culling to use.
    rasterizer.cullMode = VK_CULL_MODE_BACK_BIT; 
    rasterizer.frontFace = VK_FRONT_FACE_CLOCKWISE; 

    //depth values can be used in way that is known as 'shadow mapping'. 
    //rasterizer is capable of changing depth values through constant addition or biasing based on frags slope 
    //this is left as off for now 
    rasterizer.depthBiasEnable = VK_FALSE; 
    rasterizer.depthBiasConstantFactor = 0.0f; //optional 
    rasterizer.depthBiasClamp = 0.0f; //optional 
    rasterizer.depthBiasSlopeFactor = 0.0f; //optional

    /* Multisampling */
    //this is one of the methods of performing anti-aliasing
    //enabling requires GPU feature -- left off for this tutorial 
    VkPipelineMultisampleStateCreateInfo multisampling{}; 
    multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO; 
    multisampling.sampleShadingEnable = VK_FALSE; 
    multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT; 
    multisampling.minSampleShading = 1.0f; //optional 
    multisampling.pSampleMask = nullptr; //optional
    multisampling.alphaToCoverageEnable = VK_FALSE; //optional
    multisampling.alphaToOneEnable = VK_FALSE; //optional

    /* Depth and Stencil Testing */
    //if using depth or stencil buffer, a depth and stencil tests are neeeded
    //not needed here 

    /* Color blending */
    // after the fragShader has returned a color, it must be combined with the color already in the framebuffer
    // there are two ways to do this: 
    //      1. mix the old and new value to produce final colo r
    //      2. combine the old a new value using a bitwise operation 
    //two structs are needed to create this functionality: 
    //  1. VkPipelineColorBlendAttachmentState: configuration per attached framebuffer 
    //  2. VkPipelineColorBlendStateCreateInfo: global configuration
    //only using one framebuffer in this project -- both of these are disabled in this project
    VkPipelineColorBlendAttachmentState colorBlendAttachment{}; 
    colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT; 
    colorBlendAttachment.blendEnable = VK_FALSE; 

    VkPipelineColorBlendStateCreateInfo colorBlending{};
    colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    colorBlending.logicOpEnable = VK_FALSE;
    colorBlending.logicOp = VK_LOGIC_OP_COPY;
    colorBlending.attachmentCount = 1;
    colorBlending.pAttachments = &colorBlendAttachment;
    colorBlending.blendConstants[0] = 0.0f;
    colorBlending.blendConstants[1] = 0.0f;
    colorBlending.blendConstants[2] = 0.0f;
    colorBlending.blendConstants[3] = 0.0f;

    /* Dynamic State */
    //some parts of the pipeline can be changed without recreating the entire pipeline
    //if this is defined, the data for the dynamic structures will have to be provided at draw time
    VkDynamicState dynamicStates[] = {
        VK_DYNAMIC_STATE_VIEWPORT, 
        VK_DYNAMIC_STATE_LINE_WIDTH
    }; 
    
    VkPipelineDynamicStateCreateInfo dynamicStateInfo{}; 
    dynamicStateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    dynamicStateInfo.dynamicStateCount = 2;
    dynamicStateInfo.pDynamicStates = dynamicStates;

    /* Pipeline Layout */
    //uniform values in shaders need to be defined here 
    VkPipelineLayoutCreateInfo pipelineLayoutInfo{}; 
    pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO; 
    pipelineLayoutInfo.setLayoutCount = 0; 
    pipelineLayoutInfo.pSetLayouts = nullptr; 
    pipelineLayoutInfo.pushConstantRangeCount = 0; 
    pipelineLayoutInfo.pPushConstantRanges = nullptr; 

    if (vkCreatePipelineLayout(device, &pipelineLayoutInfo, nullptr, &pipelineLayout) != VK_SUCCESS) {
        throw std::runtime_error("failed to create pipeline layout"); 
    }

    /* Pipeline */
    VkGraphicsPipelineCreateInfo pipelineInfo{}; 
    pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO; 
    pipelineInfo.stageCount = 2; 
    pipelineInfo.pStages = shaderStages; 
    //ref all previously created structs
    pipelineInfo.pVertexInputState = &vertexInputInfo;
    pipelineInfo.pInputAssemblyState = &inputAssembly;
    pipelineInfo.pViewportState = &viewportState;
    pipelineInfo.pRasterizationState = &rasterizer;
    pipelineInfo.pMultisampleState = &multisampling;
    pipelineInfo.pDepthStencilState = nullptr; // Optional
    pipelineInfo.pColorBlendState = &colorBlending;
    pipelineInfo.pDynamicState = nullptr; // Optional
    pipelineInfo.layout = pipelineLayout;
    //render pass info 
    //  ensure renderpass is compatible with pipeline --check khronos docs
    pipelineInfo.renderPass = renderPass;
    pipelineInfo.subpass = 0; //index where the graphics pipeline will be used 
    //allow switching to new pipeline (inheritance) 
    pipelineInfo.basePipelineHandle = VK_NULL_HANDLE; // Optional -- handle to existing pipeline that is being switched to
    pipelineInfo.basePipelineIndex = -1; // Optional

    //finally creating the pipeline -- this call has the capability of creating multiple pipelines in one call
    //2nd arg is set to null -> normally for graphics pipeline cache (can be used to store and reuse data relevant to pipeline creation across multiple calls to vkCreateGraphicsPipeline)
    if (vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &graphicsPipeline) != VK_SUCCESS) {
        throw std::runtime_error("failed to create graphics pipeline"); 
    }

    //destroy the shader modules that were created 
    vkDestroyShaderModule(device, vertShaderModule, nullptr); 
    vkDestroyShaderModule(device, fragShaderModule, nullptr); 
}

void HelloTriangleApplication::createRenderPass() {
    /*  Single render pass consists of many small subpasses
        each subpasses are subsequent rendering operations that depend on the contents of framebuffers in the previous pass. 
        It is best to group these into one rendering pass, then vulkan can optimize for this in order to save memory bandwidth. 
        For this program, we are going to stick with one subpass
    */

    VkAttachmentDescription colorAttachment{}; 
    //format of color attachment needs to match the swapChain image format
    colorAttachment.format = swapChainImageFormat; 
    //no multisampling needed so leave at 1 samples
    colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT; 
    colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR; 
    colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE; 

    /* Choices for loadOp:
        1. VK_ATTACHMENT_LOAD_OP_LOAD: preserve the existing contents of the attachment 
        2. VK_ATTACHMENT_LOAD_OP_CLEAR: clear the values to a constant at the start 
        3. VK_ATTACHMENT_LOAD_OP_DONT_CARE: existing contents are undefined
    */
    //what do to with data before rendering
    colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR; 
    /* Choices for storeOp:
        1. VK_ATTACHMENT_STORE_OP_STORE: rendered contents will be stored for later use
        2. VK_ATTACHMENT_STORE_OP_DONT_CARE: contents of the frame buffer will be undefined after the rendering operation
    */
    //what to do with data after rendering
    colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE; //since we want to see the rendered triangle, we are going to store 

    /*Image layouts can change depending on what operation is being performed
    * Possible layouts are: 
    * 1. VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL: image is used as a color attachment 
    * 2. VK_IMAGE_LAYOUT_PRESENT_SRC_KHR: images to be presented in the swap chain 
    * 3. VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL: images to be used as destination for memory copy operation 
    */
    //dont care what format image is in before render - contents of image are not guaranteed to be preserved 
    colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED; 
    //want image to be ready for display 
    colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR; 

    /* Color attachment references */
    VkAttachmentReference colorAttachmentRef{}; 
    colorAttachmentRef.attachment = 0; 
    colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL; //will give best performance

    /* Subpass */
    VkSubpassDescription subpass{}; 
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS; 
    subpass.colorAttachmentCount = 1; 
    subpass.pColorAttachments = &colorAttachmentRef; 

    /* Render Pass */
    VkRenderPassCreateInfo renderPassInfo{}; 
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO; 
    renderPassInfo.attachmentCount = 1; 
    renderPassInfo.pAttachments = &colorAttachment; 
    renderPassInfo.subpassCount = 1;
    renderPassInfo.pSubpasses = &subpass; 

    /* Subpass Dependencies */
    VkSubpassDependency dependency{};
    dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
    dependency.dstSubpass = 0;
    dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency.srcAccessMask = 0;
    dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

    renderPassInfo.dependencyCount = 1;
    renderPassInfo.pDependencies = &dependency;

    if (vkCreateRenderPass(device, &renderPassInfo, nullptr, &renderPass) != VK_SUCCESS) {
        throw std::runtime_error("failed to create render pass"); 
    }
}

void HelloTriangleApplication::createFramebuffers() {
    swapChainFramebuffers.resize(swapChainImageViews.size()); 

    //iterate through each image and create a buffer for it 
    for (size_t i = 0; i < swapChainImageViews.size(); i++) {
        VkImageView attachments[] = { swapChainImageViews[i] }; 

        VkFramebufferCreateInfo framebufferInfo{}; 
        framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO; 
        //make sure that framebuffer is compatible with renderPass (same # and type of attachments)
        framebufferInfo.renderPass = renderPass; 
        //specify which vkImageView objects to bind to the attachment descriptions in the render pass pAttachment array
        framebufferInfo.attachmentCount = 1; 
        framebufferInfo.pAttachments = attachments;
        framebufferInfo.width = swapChainExtent.width; 
        framebufferInfo.height = swapChainExtent.height; 
        framebufferInfo.layers = 1; //# of layers in image arrays

        if (vkCreateFramebuffer(device, &framebufferInfo, nullptr, &swapChainFramebuffers[i]) != VK_SUCCESS) {
            throw std::runtime_error("failed to create framebuffer"); 
        }
    }
}

void HelloTriangleApplication::createCommandPool() {
    QueueFamilyIndices queueFamilyIndicies = findQueueFamilies(physicalDevice); 

    /* Command Buffers */
    //command buffers must be submitted on one of the device queues (graphics or presentation queues in this case)
    //must only be submitted on a single type of queue
    //creating commands for drawing, as such these are submitted on the graphics family
    VkCommandPoolCreateInfo commandPoolInfo{}; 
    commandPoolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO; 
    commandPoolInfo.queueFamilyIndex = queueFamilyIndicies.graphicsFamily.value(); 
    /* Two possible flags for command pools: 
        1.VK_COMMAND_POOL_CREATE_TRANSIENT_BIT: warn vulkan that the command pool is changed often 
        2.VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT: allow command buffers to be rerecorded individually, without this all command buffers are reset at the same time
    */
    commandPoolInfo.flags = 0; //optional -- will not be changing or resetting any command buffers 

    if (vkCreateCommandPool(device, &commandPoolInfo, nullptr, &commandPool) != VK_SUCCESS) {
        throw std::runtime_error("failed to create command pool"); 
    }

}

void HelloTriangleApplication::createCommandBuffers() {
    commandBuffers.resize(swapChainFramebuffers.size()); 

    VkCommandBufferAllocateInfo allocInfo{}; 
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO; 
    allocInfo.commandPool = commandPool; 
    // .level - specifies if the allocated command buffers are primay or secondary
    // ..._PRIMARY : can be submitted to a queue for execution, but cannot be called from other command buffers
    // ..._SECONDARY : cannot be submitted directly, but can be called from primary command buffers (good for reuse of common operations)
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY; 
    allocInfo.commandBufferCount = (uint32_t)commandBuffers.size(); 

    if (vkAllocateCommandBuffers(device, &allocInfo, commandBuffers.data()) != VK_SUCCESS) {
        throw std::runtime_error("failed to allocate command buffers");
    }

    /* Begin command buffer recording */
    for (size_t i = 0; i < commandBuffers.size(); i++) {
        VkCommandBufferBeginInfo beginInfo{}; 
        beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO; 

        //flags parameter specifies command buffer use 
            //VK_COMMAND_BUFFER_USEAGE_ONE_TIME_SUBMIT_BIT: command buffer recorded right after executing it once
            //VK_COMMAND_BUFFER_USEAGE_RENDER_PASS_CONTINUE_BIT: secondary command buffer that will be within a single render pass 
            //VK_COMMAND_BUFFER_USEAGE_SIMULTANEOUS_USE_BIT: command buffer can be resubmitted while another instance has already been submitted for execution
        beginInfo.flags = 0; 

        //only relevant for secondary command buffers -- which state to inherit from the calling primary command buffers 
        beginInfo.pInheritanceInfo = nullptr; 

        /* NOTE: 
            if the command buffer has already been recorded once, simply call vkBeginCommandBuffer->implicitly reset.
            commands cannot be added after creation
        */

        if (vkBeginCommandBuffer(commandBuffers[i], &beginInfo) != VK_SUCCESS) {
            throw std::runtime_error("failed to begin recording command buffer"); 
        }

        /* Begin render pass */
        //drawing starts by beginning a render pass 
        VkRenderPassBeginInfo renderPassInfo{}; 
        renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO; 

        //define the render pass we want
        renderPassInfo.renderPass = renderPass;

        //what attachments do we need to bind
        //previously created swapChainbuffers to hold this information 
        renderPassInfo.framebuffer = swapChainFramebuffers[i]; 

        //define size of render area -- should match size of attachments for best performance
        renderPassInfo.renderArea.offset = { 0, 0 }; 
        renderPassInfo.renderArea.extent = swapChainExtent; 

        //clear color for background color will be used with VK_ATTACHMENT_LOAD_OP_CLEAR
        VkClearValue clearColor = { {{0.0f, 0.0f, 0.0f, 1.0f}} };
        renderPassInfo.clearValueCount = 1; 
        renderPassInfo.pClearValues = &clearColor; 

        /* vkCmdBeginRenderPass */
        //Args: 
            //1. command buffer to set recording to 
            //2. details of the render pass
            //3. how drawing commands within the render pass will be provided
                //OPTIONS: 
                    //VK_SUBPASS_CONTENTS_INLINE: render pass commands will be embedded in the primary command buffer. No secondary command buffers executed 
                    //VK_SUBPASS_CONTENTS_SECONDARY_COMMAND_BUFFERS: render pass commands will be executed from the secondary command buffers
        vkCmdBeginRenderPass(commandBuffers[i], &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE); 

        /* Drawing Commands */
        //Args: 
            //2. compute or graphics pipeline
            //3. pipeline object
        vkCmdBindPipeline(commandBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, graphicsPipeline); 

        VkBuffer vertexBuffers[] = { vertexBuffer }; 
        VkDeviceSize offsets[] = { 0 }; 
        vkCmdBindVertexBuffers(commandBuffers[i], 0, 1, vertexBuffers, offsets); 


        //now create call to draw triangle
        //Args:    
            //2. vertexCount: how many verticies to draw
            //3. instanceCount: used for instanced render, use 1 otherwise
            //4. firstVertex: offset in VBO, defines lowest value of gl_VertexIndex
            //5. firstInstance: offset for instanced rendering, defines lowest value of gl_InstanceIndex
        vkCmdDraw(commandBuffers[i], static_cast<uint32_t>(vertices.size()), 1, 0, 0);

        //can now finis render pass
        vkCmdEndRenderPass(commandBuffers[i]); 

        //record command buffer
        if (vkEndCommandBuffer(commandBuffers[i]) != VK_SUCCESS) {
            throw std::runtime_error("failed to record command buffer"); 
        }
    }
}

void HelloTriangleApplication::createSemaphores() {
    imageAvailableSemaphores.resize(MAX_FRAMES_IN_FLIGHT); 
    renderFinishedSemaphores.resize(MAX_FRAMES_IN_FLIGHT); 

    VkSemaphoreCreateInfo semaphoreInfo{}; 
    semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO; 

    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        if (vkCreateSemaphore(device, &semaphoreInfo, nullptr, &imageAvailableSemaphores[i]) != VK_SUCCESS || vkCreateSemaphore(device, &semaphoreInfo, nullptr, &renderFinishedSemaphores[i])) {
            throw std::runtime_error("failed to create semaphores for a frame");
        }
    }
}

void HelloTriangleApplication::createFences() {
    //note: fence creation can be rolled into semaphore creation. Seperated for understanding
    inFlightFences.resize(MAX_FRAMES_IN_FLIGHT); 

    VkFenceCreateInfo fenceInfo{}; 
    fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO; 

    //create the fence in a signaled state 
    fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT; 

    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        if (vkCreateFence(device, &fenceInfo, nullptr, &inFlightFences[i]) != VK_SUCCESS) {
            throw std::runtime_error("failed to create fence object for a frame"); 
        }
    }
}

void HelloTriangleApplication::createFenceImageTracking() {
    //note: just like createFences() this too can be wrapped into semaphore creation. Seperated for understanding.
    
    //need to ensure the frame that is going to be drawn to, is the one linked to the expected fence.
    //If, for any reason, vulkan returns an image out of order, we will be able to handle that with this link
    imagesInFlight.resize(swapChainImages.size(), VK_NULL_HANDLE);

    //initially, no frame is using any image so this is going to be created without an explicit link
}

void HelloTriangleApplication::createVertexBuffer() {
    VkBufferCreateInfo bufferInfo{}; 

    bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO; 
    bufferInfo.size = sizeof(vertices[0]) * vertices.size(); //size of buffer in bytes
    bufferInfo.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT; //purpose of data in buffer
    bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE; //buffers can be owned by specific queue family or shared between them at the same time. This only used for graphics queue

    if (vkCreateBuffer(device, &bufferInfo, nullptr, &vertexBuffer) != VK_SUCCESS) {
        throw std::runtime_error("failed to create vertex buffer"); 
    }

    //need to allocate memory for the buffer object
    /* VkMemoryRequirements: 
        1. size - number of required bytes in memory
        2. alignments - offset in bytes where the buffer begins in the allocated region of memory (depends on bufferInfo.useage and bufferInfo.flags)
        3. memoryTypeBits - bit fied of the memory types that are suitable for the buffer
    */
    VkMemoryRequirements memRequirenments; 
    vkGetBufferMemoryRequirements(device, vertexBuffer, &memRequirenments);

    VkMemoryAllocateInfo allocInfo{}; 
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO; 
    allocInfo.allocationSize = memRequirenments.size; 
    allocInfo.memoryTypeIndex = findMemoryType(memRequirenments.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT); 

    if (vkAllocateMemory(device, &allocInfo, nullptr, &vertexBufferMemory) != VK_SUCCESS) {
        throw std::runtime_error("failed to allocate vertex buffer memory"); 
    }

    //4th argument: offset within the region of memory. Since memory is allocated specifically for this vertex buffer, the offset is 0
    //if not 0, required to be divisible by memRequirenments.alignment
    vkBindBufferMemory(device, vertexBuffer, vertexBufferMemory, 0); 

    /* Filling the vertex buffer */
    void* data; 

    //access a region of the specified memory recourse defined by an offset and size 
    //can also specify VK_WHOLE_SIZE to map all memory 
    //currrently no memory flags available in API (time of writing) so must be set to 0
    vkMapMemory(device, vertexBufferMemory, 0, bufferInfo.size, 0, &data); 
    memcpy(data, vertices.data(), (size_t)bufferInfo.size); //simply copy data into mapped memory
    vkUnmapMemory(device, vertexBufferMemory); //unmap memory

    /* Memory Copy Note */
    //note: driver might not immediately copy the data into the buffer memory, ex: caching
    //also possible that writes not visible to mapped memory yet
    //two ways to handle this: 
        //1. use memory heap that is host coherent (VK_MEMORY_PROPERTY_HOST_COHERENT_BIT)
        //2. call vkFlushMappedMemoryRanges after writing tot he mapped memory and then call vkInvalidateMappedMemoryRanges before reading from mapped memory
    //this project used (1) memory always matches but might lead to slightly worse performance 
    //Flushing memory ranges or using coherent calls does not mean they are passed to GPU. Vulkan does this in the background and memory is guaranteed to be on
        //on GPU before next call to vkQueueSubmit

}

#pragma region Unused Functions
/* LEFT FOR FUTURE NOTE
/// <summary>
/// finalColor.rgb = newAlpha * newColor + (1 - newAlpha) * oldColor;
/// finalColor.a = newAlpha.a;
/// </summary>
/// <returns></returns>
VkPipelineColorBlendAttachmentState HelloTriangleApplication::createAlphaColorBlending() {
    VkPipelineColorBlendAttachmentState colorBlendAttachment;
    colorBlendAttachment.blendEnable = VK_TRUE;
    colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
    colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
    colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
    colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
    colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
    colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;
}
*/
#pragma endregion