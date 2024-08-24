#include "app.hpp"

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

#define CHECK_BV_RESULT(result, operation_name) \
    if (!result.ok())                           \
    {                                           \
        throw std::runtime_error(std::format(   \
            "failed to {}: {}",                 \
            operation_name,                     \
            result.error().to_string()          \
        ).c_str());                             \
    }

namespace beva_demo
{

    static std::vector<uint8_t> read_file(const std::string& filename);
    static void glfw_error_callback(int error, const char* description);

    void App::run()
    {
        init();
        main_loop();
        cleanup();
    }

    void App::init()
    {
        init_window();
        init_context();
        setup_debug_messenger();
        create_surface();
        pick_physical_device();
        create_logical_device();
        create_swapchain();
        create_render_pass();
        create_graphics_pipeline();
        create_framebuffers();
        create_command_pool_and_buffer();
        create_sync_objects();
    }

    void App::main_loop()
    {
        while (true)
        {
            glfwPollEvents();
            draw_frame();

            if (glfwWindowShouldClose(window))
            {
                break;
            }
        }

        auto result = device->wait_idle();
        CHECK_BV_RESULT(result, "wait for device idle");
    }

    void App::cleanup()
    {
        fence_in_flight = nullptr;
        semaphore_render_finished = nullptr;
        semaphore_image_available = nullptr;
        cmd_pool = nullptr;
        swapchain_framebufs.clear();
        graphics_pipeline = nullptr;
        pipeline_layout = nullptr;
        render_pass = nullptr;
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
            "pick a physical device within the command line",
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
        CHECK_BV_RESULT(context_result, "create context");
        context = context_result.value();
    }

