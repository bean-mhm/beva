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

    static void glfw_error_callback(int error, const char* description);

    void App::run()
    {
        init_window();
        init_context();
        setup_debug_messenger();

        for (const auto& physical_device : context->physical_devices())
        {
            std::cout << physical_device.properties().device_name << '\n';
        }

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

    void App::init_context()
    {
        std::vector<std::string> layers;
        if (debug_mode)
        {
            layers.push_back("VK_LAYER_KHRONOS_validation");
        }

        std::vector<std::string> extensions;
        {
            // extensions required by GLFW
            uint32_t glfw_ext_count = 0;
            const char** glfw_exts;
            glfw_exts = glfwGetRequiredInstanceExtensions(&glfw_ext_count);
            for (uint32_t i = 0; i < glfw_ext_count; i++)
            {
                extensions.emplace_back(glfw_exts[i]);
            }

            // debug utils extension
            if (debug_mode)
            {
                extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
            }
        }

        bv::ContextConfig config{
            .will_enumerate_portability = false,
                .app_name = "beva demo",
                .app_version = bv::Version(1, 1, 0, 0),
                .engine_name = "no engine",
                .engine_version = bv::Version(1, 1, 0, 0),
                .vulkan_api_version = bv::VulkanApiVersion::Vulkan1_0,
                .layers = layers,
                .extensions = extensions
        };

        auto context_result = bv::Context::create(config);
        if (!context_result.ok())
        {
            std::string s =
                "failed to create context: "
                + context_result.error().to_string();
            throw std::runtime_error(s.c_str());
        }
        context = context_result.value();
    }

    void App::setup_debug_messenger()
    {
        if (!debug_mode)
        {
            return;
        }

        bv::DebugMessageSeverityFilter severity_filter{
            .verbose = false,
                .info = false,
                .warning = true,
                .error = true
        };

        bv::DebugMessageTypeFilter type_filter{
            .general = true,
                .validation = true,
                .performance = true,
                .device_address_binding = true
        };

        auto debug_messenger_result = bv::DebugMessenger::create(
            context,
            severity_filter,
            type_filter,
            [](
                bv::DebugMessageSeverity message_severity,
                bv::DebugMessageType message_type,
                const bv::DebugMessageData& message_data
                )
            {
                std::cout << message_data.message << '\n';
            }
        );
        if (!debug_messenger_result.ok())
        {
            std::string s =
                "failed to create debug messenger: "
                + debug_messenger_result.error().to_string();
            throw std::runtime_error(s.c_str());
        }
        debug_messenger = debug_messenger_result.value();
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
        context = nullptr;
        glfwDestroyWindow(window);
        glfwTerminate();
    }

    static void glfw_error_callback(int error, const char* description)
    {
        std::cerr << std::format("GLFW error {}: {}\n", error, description);
    }

}
