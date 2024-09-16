#include "03_deferred_rendering.hpp"

#include <iostream>
#include <fstream>
#include <format>
#include <set>
#include <limits>
#include <algorithm>
#include <stdexcept>
#include <cstdlib>
#include <cmath>

#include "stb_image/stb_image.h"

#include "tiny_obj_loader/tiny_obj_loader.h"

namespace beva_demo_03_deferred_rendering
{

    static float elapsed_since(const std::chrono::steady_clock::time_point& t);

    static float wrap(float x, float start, float end);

    // r, theta, phi
    static glm::vec3 spherical_to_cartesian(glm::vec3 s);

    // theta, phi
    static glm::vec3 spherical_to_cartesian(glm::vec2 s);

    static std::vector<uint8_t> read_file(const std::string& filename);

    static void glfw_error_callback(int error, const char* description);
    static void glfw_framebuf_resize_callback(
        GLFWwindow* window,
        int width,
        int height
    );
    void glfw_key_callback(
        GLFWwindow*
        window,
        int key,
        int scancode,
        int action,
        int mods
    );

    const bv::VertexInputBindingDescription GeometryPassVertex::binding{
        .binding = 0,
        .stride = sizeof(GeometryPassVertex),
        .input_rate = VK_VERTEX_INPUT_RATE_VERTEX
    };

    bool GeometryPassVertex::operator==(const GeometryPassVertex& other) const
    {
        return
            pos == other.pos
            && normal == other.normal
            && texcoord == other.texcoord;
    }

    const bv::VertexInputBindingDescription FlatVertex::binding{
        .binding = 0,
        .stride = sizeof(FlatVertex),
        .input_rate = VK_VERTEX_INPUT_RATE_VERTEX
    };

    static const std::vector<bv::VertexInputAttributeDescription>
        gpass_vert_attributes
    {
        bv::VertexInputAttributeDescription{
            .location = 0,
            .binding = 0,
            .format = VK_FORMAT_R32G32B32_SFLOAT,
            .offset = offsetof(GeometryPassVertex, pos)
    },
        bv::VertexInputAttributeDescription{
            .location = 1,
            .binding = 0,
            .format = VK_FORMAT_R32G32B32_SFLOAT,
            .offset = offsetof(GeometryPassVertex, normal)
    },
        bv::VertexInputAttributeDescription{
            .location = 2,
            .binding = 0,
            .format = VK_FORMAT_R32G32_SFLOAT,
            .offset = offsetof(GeometryPassVertex, texcoord)
    }
    };

    static const std::vector<bv::VertexInputAttributeDescription>
        flat_vert_attributes
    {
        bv::VertexInputAttributeDescription{
            .location = 0,
            .binding = 0,
            .format = VK_FORMAT_R32G32_SFLOAT,
            .offset = offsetof(FlatVertex, pos)
    },
        bv::VertexInputAttributeDescription{
            .location = 1,
            .binding = 0,
            .format = VK_FORMAT_R32G32_SFLOAT,
            .offset = offsetof(FlatVertex, texcoord)
    }
    };

    static const std::vector<FlatVertex> quad_vertices{
        { .pos = { -1.f, 1.f }, .texcoord = { 0.f, 1.f } }, // bl
        { .pos = { 1.f, 1.f }, .texcoord = { 1.f, 1.f } }, // br
        { .pos = { 1.f, -1.f }, .texcoord = { 1.f, 0.f } }, // tr
        { .pos = { -1.f, 1.f }, .texcoord = { 0.f, 1.f } }, // bl
        { .pos = { 1.f, -1.f }, .texcoord = { 1.f, 0.f } }, // tr
        { .pos = { -1.f, -1.f }, .texcoord = { 0.f, 0.f } } // tl
    };

    Light::Light(
        LightType type,
        const glm::vec3& col,
        const glm::vec3& pos_or_dir
    )
        : data0(col, (float)(int32_t)type),
        data1(pos_or_dir, 0.f)
    {}

    GeometryPassRecreatables::GeometryPassRecreatables(App& app)
    {
        init(app);
    }

    GeometryPassRecreatables::~GeometryPassRecreatables()
    {
        cleanup();
    }

    void GeometryPassRecreatables::init(App& app)
    {
        auto sc_extent = app.swapchain->config().image_extent;

        // images

        auto layout_tran_cmd_buf = app.begin_single_time_commands(true);

        app.create_image(
            sc_extent.width,
            sc_extent.height,
            1,
            VK_SAMPLE_COUNT_1_BIT,
            DIFFUSE_METALLIC_GBUF_FORMAT,
            VK_IMAGE_TILING_OPTIMAL,
            VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
            VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
            diffuse_metallic_img,
            diffuse_metallic_mem
        );
        diffuse_metallic_view = app.create_image_view(
            diffuse_metallic_img,
            DIFFUSE_METALLIC_GBUF_FORMAT,
            VK_IMAGE_ASPECT_COLOR_BIT,
            1
        );
        app.transition_image_layout(
            layout_tran_cmd_buf,
            diffuse_metallic_img,
            VK_IMAGE_LAYOUT_UNDEFINED,
            VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
            1
        );

        app.create_image(
            sc_extent.width,
            sc_extent.height,
            1,
            VK_SAMPLE_COUNT_1_BIT,
            NORMAL_ROUGHNESS_GBUF_FORMAT,
            VK_IMAGE_TILING_OPTIMAL,
            VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
            VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
            normal_roughness_img,
            normal_roughness_mem
        );
        normal_roughness_view = app.create_image_view(
            normal_roughness_img,
            NORMAL_ROUGHNESS_GBUF_FORMAT,
            VK_IMAGE_ASPECT_COLOR_BIT,
            1
        );
        app.transition_image_layout(
            layout_tran_cmd_buf,
            normal_roughness_img,
            VK_IMAGE_LAYOUT_UNDEFINED,
            VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
            1
        );

        VkFormat depth_format = app.find_depth_format();
        app.create_image(
            sc_extent.width,
            sc_extent.height,
            1,
            VK_SAMPLE_COUNT_1_BIT,
            depth_format,
            VK_IMAGE_TILING_OPTIMAL,

            VK_IMAGE_USAGE_SAMPLED_BIT
            | VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,

            VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
            depth_img,
            depth_img_mem
        );
        depth_imgview = app.create_image_view(
            depth_img,
            depth_format,
            VK_IMAGE_ASPECT_DEPTH_BIT,
            1
        );
        app.transition_image_layout(
            layout_tran_cmd_buf,
            depth_img,
            VK_IMAGE_LAYOUT_UNDEFINED,
            VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
            1
        );

        app.end_single_time_commands(layout_tran_cmd_buf);

        // samplers

        for (size_t i = 0; i < 3; i++)
        {
            bv::SamplerPtr& sampler =
                (i == 0) ? diffuse_metallic_sampler
                : (i == 1) ? normal_roughness_sampler
                : depth_sampler;

            sampler = bv::Sampler::create(
                app.device,
                {
                    .flags = 0,
                    .mag_filter = VK_FILTER_LINEAR,
                    .min_filter = VK_FILTER_LINEAR,
                    .mipmap_mode = VK_SAMPLER_MIPMAP_MODE_NEAREST,
                    .address_mode_u = VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT,
                    .address_mode_v = VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT,
                    .address_mode_w = VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT,
                    .mip_lod_bias = 0.f,
                    .anisotropy_enable = false,
                    .max_anisotropy = 1.,
                    .compare_enable = false,
                    .compare_op = VK_COMPARE_OP_ALWAYS,
                    .min_lod = 0.,
                    .max_lod = VK_LOD_CLAMP_NONE,
                    .border_color = VK_BORDER_COLOR_INT_OPAQUE_BLACK,
                    .unnormalized_coordinates = false
                }
            );
        }

        // render pass

        bv::Attachment diffuse_metallic_attachment{
            .flags = 0,
            .format = DIFFUSE_METALLIC_GBUF_FORMAT,
            .samples = VK_SAMPLE_COUNT_1_BIT,
            .load_op = VK_ATTACHMENT_LOAD_OP_CLEAR,
            .store_op = VK_ATTACHMENT_STORE_OP_STORE,
            .stencil_load_op = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
            .stencil_store_op = VK_ATTACHMENT_STORE_OP_DONT_CARE,
            .initial_layout = VK_IMAGE_LAYOUT_UNDEFINED,
            .final_layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
        };

        bv::AttachmentReference diffuse_metallic_attachment_ref{
            .attachment = 0,
            .layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
        };

        bv::Attachment normal_roughness_attachment{
            .flags = 0,
            .format = NORMAL_ROUGHNESS_GBUF_FORMAT,
            .samples = VK_SAMPLE_COUNT_1_BIT,
            .load_op = VK_ATTACHMENT_LOAD_OP_CLEAR,
            .store_op = VK_ATTACHMENT_STORE_OP_STORE,
            .stencil_load_op = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
            .stencil_store_op = VK_ATTACHMENT_STORE_OP_DONT_CARE,
            .initial_layout = VK_IMAGE_LAYOUT_UNDEFINED,
            .final_layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
        };

        bv::AttachmentReference normal_roughness_attachment_ref{
            .attachment = 1,
            .layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
        };

        bv::Attachment depth_attachment{
            .flags = 0,
            .format = depth_format,
            .samples = VK_SAMPLE_COUNT_1_BIT,
            .load_op = VK_ATTACHMENT_LOAD_OP_CLEAR,
            .store_op = VK_ATTACHMENT_STORE_OP_STORE,
            .stencil_load_op = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
            .stencil_store_op = VK_ATTACHMENT_STORE_OP_DONT_CARE,
            .initial_layout = VK_IMAGE_LAYOUT_UNDEFINED,
            .final_layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
        };

        bv::AttachmentReference depth_attachment_ref{
            .attachment = 2,
            .layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL
        };

        bv::Subpass subpass{
            .flags = 0,
            .pipeline_bind_point = VK_PIPELINE_BIND_POINT_GRAPHICS,
            .input_attachments = {},
            .color_attachments = {
                diffuse_metallic_attachment_ref,
                normal_roughness_attachment_ref
        },
            .resolve_attachments = {},
            .depth_stencil_attachment = depth_attachment_ref,
            .preserve_attachment_indices = {}
        };

        bv::SubpassDependency dependency{
            .src_subpass = VK_SUBPASS_EXTERNAL,
            .dst_subpass = 0,

            .src_stage_mask =
            VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT
            | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT,

            .dst_stage_mask =
            VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT
            | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT,

            .src_access_mask =
            VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT
            | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,

            .dst_access_mask =
            VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT
            | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,

            .dependency_flags = 0
        };

        render_pass = bv::RenderPass::create(
            app.device,
            {
                .flags = 0,
                .attachments = {
                diffuse_metallic_attachment,
                normal_roughness_attachment,
                depth_attachment
            },
            .subpasses = { subpass },
            .dependencies = { dependency }
            }
        );

        // framebuffer
        framebuf = bv::Framebuffer::create(
            app.device,
            {
                .flags = 0,
                .render_pass = render_pass,
                .attachments = {
                diffuse_metallic_view,
                normal_roughness_view,
                depth_imgview
            },
            .width = sc_extent.width,
            .height = sc_extent.height,
            .layers = 1
            }
        );
    }

