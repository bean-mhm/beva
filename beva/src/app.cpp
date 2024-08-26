#include "app.hpp"

#include <iostream>
#include <fstream>
#include <format>
#include <set>
#include <limits>
#include <algorithm>
#include <stdexcept>
#include <cstdlib>

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
    static void glfw_framebuf_resize_callback(
        GLFWwindow* window,
        int width,
        int height
    );

    const bv::VertexInputBindingDescription Vertex::binding_description{
        .binding = 0,
        .stride = sizeof(Vertex),
        .input_rate = VK_VERTEX_INPUT_RATE_VERTEX
    };

    const std::vector<bv::VertexInputAttributeDescription>
        Vertex::attribute_descriptions
    {
        bv::VertexInputAttributeDescription{
            .location = 0,
            .binding = 0,
            .format = VK_FORMAT_R32G32_SFLOAT,
            .offset = offsetof(Vertex, pos)
    },
        bv::VertexInputAttributeDescription{
            .location = 1,
            .binding = 0,
            .format = VK_FORMAT_R32G32B32_SFLOAT,
            .offset = offsetof(Vertex, col)
    }
    };

    void App::run()
    {
        init();
        main_loop();
        cleanup();
    }

    void App::init()
    {
        start_time = std::chrono::high_resolution_clock::now();

        init_window();
        init_context();
        setup_debug_messenger();
        create_surface();
        pick_physical_device();
        create_logical_device();
        create_swapchain();
        create_render_pass();
        create_descriptor_set_layout();
        create_graphics_pipeline();
        create_framebuffers();
        create_command_pools();
        create_vertex_buffer();
        create_index_buffer();
        create_uniform_buffers();
        create_descriptor_pool();
        create_descriptor_sets();
        create_command_buffers();
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
        cleanup_swapchain();

        uniform_bufs.clear();
        uniform_bufs_mem.clear();

        descriptor_pool = nullptr;

        descriptor_set_layout = nullptr;

        index_buf = nullptr;
        index_buf_mem = nullptr;

        vertex_buf = nullptr;
        vertex_buf_mem = nullptr;

        graphics_pipeline = nullptr;
        pipeline_layout = nullptr;

        render_pass = nullptr;

        fences_in_flight.clear();
        semaphs_render_finished.clear();
        semaphs_image_available.clear();

        transfer_cmd_pool = nullptr;
        cmd_pool = nullptr;

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
        glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);

        window = glfwCreateWindow(
            INITIAL_WIDTH,
            INITIAL_HEIGHT,
            TITLE,
            nullptr,
            nullptr
        );
        if (!window)
        {
            glfwTerminate();
            throw std::runtime_error("failed to create a window");
        }

        glfwSetWindowUserPointer(window, this);
        glfwSetFramebufferSizeCallback(window, glfw_framebuf_resize_callback);
    }

    void App::init_context()
    {
        std::vector<std::string> layers;
        if (DEBUG_MODE)
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
            if (DEBUG_MODE)
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
        if (!DEBUG_MODE)
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

        auto messenger_result = bv::DebugMessenger::create(
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
        CHECK_BV_RESULT(messenger_result, "create debug messenger");
        debug_messenger = messenger_result.value();
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
                (bv::ApiResult)vk_result,
                false
            ).to_string().c_str());
        }
        surface = bv::Surface::create(context, vk_surface);
    }

    void App::pick_physical_device()
    {
        auto physical_devices_result = context->fetch_physical_devices(surface);
        CHECK_BV_RESULT(physical_devices_result, "fetch physical devices");
        auto all_physical_devices = physical_devices_result.value();

        std::vector<bv::PhysicalDevicePtr> supported_physical_devices;
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

        glfwSetWindowTitle(
            window,
            "pick a physical device within the command line"
        );

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

        glfwSetWindowTitle(window, TITLE);
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

        graphics_queue =
            bv::Device::retrieve_queue(device, graphics_family_idx, 0);

        presentation_queue =
            bv::Device::retrieve_queue(device, presentation_family_idx, 0);
    }

    void App::create_swapchain()
    {
        auto result = physical_device->update_swapchain_support(surface);
        CHECK_BV_RESULT(result, "update swapchain support details");

        if (!physical_device->swapchain_support().has_value())
        {
            throw std::runtime_error("presentation no longer supported");
        }
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

        swapchain_imgviews.clear();
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

    void App::create_descriptor_set_layout()
    {
        bv::DescriptorSetLayoutBinding binding{
            .binding = 0,
            .descriptor_type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
            .descriptor_count = 1,
            .stage_flags = VK_SHADER_STAGE_VERTEX_BIT,
            .immutable_samplers = {}
        };

        auto layout_result = bv::DescriptorSetLayout::create(
            device,
            {
                .flags = 0,
                .bindings = { binding }
            }
        );
        CHECK_BV_RESULT(layout_result, "create descriptor set layout");
        descriptor_set_layout = layout_result.value();
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
            .binding_descriptions = { Vertex::binding_description },
            .attribute_descriptions = Vertex::attribute_descriptions
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
            .front_face = VK_FRONT_FACE_COUNTER_CLOCKWISE,
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
            {
                .flags = 0,
                .set_layouts = { descriptor_set_layout },
                .push_constant_ranges = {}
            }
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
        swapchain_framebufs.clear();
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

    void App::create_command_pools()
    {
        auto pool_result = bv::CommandPool::create(
            device,
            {
                .flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
                .queue_family_index = graphics_family_idx
            }
        );
        CHECK_BV_RESULT(pool_result, "create command pool");
        cmd_pool = pool_result.value();

        auto transfer_pool_result = bv::CommandPool::create(
            device,
            {
                .flags = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT,
                .queue_family_index = graphics_family_idx
            }
        );
        CHECK_BV_RESULT(transfer_pool_result, "create transfer command pool");
        transfer_cmd_pool = transfer_pool_result.value();
    }

    void App::create_vertex_buffer()
    {
        VkDeviceSize size = sizeof(vertices[0]) * vertices.size();

        bv::BufferPtr staging_buf;
        bv::DeviceMemoryPtr staging_buf_mem;
        create_buffer(
            size,
            VK_BUFFER_USAGE_TRANSFER_SRC_BIT,

            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT
            | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,

            staging_buf,
            staging_buf_mem
        );

        auto upload_result = staging_buf_mem->upload(
            (void*)vertices.data(),
            size
        );
        CHECK_BV_RESULT(upload_result, "upload vertex data");

        create_buffer(
            size,

            VK_BUFFER_USAGE_TRANSFER_DST_BIT
            | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,

            VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
            vertex_buf,
            vertex_buf_mem
        );
        copy_buffer(staging_buf, vertex_buf, size);

        staging_buf = nullptr;
        staging_buf_mem = nullptr;
    }

    void App::create_index_buffer()
    {
        VkDeviceSize size = sizeof(indices[0]) * indices.size();

        bv::BufferPtr staging_buf;
        bv::DeviceMemoryPtr staging_buf_mem;
        create_buffer(
            size,
            VK_BUFFER_USAGE_TRANSFER_SRC_BIT,

            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT
            | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,

            staging_buf,
            staging_buf_mem
        );

        auto upload_result = staging_buf_mem->upload(
            (void*)indices.data(),
            size
        );
        CHECK_BV_RESULT(upload_result, "upload index data");

        create_buffer(
            size,

            VK_BUFFER_USAGE_TRANSFER_DST_BIT
            | VK_BUFFER_USAGE_INDEX_BUFFER_BIT,

            VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
            index_buf,
            index_buf_mem
        );
        copy_buffer(staging_buf, index_buf, size);

        staging_buf = nullptr;
        staging_buf_mem = nullptr;
    }

    void App::create_uniform_buffers()
    {
        VkDeviceSize size = sizeof(UniformBufferObject);

        uniform_bufs.resize(MAX_FRAMES_IN_FLIGHT);
        uniform_bufs_mem.resize(MAX_FRAMES_IN_FLIGHT);
        uniform_bufs_mapped.resize(MAX_FRAMES_IN_FLIGHT);

        for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
        {
            create_buffer(
                size,
                VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,

                VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT
                | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,

                uniform_bufs[i],
                uniform_bufs_mem[i]
            );

            auto map_result = uniform_bufs_mem[i]->map(0, size);
            CHECK_BV_RESULT(map_result, "map memory");
            uniform_bufs_mapped[i] = map_result.value();
        }
    }

    void App::create_descriptor_pool()
    {
        bv::DescriptorPoolSize pool_size{
            .type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
            .descriptor_count = MAX_FRAMES_IN_FLIGHT
        };

        auto pool_result = bv::DescriptorPool::create(
            device,
            {
                .flags = 0,
                .max_sets = MAX_FRAMES_IN_FLIGHT,
                .pool_sizes = { pool_size }
            }
        );
        CHECK_BV_RESULT(pool_result, "create descriptor pool");
        descriptor_pool = pool_result.value();
    }

    void App::create_descriptor_sets()
    {
        auto sets_result = bv::DescriptorPool::allocate_sets(
            descriptor_pool,
            MAX_FRAMES_IN_FLIGHT,
            std::vector<bv::DescriptorSetLayoutPtr>(
                MAX_FRAMES_IN_FLIGHT,
                descriptor_set_layout
            )
        );
        CHECK_BV_RESULT(sets_result, "allocate descriptor sets");
        descriptor_sets = sets_result.value();

        for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
        {
            bv::DescriptorBufferInfo buffer_info{
                .buffer = uniform_bufs[i],
                .offset = 0,
                .range = sizeof(UniformBufferObject)
            };

            bv::WriteDescriptorSet descriptor_write{
                .dst_set = descriptor_sets[i],
                .dst_binding = 0,
                .dst_array_element = 0,
                .descriptor_count = 1,
                .descriptor_type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
                .image_infos = {},
                .buffer_infos = { buffer_info },
                .texel_buffer_views = {}
            };

            bv::DescriptorSet::update_sets(device, { descriptor_write }, {});
        }
    }

    void App::create_command_buffers()
    {
        auto cmd_bufs_result = bv::CommandPool::allocate_buffers(
            cmd_pool,
            VK_COMMAND_BUFFER_LEVEL_PRIMARY,
            MAX_FRAMES_IN_FLIGHT
        );
        CHECK_BV_RESULT(cmd_bufs_result, "allocate command buffers");
        cmd_bufs = cmd_bufs_result.value();
    }

    void App::create_sync_objects()
    {
        for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
        {
            auto semaph_result = bv::Semaphore::create(device);
            CHECK_BV_RESULT(semaph_result, "create semaphore");
            semaphs_image_available.push_back(semaph_result.value());

            semaph_result = bv::Semaphore::create(device);
            CHECK_BV_RESULT(semaph_result, "create semaphore");
            semaphs_render_finished.push_back(semaph_result.value());

            auto fence_result = bv::Fence::create(
                device,
                VK_FENCE_CREATE_SIGNALED_BIT
            );
            CHECK_BV_RESULT(fence_result, "create fence");
            fences_in_flight.push_back(fence_result.value());
        }
    }

    void App::draw_frame()
    {
        auto result = fences_in_flight[frame_idx]->wait();
        CHECK_BV_RESULT(result, "wait for fence");

        bv::ApiResult acquire_api_result;
        auto acquire_result = swapchain->acquire_next_image(
            semaphs_image_available[frame_idx],
            nullptr,
            UINT64_MAX,
            &acquire_api_result
        );
        if (acquire_api_result == bv::ApiResult::ErrorOutOfDateKhr)
        {
            recreate_swapchain();
            return;
        }
        else if (!acquire_result.ok())
        {
            throw std::runtime_error(std::format(
                "failed to acquire the next swapchain image: {}",
                acquire_result.error().to_string()
            ).c_str());
        };

        uint32_t img_idx = acquire_result.value();

        update_uniform_buffer(frame_idx);

        result = fences_in_flight[frame_idx]->reset();
        CHECK_BV_RESULT(result, "reset fence");

        result = cmd_bufs[frame_idx]->reset(0);
        CHECK_BV_RESULT(result, "reset command buffer");
        record_command_buffer(cmd_bufs[frame_idx], img_idx);

        result = graphics_queue->submit(
            { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT },
            { semaphs_image_available[frame_idx] },
            { cmd_bufs[frame_idx] },
            { semaphs_render_finished[frame_idx] },
            fences_in_flight[frame_idx]
        );
        CHECK_BV_RESULT(result, "submit command buffer");

        bv::ApiResult present_api_result;
        auto present_result = presentation_queue->present(
            { semaphs_render_finished[frame_idx] },
            swapchain,
            img_idx,
            &present_api_result
        );
        if (present_api_result == bv::ApiResult::ErrorOutOfDateKhr
            || present_api_result == bv::ApiResult::SuboptimalKhr
            || framebuf_resized)
        {
            framebuf_resized = false;
            recreate_swapchain();
        }
        else if (!present_result.ok())
        {
            throw std::runtime_error(std::format(
                "failed to present image: {}",
                present_result.error().to_string()
            ).c_str());
        }

        frame_idx = (frame_idx + 1) % MAX_FRAMES_IN_FLIGHT;
    }

    void App::cleanup_swapchain()
    {
        swapchain_framebufs.clear();
        swapchain_imgviews.clear();
        swapchain = nullptr;
    }

    void App::recreate_swapchain()
    {
        int width = 0, height = 0;
        glfwGetFramebufferSize(window, &width, &height);
        while (width == 0 || height == 0)
        {
            glfwGetFramebufferSize(window, &width, &height);
            glfwWaitEvents();
        }

        auto result = device->wait_idle();
        CHECK_BV_RESULT(result, "wait for device idle");

        cleanup_swapchain();
        create_swapchain();
        create_framebuffers();
    }

    uint32_t App::find_memory_type_idx(
        uint32_t supported_type_bits,
        VkMemoryPropertyFlags required_properties
    )
    {
        const auto& mem_props = physical_device->memory_properties();
        for (uint32_t i = 0; i < mem_props.memory_types.size(); i++)
        {
            bool has_required_properties =
                (required_properties & mem_props.memory_types[i].property_flags)
                == required_properties;
            if ((supported_type_bits & (1 << i)) && has_required_properties)
            {
                return i;
            }
        }
        throw std::runtime_error("failed to find a suitable memory type");
    }

    void App::create_buffer(
        VkDeviceSize size,
        VkBufferUsageFlags usage,
        VkMemoryPropertyFlags properties,
        bv::BufferPtr& out_buffer,
        bv::DeviceMemoryPtr& out_buffer_memory
    )
    {
        // create buffer
        auto buf_result = bv::Buffer::create(
            device,
            {
                .flags = 0,
                .size = size,
                .usage = usage,
                .sharing_mode = VK_SHARING_MODE_EXCLUSIVE,
                .queue_family_indices = {}
            }
        );
        CHECK_BV_RESULT(buf_result, "create buffer");
        out_buffer = buf_result.value();

        // create memory
        uint32_t memory_type_idx = find_memory_type_idx(
            out_buffer->memory_requirements().memory_type_bits,
            properties
        );
        auto buf_mem_result = bv::DeviceMemory::allocate(
            device,
            {
                .allocation_size = out_buffer->memory_requirements().size,
                .memory_type_index = memory_type_idx
            }
        );
        CHECK_BV_RESULT(buf_mem_result, "allocate buffer memory");
        out_buffer_memory = buf_mem_result.value();

        // bind memory
        auto bind_result = out_buffer->bind_memory(out_buffer_memory, 0);
        CHECK_BV_RESULT(bind_result, "bind buffer memory");
    }

    void App::copy_buffer(
        bv::BufferPtr src,
        bv::BufferPtr dst,
        VkDeviceSize size
    )
    {
        auto transfer_cmd_buf_result = bv::CommandPool::allocate_buffer(
            transfer_cmd_pool,
            VK_COMMAND_BUFFER_LEVEL_PRIMARY
        );
        CHECK_BV_RESULT(transfer_cmd_buf_result, "allocate command buffer");
        auto transfer_cmd_buf = transfer_cmd_buf_result.value();

        auto result = transfer_cmd_buf->begin(
            VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT
        );
        CHECK_BV_RESULT(result, "begin command buffer");

        VkBufferCopy copy_region{
            .srcOffset = 0,
            .dstOffset = 0,
            .size = size
        };
        vkCmdCopyBuffer(
            transfer_cmd_buf->handle(),
            src->handle(),
            dst->handle(),
            1,
            &copy_region
        );

        result = transfer_cmd_buf->end();
        CHECK_BV_RESULT(result, "end command buffer");

        result = graphics_queue->submit({}, {}, { transfer_cmd_buf }, {});
        CHECK_BV_RESULT(result, "submit command buffer");

        result = graphics_queue->wait_idle();
        CHECK_BV_RESULT(result, "wait for queue idle");
    }

    void App::record_command_buffer(
        const bv::CommandBufferPtr& cmd_buf,
        uint32_t img_idx
    )
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

        VkBuffer vk_vertex_bufs[] = { vertex_buf->handle() };
        VkDeviceSize offsets[] = { 0 };
        vkCmdBindVertexBuffers(
            cmd_buf->handle(),
            0,
            1,
            vk_vertex_bufs,
            offsets
        );

        vkCmdBindIndexBuffer(
            cmd_buf->handle(),
            index_buf->handle(),
            0,
            VK_INDEX_TYPE_UINT16
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

        auto vk_descriptor_set = descriptor_sets[frame_idx]->handle();
        vkCmdBindDescriptorSets(
            cmd_buf->handle(),
            VK_PIPELINE_BIND_POINT_GRAPHICS,
            pipeline_layout->handle(),
            0,
            1,
            &vk_descriptor_set,
            0,
            nullptr
        );

        vkCmdDrawIndexed(
            cmd_buf->handle(),
            (uint32_t)(indices.size()),
            1, 0, 0, 0
        );

        vkCmdEndRenderPass(cmd_buf->handle());

        auto end_result = cmd_buf->end();
        CHECK_BV_RESULT(end_result, "end recording command buffer");
    }

    void App::update_uniform_buffer(uint32_t frame_idx)
    {
        auto curr_time = std::chrono::high_resolution_clock::now();
        float elapsed =
            std::chrono::duration<float>(curr_time - start_time).count();

        UniformBufferObject ubo{};

        ubo.model = glm::rotate(
            glm::mat4(1.f),
            elapsed * glm::radians(90.f),
            glm::vec3(0.f, 0.f, 1.f)
        );

        ubo.view = glm::lookAt(
            glm::vec3(2.f, 2.f, 2.f),
            glm::vec3(0.f, 0.f, 0.f),
            glm::vec3(0.f, 0.f, 1.f)
        );

        auto sc_extent = swapchain->config().image_extent;
        ubo.proj = glm::perspective(
            glm::radians(45.f),
            (float)sc_extent.width / (float)sc_extent.height,
            .1f,
            10.f
        );
        ubo.proj[1][1] *= -1.f;

        std::copy(
            &ubo,
            &ubo + 1,
            (UniformBufferObject*)uniform_bufs_mapped[frame_idx]
        );
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

    static void glfw_framebuf_resize_callback(
        GLFWwindow* window,
        int width,
        int height
    )
    {
        App* app = (App*)(glfwGetWindowUserPointer(window));
        app->framebuf_resized = true;
    }

}
