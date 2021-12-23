/*
* Date: 12/12/2021
* Based on code from vulkan-tutorial.com -- "Drawing a triangle" 
*/
#include <iostream>

#include "HelloTriangleApplication.h"

int main() {
    HelloTriangleApplication app;

    try {
        app.run();
    }
    catch (const std::exception& e) {
        std::cerr << e.what() << std::endl;
        do {
            std::cout << "Press a key to exit..." << std::endl; 
        } while (std::cin.get() != '\n'); 
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}