    void GeometryPassRecreatables::cleanup()
    {
        framebuf = nullptr;
        render_pass = nullptr;

        depth_sampler = nullptr;
        depth_imgview = nullptr;
        depth_img = nullptr;
        depth_img_mem = nullptr;

        normal_roughness_sampler = nullptr;
        normal_roughness_view = nullptr;
        normal_roughness_img = nullptr;
        normal_roughness_mem = nullptr;

        diffuse_metallic_sampler = nullptr;
        diffuse_metallic_view = nullptr;
        diffuse_metallic_img = nullptr;
        diffuse_metallic_mem = nullptr;
    }

    GeometryPass::GeometryPass(App& app)
        : recreatables(app)
    {
        // uniform buffers

        VkDeviceSize ubo_size = sizeof(GeometryPassUniforms);

        bv::clear(uniform_bufs);
        uniform_bufs.resize(App::MAX_FRAMES_IN_FLIGHT);

        bv::clear(uniform_bufs_mem);
        uniform_bufs_mem.resize(App::MAX_FRAMES_IN_FLIGHT);

        uniform_bufs_mapped.resize(App::MAX_FRAMES_IN_FLIGHT);

        for (size_t i = 0; i < App::MAX_FRAMES_IN_FLIGHT; i++)
        {
            app.create_buffer(
                ubo_size,
                VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,

                VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT
                | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,

                uniform_bufs[i],
                uniform_bufs_mem[i]
            );
            uniform_bufs_mapped[i] = uniform_bufs_mem[i]->mapped();
        }

        // descriptor set layout

        bv::DescriptorSetLayoutBinding ubo_layout_binding{
            .binding = 0,
            .descriptor_type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
            .descriptor_count = 1,
            .stage_flags = VK_SHADER_STAGE_VERTEX_BIT,
            .immutable_samplers = {}
        };

        bv::DescriptorSetLayoutBinding sampler0_layout_binding{
            .binding = 1,
            .descriptor_type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
            .descriptor_count = 1,
            .stage_flags = VK_SHADER_STAGE_FRAGMENT_BIT,
            .immutable_samplers = {}
        };

        bv::DescriptorSetLayoutBinding sampler1_layout_binding{
            .binding = 2,
            .descriptor_type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
            .descriptor_count = 1,
            .stage_flags = VK_SHADER_STAGE_FRAGMENT_BIT,
            .immutable_samplers = {}
        };

        descriptor_set_layout = bv::DescriptorSetLayout::create(
            app.device,
            {
                .flags = 0,
                .bindings = {
                ubo_layout_binding,
                sampler0_layout_binding,
                sampler1_layout_binding
            }
            }
        );

        // graphics pipeline

        auto vert_shader_module = bv::ShaderModule::create(
            app.device,
            std::move(read_file("./shaders/demo_03_gpass_vert.spv"))
        );
        auto frag_shader_module = bv::ShaderModule::create(
            app.device,
            std::move(read_file("./shaders/demo_03_gpass_frag.spv"))
        );

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
            .binding_descriptions = { GeometryPassVertex::binding },
            .attribute_descriptions = gpass_vert_attributes
        };

        bv::InputAssemblyState input_assembly_state{
            .topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
            .primitive_restart_enable = false
        };

        auto sc_extent = app.swapchain->config().image_extent;

        bv::Viewport viewport{
            .x = 0.f,
            .y = 0.f,
            .width = (float)sc_extent.width,
            .height = (float)sc_extent.height,
            .min_depth = 0.f,
            .max_depth = 1.f
        };

        bv::Rect2d scissor{
            .offset = { 0, 0 },
            .extent = sc_extent
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
            .attachments = { color_blend_attachment, color_blend_attachment },
            .blend_constants = { 0.f, 0.f, 0.f, 0.f }
        };

        bv::DynamicStates dynamic_states{
            VK_DYNAMIC_STATE_VIEWPORT,
            VK_DYNAMIC_STATE_SCISSOR
        };

        pipeline_layout = bv::PipelineLayout::create(
            app.device,
            {
                .flags = 0,
                .set_layouts = { descriptor_set_layout },
                .push_constant_ranges = {}
            }
        );

