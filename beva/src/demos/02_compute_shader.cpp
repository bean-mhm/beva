#include "02_compute_shader.hpp"

#include <iostream>
#include <fstream>
#include <format>
#include <set>
#include <limits>
#include <algorithm>
#include <stdexcept>
#include <cstdlib>
#include <cmath>

#define IDIV_CEIL(a, b) (((a) + (b) - 1) / (b))

namespace beva_demo_02_compute_shader
{

    static std::vector<uint8_t> read_file(const std::string& filename);
    static void glfw_error_callback(int error, const char* description);
    static void glfw_framebuf_resize_callback(
        GLFWwindow* window,
        int width,
        int height
    );

    const bv::VertexInputBindingDescription Vertex::binding{
        .binding = 0,
        .stride = sizeof(Vertex),
        .input_rate = VK_VERTEX_INPUT_RATE_VERTEX
    };

    static const std::vector<bv::VertexInputAttributeDescription>
        attributes
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
            .format = VK_FORMAT_R32G32_SFLOAT,
            .offset = offsetof(Vertex, texcoord)
    }
    };

    static const std::vector<Vertex> vertices{
        { .pos = { -1.f, 1.f }, .texcoord = { 0.f, 1.f } }, // bl
        { .pos = { 1.f, 1.f }, .texcoord = { 1.f, 1.f } }, // br
        { .pos = { 1.f, -1.f }, .texcoord = { 1.f, 0.f } }, // tr
        { .pos = { -1.f, 1.f }, .texcoord = { 0.f, 1.f } }, // bl
        { .pos = { 1.f, -1.f }, .texcoord = { 1.f, 0.f } }, // tr
        { .pos = { -1.f, -1.f }, .texcoord = { 0.f, 0.f } } // tl
    };

    void App::run()
    {
        try
        {
            init();
            main_loop();
            cleanup();
        }
        catch (const bv::Error& e)
        {
            throw std::runtime_error(e.to_string().c_str());
        }
    }

    void App::init()
    {
        std::cout <<
            "controls:\n"
            "[Drag Mouse]: draw waves\n\n";

        start_time = std::chrono::high_resolution_clock::now();

        init_window();
        init_context();
        setup_debug_messenger();
        create_surface();
        pick_physical_device();
        create_logical_device();
        create_memory_bank();
        create_swapchain();

        create_render_pass();
        create_graphics_descriptor_set_layout();
        create_graphics_pipeline();

        create_compute_descriptor_set_layout();
        create_compute_pipeline();

        create_command_pools();
        create_swapchain_framebuffers();
        create_storage_images();
        create_vertex_buffer();

        create_graphics_descriptor_pool();
        create_graphics_descriptor_sets();

        create_compute_descriptor_pool();
        create_compute_descriptor_sets();

        create_command_buffers();
        create_sync_objects();
    }

    void App::main_loop()
    {
        start_time = std::chrono::high_resolution_clock::now();
        while (true)
        {
            glfwPollEvents();
            draw_frame();

            if (glfwWindowShouldClose(window))
            {
                break;
            }
        }
        device->wait_idle();
    }

    void App::cleanup()
    {
        bv::clear(fences_in_flight);
        bv::clear(semaphs_render_finished);
        bv::clear(semaphs_image_available);

        compute_descriptor_pool = nullptr;
        graphics_descriptor_pool = nullptr;

        vertex_buf = nullptr;
        vertex_buf_mem = nullptr;

        bv::clear(storage_imgviews);
        bv::clear(storage_imgs);
        bv::clear(storage_imgs_mem);

        cleanup_swapchain();

        transient_cmd_pool = nullptr;
        cmd_pool = nullptr;

        compute_pipeline = nullptr;
        compute_pipeline_layout = nullptr;
        compute_descriptor_set_layout = nullptr;

        graphics_pipeline = nullptr;
        graphics_pipeline_layout = nullptr;
        graphics_descriptor_set_layout = nullptr;

        render_pass = nullptr;

        mem_bank = nullptr;
        device = nullptr;
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
        glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);

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

        context = bv::Context::create({
            .will_enumerate_portability = false,
            .app_name = "beva demo",
            .app_version = bv::Version(1, 1, 0, 0),
            .engine_name = "no engine",
            .engine_version = bv::Version(1, 1, 0, 0),
            .vulkan_api_version = bv::VulkanApiVersion::Vulkan1_0,
            .layers = layers,
            .extensions = extensions
            });
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

        debug_messenger = bv::DebugMessenger::create(
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
            throw bv::Error(
                "failed to create window surface",
                vk_result,
                false
            );
        }
        surface = bv::Surface::create(context, vk_surface);
    }

    void App::pick_physical_device()
    {
        // make a list of devices we approve of
        auto all_physical_devices = context->fetch_physical_devices(surface);
        std::vector<bv::PhysicalDevice> supported_physical_devices;
        for (const auto& pdev : all_physical_devices)
        {
            bool has_graphics_compute_family = false;
            for (const auto& family : pdev.queue_families())
            {
                if ((family.queue_flags & VK_QUEUE_GRAPHICS_BIT)
                    && (family.queue_flags & VK_QUEUE_COMPUTE_BIT))
                {
                    has_graphics_compute_family = true;
                    break;
                }
            }
            if (!has_graphics_compute_family) continue;

            if (!pdev.queue_family_indices().presentation.has_value())
            {
                continue;
            }

            if (!pdev.swapchain_support().has_value())
            {
                continue;
            }

            const auto& swapchain_support = pdev.swapchain_support().value();
            if (swapchain_support.present_modes.empty()
                || swapchain_support.surface_formats.empty())
            {
                continue;
            }

            try
            {
                auto format_props = pdev.fetch_image_format_properties(
                    SIM_IMAGE_FORMAT,
                    VK_IMAGE_TYPE_2D,
                    VK_IMAGE_TILING_OPTIMAL,
                    VK_IMAGE_USAGE_STORAGE_BIT,
                    0
                );
            }
            catch (const bv::Error&)
            {
                continue;
            }

            if (!pdev.features().sampler_anisotropy)
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
            switch (pdev.properties().device_type)
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
                pdev.properties().device_name,
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
        std::cout << '\n';

        physical_device = supported_physical_devices[idx];

        glfwShowWindow(window);
    }

    void App::create_logical_device()
    {
        const auto& families = physical_device.value().queue_families();
        for (uint32_t i = 0; i < families.size(); i++)
        {
            if ((families[i].queue_flags & VK_QUEUE_GRAPHICS_BIT)
                && (families[i].queue_flags & VK_QUEUE_COMPUTE_BIT))
            {
                graphics_compute_family_idx = i;
                break;
            }
        }

        presentation_family_idx =
            physical_device->queue_family_indices().presentation.value();

        std::set<uint32_t> unique_queue_family_indices = {
            graphics_compute_family_idx,
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

        bv::PhysicalDeviceFeatures enabled_features{};
        enabled_features.sampler_anisotropy = true;

        device = bv::Device::create(
            context,
            physical_device.value(),
            {
                .queue_requests = queue_requests,
                .extensions = { VK_KHR_SWAPCHAIN_EXTENSION_NAME },
                .enabled_features = enabled_features
            }
        );

        graphics_compute_queue =
            bv::Device::retrieve_queue(device, graphics_compute_family_idx, 0);

        presentation_queue =
            bv::Device::retrieve_queue(device, presentation_family_idx, 0);
    }

    void App::create_memory_bank()
    {
        mem_bank = bv::MemoryBank::create(device);
    }

    void App::create_swapchain()
    {
        physical_device->update_swapchain_support(surface);
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
        if (graphics_compute_family_idx != presentation_family_idx)
        {
            image_sharing_mode = VK_SHARING_MODE_CONCURRENT;
            queue_family_indices = {
                graphics_compute_family_idx,
                presentation_family_idx
            };
        }
        else
        {
            image_sharing_mode = VK_SHARING_MODE_EXCLUSIVE;
        }

        auto pre_transform = swapchain_support.capabilities.current_transform;

        // create swapchain
        swapchain = bv::Swapchain::create(
            device,
            surface,
            {
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
            }
        );

        // create swapchain image views
        bv::clear(swapchain_imgviews);
        for (size_t i = 0; i < swapchain->images().size(); i++)
        {
            swapchain_imgviews.push_back(create_image_view(
                swapchain->images()[i],
                surface_format.format,
                VK_IMAGE_ASPECT_COLOR_BIT,
                1
            ));
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

        render_pass = bv::RenderPass::create(
            device,
            {
                .flags = 0,
                .attachments = { color_attachment },
                .subpasses = { subpass },
                .dependencies = { dependency }
            }
        );
    }

    void App::create_graphics_descriptor_set_layout()
    {
        bv::DescriptorSetLayoutBinding fragment_img_binding{
            .binding = 0,
            .descriptor_type = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
            .descriptor_count = 1,
            .stage_flags = VK_SHADER_STAGE_FRAGMENT_BIT,
            .immutable_samplers = {}
        };

        graphics_descriptor_set_layout = bv::DescriptorSetLayout::create(
            device,
            {
                .flags = 0,
                .bindings = { fragment_img_binding }
            }
        );
    }

    void App::create_graphics_pipeline()
    {
        // shader modules
        // they are local variables because they're only needed until pipeline
        // creation.

        auto vert_shader_code = read_file("./shaders/demo_02_vert.spv");
        auto frag_shader_code = read_file("./shaders/demo_02_frag.spv");

        auto vert_shader_module = bv::ShaderModule::create(
            device,
            std::move(vert_shader_code)
        );

        auto frag_shader_module = bv::ShaderModule::create(
            device,
            std::move(frag_shader_code)
        );

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
            .binding_descriptions = { Vertex::binding },
            .attribute_descriptions = attributes
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

        bv::DepthStencilState depth_stencil_state{
            .flags = 0,
            .depth_test_enable = true,
            .depth_write_enable = true,
            .depth_compare_op = VK_COMPARE_OP_LESS,
            .depth_bounds_test_enable = false,
            .stencil_test_enable = false,
            .front = {},
            .back = {},
            .min_depth_bounds = 0.f,
            .max_depth_bounds = 1.f
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

        graphics_pipeline_layout = bv::PipelineLayout::create(
            device,
            {
                .flags = 0,
                .set_layouts = { graphics_descriptor_set_layout },
                .push_constant_ranges = {}
            }
        );

        graphics_pipeline = bv::GraphicsPipeline::create(
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
                .depth_stencil_state = depth_stencil_state,
                .color_blend_state = color_blend_state,
                .dynamic_states = dynamic_states,
                .layout = graphics_pipeline_layout,
                .render_pass = render_pass,
                .subpass_index = 0,
                .base_pipeline = std::nullopt
            }
        );

        vert_shader_module = nullptr;
        frag_shader_module = nullptr;
    }

    void App::create_compute_descriptor_set_layout()
    {
        bv::DescriptorSetLayoutBinding compute_input_img_binding{
            .binding = 0,
            .descriptor_type = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
            .descriptor_count = 1,
            .stage_flags = VK_SHADER_STAGE_COMPUTE_BIT,
            .immutable_samplers = {}
        };
        bv::DescriptorSetLayoutBinding compute_output_img_binding{
            .binding = 1,
            .descriptor_type = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
            .descriptor_count = 1,
            .stage_flags = VK_SHADER_STAGE_COMPUTE_BIT,
            .immutable_samplers = {}
        };

        compute_descriptor_set_layout = bv::DescriptorSetLayout::create(
            device,
            {
                .flags = 0,
                .bindings = {
                compute_input_img_binding,
                compute_output_img_binding
            }
            }
        );
    }

    void App::create_compute_pipeline()
    {
        auto shader_module = bv::ShaderModule::create(
            device,
            std::move(read_file("./shaders/demo_02_comp.spv"))
        );

        std::vector<bv::SpecializationMapEntry> map_entries{
            {
                .constant_id = 0,
                .offset = 0,
                .size = 4
            },
            {
                .constant_id = 1,
                .offset = 4,
                .size = 4
            },
            {
                .constant_id = 2,
                .offset = 8,
                .size = 4
            }
        };

        const auto& limits = physical_device->properties().limits;
        compute_local_size = {
            std::clamp(limits.max_compute_work_group_size[0], 1u, 64u),
            std::clamp(limits.max_compute_work_group_size[1], 1u, 64u),
            1
        };
        while (
            compute_local_size.x
            * compute_local_size.y
            * compute_local_size.z > limits.max_compute_work_group_invocations)
        {
            compute_local_size.x = std::max(1u, compute_local_size.x / 2u);
            compute_local_size.y = std::max(1u, compute_local_size.y / 2u);
            compute_local_size.z = std::max(1u, compute_local_size.z / 2u);

            if (limits.max_compute_work_group_invocations < 1) break;
        }

        bv::SpecializationInfo specialization_info{
            .map_entries = map_entries,
            .data = std::vector<uint8_t>(sizeof(compute_local_size))
        };
        std::copy(
            &compute_local_size,
            &compute_local_size + 1,
            reinterpret_cast<glm::uvec3*>(specialization_info.data.data())
        );

        bv::ShaderStage shader_stage{
            .flags = {},
            .stage = VK_SHADER_STAGE_COMPUTE_BIT,
            .module = shader_module,
            .entry_point = "main",
            .specialization_info = specialization_info
        };

        bv::PushConstantRange push_constants{
            .stage_flags = VK_SHADER_STAGE_COMPUTE_BIT,
            .offset = 0,
            .size = sizeof(ComputeShaderPushConstants)
        };

        compute_pipeline_layout = bv::PipelineLayout::create(
            device,
            {
                .flags = 0,
                .set_layouts = { compute_descriptor_set_layout },
                .push_constant_ranges = { push_constants }
            }
        );

        compute_pipeline = bv::ComputePipeline::create(
            device,
            {
                .flags = 0,
                .stage = shader_stage,
                .layout = compute_pipeline_layout,
                .base_pipeline = std::nullopt
            }
        );

        shader_module = nullptr;
    }

    void App::create_command_pools()
    {
        cmd_pool = bv::CommandPool::create(
            device,
            {
                .flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
                .queue_family_index = graphics_compute_family_idx
            }
        );
        transient_cmd_pool = bv::CommandPool::create(
            device,
            {
                .flags = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT,
                .queue_family_index = graphics_compute_family_idx
            }
        );
    }

    void App::create_swapchain_framebuffers()
    {
        bv::clear(swapchain_framebufs);
        for (size_t i = 0; i < swapchain_imgviews.size(); i++)
        {
            swapchain_framebufs.push_back(bv::Framebuffer::create(
                device,
                {
                    .flags = 0,
                    .render_pass = render_pass,
                    .attachments = { swapchain_imgviews[i] },
                    .width = swapchain->config().image_extent.width,
                    .height = swapchain->config().image_extent.height,
                    .layers = 1
                }
            ));
        }
    }

    void App::create_storage_images()
    {
        storage_imgs.resize(MAX_FRAMES_IN_FLIGHT);
        storage_imgs_mem.resize(MAX_FRAMES_IN_FLIGHT);
        storage_imgviews.resize(MAX_FRAMES_IN_FLIGHT);

        auto cmd_buf = begin_single_time_commands(true);
        for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
        {
            create_image(
                SIM_RESOLUTION,
                SIM_RESOLUTION,
                1,
                VK_SAMPLE_COUNT_1_BIT,
                SIM_IMAGE_FORMAT,
                VK_IMAGE_TILING_OPTIMAL,
                VK_IMAGE_USAGE_STORAGE_BIT,
                VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                storage_imgs[i],
                storage_imgs_mem[i]
            );
            storage_imgviews[i] = create_image_view(
                storage_imgs[i],
                SIM_IMAGE_FORMAT,
                VK_IMAGE_ASPECT_COLOR_BIT,
                1
            );

            // transition layout to general
            VkImageMemoryBarrier barrier{
                .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
                .pNext = nullptr,
                .srcAccessMask = 0,
                .dstAccessMask = 0,
                .oldLayout = VK_IMAGE_LAYOUT_UNDEFINED,
                .newLayout = VK_IMAGE_LAYOUT_GENERAL,
                .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
                .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
                .image = storage_imgs[i]->handle(),
                .subresourceRange = VkImageSubresourceRange{
                    .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                    .baseMipLevel = 0,
                    .levelCount = 1,
                    .baseArrayLayer = 0,
                    .layerCount = 1
            }
            };
            vkCmdPipelineBarrier(
                cmd_buf->handle(),
                VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
                VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
                0,
                0, nullptr,
                0, nullptr,
                1, &barrier
            );
        }
        end_single_time_commands(cmd_buf);
    }

    void App::create_vertex_buffer()
    {
        VkDeviceSize size = sizeof(vertices[0]) * vertices.size();

        bv::BufferPtr staging_buf;
        bv::MemoryChunkPtr staging_buf_mem;
        create_buffer(
            size,
            VK_BUFFER_USAGE_TRANSFER_SRC_BIT,

            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT
            | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,

            staging_buf,
            staging_buf_mem
        );

        void* mapped = staging_buf_mem->mapped();
        std::copy(
            vertices.data(),
            vertices.data() + vertices.size(),
            (Vertex*)mapped
        );
        staging_buf_mem->flush();

        create_buffer(
            size,

            VK_BUFFER_USAGE_TRANSFER_DST_BIT
            | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,

            VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
            vertex_buf,
            vertex_buf_mem
        );

        auto cmd_buf = begin_single_time_commands(true);
        copy_buffer(cmd_buf, staging_buf, vertex_buf, size);
        end_single_time_commands(cmd_buf);

        staging_buf = nullptr;
        staging_buf_mem = nullptr;
    }

    void App::create_graphics_descriptor_pool()
    {
        std::vector<bv::DescriptorPoolSize> pool_sizes;
        pool_sizes.push_back({
            .type = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
            .descriptor_count = MAX_FRAMES_IN_FLIGHT
            });

        graphics_descriptor_pool = bv::DescriptorPool::create(
            device,
            {
                .flags = 0,
                .max_sets = MAX_FRAMES_IN_FLIGHT,
                .pool_sizes = pool_sizes
            }
        );
    }

    void App::create_graphics_descriptor_sets()
    {
        graphics_descriptor_sets = bv::DescriptorPool::allocate_sets(
            graphics_descriptor_pool,
            MAX_FRAMES_IN_FLIGHT,
            std::vector<bv::DescriptorSetLayoutPtr>(
                MAX_FRAMES_IN_FLIGHT,
                graphics_descriptor_set_layout
            )
        );

        for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
        {
            bv::DescriptorImageInfo img_info{
                .sampler = std::nullopt,
                .image_view = storage_imgviews[i],
                .image_layout = VK_IMAGE_LAYOUT_GENERAL
            };

            std::vector<bv::WriteDescriptorSet> descriptor_writes;
            descriptor_writes.push_back({
                .dst_set = graphics_descriptor_sets[i],
                .dst_binding = 0,
                .dst_array_element = 0,
                .descriptor_count = 1,
                .descriptor_type = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
                .image_infos = { img_info },
                .buffer_infos = {},
                .texel_buffer_views = {}
                });
            bv::DescriptorSet::update_sets(device, descriptor_writes, {});
        }
    }

    void App::create_compute_descriptor_pool()
    {
        std::vector<bv::DescriptorPoolSize> pool_sizes;
        pool_sizes.push_back({
            .type = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
            .descriptor_count = MAX_FRAMES_IN_FLIGHT
            });
        pool_sizes.push_back({
            .type = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
            .descriptor_count = MAX_FRAMES_IN_FLIGHT
            });

        compute_descriptor_pool = bv::DescriptorPool::create(
            device,
            {
                .flags = 0,
                .max_sets = MAX_FRAMES_IN_FLIGHT,
                .pool_sizes = pool_sizes
            }
        );
    }

    void App::create_compute_descriptor_sets()
    {
        compute_descriptor_sets = bv::DescriptorPool::allocate_sets(
            compute_descriptor_pool,
            MAX_FRAMES_IN_FLIGHT,
            std::vector<bv::DescriptorSetLayoutPtr>(
                MAX_FRAMES_IN_FLIGHT,
                compute_descriptor_set_layout
            )
        );

        for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
        {
            bv::DescriptorImageInfo input_img_info{
                .sampler = std::nullopt,
                .image_view = storage_imgviews[(i - 1) % MAX_FRAMES_IN_FLIGHT],
                .image_layout = VK_IMAGE_LAYOUT_GENERAL
            };

            bv::DescriptorImageInfo output_img_info{
                .sampler = std::nullopt,
                .image_view = storage_imgviews[i],
                .image_layout = VK_IMAGE_LAYOUT_GENERAL
            };

            std::vector<bv::WriteDescriptorSet> descriptor_writes;
            descriptor_writes.push_back({
                .dst_set = compute_descriptor_sets[i],
                .dst_binding = 0,
                .dst_array_element = 0,
                .descriptor_count = 1,
                .descriptor_type = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
                .image_infos = { input_img_info },
                .buffer_infos = {},
                .texel_buffer_views = {}
                });
            descriptor_writes.push_back({
                .dst_set = compute_descriptor_sets[i],
                .dst_binding = 1,
                .dst_array_element = 0,
                .descriptor_count = 1,
                .descriptor_type = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
                .image_infos = { output_img_info },
                .buffer_infos = {},
                .texel_buffer_views = {}
                });
            bv::DescriptorSet::update_sets(device, descriptor_writes, {});
        }
    }

    void App::create_command_buffers()
    {
        cmd_bufs = bv::CommandPool::allocate_buffers(
            cmd_pool,
            VK_COMMAND_BUFFER_LEVEL_PRIMARY,
            MAX_FRAMES_IN_FLIGHT
        );
    }

    void App::create_sync_objects()
    {
        bv::clear(semaphs_image_available);
        bv::clear(semaphs_render_finished);
        bv::clear(fences_in_flight);

        for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
        {
            semaphs_image_available.push_back(bv::Semaphore::create(device));
            semaphs_render_finished.push_back(bv::Semaphore::create(device));
            fences_in_flight.push_back(bv::Fence::create(
                device,
                VK_FENCE_CREATE_SIGNALED_BIT
            ));
        }
    }

    void App::draw_frame()
    {
        fences_in_flight[frame_idx]->wait();

        uint32_t img_idx;
        VkResult acquire_next_image_vk_result;
        try
        {
            img_idx = swapchain->acquire_next_image(
                semaphs_image_available[frame_idx],
                nullptr,
                UINT64_MAX,
                &acquire_next_image_vk_result
            );
        }
        catch (const bv::Error& e)
        {
            if (acquire_next_image_vk_result == VK_ERROR_OUT_OF_DATE_KHR)
            {
                recreate_swapchain();
                return;
            }
            else
            {
                throw e;
            }
        }

        const auto curr_time = std::chrono::high_resolution_clock::now();
        const float elapsed =
            std::chrono::duration<float>(curr_time - start_time).count();

        fences_in_flight[frame_idx]->reset();

        cmd_bufs[frame_idx]->reset(0);
        record_command_buffer(cmd_bufs[frame_idx], img_idx, elapsed);

        graphics_compute_queue->submit(
            { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT },
            { semaphs_image_available[frame_idx] },
            { cmd_bufs[frame_idx] },
            { semaphs_render_finished[frame_idx] },
            fences_in_flight[frame_idx]
        );

        VkResult present_vk_result;
        try
        {
            presentation_queue->present(
                { semaphs_render_finished[frame_idx] },
                swapchain,
                img_idx,
                &present_vk_result
            );
        }
        catch (const bv::Error& e)
        {
            if (present_vk_result != VK_ERROR_OUT_OF_DATE_KHR
                && present_vk_result != VK_SUBOPTIMAL_KHR)
            {
                throw e;
            }
        }
        if (present_vk_result == VK_ERROR_OUT_OF_DATE_KHR
            || present_vk_result == VK_SUBOPTIMAL_KHR
            || framebuf_resized)
        {
            framebuf_resized = false;
            recreate_swapchain();
        }

        frame_idx = (frame_idx + 1) % MAX_FRAMES_IN_FLIGHT;
        global_frame_idx++;
    }

    void App::cleanup_swapchain()
    {
        bv::clear(swapchain_framebufs);
        bv::clear(swapchain_imgviews);
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

        device->wait_idle();

        cleanup_swapchain();

        create_swapchain();
        create_swapchain_framebuffers();
    }

    bv::CommandBufferPtr App::begin_single_time_commands(
        bool use_transient_pool
    )
    {
        auto cmd_buf = bv::CommandPool::allocate_buffer(
            use_transient_pool ? transient_cmd_pool : cmd_pool,
            VK_COMMAND_BUFFER_LEVEL_PRIMARY
        );
        cmd_buf->begin(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);
        return cmd_buf;
    }

    void App::end_single_time_commands(
        bv::CommandBufferPtr& cmd_buf,
        const bv::FencePtr fence
    )
    {
        cmd_buf->end();
        graphics_compute_queue->submit({}, {}, { cmd_buf }, {}, fence);
        if (fence == nullptr)
        {
            graphics_compute_queue->wait_idle();
        }
        cmd_buf = nullptr;
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

    VkFormat App::find_depth_format()
    {
        auto format = physical_device->find_supported_image_format(
            {
                VK_FORMAT_D32_SFLOAT,
                VK_FORMAT_D32_SFLOAT_S8_UINT,
                VK_FORMAT_D24_UNORM_S8_UINT
            },
            VK_IMAGE_TILING_OPTIMAL,
            VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT
        );
        if (!format.has_value())
        {
            throw std::runtime_error("failed to find a supported depth format");
        }
        return format.value();
    }

    void App::create_image(
        uint32_t width,
        uint32_t height,
        uint32_t mip_levels,
        VkSampleCountFlagBits num_samples,
        VkFormat format,
        VkImageTiling tiling,
        VkImageUsageFlags usage,
        VkMemoryPropertyFlags memory_properties,
        bv::ImagePtr& out_image,
        bv::MemoryChunkPtr& out_memory_chunk
    )
    {
        bv::Extent3d extent{
            .width = width,
            .height = height,
            .depth = 1
        };
        out_image = bv::Image::create(
            device,
            {
                .flags = 0,
                .image_type = VK_IMAGE_TYPE_2D,
                .format = format,
                .extent = extent,
                .mip_levels = mip_levels,
                .array_layers = 1,
                .samples = num_samples,
                .tiling = tiling,
                .usage = usage,
                .sharing_mode = VK_SHARING_MODE_EXCLUSIVE,
                .queue_family_indices = {},
                .initial_layout = VK_IMAGE_LAYOUT_UNDEFINED
            }
        );

        out_memory_chunk = mem_bank->allocate(
            out_image->memory_requirements(),
            memory_properties
        );
        out_memory_chunk->bind(out_image);
    }

    bv::ImageViewPtr App::create_image_view(
        const bv::ImagePtr& image,
        VkFormat format,
        VkImageAspectFlags aspect_flags,
        uint32_t mip_levels
    )
    {
        bv::ImageSubresourceRange subresource_range{
            .aspect_mask = aspect_flags,
            .base_mip_level = 0,
            .level_count = mip_levels,
            .base_array_layer = 0,
            .layer_count = 1
        };

        return bv::ImageView::create(
            device,
            image,
            {
                .flags = 0,
                .view_type = VK_IMAGE_VIEW_TYPE_2D,
                .format = format,
                .components = {},
                .subresource_range = subresource_range
            }
        );
    }

    void App::create_buffer(
        VkDeviceSize size,
        VkBufferUsageFlags usage,
        VkMemoryPropertyFlags memory_properties,
        bv::BufferPtr& out_buffer,
        bv::MemoryChunkPtr& out_memory_chunk
    )
    {
        out_buffer = bv::Buffer::create(
            device,
            {
                .flags = 0,
                .size = size,
                .usage = usage,
                .sharing_mode = VK_SHARING_MODE_EXCLUSIVE,
                .queue_family_indices = {}
            }
        );

        out_memory_chunk = mem_bank->allocate(
            out_buffer->memory_requirements(),
            memory_properties
        );
        out_memory_chunk->bind(out_buffer);
    }

    void App::copy_buffer(
        const bv::CommandBufferPtr& cmd_buf,
        bv::BufferPtr src,
        bv::BufferPtr dst,
        VkDeviceSize size
    )
    {
        VkBufferCopy copy_region{
            .srcOffset = 0,
            .dstOffset = 0,
            .size = size
        };
        vkCmdCopyBuffer(
            cmd_buf->handle(),
            src->handle(),
            dst->handle(),
            1,
            &copy_region
        );
    }

    void App::record_command_buffer(
        const bv::CommandBufferPtr& cmd_buf,
        uint32_t img_idx,
        float elapsed
    )
    {
        cmd_buf->begin(0);

        vkCmdBindPipeline(
            cmd_buf->handle(),
            VK_PIPELINE_BIND_POINT_COMPUTE,
            compute_pipeline->handle()
        );

        auto vk_descriptor_set = compute_descriptor_sets[frame_idx]->handle();
        vkCmdBindDescriptorSets(
            cmd_buf->handle(),
            VK_PIPELINE_BIND_POINT_COMPUTE,
            compute_pipeline_layout->handle(),
            0,
            1,
            &vk_descriptor_set,
            0,
            0
        );

        glm::ivec2 mouse_icoord{ -1, -1 };
        if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS)
        {
            double xpos, ypos;
            glfwGetCursorPos(window, &xpos, &ypos);

            auto extent = swapchain->config().image_extent;
            mouse_icoord = {
                int32_t((double)SIM_RESOLUTION * xpos / (double)extent.width),
                int32_t((double)SIM_RESOLUTION * ypos / (double)extent.height)
            };
        }

        ComputeShaderPushConstants compute_push_constants{
            .emitter_icoord = mouse_icoord,
            .global_frame_idx = (uint32_t)global_frame_idx
        };
        vkCmdPushConstants(
            cmd_buf->handle(),
            compute_pipeline_layout->handle(),
            VK_SHADER_STAGE_COMPUTE_BIT,
            0,
            sizeof(compute_push_constants),
            &compute_push_constants
        );

        vkCmdDispatch(
            cmd_buf->handle(),
            IDIV_CEIL(SIM_RESOLUTION, compute_local_size.x),
            IDIV_CEIL(SIM_RESOLUTION, compute_local_size.y),
            1
        );

        VkClearValue clear_val;
        clear_val.color = { { 0.f, 0.f, 0.f, 1.f } };

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

        VkBuffer vk_vertex_buf = vertex_buf->handle();
        VkDeviceSize offset = 0;
        vkCmdBindVertexBuffers(
            cmd_buf->handle(),
            0,
            1,
            &vk_vertex_buf,
            &offset
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

        vk_descriptor_set = graphics_descriptor_sets[frame_idx]->handle();
        vkCmdBindDescriptorSets(
            cmd_buf->handle(),
            VK_PIPELINE_BIND_POINT_GRAPHICS,
            graphics_pipeline_layout->handle(),
            0,
            1,
            &vk_descriptor_set,
            0,
            nullptr
        );

        vkCmdDraw(
            cmd_buf->handle(),
            (uint32_t)vertices.size(),
            1,
            0,
            0
        );

        vkCmdEndRenderPass(cmd_buf->handle());

        cmd_buf->end();
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
