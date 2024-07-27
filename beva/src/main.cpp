#include <iostream>
#include <string>
#include <format>

#ifndef GLEW_STATIC
#define GLEW_STATIC
#endif
#include "GL/glew.h"
#include "GLFW/glfw3.h"

static constexpr const char* initial_title = "beva demo";
static constexpr int initial_width = 960;
static constexpr int initial_height = 720;

GLFWwindow* window;

static void init_context();
static void cleanup_context();
static void glfw_error_callback(int error, const char* description);

int main()
{
    init_context();
    while (true)
    {
        // clear the screen
        glClearColor(.2f, .22f, .26f, 1.f);
        glClear(GL_COLOR_BUFFER_BIT);

        // swap front and back buffers
        glfwSwapBuffers(window);

        // poll for and process events
        glfwPollEvents();

        // stop looping if the user closes the window
        if (glfwWindowShouldClose(window))
        {
            break;
        }
    }
    cleanup_context();
    return 0;
}

static void init_context()
{
    // initialize GLFW
    glfwSetErrorCallback(glfw_error_callback);
    if (!glfwInit())
    {
        throw std::runtime_error("failed to initialize GLFW");
    }

    // window hints
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);

    // create window with graphics context
    window = glfwCreateWindow(
        initial_width,
        initial_height,
        initial_title,
        nullptr,
        nullptr
    );
    if (!window)
    {
        glfwTerminate();
        throw std::runtime_error("failed to create a window");
    }

    // make the window's context current
    glfwMakeContextCurrent(window);

    // enable VSync
    glfwSwapInterval(1);

    // initialize GLEW for loading OpenGL extensions
    glewExperimental = GL_TRUE;
    GLenum glew_result = glewInit();
    if (glew_result != GLEW_OK)
    {
        cleanup_context();
        throw std::runtime_error(
            "failed to initialize GLEW: " + std::to_string(glew_result)
        );
    }
}

static void cleanup_context()
{
    glfwDestroyWindow(window);
    glfwTerminate();
}

static void glfw_error_callback(int error, const char* description)
{
    std::cerr << std::format("GLFW error {}: {}\n", error, description);
}