        graphics_pipeline = bv::GraphicsPipeline::create(
            app.device,
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
                .layout = pipeline_layout,
                .render_pass = recreatables.render_pass,
                .subpass_index = 0,
                .base_pipeline = std::nullopt
            }
        );

        vert_shader_module = nullptr;
        frag_shader_module = nullptr;

        // descriptor pool

        std::vector<bv::DescriptorPoolSize> pool_sizes;
        pool_sizes.push_back({
            .type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
            .descriptor_count = App::MAX_FRAMES_IN_FLIGHT
            });
        pool_sizes.push_back({
            .type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
            .descriptor_count = 2 * App::MAX_FRAMES_IN_FLIGHT
            });

        descriptor_pool = bv::DescriptorPool::create(
            app.device,
            {
                .flags = 0,
                .max_sets = App::MAX_FRAMES_IN_FLIGHT,
                .pool_sizes = pool_sizes
            }
        );

        // descriptor sets

        descriptor_sets = bv::DescriptorPool::allocate_sets(
            descriptor_pool,
            App::MAX_FRAMES_IN_FLIGHT,
            std::vector<bv::DescriptorSetLayoutPtr>(
                App::MAX_FRAMES_IN_FLIGHT,
                descriptor_set_layout
            )
        );

        for (size_t i = 0; i < App::MAX_FRAMES_IN_FLIGHT; i++)
        {
            bv::DescriptorBufferInfo uniform_buffer_info{
                .buffer = uniform_bufs[i],
                .offset = 0,
                .range = sizeof(GeometryPassUniforms)
            };

            bv::DescriptorImageInfo sampler0_image_info{
                .sampler = app.tex_diffuse_metallic_sampler,
                .image_view = app.tex_diffuse_metallic_view,
                .image_layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
            };

            bv::DescriptorImageInfo sampler1_image_info{
                .sampler = app.tex_normal_roughness_sampler,
                .image_view = app.tex_normal_roughness_view,
                .image_layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
            };

            std::vector<bv::WriteDescriptorSet> descriptor_writes;

            descriptor_writes.push_back({
                .dst_set = descriptor_sets[i],
                .dst_binding = 0,
                .dst_array_element = 0,
                .descriptor_count = 1,
                .descriptor_type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
                .image_infos = {},
                .buffer_infos = { uniform_buffer_info },
                .texel_buffer_views = {}
                });

            descriptor_writes.push_back({
                .dst_set = descriptor_sets[i],
                .dst_binding = 1,
                .dst_array_element = 0,
                .descriptor_count = 1,
                .descriptor_type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                .image_infos = { sampler0_image_info },
                .buffer_infos = {},
                .texel_buffer_views = {}
                });

            descriptor_writes.push_back({
                .dst_set = descriptor_sets[i],
                .dst_binding = 2,
                .dst_array_element = 0,
                .descriptor_count = 1,
                .descriptor_type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                .image_infos = { sampler1_image_info },
                .buffer_infos = {},
                .texel_buffer_views = {}
                });

            bv::DescriptorSet::update_sets(app.device, descriptor_writes, {});
        }
    }

    GeometryPass::~GeometryPass()
    {
        bv::clear(uniform_bufs);
        bv::clear(uniform_bufs_mem);

        descriptor_pool = nullptr;

        graphics_pipeline = nullptr;
        pipeline_layout = nullptr;
        descriptor_set_layout = nullptr;
    }

    void GeometryPass::recreate(App& app)
    {
        recreatables.cleanup();
        recreatables.init(app);
    }

    LightingPassRecreatables::LightingPassRecreatables(App& app)
    {
        init(app);
    }

    LightingPassRecreatables::~LightingPassRecreatables()
    {
        cleanup();
    }

    void LightingPassRecreatables::init(App& app)
    {
        auto sc_extent = app.swapchain->config().image_extent;

        // color image

        app.create_image(
            sc_extent.width,
            sc_extent.height,
            1,
            VK_SAMPLE_COUNT_1_BIT,
            LPASS_COLOR_FORMAT,
            VK_IMAGE_TILING_OPTIMAL,
            VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
            VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
            color_img,
            color_img_mem
        );
        color_imgview = app.create_image_view(
            color_img,
            LPASS_COLOR_FORMAT,
            VK_IMAGE_ASPECT_COLOR_BIT,
            1
        );

        auto layout_tran_cmd_buf = app.begin_single_time_commands(true);
        app.transition_image_layout(
            layout_tran_cmd_buf,
            color_img,
            VK_IMAGE_LAYOUT_UNDEFINED,
            VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
            1
        );
        app.end_single_time_commands(layout_tran_cmd_buf);

        // sampler
        color_img_sampler = bv::Sampler::create(
            app.device,
            {
                .flags = 0,
                .mag_filter = VK_FILTER_LINEAR,
                .min_filter = VK_FILTER_LINEAR,
                .mipmap_mode = VK_SAMPLER_MIPMAP_MODE_NEAREST,
                .address_mode_u = VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT,
                .address_mode_v = VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT,
                .address_mode_w = VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT,
                .mip_lod_bias = 0.f,
                .anisotropy_enable = false,
                .max_anisotropy = 1.,
                .compare_enable = false,
                .compare_op = VK_COMPARE_OP_ALWAYS,
                .min_lod = 0.,
                .max_lod = VK_LOD_CLAMP_NONE,
                .border_color = VK_BORDER_COLOR_INT_OPAQUE_BLACK,
                .unnormalized_coordinates = false
            }
        );

        // render pass

        bv::Attachment color_attachment{
            .flags = 0,
            .format = LPASS_COLOR_FORMAT,
            .samples = VK_SAMPLE_COUNT_1_BIT,
            .load_op = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
            .store_op = VK_ATTACHMENT_STORE_OP_STORE,
            .stencil_load_op = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
            .stencil_store_op = VK_ATTACHMENT_STORE_OP_DONT_CARE,
            .initial_layout = VK_IMAGE_LAYOUT_UNDEFINED,
            .final_layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
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
            app.device,
            {
                .flags = 0,
                .attachments = { color_attachment },
                .subpasses = { subpass },
                .dependencies = { dependency }
            }
        );

        // framebuffers
        for (size_t i = 0; i < app.swapchain_imgviews.size(); i++)
        {
            framebuf = bv::Framebuffer::create(
                app.device,
                {
                    .flags = 0,
                    .render_pass = render_pass,
                    .attachments = { color_imgview },
                    .width = sc_extent.width,
                    .height = sc_extent.height,
                    .layers = 1
                }
            );
        }
    }

    void LightingPassRecreatables::cleanup()
    {
        framebuf = nullptr;
        render_pass = nullptr;

        color_img_sampler = nullptr;
        color_imgview = nullptr;
        color_img = nullptr;
        color_img_mem = nullptr;
    }

    LightingPass::LightingPass(App& app)
        : recreatables(app)
    {
        // light buffers

        VkDeviceSize light_buf_size = sizeof(lights[0]) * lights.size();

        bv::clear(light_bufs);
        light_bufs.resize(App::MAX_FRAMES_IN_FLIGHT);

        bv::clear(light_bufs_mem);
        light_bufs_mem.resize(App::MAX_FRAMES_IN_FLIGHT);

        light_bufs_mapped.resize(App::MAX_FRAMES_IN_FLIGHT);

        for (size_t i = 0; i < App::MAX_FRAMES_IN_FLIGHT; i++)
        {
            app.create_buffer(
                light_buf_size,
                VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
                VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT,
                light_bufs[i],
                light_bufs_mem[i]
            );
            light_bufs_mapped[i] = light_bufs_mem[i]->mapped();
        }

        // descriptor set layout

        bv::DescriptorSetLayoutBinding sampler0_layout_binding{
            .binding = 0,
            .descriptor_type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
            .descriptor_count = 1,
            .stage_flags = VK_SHADER_STAGE_FRAGMENT_BIT,
            .immutable_samplers = {}
        };

        bv::DescriptorSetLayoutBinding sampler1_layout_binding{
            .binding = 1,
            .descriptor_type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
            .descriptor_count = 1,
            .stage_flags = VK_SHADER_STAGE_FRAGMENT_BIT,
            .immutable_samplers = {}
        };

        bv::DescriptorSetLayoutBinding sampler2_layout_binding{
            .binding = 2,
            .descriptor_type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
            .descriptor_count = 1,
            .stage_flags = VK_SHADER_STAGE_FRAGMENT_BIT,
            .immutable_samplers = {}
        };

        bv::DescriptorSetLayoutBinding light_buf_layout_binding{
            .binding = 3,
            .descriptor_type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
            .descriptor_count = 1,
            .stage_flags = VK_SHADER_STAGE_FRAGMENT_BIT,
            .immutable_samplers = {}
        };

        descriptor_set_layout = bv::DescriptorSetLayout::create(
            app.device,
            {
                .flags = 0,
                .bindings = {
                sampler0_layout_binding,
                sampler1_layout_binding,
                sampler2_layout_binding,
                light_buf_layout_binding
            }
            }
        );

        // graphics pipeline

        auto vert_shader_module = bv::ShaderModule::create(
            app.device,
            std::move(read_file("./shaders/demo_03_flat_vert.spv"))
        );
        auto frag_shader_module = bv::ShaderModule::create(
            app.device,
            std::move(read_file("./shaders/demo_03_lpass_frag.spv"))
        );

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
            .binding_descriptions = { FlatVertex::binding },
            .attribute_descriptions = flat_vert_attributes
        };

        bv::InputAssemblyState input_assembly_state{
            .topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
            .primitive_restart_enable = false
        };

        auto sc_extent = app.swapchain->config().image_extent;

        bv::Viewport viewport{
            .x = 0.f,
            .y = 0.f,
            .width = (float)sc_extent.width,
            .height = (float)sc_extent.height,
            .min_depth = 0.f,
            .max_depth = 1.f
        };

        bv::Rect2d scissor{
            .offset = { 0, 0 },
            .extent = sc_extent
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

        bv::PushConstantRange push_constants{
            .stage_flags = VK_SHADER_STAGE_FRAGMENT_BIT,
            .offset = 0,
            .size = sizeof(frag_push_constants)
        };

        pipeline_layout = bv::PipelineLayout::create(
            app.device,
            {
                .flags = 0,
                .set_layouts = { descriptor_set_layout },
                .push_constant_ranges = { push_constants }
            }
        );

        graphics_pipeline = bv::GraphicsPipeline::create(
            app.device,
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
                .render_pass = recreatables.render_pass,
                .subpass_index = 0,
                .base_pipeline = std::nullopt
            }
        );

        vert_shader_module = nullptr;
        frag_shader_module = nullptr;

        // descriptor pool

        std::vector<bv::DescriptorPoolSize> pool_sizes;
        pool_sizes.push_back({
            .type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
            .descriptor_count = 3 * App::MAX_FRAMES_IN_FLIGHT
            });
        pool_sizes.push_back({
            .type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
            .descriptor_count = App::MAX_FRAMES_IN_FLIGHT
            });

        descriptor_pool = bv::DescriptorPool::create(
            app.device,
            {
                .flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT,
                .max_sets = App::MAX_FRAMES_IN_FLIGHT,
                .pool_sizes = pool_sizes
            }
        );

        // descriptor sets
        recreate_descriptor_sets(app);
    }

    LightingPass::~LightingPass()
    {
        bv::clear(light_bufs);
        bv::clear(light_bufs_mem);

        descriptor_pool = nullptr;

        graphics_pipeline = nullptr;
        pipeline_layout = nullptr;
        descriptor_set_layout = nullptr;
    }

    void LightingPass::recreate(App& app)
    {
        recreatables.cleanup();
        recreatables.init(app);

        recreate_descriptor_sets(app);
    }

    void LightingPass::recreate_descriptor_sets(App& app)
    {
        bv::clear(descriptor_sets);
        descriptor_sets = bv::DescriptorPool::allocate_sets(
            descriptor_pool,
            App::MAX_FRAMES_IN_FLIGHT,
            std::vector<bv::DescriptorSetLayoutPtr>(
                App::MAX_FRAMES_IN_FLIGHT,
                descriptor_set_layout
            )
        );

        for (size_t i = 0; i < App::MAX_FRAMES_IN_FLIGHT; i++)
        {
            bv::DescriptorImageInfo sampler0_image_info{
                .sampler = app.gpass->recreatables.diffuse_metallic_sampler,
                .image_view = app.gpass->recreatables.diffuse_metallic_view,
                .image_layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
            };

            bv::DescriptorImageInfo sampler1_image_info{
                .sampler = app.gpass->recreatables.normal_roughness_sampler,
                .image_view = app.gpass->recreatables.normal_roughness_view,
                .image_layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
            };

            bv::DescriptorImageInfo sampler2_image_info{
                .sampler = app.gpass->recreatables.depth_sampler,
                .image_view = app.gpass->recreatables.depth_imgview,
                .image_layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
            };

            bv::DescriptorBufferInfo light_buffer_info{
                .buffer = light_bufs[i],
                .offset = 0,
                .range = sizeof(lights[0]) * lights.size()
            };

            std::vector<bv::WriteDescriptorSet> descriptor_writes;

            descriptor_writes.push_back({
                .dst_set = descriptor_sets[i],
                .dst_binding = 0,
                .dst_array_element = 0,
                .descriptor_count = 1,
                .descriptor_type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                .image_infos = { sampler0_image_info },
                .buffer_infos = {},
                .texel_buffer_views = {}
                });

            descriptor_writes.push_back({
                .dst_set = descriptor_sets[i],
                .dst_binding = 1,
                .dst_array_element = 0,
                .descriptor_count = 1,
                .descriptor_type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                .image_infos = { sampler1_image_info },
                .buffer_infos = {},
                .texel_buffer_views = {}
                });

            descriptor_writes.push_back({
                .dst_set = descriptor_sets[i],
                .dst_binding = 2,
                .dst_array_element = 0,
                .descriptor_count = 1,
                .descriptor_type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                .image_infos = { sampler2_image_info },
                .buffer_infos = {},
                .texel_buffer_views = {}
                });

            descriptor_writes.push_back({
                .dst_set = descriptor_sets[i],
                .dst_binding = 3,
                .dst_array_element = 0,
                .descriptor_count = 1,
                .descriptor_type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
                .image_infos = {},
                .buffer_infos = { light_buffer_info },
                .texel_buffer_views = {}
                });

            bv::DescriptorSet::update_sets(app.device, descriptor_writes, {});
        }
    }

    FxaaPassRecreatables::FxaaPassRecreatables(App& app)
    {
        init(app);
    }

    FxaaPassRecreatables::~FxaaPassRecreatables()
    {
        cleanup();
    }

    void FxaaPassRecreatables::init(App& app)
    {
        // render pass

        bv::Attachment color_attachment{
            .flags = 0,
            .format = app.swapchain->config().image_format,
            .samples = VK_SAMPLE_COUNT_1_BIT,
            .load_op = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
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
            app.device,
            {
                .flags = 0,
                .attachments = { color_attachment },
                .subpasses = { subpass },
                .dependencies = { dependency }
            }
        );

        // framebuffers

        auto sc_extent = app.swapchain->config().image_extent;

        bv::clear(swapchain_framebufs);
        for (size_t i = 0; i < app.swapchain_imgviews.size(); i++)
        {
            swapchain_framebufs.push_back(bv::Framebuffer::create(
                app.device,
                {
                    .flags = 0,
                    .render_pass = render_pass,
                    .attachments = { app.swapchain_imgviews[i] },
                    .width = sc_extent.width,
                    .height = sc_extent.height,
                    .layers = 1
                }
            ));
        }
    }

    void FxaaPassRecreatables::cleanup()
    {
        bv::clear(swapchain_framebufs);
        render_pass = nullptr;
    }

    FxaaPass::FxaaPass(App& app)
        : recreatables(app)
    {
        // descriptor set layout

        bv::DescriptorSetLayoutBinding sampler0_layout_binding{
            .binding = 0,
            .descriptor_type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
            .descriptor_count = 1,
            .stage_flags = VK_SHADER_STAGE_FRAGMENT_BIT,
            .immutable_samplers = {}
        };

        descriptor_set_layout = bv::DescriptorSetLayout::create(
            app.device,
            {
                .flags = 0,
                .bindings = { sampler0_layout_binding }
            }
        );

        // graphics pipeline

        auto vert_shader_module = bv::ShaderModule::create(
            app.device,
            std::move(read_file("./shaders/demo_03_flat_vert.spv"))
        );
        auto frag_shader_module = bv::ShaderModule::create(
            app.device,
            std::move(read_file("./shaders/demo_03_fxaa_frag.spv"))
        );

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
            .binding_descriptions = { FlatVertex::binding },
            .attribute_descriptions = flat_vert_attributes
        };

        bv::InputAssemblyState input_assembly_state{
            .topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
            .primitive_restart_enable = false
        };

        auto sc_extent = app.swapchain->config().image_extent;

        bv::Viewport viewport{
            .x = 0.f,
            .y = 0.f,
            .width = (float)sc_extent.width,
            .height = (float)sc_extent.height,
            .min_depth = 0.f,
            .max_depth = 1.f
        };

        bv::Rect2d scissor{
            .offset = { 0, 0 },
            .extent = sc_extent
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

        bv::PushConstantRange push_constants{
            .stage_flags = VK_SHADER_STAGE_FRAGMENT_BIT,
            .offset = 0,
            .size = sizeof(frag_push_constants)
        };

        pipeline_layout = bv::PipelineLayout::create(
            app.device,
            {
                .flags = 0,
                .set_layouts = { descriptor_set_layout },
                .push_constant_ranges = { push_constants }
            }
        );

        graphics_pipeline = bv::GraphicsPipeline::create(
            app.device,
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
                .render_pass = recreatables.render_pass,
                .subpass_index = 0,
                .base_pipeline = std::nullopt
            }
        );

        vert_shader_module = nullptr;
        frag_shader_module = nullptr;

        // descriptor pool

        std::vector<bv::DescriptorPoolSize> pool_sizes;
        pool_sizes.push_back({
            .type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
            .descriptor_count = App::MAX_FRAMES_IN_FLIGHT
            });

        descriptor_pool = bv::DescriptorPool::create(
            app.device,
            {
                .flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT,
                .max_sets = App::MAX_FRAMES_IN_FLIGHT,
                .pool_sizes = pool_sizes
            }
        );

        // descriptor sets
        recreate_descriptor_sets(app);
    }

    FxaaPass::~FxaaPass()
    {
        descriptor_pool = nullptr;

        graphics_pipeline = nullptr;
        pipeline_layout = nullptr;
        descriptor_set_layout = nullptr;
    }

    void FxaaPass::recreate(App& app)
    {
        recreatables.cleanup();
        recreatables.init(app);

        recreate_descriptor_sets(app);
    }

    void FxaaPass::recreate_descriptor_sets(App& app)
    {
        bv::clear(descriptor_sets);
        descriptor_sets = bv::DescriptorPool::allocate_sets(
            descriptor_pool,
            App::MAX_FRAMES_IN_FLIGHT,
            std::vector<bv::DescriptorSetLayoutPtr>(
                App::MAX_FRAMES_IN_FLIGHT,
                descriptor_set_layout
            )
        );

        for (size_t i = 0; i < App::MAX_FRAMES_IN_FLIGHT; i++)
        {
            bv::DescriptorImageInfo sampler0_image_info{
                .sampler = app.lpass->recreatables.color_img_sampler,
                .image_view = app.lpass->recreatables.color_imgview,
                .image_layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
            };

            std::vector<bv::WriteDescriptorSet> descriptor_writes;
            descriptor_writes.push_back({
                .dst_set = descriptor_sets[i],
                .dst_binding = 0,
                .dst_array_element = 0,
                .descriptor_count = 1,
                .descriptor_type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                .image_infos = { sampler0_image_info },
                .buffer_infos = {},
                .texel_buffer_views = {}
                });
            bv::DescriptorSet::update_sets(app.device, descriptor_writes, {});
        }
    }

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
            "[WASD+QE]: move\n"
            "[Hold Shift]: sprint\n"
            "[Space]: switch render mode\n\n";

        start_time = std::chrono::high_resolution_clock::now();

        init_window();
        init_context();
        setup_debug_messenger();
        create_surface();
        pick_physical_device();
        create_logical_device();
        create_memory_bank();
        create_command_pools();
        create_swapchain();

        load_textures();
        load_model();

        create_vertex_buffer();
        create_index_buffer();
        create_quad_vertex_buffer();

        create_passes();

        create_command_buffers();
        create_sync_objects();
    }

    void App::main_loop()
    {
        start_time = std::chrono::high_resolution_clock::now();
        frame_start_time = start_time;

        while (true)
        {
            // time
            scene_time = elapsed_since(start_time);
            delta_time = elapsed_since(frame_start_time);
            frame_start_time = std::chrono::high_resolution_clock::now();

            // mouse down and drag mode
            bool new_mouse_down = glfwGetMouseButton(
                window, GLFW_MOUSE_BUTTON_LEFT
            ) == GLFW_PRESS;
            if (!mouse_down && new_mouse_down) drag_mode = true;
            else if (mouse_down && !new_mouse_down) drag_mode = false;
            mouse_down = new_mouse_down;

            // cursor position and its delta from the last frame
            double new_cursor_xpos, new_cursor_ypos;
            glfwGetCursorPos(window, &new_cursor_xpos, &new_cursor_ypos);
            glm::ivec2 new_cursor_pos = {
                (int32_t)new_cursor_xpos, (int32_t)new_cursor_ypos
            };
            delta_cursor_pos = new_cursor_pos - cursor_pos;
            cursor_pos = new_cursor_pos;

            glfwPollEvents();
            update_lights();
            update_camera();
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
        fxaa_pass = nullptr;
        lpass = nullptr;
        gpass = nullptr;

        bv::clear(fences_in_flight);
        bv::clear(semaphs_render_finished);
        bv::clear(semaphs_image_available);

        quad_vertex_buf = nullptr;
        quad_vertex_buf_mem = nullptr;

        index_buf = nullptr;
        index_buf_mem = nullptr;

        vertex_buf = nullptr;
        vertex_buf_mem = nullptr;

        bv::clear(indices);
        bv::clear(vertices);

        tex_normal_roughness_sampler = nullptr;
        tex_normal_roughness_view = nullptr;
        tex_normal_roughness_img = nullptr;
        tex_normal_roughness_mem = nullptr;

        tex_diffuse_metallic_sampler = nullptr;
        tex_diffuse_metallic_view = nullptr;
        tex_diffuse_metallic_img = nullptr;
        tex_diffuse_metallic_mem = nullptr;

        cleanup_swapchain();

        transient_cmd_pool = nullptr;
        cmd_pool = nullptr;
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
        glfwSetKeyCallback(window, glfw_key_callback);
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
            if (!pdev.queue_family_indices().graphics.has_value())
            {
                continue;
            }
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

        graphics_queue =
            bv::Device::retrieve_queue(device, graphics_family_idx, 0);

        presentation_queue =
            bv::Device::retrieve_queue(device, presentation_family_idx, 0);
    }

    void App::create_memory_bank()
    {
        mem_bank = bv::MemoryBank::create(device);
    }

    void App::create_command_pools()
    {
        cmd_pool = bv::CommandPool::create(
            device,
            {
                .flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
                .queue_family_index = graphics_family_idx
            }
        );
        transient_cmd_pool = bv::CommandPool::create(
            device,
            {
                .flags = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT,
                .queue_family_index = graphics_family_idx
            }
        );
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

    void App::load_textures()
    {
        int diffuse_metallic_w, diffuse_metallic_h, metallic_w, metallic_h,
            normal_roughness_w, normal_roughness_h, roughness_w, roughness_h, _;

        // load diffuse texture
        stbi_uc* diffuse_metallic_pixels = stbi_load(
            DIFFUSE_TEX_PATH,
            &diffuse_metallic_w,
            &diffuse_metallic_h,
            &_,
            STBI_rgb_alpha
        );
        if (!diffuse_metallic_pixels)
        {
            throw std::runtime_error("failed to load the diffuse texture");
        }

        // load metallic texture
        stbi_uc* metallic_pixels = stbi_load(
            METALLIC_TEX_PATH,
            &metallic_w,
            &metallic_h,
            &_,
            STBI_grey
        );
        if (!metallic_pixels)
        {
            throw std::runtime_error("failed to load the metallic texture");
        }

        // combine metallic into the alpha channel of the diffuse texture
        if (diffuse_metallic_w != metallic_w
            || diffuse_metallic_h != metallic_h)
        {
            throw std::runtime_error(
                "diffuse and metallic textures must have the same size"
            );
        }
        for (size_t i = 0; i < diffuse_metallic_w * diffuse_metallic_h; i++)
        {
            diffuse_metallic_pixels[4 * i + 3] = metallic_pixels[i];
        }
        stbi_image_free(metallic_pixels);

        // load normal texture
        stbi_us* normal_roughness_pixels = stbi_load_16(
            NORMAL_TEX_PATH,
            &normal_roughness_w,
            &normal_roughness_h,
            &_,
            STBI_rgb_alpha
        );
        if (!normal_roughness_pixels)
        {
            throw std::runtime_error("failed to load the normal texture");
        }

        // load roughness texture
        stbi_us* roughness_pixels = stbi_load_16(
            ROUGHNESS_TEX_PATH,
            &roughness_w,
            &roughness_h,
            &_,
            STBI_grey
        );
        if (!roughness_pixels)
        {
            throw std::runtime_error("failed to load the roughness texture");
        }

        // combine roughness into the blue channel of the normal texture
        if (normal_roughness_w != roughness_w
            || normal_roughness_h != roughness_h)
        {
            throw std::runtime_error(
                "normal and roughness textures must have the same size"
            );
        }
        for (size_t i = 0; i < normal_roughness_w * normal_roughness_h; i++)
        {
            normal_roughness_pixels[4 * i + 2] = roughness_pixels[i];

            // alpha is unused for now
            normal_roughness_pixels[4 * i + 3] = 0;
        }
        stbi_image_free(roughness_pixels);

        // size info
        VkDeviceSize diffuse_metallic_size =
            (uint64_t)diffuse_metallic_w * (uint64_t)diffuse_metallic_h
            * 4ull // channels per pixel
            * 1ull; // bytes per channel
        VkDeviceSize normal_roughness_size =
            (uint64_t)normal_roughness_w * (uint64_t)normal_roughness_h
            * 4ull // channels per pixel
            * 2ull; // bytes per channel
        VkDeviceSize total_size = diffuse_metallic_size + normal_roughness_size;

        // upload to staging buffer and free

        bv::BufferPtr staging_buf;
        bv::MemoryChunkPtr staging_buf_mem;
        create_buffer(
            total_size,
            VK_BUFFER_USAGE_TRANSFER_SRC_BIT,

            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT
            | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,

            staging_buf,
            staging_buf_mem
        );

        void* mapped = staging_buf_mem->mapped();
        std::copy(
            (uint8_t*)diffuse_metallic_pixels,
            (uint8_t*)diffuse_metallic_pixels + diffuse_metallic_size,
            (uint8_t*)mapped
        );
        std::copy(
            (uint8_t*)normal_roughness_pixels,
            (uint8_t*)normal_roughness_pixels + normal_roughness_size,
            (uint8_t*)mapped + diffuse_metallic_size
        );
        staging_buf_mem->flush();

        stbi_image_free(diffuse_metallic_pixels);
        stbi_image_free(normal_roughness_pixels);

        // create images
        create_image(
            (uint32_t)diffuse_metallic_w,
            (uint32_t)diffuse_metallic_h,
            1,
            VK_SAMPLE_COUNT_1_BIT,
            DIFFUSE_METALLIC_TEX_FORMAT,
            VK_IMAGE_TILING_OPTIMAL,
            VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
            VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
            tex_diffuse_metallic_img,
            tex_diffuse_metallic_mem
        );
        create_image(
            (uint32_t)normal_roughness_w,
            (uint32_t)normal_roughness_h,
            1,
            VK_SAMPLE_COUNT_1_BIT,
            NORMAL_ROUGHNESS_TEX_FORMAT,
            VK_IMAGE_TILING_OPTIMAL,
            VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
            VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
            tex_normal_roughness_img,
            tex_normal_roughness_mem
        );

        // copy from staging buffer to the images
        auto cmd_buf = begin_single_time_commands(true);
        {
            transition_image_layout(
                cmd_buf,
                tex_diffuse_metallic_img,
                VK_IMAGE_LAYOUT_UNDEFINED,
                VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                1
            );
            transition_image_layout(
                cmd_buf,
                tex_normal_roughness_img,
                VK_IMAGE_LAYOUT_UNDEFINED,
                VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                1
            );

            copy_buffer_to_image(
                cmd_buf,
                staging_buf,
                tex_diffuse_metallic_img,
                (uint32_t)(diffuse_metallic_w),
                (uint32_t)(diffuse_metallic_h),
                0 // staging buffer offset
            );
            copy_buffer_to_image(
                cmd_buf,
                staging_buf,
                tex_normal_roughness_img,
                (uint32_t)(normal_roughness_w),
                (uint32_t)(normal_roughness_h),
                diffuse_metallic_size // staging buffer offset
            );

            transition_image_layout(
                cmd_buf,
                tex_diffuse_metallic_img,
                VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                1
            );
            transition_image_layout(
                cmd_buf,
                tex_normal_roughness_img,
                VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                1
            );
        }
        end_single_time_commands(cmd_buf);

        // destroy the staging buffer
        staging_buf = nullptr;
        staging_buf_mem = nullptr;

        // create image views
        tex_diffuse_metallic_view = create_image_view(
            tex_diffuse_metallic_img,
            DIFFUSE_METALLIC_TEX_FORMAT,
            VK_IMAGE_ASPECT_COLOR_BIT,
            1
        );
        tex_normal_roughness_view = create_image_view(
            tex_normal_roughness_img,
            NORMAL_ROUGHNESS_TEX_FORMAT,
            VK_IMAGE_ASPECT_COLOR_BIT,
            1
        );

        // create samplers

        const float max_anisotropy = std::clamp(
            physical_device->properties().limits.max_sampler_anisotropy,
            1.f,
            16.f
        );

        for (size_t i = 0; i < 2; i++)
        {
            bv::SamplerPtr& sampler = (i == 0)
                ? tex_diffuse_metallic_sampler : tex_normal_roughness_sampler;

            sampler = bv::Sampler::create(
                device,
                {
                    .flags = 0,
                    .mag_filter = VK_FILTER_LINEAR,
                    .min_filter = VK_FILTER_LINEAR,
                    .mipmap_mode = VK_SAMPLER_MIPMAP_MODE_NEAREST,
                    .address_mode_u = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
                    .address_mode_v = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
                    .address_mode_w = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
                    .mip_lod_bias = 0.f,
                    .anisotropy_enable = true,
                    .max_anisotropy = max_anisotropy,
                    .compare_enable = false,
                    .compare_op = VK_COMPARE_OP_ALWAYS,
                    .min_lod = 0.,
                    .max_lod = VK_LOD_CLAMP_NONE,
                    .border_color = VK_BORDER_COLOR_INT_OPAQUE_BLACK,
                    .unnormalized_coordinates = false
                }
            );
        }
    }

    void App::load_model()
    {
        tinyobj::attrib_t attrib;
        std::vector<tinyobj::shape_t> shapes;
        std::vector<tinyobj::material_t> materials;
        std::string err;
        if (!tinyobj::LoadObj(
            &attrib,
            &shapes,
            &materials,
            &err,
            MODEL_PATH
        ))
        {
            throw std::runtime_error(("failed to load model: " + err).c_str());
        }

        bv::clear(vertices);
        bv::clear(indices);
        std::unordered_map<GeometryPassVertex, uint32_t> unique_vertices{};
        for (const auto& shape : shapes)
        {
            for (const auto& index : shape.mesh.indices)
            {
                GeometryPassVertex vert{};

                vert.pos = {
                    attrib.vertices[3 * index.vertex_index + 0],
                    attrib.vertices[3 * index.vertex_index + 1],
                    attrib.vertices[3 * index.vertex_index + 2]
                };

                vert.normal = {
                    attrib.normals[3 * index.normal_index + 0],
                    attrib.normals[3 * index.normal_index + 1],
                    attrib.normals[3 * index.normal_index + 2]
                };

                vert.texcoord = {
                    attrib.texcoords[2 * index.texcoord_index + 0],
                    1.f - attrib.texcoords[2 * index.texcoord_index + 1]
                };

                if (unique_vertices.count(vert) == 0)
                {
                    unique_vertices[vert] = (uint32_t)(vertices.size());
                    indices.push_back((uint32_t)(vertices.size()));
                    vertices.push_back(vert);
                }
                else
                {
                    indices.push_back(unique_vertices[vert]);
                }
            }
        }

        std::cout << std::format(
            "loaded model: {} vertices, {} indices\n",
            vertices.size(),
            indices.size()
        );
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
            (GeometryPassVertex*)mapped
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

    void App::create_index_buffer()
    {
        VkDeviceSize size = sizeof(indices[0]) * indices.size();

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
            indices.data(),
            indices.data() + indices.size(),
            (uint32_t*)mapped
        );
        staging_buf_mem->flush();

        create_buffer(
            size,

            VK_BUFFER_USAGE_TRANSFER_DST_BIT
            | VK_BUFFER_USAGE_INDEX_BUFFER_BIT,

            VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
            index_buf,
            index_buf_mem
        );

        auto cmd_buf = begin_single_time_commands(true);
        copy_buffer(cmd_buf, staging_buf, index_buf, size);
        end_single_time_commands(cmd_buf);

        staging_buf = nullptr;
        staging_buf_mem = nullptr;
    }

    void App::create_quad_vertex_buffer()
    {
        VkDeviceSize size = sizeof(quad_vertices[0]) * quad_vertices.size();

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
            quad_vertices.data(),
            quad_vertices.data() + quad_vertices.size(),
            (FlatVertex*)mapped
        );
        staging_buf_mem->flush();

        create_buffer(
            size,

            VK_BUFFER_USAGE_TRANSFER_DST_BIT
            | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,

            VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
            quad_vertex_buf,
            quad_vertex_buf_mem
        );

        auto cmd_buf = begin_single_time_commands(true);
        copy_buffer(cmd_buf, staging_buf, quad_vertex_buf, size);
        end_single_time_commands(cmd_buf);

        staging_buf = nullptr;
        staging_buf_mem = nullptr;
    }

    void App::create_passes()
    {
        gpass = std::make_shared<GeometryPass>(*this);
        lpass = std::make_shared<LightingPass>(*this);
        fxaa_pass = std::make_shared<FxaaPass>(*this);
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

    void App::update_lights()
    {
        if (lpass->light_bufs[frame_idx] == nullptr)
            return;

        const float angle_offset = 35.f * scene_time + 95.f;

        float ang = glm::radians(90.f + angle_offset);
        lpass->lights[0] = Light(
            LightType::Directional,
            { .8f, .7f, .6f },
            glm::normalize(glm::vec3(std::cos(ang), std::sin(ang), 1.2f))
        );

        ang = glm::radians(-18.f + angle_offset);
        float dist = 1.25f;
        lpass->lights[1] = Light(
            LightType::Point,
            { .5f, .3f, .15f },
            { dist * std::cos(ang), dist * std::sin(ang), .6f }
        );

        ang = glm::radians(198.f + angle_offset);
        dist = 1.25f;
        lpass->lights[2] = Light(
            LightType::Point,
            { .15f, .3f, .5f },
            { dist * std::cos(ang), dist * std::sin(ang), .3f }
        );

        lpass->lights[3] = Light(
            LightType::Ambient,
            { .015f, .015f, .02f },
            {}
        );

        std::copy(
            lpass->lights.data(),
            lpass->lights.data() + lpass->lights.size(),
            (Light*)lpass->light_bufs_mapped[frame_idx]
        );
        lpass->light_bufs_mem[frame_idx]->flush();
    }

    void App::update_camera()
    {
        // look around
        if (drag_mode)
        {
            cam_dir_spherical.x += .003f * (float)delta_cursor_pos.y;
            cam_dir_spherical.x = std::clamp(
                cam_dir_spherical.x,
                glm::radians(10.f),
                glm::radians(170.f)
            );

            cam_dir_spherical.y -= .003f * (float)delta_cursor_pos.x;
            cam_dir_spherical.y = wrap(
                cam_dir_spherical.y,
                0.f,
                glm::tau<float>()
            );
        }

        // camera basis vectors
        glm::vec3 cam_forward = spherical_to_cartesian(
            glm::vec2{ glm::half_pi<float>(), cam_dir_spherical.y }
        );
        glm::vec3 cam_right =
            glm::normalize(glm::cross(cam_forward, glm::vec3(0, 0, 1)));
        glm::vec3 cam_up = glm::vec3(0, 0, 1);

        // movement direction (WASD+QE)
        glm::vec3 vel(0);
        if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
            vel += cam_forward;
        if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
            vel -= cam_right;
        if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
            vel -= cam_forward;
        if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
            vel += cam_right;
        if (glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS)
            vel -= cam_up;
        if (glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS)
            vel += cam_up;

        // movement speed (Shift)
        if (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS)
            vel *= .7f;
        else
            vel *= .3f;

        // move
        lpass->frag_push_constants.cam_pos += delta_time * vel;
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

        update_uniform_buffer(frame_idx);

        fences_in_flight[frame_idx]->reset();

        cmd_bufs[frame_idx]->reset(0);
        record_command_buffer(cmd_bufs[frame_idx], img_idx);

        graphics_queue->submit(
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

        gpass->recreate(*this);
        lpass->recreate(*this);
        fxaa_pass->recreate(*this);
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
        graphics_queue->submit({}, {}, { cmd_buf }, {}, fence);
        if (fence == nullptr)
        {
            graphics_queue->wait_idle();
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

    void App::transition_image_layout(
        const bv::CommandBufferPtr& cmd_buf,
        const bv::ImagePtr& image,
        VkImageLayout old_layout,
        VkImageLayout new_layout,
        uint32_t mip_levels,
        bool vertex_shader
    )
    {
        VkAccessFlags src_access_mask = 0;
        VkAccessFlags dst_access_mask = 0;
        VkPipelineStageFlags src_stage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
        VkPipelineStageFlags dst_stage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;

        VkPipelineStageFlags stage_for_shader_read =
            vertex_shader
            ? VK_PIPELINE_STAGE_VERTEX_SHADER_BIT
            : VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;

        if (old_layout == VK_IMAGE_LAYOUT_UNDEFINED
            && new_layout == VK_IMAGE_LAYOUT_GENERAL)
        {
            src_access_mask = 0;
            dst_access_mask = 0;

            src_stage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
            dst_stage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
        }
        else if (old_layout == VK_IMAGE_LAYOUT_UNDEFINED
            && new_layout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL)
        {
            src_access_mask = 0;
            dst_access_mask = VK_ACCESS_TRANSFER_WRITE_BIT;

            src_stage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
            dst_stage = VK_PIPELINE_STAGE_TRANSFER_BIT;
        }
        else if (old_layout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL
            && new_layout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
        {
            src_access_mask = VK_ACCESS_TRANSFER_WRITE_BIT;
            dst_access_mask = VK_ACCESS_SHADER_READ_BIT;

            src_stage = VK_PIPELINE_STAGE_TRANSFER_BIT;
            dst_stage = stage_for_shader_read;
        }
        else if (old_layout == VK_IMAGE_LAYOUT_UNDEFINED
            && new_layout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
        {
            src_access_mask = 0;
            dst_access_mask = VK_ACCESS_SHADER_READ_BIT;

            src_stage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
            dst_stage = stage_for_shader_read;
        }
        else if (old_layout == VK_IMAGE_LAYOUT_UNDEFINED
            && new_layout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL)
        {
            src_access_mask = 0;
            dst_access_mask =
                VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT
                | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

            src_stage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
            dst_stage = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
        }
        else if (old_layout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
            && new_layout == VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL)
        {
            src_access_mask = VK_ACCESS_SHADER_READ_BIT;
            dst_access_mask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

            src_stage = stage_for_shader_read;
            dst_stage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        }
        else if (old_layout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
            && new_layout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL)
        {
            src_access_mask = VK_ACCESS_SHADER_READ_BIT;
            dst_access_mask =
                VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT
                | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

            src_stage = stage_for_shader_read;
            dst_stage = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
        }
        else if (old_layout == VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
            && new_layout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
        {
            src_access_mask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
            dst_access_mask = VK_ACCESS_SHADER_READ_BIT;

            src_stage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
            dst_stage = stage_for_shader_read;
        }
        else if (old_layout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL
            && new_layout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
        {
            src_access_mask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
            dst_access_mask = VK_ACCESS_SHADER_READ_BIT;

            src_stage = VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
            dst_stage = stage_for_shader_read;
        }
        else
        {
            throw std::invalid_argument("unsupported image layout transition");
        }

        VkImageAspectFlags aspect_mask = VK_IMAGE_ASPECT_COLOR_BIT;
        if (bv::format_has_depth_component(image->config().format)
            || bv::format_has_stencil_component(image->config().format))
        {
            aspect_mask = 0;
            if (bv::format_has_depth_component(image->config().format))
            {
                aspect_mask |= VK_IMAGE_ASPECT_DEPTH_BIT;
            }
            if (bv::format_has_stencil_component(image->config().format))
            {
                aspect_mask |= VK_IMAGE_ASPECT_STENCIL_BIT;
            }
        }

        VkImageMemoryBarrier barrier{
            .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
            .pNext = nullptr,
            .srcAccessMask = src_access_mask,
            .dstAccessMask = dst_access_mask,
            .oldLayout = old_layout,
            .newLayout = new_layout,
            .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
            .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
            .image = image->handle(),
            .subresourceRange = VkImageSubresourceRange{
                .aspectMask = aspect_mask,
                .baseMipLevel = 0,
                .levelCount = mip_levels,
                .baseArrayLayer = 0,
                .layerCount = 1
        }
        };

        vkCmdPipelineBarrier(
            cmd_buf->handle(),
            src_stage,
            dst_stage,
            0,
            0, nullptr,
            0, nullptr,
            1, &barrier
        );
    }

    void App::copy_buffer_to_image(
        const bv::CommandBufferPtr& cmd_buf,
        const bv::BufferPtr& buffer,
        const bv::ImagePtr& image,
        uint32_t width,
        uint32_t height,
        VkDeviceSize buffer_offset
    )
    {
        VkBufferImageCopy region{
            .bufferOffset = buffer_offset,
            .bufferRowLength = 0,
            .bufferImageHeight = 0,
            .imageSubresource = VkImageSubresourceLayers{
                .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                .mipLevel = 0,
                .baseArrayLayer = 0,
                .layerCount = 1
        },
            .imageOffset = { 0, 0, 0 },
            .imageExtent = { width, height, 1 }
        };

        vkCmdCopyBufferToImage(
            cmd_buf->handle(),
            buffer->handle(),
            image->handle(),
            VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
            1,
            &region
        );
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
        uint32_t img_idx
    )
    {
        cmd_buf->begin(0);

        // geometry pass

        std::array<VkClearValue, 3> gpass_clear_vals{};
        gpass_clear_vals[0].color = { { .159f, .168f, .196f, 0.f } };
        gpass_clear_vals[1].color = { { 0.f, 0.f, 0.f, 0.f } };
        gpass_clear_vals[2].depthStencil = { 1.f, 0 };

        VkRenderPassBeginInfo gpass_render_pass_info{
            .sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
            .pNext = nullptr,
            .renderPass = gpass->recreatables.render_pass->handle(),
            .framebuffer = gpass->recreatables.framebuf->handle(),
            .renderArea = VkRect2D{
                .offset = { 0, 0 },
                .extent = bv::Extent2d_to_vk(swapchain->config().image_extent)
        },
            .clearValueCount = (uint32_t)gpass_clear_vals.size(),
            .pClearValues = gpass_clear_vals.data()
        };
        vkCmdBeginRenderPass(
            cmd_buf->handle(),
            &gpass_render_pass_info,
            VK_SUBPASS_CONTENTS_INLINE
        );

        vkCmdBindPipeline(
            cmd_buf->handle(),
            VK_PIPELINE_BIND_POINT_GRAPHICS,
            gpass->graphics_pipeline->handle()
        );

        VkBuffer gpass_vk_vertex_bufs[]{ vertex_buf->handle() };
        VkDeviceSize gpass_vb_offsets[] = { 0 };
        vkCmdBindVertexBuffers(
            cmd_buf->handle(),
            0,
            1,
            gpass_vk_vertex_bufs,
            gpass_vb_offsets
        );

        vkCmdBindIndexBuffer(
            cmd_buf->handle(),
            index_buf->handle(),
            0,
            VK_INDEX_TYPE_UINT32
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

        auto gpass_vk_descriptor_set =
            gpass->descriptor_sets[frame_idx]->handle();
        vkCmdBindDescriptorSets(
            cmd_buf->handle(),
            VK_PIPELINE_BIND_POINT_GRAPHICS,
            gpass->pipeline_layout->handle(),
            0,
            1,
            &gpass_vk_descriptor_set,
            0,
            nullptr
        );

        vkCmdDrawIndexed(
            cmd_buf->handle(),
            (uint32_t)(indices.size()),
            1,
            0,
            0,
            0
        );

        vkCmdEndRenderPass(cmd_buf->handle());

        // lighting pass

        VkRenderPassBeginInfo lpass_render_pass_info{
            .sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
            .pNext = nullptr,
            .renderPass = lpass->recreatables.render_pass->handle(),
            .framebuffer = lpass->recreatables.framebuf->handle(),
            .renderArea = VkRect2D{
                .offset = { 0, 0 },
                .extent = bv::Extent2d_to_vk(swapchain->config().image_extent)
        },
            .clearValueCount = 0,
            .pClearValues = nullptr
        };
        vkCmdBeginRenderPass(
            cmd_buf->handle(),
            &lpass_render_pass_info,
            VK_SUBPASS_CONTENTS_INLINE
        );

        vkCmdBindPipeline(
            cmd_buf->handle(),
            VK_PIPELINE_BIND_POINT_GRAPHICS,
            lpass->graphics_pipeline->handle()
        );

        VkBuffer lpass_vk_vertex_bufs[]{ quad_vertex_buf->handle() };
        VkDeviceSize lpass_vb_offsets[] = { 0 };
        vkCmdBindVertexBuffers(
            cmd_buf->handle(),
            0,
            1,
            lpass_vk_vertex_bufs,
            lpass_vb_offsets
        );

        vkCmdSetViewport(cmd_buf->handle(), 0, 1, &viewport);
        vkCmdSetScissor(cmd_buf->handle(), 0, 1, &scissor);

        auto lpass_vk_descriptor_set =
            lpass->descriptor_sets[frame_idx]->handle();
        vkCmdBindDescriptorSets(
            cmd_buf->handle(),
            VK_PIPELINE_BIND_POINT_GRAPHICS,
            lpass->pipeline_layout->handle(),
            0,
            1,
            &lpass_vk_descriptor_set,
            0,
            nullptr
        );

        vkCmdPushConstants(
            cmd_buf->handle(),
            lpass->pipeline_layout->handle(),
            VK_SHADER_STAGE_FRAGMENT_BIT,
            0,
            sizeof(lpass->frag_push_constants),
            &lpass->frag_push_constants
        );

        vkCmdDraw(
            cmd_buf->handle(),
            (uint32_t)quad_vertices.size(),
            1,
            0,
            0
        );

        vkCmdEndRenderPass(cmd_buf->handle());

        // FXAA (+ post processing) pass

        VkRenderPassBeginInfo fxaa_pass_render_pass_info{
            .sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
            .pNext = nullptr,
            .renderPass = fxaa_pass->recreatables.render_pass->handle(),

            .framebuffer =
            fxaa_pass->recreatables.swapchain_framebufs[img_idx]->handle(),

            .renderArea = VkRect2D{
                .offset = { 0, 0 },
                .extent = bv::Extent2d_to_vk(swapchain->config().image_extent)
        },
            .clearValueCount = 0,
            .pClearValues = nullptr
        };
        vkCmdBeginRenderPass(
            cmd_buf->handle(),
            &fxaa_pass_render_pass_info,
            VK_SUBPASS_CONTENTS_INLINE
        );

        vkCmdBindPipeline(
            cmd_buf->handle(),
            VK_PIPELINE_BIND_POINT_GRAPHICS,
            fxaa_pass->graphics_pipeline->handle()
        );

        VkBuffer fxaa_pass_vk_vertex_bufs[]{ quad_vertex_buf->handle() };
        VkDeviceSize fxaa_pass_vb_offsets[] = { 0 };
        vkCmdBindVertexBuffers(
            cmd_buf->handle(),
            0,
            1,
            fxaa_pass_vk_vertex_bufs,
            fxaa_pass_vb_offsets
        );

        vkCmdSetViewport(cmd_buf->handle(), 0, 1, &viewport);
        vkCmdSetScissor(cmd_buf->handle(), 0, 1, &scissor);

        auto fxaa_pass_vk_descriptor_set =
            fxaa_pass->descriptor_sets[frame_idx]->handle();
        vkCmdBindDescriptorSets(
            cmd_buf->handle(),
            VK_PIPELINE_BIND_POINT_GRAPHICS,
            fxaa_pass->pipeline_layout->handle(),
            0,
            1,
            &fxaa_pass_vk_descriptor_set,
            0,
            nullptr
        );

        fxaa_pass->frag_push_constants = {
            .do_nothing =
            lpass->frag_push_constants.render_mode != RenderMode::Lit
            ? 1 : 0,

            .global_frame_idx = (uint32_t)global_frame_idx
        };
        vkCmdPushConstants(
            cmd_buf->handle(),
            fxaa_pass->pipeline_layout->handle(),
            VK_SHADER_STAGE_FRAGMENT_BIT,
            0,
            sizeof(fxaa_pass->frag_push_constants),
            &fxaa_pass->frag_push_constants
        );

        vkCmdDraw(
            cmd_buf->handle(),
            (uint32_t)quad_vertices.size(),
            1,
            0,
            0
        );

        vkCmdEndRenderPass(cmd_buf->handle());

        cmd_buf->end();
    }

    void App::update_uniform_buffer(uint32_t frame_idx)
    {
        GeometryPassUniforms ubo{};

        ubo.model = glm::mat4(1);

        ubo.view = glm::lookAt(
            lpass->frag_push_constants.cam_pos,

            lpass->frag_push_constants.cam_pos
            + spherical_to_cartesian(cam_dir_spherical),

            glm::vec3(0, 0, 1)
        );

        auto sc_extent = swapchain->config().image_extent;
        ubo.proj = glm::perspective(
            glm::radians(60.f),
            (float)sc_extent.width / (float)sc_extent.height,
            DEPTH_NEAR,
            DEPTH_FAR
        );
        ubo.proj[1][1] *= -1.f;

        std::copy(
            &ubo,
            &ubo + 1,
            (GeometryPassUniforms*)gpass->uniform_bufs_mapped[frame_idx]
        );

        // also update deferred frag shader's push constants
        lpass->frag_push_constants.inv_view_proj =
            glm::inverse(ubo.proj * ubo.view);
    }

    static float elapsed_since(const std::chrono::steady_clock::time_point& t)
    {
        const auto curr_time = std::chrono::high_resolution_clock::now();
        return std::chrono::duration<float>(curr_time - t).count();
    }

    static float wrap(float x, float start, float end)
    {
        return start + std::fmod(x - start, end - start);
    }

    static glm::vec3 spherical_to_cartesian(glm::vec3 s)
    {
        const float sin_theta = std::sin(s.y);
        return s.x * glm::vec3(
            sin_theta * std::cos(s.z),
            sin_theta * std::sin(s.z),
            std::cos(s.y)
        );
    }

    static glm::vec3 spherical_to_cartesian(glm::vec2 s)
    {
        const float sin_theta = std::sin(s.x);
        return glm::vec3(
            sin_theta * std::cos(s.y),
            sin_theta * std::sin(s.y),
            std::cos(s.x)
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

    void glfw_key_callback(
        GLFWwindow*
        window,
        int key,
        int scancode,
        int action,
        int mods
    )
    {
        if (key != GLFW_KEY_SPACE || action != GLFW_PRESS)
        {
            return;
        }

        App& app = *(App*)(glfwGetWindowUserPointer(window));

        app.lpass->frag_push_constants.render_mode = (RenderMode)
            (((int32_t)app.lpass->frag_push_constants.render_mode + 1)
                % RenderMode_count);

        std::string new_title = App::TITLE;
        switch (app.lpass->frag_push_constants.render_mode)
        {
        case RenderMode::Lit:
        {
            break;
        }
        case RenderMode::Diffuse:
            new_title += " (render mode: diffuse)";
            break;
        case RenderMode::Normal:
            new_title += " (render mode: normal)";
            break;
        case RenderMode::MetallicRoughness:
            new_title += " (render mode: metallic + roughness)";
            break;
        case RenderMode::Depth:
            new_title += " (render mode: depth)";
            break;
        case RenderMode::PositionDerived:
            new_title += " (render mode: position (derived))";
            break;
        default:
            break;
        }
        glfwSetWindowTitle(app.window, new_title.c_str());
    }

}