    void App::setup_debug_messenger()
    {
        if (!debug_mode)
        {
            return;
        }

        VkDebugUtilsMessageSeverityFlagsEXT severity_filter =
            VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT
            | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;

        VkDebugUtilsMessageTypeFlagsEXT tpye_filter =
            VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT
            | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT
            | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT
            | VK_DEBUG_UTILS_MESSAGE_TYPE_DEVICE_ADDRESS_BINDING_BIT_EXT;

        auto debug_messenger_result = bv::DebugMessenger::create(
            context,
            severity_filter,
            tpye_filter,
            [](
                VkDebugUtilsMessageSeverityFlagBitsEXT message_severity,
                VkDebugUtilsMessageTypeFlagsEXT message_types,
                const bv::DebugMessageData& message_data
                )
            {
                std::cout << message_data.message << '\n';
            }
        );
        CHECK_BV_RESULT(debug_messenger_result, "create debug messenger");
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
        CHECK_BV_RESULT(physical_devices_result, "fetch physical devices");
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

            std::string s_device_type = "unknown device type";
            switch (pdev->properties().device_type)
            {
            case VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU:
                s_device_type = "integrated GPU";
                break;
            case VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU:
                s_device_type = "discrete GPU";
                break;
            case VK_PHYSICAL_DEVICE_TYPE_VIRTUAL_GPU:
                s_device_type = "virtual GPU";
                break;
            case VK_PHYSICAL_DEVICE_TYPE_CPU:
                s_device_type = "CPU";
                break;
            default:
                break;
            }

            std::cout << std::format(
                "{}: {} ({})\n",
                i,
                pdev->properties().device_name,
                s_device_type
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

        glfwSetWindowTitle(window, title);
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
                .flags = 0,
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
        CHECK_BV_RESULT(device_result, "create device");
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
            if (sfmt.color_space == VK_COLORSPACE_SRGB_NONLINEAR_KHR)
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

        VkSharingMode image_sharing_mode;
        std::vector<uint32_t> queue_family_indices;
        if (graphics_family_idx != presentation_family_idx)
        {
            image_sharing_mode = VK_SHARING_MODE_CONCURRENT;
            queue_family_indices = {
                graphics_family_idx,
                presentation_family_idx
            };
        }
        else
        {
            image_sharing_mode = VK_SHARING_MODE_EXCLUSIVE;
        }

        auto pre_transform = swapchain_support.capabilities.current_transform;

        bv::SwapchainConfig config{
            .flags = {},
            .min_image_count = image_count,
            .image_format = surface_format.format,
            .image_color_space = surface_format.color_space,
            .image_extent = extent,
            .image_array_layers = 1,
            .image_usage = { VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT },
            .image_sharing_mode = image_sharing_mode,
            .queue_family_indices = queue_family_indices,
            .pre_transform = pre_transform,
            .composite_alpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
            .present_mode = VK_PRESENT_MODE_FIFO_KHR,
            .clipped = true
        };

        auto swapchain_result = bv::Swapchain::create(device, surface, config);
        CHECK_BV_RESULT(swapchain_result, "create swapchain");
        swapchain = swapchain_result.value();

        for (size_t i = 0; i < swapchain->images().size(); i++)
        {
            bv::ImageViewConfig config{
                .flags = {},
                .view_type = VK_IMAGE_VIEW_TYPE_2D,
                .format = surface_format.format,
                .components = {},
                .subresource_range = bv::ImageSubresourceRange{
                    .aspect_mask = { VK_IMAGE_ASPECT_COLOR_BIT },
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

    void App::create_render_pass()
    {
        bv::Attachment color_attachment{
            .flags = 0,
            .format = swapchain->config().image_format,
            .samples = VK_SAMPLE_COUNT_1_BIT,
            .load_op = VK_ATTACHMENT_LOAD_OP_CLEAR,
            .store_op = VK_ATTACHMENT_STORE_OP_STORE,
            .stencil_load_op = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
            .stencil_store_op = VK_ATTACHMENT_STORE_OP_DONT_CARE,
            .initial_layout = VK_IMAGE_LAYOUT_UNDEFINED,
            .final_layout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR
        };

        bv::AttachmentReference color_attachment_ref{
            .attachment = 0,
            .layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
        };

        bv::Subpass subpass{
            .flags = 0,
            .pipeline_bind_point = VK_PIPELINE_BIND_POINT_GRAPHICS,
            .input_attachments = {},
            .color_attachments = { color_attachment_ref },
            .resolve_attachments = {},
            .depth_stencil_attachment = std::nullopt,
            .preserve_attachment_indices = {}
        };

        bv::SubpassDependency dependency{
            .src_subpass = VK_SUBPASS_EXTERNAL,
            .dst_subpass = 0,
            .src_stage_mask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
            .dst_stage_mask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
            .src_access_mask = 0,
            .dst_access_mask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
            .dependency_flags = 0
        };

        auto render_pass_result = bv::RenderPass::create(
            device,
            {
                .flags = 0,
                .attachments = { color_attachment },
                .subpasses = { subpass },
                .dependencies = { dependency }
            }
        );
        CHECK_BV_RESULT(render_pass_result, "create render pass");
        render_pass = render_pass_result.value();
    }

    void App::create_graphics_pipeline()
    {
        // shader modules
        // they are local variables because they're only needed until pipeline
        // creation.

        auto vert_shader_code = read_file("./shaders/vert.spv");
        auto frag_shader_code = read_file("./shaders/frag.spv");

        auto vert_module_result = bv::ShaderModule::create(
            device,
            std::move(vert_shader_code)
        );
        CHECK_BV_RESULT(vert_module_result, "create vertex shader module");
        auto vert_shader_module = vert_module_result.value();

        auto frag_module_result = bv::ShaderModule::create(
            device,
            std::move(frag_shader_code)
        );
        CHECK_BV_RESULT(frag_module_result, "create fragment shader module");
        auto frag_shader_module = frag_module_result.value();

        // shader stages
        std::vector<bv::ShaderStage> shader_stages;
        shader_stages.push_back(bv::ShaderStage{
            .flags = {},
            .stage = VK_SHADER_STAGE_VERTEX_BIT,
            .module = vert_shader_module,
            .entry_point = "main",
            .specialization_info = std::nullopt
            });
        shader_stages.push_back(bv::ShaderStage{
            .flags = {},
            .stage = VK_SHADER_STAGE_FRAGMENT_BIT,
            .module = frag_shader_module,
            .entry_point = "main",
            .specialization_info = std::nullopt
            });

        bv::VertexInputState vertex_input_state{
            .binding_descriptions = {},
            .attribute_descriptions = {}
        };

        bv::InputAssemblyState input_assembly_state{
            .topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
            .primitive_restart_enable = false
        };

        bv::Viewport viewport{
            .x = 0.f,
            .y = 0.f,
            .width = (float)swapchain->config().image_extent.width,
            .height = (float)swapchain->config().image_extent.height,
            .min_depth = 0.f,
            .max_depth = 1.f
        };

        bv::Rect2d scissor{
            .offset = { 0, 0 },
            .extent = swapchain->config().image_extent
        };

        bv::ViewportState viewport_state{
            .viewports = { viewport },
            .scissors = { scissor }
        };

        bv::RasterizationState rasterization_state{
            .depth_clamp_enable = false,
            .rasterizer_discard_enable = false,
            .polygon_mode = VK_POLYGON_MODE_FILL,
            .cull_mode = VK_CULL_MODE_BACK_BIT,
            .front_face = VK_FRONT_FACE_CLOCKWISE,
            .depth_bias_enable = false,
            .depth_bias_constant_factor = 0.f,
            .depth_bias_clamp = 0.f,
            .depth_bias_slope_factor = 0.f,
            .line_width = 1.f
        };

        bv::MultisampleState multisample_state{
            .rasterization_samples = VK_SAMPLE_COUNT_1_BIT,
            .sample_shading_enable = false,
            .min_sample_shading = 1.f,
            .sample_mask = {},
            .alpha_to_coverage_enable = false,
            .alpha_to_one_enable = false
        };

        bv::ColorBlendAttachment color_blend_attachment{
            .blend_enable = false,
            .src_color_blend_factor = VK_BLEND_FACTOR_ONE,
            .dst_color_blend_factor = VK_BLEND_FACTOR_ZERO,
            .color_blend_op = VK_BLEND_OP_ADD,
            .src_alpha_blend_factor = VK_BLEND_FACTOR_ONE,
            .dst_alpha_blend_factor = VK_BLEND_FACTOR_ZERO,
            .alpha_blend_op = VK_BLEND_OP_ADD,
            .color_write_mask =
            VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT
            | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT
        };

        bv::ColorBlendState color_blend_state{
            .flags = 0,
            .logic_op_enable = false,
            .logic_op = VK_LOGIC_OP_COPY,
            .attachments = { color_blend_attachment },
            .blend_constants = { 0.f, 0.f, 0.f, 0.f }
        };

        bv::DynamicStates dynamic_states{
            VK_DYNAMIC_STATE_VIEWPORT,
            VK_DYNAMIC_STATE_SCISSOR
        };

        auto pipeline_layout_result = bv::PipelineLayout::create(
            device,
            { .flags = 0, .set_layouts = {}, .push_constant_ranges = {} }
        );
        CHECK_BV_RESULT(pipeline_layout_result, "create pipeline layout");
        pipeline_layout = pipeline_layout_result.value();

        auto graphics_pipeline_result = bv::GraphicsPipeline::create(
            device,
            {
                .flags = 0,
                .stages = shader_stages,
                .vertex_input_state = vertex_input_state,
                .input_assembly_state = input_assembly_state,
                .tessellation_state = std::nullopt,
                .viewport_state = viewport_state,
                .rasterization_state = rasterization_state,
                .multisample_state = multisample_state,
                .depth_stencil_state = std::nullopt,
                .color_blend_state = color_blend_state,
                .dynamic_states = dynamic_states,
                .layout = pipeline_layout,
                .render_pass = render_pass,
                .subpass_index = 0,
                .base_pipeline = nullptr
            }
        );
        CHECK_BV_RESULT(graphics_pipeline_result, "create graphics pipeline");
        graphics_pipeline = graphics_pipeline_result.value();
    }

    void App::create_framebuffers()
    {
        for (size_t i = 0; i < swapchain_imgviews.size(); i++)
        {
            auto framebuf_result = bv::Framebuffer::create(
                device,
                {
                    .flags = 0,
                    .render_pass = render_pass,
                    .attachments = { swapchain_imgviews[i] },
                    .width = swapchain->config().image_extent.width,
                    .height = swapchain->config().image_extent.height,
                    .layers = 1
                }
            );
            if (!framebuf_result.ok())
            {
                throw std::runtime_error(std::format(
                    "failed to create swapchain framebuffer at index {}: {}",
                    i,
                    framebuf_result.error().to_string()
                ).c_str());
            }
            swapchain_framebufs.push_back(framebuf_result.value());
        }
    }

    void App::create_command_pool_and_buffer()
    {
        auto cmd_pool_result = bv::CommandPool::create(
            device,
            {
                .flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
                .queue_family_index = graphics_family_idx
            }
        );
        CHECK_BV_RESULT(cmd_pool_result, "create command pool");
        cmd_pool = cmd_pool_result.value();

        auto cmd_buf_result = cmd_pool->allocate_buffer(
            VK_COMMAND_BUFFER_LEVEL_PRIMARY
        );
        CHECK_BV_RESULT(cmd_buf_result, "allocate command buffer");
        cmd_buf = cmd_buf_result.value();
    }

    void App::create_sync_objects()
    {
        auto semaphore_image_available_result = bv::Semaphore::create(device);
        CHECK_BV_RESULT(semaphore_image_available_result, "create semaphore");
        semaphore_image_available = semaphore_image_available_result.value();

        auto semaphore_render_finished_result = bv::Semaphore::create(device);
        CHECK_BV_RESULT(semaphore_render_finished_result, "create semaphore");
        semaphore_render_finished = semaphore_render_finished_result.value();

        auto fence_in_flight_result = bv::Fence::create(
            device,
            VK_FENCE_CREATE_SIGNALED_BIT
        );
        CHECK_BV_RESULT(fence_in_flight_result, "create fence");
        fence_in_flight = fence_in_flight_result.value();
    }

    void App::draw_frame()
    {
        auto result = fence_in_flight->wait();
        CHECK_BV_RESULT(result, "wait for fence");

        result = fence_in_flight->reset();
        CHECK_BV_RESULT(result, "reset fence");

        auto acquire_result = swapchain->acquire_next_image(
            semaphore_image_available,
            nullptr,
            UINT64_MAX
        );
        CHECK_BV_RESULT(acquire_result, "acquire the next swapchain image");
        uint32_t img_idx = acquire_result.value().first;

        result = cmd_buf->reset(0);
        CHECK_BV_RESULT(result, "reset command buffer");
        record_command_buffer(img_idx);

        result = graphics_queue->submit(
            { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT },
            { semaphore_image_available },
            { cmd_buf },
            { semaphore_render_finished },
            fence_in_flight
        );
        CHECK_BV_RESULT(result, "submit command buffer");

        auto present_result = presentation_queue->present(
            { semaphore_render_finished },
            swapchain,
            img_idx
        );
        CHECK_BV_RESULT(present_result, "present image");
    }

    void App::record_command_buffer(uint32_t img_idx)
    {
        auto begin_result = cmd_buf->begin(0);
        CHECK_BV_RESULT(begin_result, "begin recording command buffer");

        VkClearValue clear_val{ { { .15f, .16f, .2f, 1.f } } };
        VkRenderPassBeginInfo render_pass_info{
            .sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
            .pNext = nullptr,
            .renderPass = render_pass->handle(),
            .framebuffer = swapchain_framebufs[img_idx]->handle(),
            .renderArea = VkRect2D{
                .offset = { 0, 0 },
                .extent = bv::Extent2d_to_vk(swapchain->config().image_extent)
        },
            .clearValueCount = 1,
            .pClearValues = &clear_val
        };
        vkCmdBeginRenderPass(
            cmd_buf->handle(),
            &render_pass_info,
            VK_SUBPASS_CONTENTS_INLINE
        );

        vkCmdBindPipeline(
            cmd_buf->handle(),
            VK_PIPELINE_BIND_POINT_GRAPHICS,
            graphics_pipeline->handle()
        );

        VkViewport viewport{
            .x = 0.f,
            .y = 0.f,
            .width = (float)(swapchain->config().image_extent.width),
            .height = (float)(swapchain->config().image_extent.height),
            .minDepth = 0.f,
            .maxDepth = 1.f
        };
        vkCmdSetViewport(cmd_buf->handle(), 0, 1, &viewport);

        VkRect2D scissor{
            .offset = { 0, 0 },
            .extent = bv::Extent2d_to_vk(swapchain->config().image_extent)
        };
        vkCmdSetScissor(cmd_buf->handle(), 0, 1, &scissor);

        vkCmdDraw(cmd_buf->handle(), 3, 1, 0, 0);

        vkCmdEndRenderPass(cmd_buf->handle());

        auto end_result = cmd_buf->end();
        CHECK_BV_RESULT(end_result, "end recording command buffer");
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
