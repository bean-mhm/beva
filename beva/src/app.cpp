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
        // print available layers
        {
            auto available_layers = beva::Context::available_layers();
            if (!available_layers.ok())
            {
                throw std::runtime_error(
                    available_layers.error().to_string().c_str()
                );
            }

            std::cout
                << available_layers.value().size()
                << " available layers\n";
            for (const auto& layer : available_layers.value())
            {
                std::cout << std::format(
                    "{} ({}, {}): {}\n",
                    layer.name,
                    layer.spec_version.to_string(),
                    layer.implementation_version,
                    layer.description
                );
            }
            std::cout << '\n';
        }

        // print available extensions
        {
            auto available_extensions = beva::Context::available_extensions();
            if (!available_extensions.ok())
            {
                throw std::runtime_error(
                    available_extensions.error().to_string().c_str()
                );
            }

            std::cout
                << available_extensions.value().size()
                << " available extensions\n";
            for (const auto& ext : available_extensions.value())
            {
                std::cout << std::format(
                    "{} ({})\n",
                    ext.name,
                    ext.spec_version
                );
            }
            std::cout << '\n';
        }

        std::vector<std::string> layers{
            "VK_LAYER_KHRONOS_validation"
        };

        std::vector<std::string> extensions;
        {
            // add extensions required by GLFW
            uint32_t glfw_ext_count = 0;
            const char** glfw_exts;
            glfw_exts = glfwGetRequiredInstanceExtensions(&glfw_ext_count);
            for (uint32_t i = 0; i < glfw_ext_count; i++)
            {
                extensions.emplace_back(glfw_exts[i]);
            }
        }

        beva::ContextConfig config{
            .will_enumerate_portability = false,
                .app_name = "beva demo",
                .app_version = beva::Version(1, 1, 0, 0),
                .engine_name = "no engine",
                .engine_version = beva::Version(1, 1, 0, 0),
                .api_version = beva::ApiVersion::Vulkan1_0,
                .layers = layers,
                .extensions = extensions
        };

        auto context_result = beva::Context::create(config);
        if (!context_result.ok())
        {
            std::string s =
                "failed to create context: "
                + context_result.error().to_string();
            throw std::runtime_error(s.c_str());
        }
        context = std::make_unique<beva::Context>(
            std::move(context_result.value())
        );
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
