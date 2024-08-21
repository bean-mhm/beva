#include "app.h"

#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <format>
#include <set>
#include <limits>
#include <algorithm>
#include <stdexcept>
#include <cstdlib>
#include <cstdint>

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/vec4.hpp>
#include <glm/mat4x4.hpp>

namespace beva_demo
{

    static std::vector<uint8_t> read_file(const std::string& filename);
    static void glfw_error_callback(int error, const char* description);

    void App::run()
    {
        init_window();
        init_context();
        setup_debug_messenger();
        create_surface();
        pick_physical_device();
        create_logical_device();
        create_swapchain();
        create_graphics_pipeline();

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
            throw std::runtime_error(std::format(
                "failed to create context: {}",
                context_result.error().to_string()
            ).c_str());
        }
        context = context_result.value();
    }

    void App::setup_debug_messenger()
    {
        if (!debug_mode)
        {
            return;
        }

        bv::DebugMessageSeverityFlags severity_filter{
            .verbose = false,
            .info = false,
            .warning = true,
            .error = true
        };

        bv::DebugMessageTypeFlags type_filter{
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
                bv::DebugMessageTypeFlags message_type_flags,
                const bv::DebugMessageData& message_data
                )
            {
                std::cout << message_data.message << '\n';
            }
        );
        if (!debug_messenger_result.ok())
        {
            throw std::runtime_error(std::format(
                "failed to create debug messenger: {}",
                debug_messenger_result.error().to_string()
            ).c_str());
        }
        debug_messenger = debug_messenger_result.value();
    }

    void App::create_surface()
    {
        VkSurfaceKHR vk_surface;

        VkResult vk_result = glfwCreateWindowSurface(
            context->vk_instance(),
            window,
            context->vk_allocator_ptr(),
            &vk_surface
        );
        if (vk_result != VK_SUCCESS)
        {
            throw std::runtime_error(bv::Error(
                "failed to create window surface",
                vk_result
            ).to_string().c_str());
        }

        surface = bv::Surface::create(context, vk_surface);
    }

    void App::pick_physical_device()
    {
        auto physical_devices_result = context->fetch_physical_devices(surface);
        if (!physical_devices_result.ok())
        {
            throw std::runtime_error(std::format(
                "failed to fetch physical devices: {}",
                physical_devices_result.error().to_string()
            ).c_str());
        }
        auto all_physical_devices = physical_devices_result.value();

        std::vector<bv::PhysicalDevice::ptr> supported_physical_devices;
        for (const auto& pdev : all_physical_devices)
        {
            if (!pdev->queue_family_indices().graphics.has_value())
            {
                continue;
            }
            if (!pdev->queue_family_indices().presentation.has_value())
            {
                continue;
            }

            if (!pdev->swapchain_support().has_value())
            {
                continue;
            }

            const auto& swapchain_support = pdev->swapchain_support().value();
            if (swapchain_support.present_modes.empty()
                || swapchain_support.surface_formats.empty())
            {
                continue;
            }

            supported_physical_devices.push_back(pdev);
        }
        if (supported_physical_devices.empty())
        {
            throw std::runtime_error("no supported physical devices");
        }

        std::cout << "pick a physical device by entering its index:\n";
        for (size_t i = 0; i < supported_physical_devices.size(); i++)
        {
            const auto& pdev = supported_physical_devices[i];
            std::cout << std::format(
                "{}: {} ({})\n",
                i,
                pdev->properties().device_name,
                bv::PhysicalDeviceType_to_string(pdev->properties().device_type)
            );
        }

        int32_t idx;
        while (true)
        {
            std::string s_idx;
            std::getline(std::cin, s_idx);
            try
            {
                idx = std::stoi(s_idx);
                if (idx < 0 || idx >= supported_physical_devices.size())
                {
                    throw std::exception();
                }
                break;
            }
            catch (const std::exception&)
            {
                std::cout << "enter a valid physical device index\n";
            }
        }

        physical_device = supported_physical_devices[idx];
    }

    void App::create_logical_device()
    {
        graphics_family_idx =
            physical_device->queue_family_indices().graphics.value();

        presentation_family_idx =
            physical_device->queue_family_indices().presentation.value();

        std::set<uint32_t> unique_queue_family_indices = {
            graphics_family_idx,
            presentation_family_idx
        };

        std::vector<bv::QueueRequest> queue_requests;
        for (auto family_idx : unique_queue_family_indices)
        {
            queue_requests.push_back(bv::QueueRequest{
                .flags = bv::QueueRequestFlags{},
                .queue_family_index = family_idx,
                .num_queues_to_create = 1,
                .priorities = { 1.f }
                });
        }

        bv::DeviceConfig config{
            .queue_requests = queue_requests,
            .extensions = { VK_KHR_SWAPCHAIN_EXTENSION_NAME },
            .enabled_features = bv::PhysicalDeviceFeatures{}
        };

        auto device_result = bv::Device::create(
            context,
            physical_device,
            config
        );
        if (!device_result.ok())
        {
            throw std::runtime_error(std::format(
                "failed to create device: {}",
                device_result.error().to_string()
            ).c_str());
        }
        device = device_result.value();

        graphics_queue = device->retrieve_queue(graphics_family_idx, 0);
        presentation_queue = device->retrieve_queue(presentation_family_idx, 0);
    }

    void App::create_swapchain()
    {
        const auto& swapchain_support =
            physical_device->swapchain_support().value();

        bv::SurfaceFormat surface_format;
        bool found_surface_format = false;
        for (const auto& sfmt : swapchain_support.surface_formats)
        {
            if (sfmt.color_space == bv::ColorSpace::SrgbNonlinear)
            {
                surface_format = sfmt;
                found_surface_format = true;
                break;
            }
        }
        if (!found_surface_format)
        {
            throw std::runtime_error("no supported surface format");
        }

        bv::Extent2d extent = swapchain_support.capabilities.current_extent;
        if (extent.width == 0
            || extent.width == std::numeric_limits<uint32_t>::max()
            || extent.height == 0
            || extent.height == std::numeric_limits<uint32_t>::max())
        {
            int width, height;
            glfwGetFramebufferSize(window, &width, &height);

            extent = {
                .width = (uint32_t)width,
                .height = (uint32_t)height
            };

            extent.width = std::clamp(
                extent.width,
                swapchain_support.capabilities.min_image_extent.width,
                swapchain_support.capabilities.max_image_extent.width
            );
            extent.height = std::clamp(
                extent.height,
                swapchain_support.capabilities.min_image_extent.height,
                swapchain_support.capabilities.max_image_extent.height
            );
        }

        uint32_t image_count =
            swapchain_support.capabilities.min_image_count + 1;
        if (swapchain_support.capabilities.max_image_count > 0
            && image_count > swapchain_support.capabilities.max_image_count)
        {
            image_count = swapchain_support.capabilities.max_image_count;
        }

        bv::SharingMode image_sharing_mode;
        std::vector<uint32_t> queue_family_indices;
        if (graphics_family_idx != presentation_family_idx)
        {
            image_sharing_mode = bv::SharingMode::Concurrent;
            queue_family_indices = {
                graphics_family_idx,
                presentation_family_idx
            };
        }
        else
        {
            image_sharing_mode = bv::SharingMode::Exclusive;
        }

        auto pre_transform = swapchain_support.capabilities.current_transform;

        bv::SwapchainConfig config{
            .flags = {},
            .min_image_count = image_count,
            .image_format = surface_format.format,
            .image_color_space = surface_format.color_space,
            .image_extent = extent,
            .image_array_layers = 1,
            .image_usage = { .color_attachment = true },
            .image_sharing_mode = image_sharing_mode,
            .queue_family_indices = queue_family_indices,
            .pre_transform = pre_transform,
            .composite_alpha = bv::CompositeAlpha::Opaque,
            .present_mode = bv::PresentMode::Fifo,
            .clipped = true
        };

        auto swapchain_result = bv::Swapchain::create(device, surface, config);
        if (!swapchain_result.ok())
        {
            throw std::runtime_error(std::format(
                "failed to create swapchain: {}",
                swapchain_result.error().to_string()
            ).c_str());
        }
        swapchain = swapchain_result.value();

        for (size_t i = 0; i < swapchain->images().size(); i++)
        {
            bv::ImageViewConfig config{
                .flags = {},
                .view_type = bv::ImageViewType::_2d,
                .format = surface_format.format,
                .components = {},
                .subresource_range = bv::ImageSubresourceRange{
                    .aspect_mask = { .color = true },
                    .base_mip_level = 0,
                    .level_count = 1,
                    .base_array_layer = 0,
                    .layer_count = 1
            }
            };

            auto image_view_result = bv::ImageView::create(
                device,
                swapchain->images()[i],
                config
            );
            if (!image_view_result.ok())
            {
                throw std::runtime_error(std::format(
                    "failed to create image view for swapchain image at index "
                    "{}: {}",
                    i,
                    image_view_result.error().to_string()
                ).c_str());
            }
            swapchain_imgviews.push_back(image_view_result.value());
        }
    }

    void App::create_graphics_pipeline()
    {
        // shader modules
        // they are local variables because they're only needed until pipeline
        // creation.

        auto vert_shader_code = read_file("./shaders/vert.spv");
        auto frag_shader_code = read_file("./shaders/frag.spv");

        auto vert_shader_module_result = bv::ShaderModule::create(
            device,
            std::move(vert_shader_code)
        );
        if (!vert_shader_module_result.ok())
        {
            throw std::runtime_error(std::format(
                "failed to create vertex shader module: {}",
                vert_shader_module_result.error().to_string()
            ).c_str());
        }
        auto vert_shader_module = vert_shader_module_result.value();

        auto frag_shader_module_result = bv::ShaderModule::create(
            device,
            std::move(frag_shader_code)
        );
        if (!frag_shader_module_result.ok())
        {
            throw std::runtime_error(std::format(
                "failed to create fragment shader module: {}",
                frag_shader_module_result.error().to_string()
            ).c_str());
        }
        auto frag_shader_module = frag_shader_module_result.value();

        // shader stages
        std::vector<bv::ShaderStage> shader_stages;
        shader_stages.push_back(bv::ShaderStage{
            .flags = {},
            .type = bv::ShaderStageType::Vertex,
            .module = vert_shader_module,
            .entry_point = "main",
            .specialization_info = std::nullopt
            });
        shader_stages.push_back(bv::ShaderStage{
            .flags = {},
            .type = bv::ShaderStageType::Fragment,
            .module = frag_shader_module,
            .entry_point = "main",
            .specialization_info = std::nullopt
            });
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
        swapchain_imgviews.clear();
        swapchain = nullptr;
        presentation_queue = nullptr;
        graphics_queue = nullptr;
        device = nullptr;
        physical_device = nullptr;
        surface = nullptr;
        debug_messenger = nullptr;
        context = nullptr;

        glfwDestroyWindow(window);
        glfwTerminate();
    }

    static std::vector<uint8_t> read_file(const std::string& filename)
    {
        std::ifstream f(filename, std::ios::ate | std::ios::binary);
        if (!f.is_open())
        {
            throw std::runtime_error(std::format(
                "failed to read file \"{}\"",
                filename
            ).c_str());
        }

        size_t size_in_chars = (size_t)f.tellg();
        size_t size_in_bytes = size_in_chars * sizeof(char);

        std::vector<uint8_t> buf(size_in_bytes);
        f.seekg(0);
        f.read(reinterpret_cast<char*>(buf.data()), size_in_chars);
        f.close();

        return buf;
    }

    static void glfw_error_callback(int error, const char* description)
    {
        std::cerr << std::format("GLFW error {}: {}\n", error, description);
    }

}
