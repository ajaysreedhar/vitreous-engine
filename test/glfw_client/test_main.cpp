#define GLFW_INCLUDE_NONE

#include <iostream>
#include <GLFW/glfw3.h>
#include "platform/logger.hpp"

int main() {

    if (!glfwInit()) {
        vtrs::Logger::fatal("Unable to initialise GLFW!");
        return EXIT_FAILURE;
    }

    GLFWwindow* window = glfwCreateWindow(800, 640, "Vitreous GLFW Demo", nullptr, nullptr);

    if (window == nullptr){
        glfwTerminate();
        vtrs::Logger::fatal("Unable to create window!");
        return EXIT_FAILURE;
    }

    /* Make the window's context current */
    glfwMakeContextCurrent(window);

    /* Loop until the user closes the window */
    while (!glfwWindowShouldClose(window)) {

        /* Swap front and back buffers */
        glfwSwapBuffers(window);

        /* Poll for and process events */
        glfwPollEvents();
    }

    glfwTerminate();

    return EXIT_SUCCESS;
}
