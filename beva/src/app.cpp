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
        pick_physical_device();
        create_logical_device();
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

    void App::pick_physical_device()
    {
        if (context->physical_devices().empty())
        {
            throw std::runtime_error(
                "there's no physical device that supports Vulkan on this "
                "machine"
            );
        }

        std::cout << "pick a physical device by entering its index:\n";
        for (size_t i = 0; i < context->physical_devices().size(); i++)
        {
            const auto& physical_device = context->physical_devices()[i];
            std::cout << std::format(
                "{}: {} ({})\n",
                i,
                physical_device->properties().device_name,
                bv::PhysicalDeviceType_to_string(
                    physical_device->properties().device_type
                )
            );
        }

        int32_t idx;
        while (true)
        {
            std::cin >> idx;
            if (idx < 0 || idx >= context->physical_devices().size())
            {
                std::cout << "enter a valid physical device index\n";
            }
            {
                break;
            }
        }

        physical_device = context->physical_devices()[idx];
    }

    void App::create_logical_device()
    {
        if (!physical_device->queue_family_indices().graphics.has_value())
        {
            throw std::runtime_error(
                "the selected physical device doesn't have a queue family that "
                "supports graphics operations"
            );
        }

        uint32_t graphics_family_idx =
            physical_device->queue_family_indices().graphics.value();

        std::vector<bv::QueueRequest> queue_requests;
        queue_requests.push_back(bv::QueueRequest{
            .flags = bv::QueueRequestFlags{},
                .queue_family_index = graphics_family_idx,
                .num_queues_to_create = 1,
                .priorities = { 1.f }
        });

        bv::DeviceConfig config{
            .queue_requests = queue_requests,
                .extensions = {},
                .enabled_features = bv::PhysicalDeviceFeatures{}
        };

        auto device_result = bv::Device::create(
            context,
            physical_device,
            config
        );
        if (!device_result.ok())
        {
            std::string s =
                "failed to create device: "
                + device_result.error().to_string();
            throw std::runtime_error(s.c_str());
        }
        device = device_result.value();

        graphics_queue = device->retrieve_queue(graphics_family_idx, 0);
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
