#include "app.h"

#include <iostream>
#include <vector>
#include <string>
#include <format>
#include <stdexcept>
#include <cstdlib>
#include <cstdint>

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/vec4.hpp>
#include <glm/mat4x4.hpp>

namespace beva_demo
{

    static constexpr VkAllocationCallbacks* vk_alloc = nullptr;

    static void glfw_error_callback(int error, const char* description);

    void App::run()
    {
        init_window();
        init_vulkan();
        main_loop();
        cleanup();
    }

    void App::init_window()
    {
        glfwSetErrorCallback(glfw_error_callback);

        if (!glfwInit())
        {
            throw std::runtime_error("failed to initialize GLFW");
        }

        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
        glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

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
    }

    void App::init_vulkan()
    {
        create_instance();
    }

    void App::main_loop()
    {
        while (true)
        {
            glfwPollEvents();

            if (glfwWindowShouldClose(window))
            {
                break;
            }
        }
    }

    void App::cleanup()
    {
        vkDestroyInstance(instance, vk_alloc);

        glfwDestroyWindow(window);
        glfwTerminate();
    }

    void App::create_instance()
    {
        VkApplicationInfo app_info{};
        app_info.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
        app_info.pApplicationName = initial_title;
        app_info.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
        app_info.pEngineName = "No Engine";
        app_info.engineVersion = VK_MAKE_VERSION(1, 0, 0);
        app_info.apiVersion = VK_API_VERSION_1_0;

        VkInstanceCreateInfo create_info{};
        create_info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
        create_info.pApplicationInfo = &app_info;

        // extensions
        {
            std::vector<const char*> required_exts;

            // add extensions required by GLFW
            uint32_t glfw_ext_count = 0;
            const char** glfw_exts;
            glfw_exts = glfwGetRequiredInstanceExtensions(&glfw_ext_count);
            for (uint32_t i = 0; i < glfw_ext_count; i++)
            {
                required_exts.emplace_back(glfw_exts[i]);
            }

            required_exts.emplace_back(
                VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME
            );

            create_info.flags |=
                VK_INSTANCE_CREATE_ENUMERATE_PORTABILITY_BIT_KHR;

            create_info.enabledExtensionCount = (uint32_t)required_exts.size();
            create_info.ppEnabledExtensionNames = required_exts.data();
        }

        create_info.enabledLayerCount = 0;

        if (vkCreateInstance(&create_info, vk_alloc, &instance) != VK_SUCCESS)
        {
            throw std::runtime_error("failed to create Vulkan instance");
        }
    }

    static void glfw_error_callback(int error, const char* description)
    {
        std::cerr << std::format("GLFW error {}: {}\n", error, description);
    }

}
