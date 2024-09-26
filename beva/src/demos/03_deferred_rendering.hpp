#pragma once

#include <string>
#include <vector>
#include <array>
#include <unordered_map>
#include <chrono>
#include <cstdint>

#include "vulkan/vulkan.h"
#include "GLFW/glfw3.h"

#include "beva/beva.hpp"

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#define GLM_ENABLE_EXPERIMENTAL
#include "glm/glm.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include <glm/gtx/hash.hpp>

namespace beva_demo_03_deferred_rendering
{

    static constexpr float DEPTH_NEAR = .01f;
    static constexpr float DEPTH_FAR = 10.f;

    static constexpr glm::vec3 DEFAULT_CAM_POS{ 0.f, -.9f, .35f };
    static constexpr glm::vec2 DEFAULT_CAM_DIR_SPHERICAL{
        glm::pi<float>() / 2.f + glm::radians(4.f),
        glm::pi<float>() / 2.f
    };

    enum class RenderMode : int32_t
    {
        Lit,
        Diffuse,
        Normal,
        MetallicRoughness,
        Depth,
        PositionDerived
    };
    constexpr int32_t RenderMode_count = 6;

    enum class LightType : int32_t
    {
        Ambient,
        Point,
        Directional
    };

    struct Light
    {
        // xyz = col, w = type
        glm::vec4 data0{ 0.f };

        // xyz = pos_or_dir, w = useless
        glm::vec4 data1{ 0.f };

        Light() = default;
        Light(
            LightType type,
            const glm::vec3& col,
            const glm::vec3& pos_or_dir
        );
    };

    struct GeometryPassUniforms
    {
        alignas(16) glm::mat4 model;
        alignas(16) glm::mat4 view;
        alignas(16) glm::mat4 proj;
    };

    struct GeometryPassVertex
    {
        glm::vec3 pos;
        glm::vec3 normal;
        glm::vec2 texcoord;

        static const bv::VertexInputBindingDescription binding;
        bool operator==(const GeometryPassVertex& other) const;
    };

    struct FlatVertex
    {
        glm::vec2 pos;
        glm::vec2 texcoord;

        static const bv::VertexInputBindingDescription binding;
    };

    struct LightingPassFragPushConstants
    {
        glm::mat4 inv_view_proj{ 1 };
        glm::vec3 cam_pos = DEFAULT_CAM_POS;
        float z_near = DEPTH_NEAR;
        float z_far = DEPTH_FAR;
        RenderMode render_mode = RenderMode::Lit;
    };

    struct FxaaPassFragPushConstants
    {
        int32_t do_nothing = 0;
        uint32_t global_frame_idx = 0;
    };

}

template<>
struct std::hash<beva_demo_03_deferred_rendering::GeometryPassVertex>
{
    size_t operator()(const beva_demo_03_deferred_rendering::GeometryPassVertex& v)
        const
    {
        return ((std::hash<glm::vec3>()(v.pos) ^
            (std::hash<glm::vec3>()(v.normal) << 1)) >> 1) ^
            (std::hash<glm::vec2>()(v.texcoord) << 1);
    }
};

namespace beva_demo_03_deferred_rendering
{

    class App;

    struct GeometryPassRecreatables
    {
        // alpha = metallic
        static constexpr VkFormat DIFFUSE_METALLIC_GBUF_FORMAT =
            VK_FORMAT_R8G8B8A8_UNORM;

        // rg = normal in spherical coords, b = roughnees, a = pixel is lit?
        static constexpr VkFormat NORMAL_ROUGHNESS_GBUF_FORMAT =
            VK_FORMAT_R16G16B16A16_UNORM;

        bv::ImagePtr diffuse_metallic_img = nullptr;
        bv::MemoryChunkPtr diffuse_metallic_mem = nullptr;
        bv::ImageViewPtr diffuse_metallic_view = nullptr;
        bv::SamplerPtr diffuse_metallic_sampler = nullptr;

        bv::ImagePtr normal_roughness_img = nullptr;
        bv::MemoryChunkPtr normal_roughness_mem = nullptr;
        bv::ImageViewPtr normal_roughness_view = nullptr;
        bv::SamplerPtr normal_roughness_sampler = nullptr;

        bv::ImagePtr depth_img = nullptr;
        bv::MemoryChunkPtr depth_img_mem = nullptr;
        bv::ImageViewPtr depth_imgview = nullptr;
        bv::SamplerPtr depth_sampler = nullptr;

        bv::RenderPassPtr render_pass = nullptr;
        bv::FramebufferPtr framebuf = nullptr;

        GeometryPassRecreatables(App& app);
        ~GeometryPassRecreatables();

    private:
        void init(App& app);
        void cleanup();

        friend struct GeometryPass;

    };

    struct GeometryPass
    {
        std::vector<bv::BufferPtr> uniform_bufs;
        std::vector<bv::MemoryChunkPtr> uniform_bufs_mem;
        std::vector<void*> uniform_bufs_mapped;

        bv::DescriptorSetLayoutPtr descriptor_set_layout = nullptr;
        bv::PipelineLayoutPtr pipeline_layout = nullptr;
        bv::GraphicsPipelinePtr graphics_pipeline = nullptr;

        bv::DescriptorPoolPtr descriptor_pool = nullptr;
        std::vector<bv::DescriptorSetPtr> descriptor_sets;

        GeometryPassRecreatables recreatables;

        GeometryPass(App& app);
        ~GeometryPass();

        void recreate(App& app);
    };

    struct LightingPassRecreatables
    {
        static constexpr VkFormat LPASS_COLOR_FORMAT =
            VK_FORMAT_R16G16B16A16_SFLOAT;

        bv::ImagePtr color_img = nullptr;
        bv::MemoryChunkPtr color_img_mem = nullptr;
        bv::ImageViewPtr color_imgview = nullptr;
        bv::SamplerPtr color_img_sampler = nullptr;

        bv::RenderPassPtr render_pass = nullptr;
        bv::FramebufferPtr framebuf;

        LightingPassRecreatables(App& app);
        ~LightingPassRecreatables();

    private:
        void init(App& app);
        void cleanup();

        friend struct LightingPass;

    };

    struct LightingPass
    {
        std::array<Light, 4> lights;

        std::vector<bv::BufferPtr> light_bufs;
        std::vector<bv::MemoryChunkPtr> light_bufs_mem;
        std::vector<void*> light_bufs_mapped;

        bv::DescriptorSetLayoutPtr descriptor_set_layout = nullptr;
        bv::PipelineLayoutPtr pipeline_layout = nullptr;
        bv::GraphicsPipelinePtr graphics_pipeline = nullptr;

        bv::DescriptorPoolPtr descriptor_pool = nullptr;
        std::vector<bv::DescriptorSetPtr> descriptor_sets;

        LightingPassRecreatables recreatables;

        LightingPassFragPushConstants frag_push_constants;

        LightingPass(App& app);
        ~LightingPass();

        void recreate(App& app);

    private:
        void recreate_descriptor_sets(App& app);

    };

    struct FxaaPassRecreatables
    {
        bv::RenderPassPtr render_pass = nullptr;
        std::vector<bv::FramebufferPtr> swapchain_framebufs;

        FxaaPassRecreatables(App& app);
        ~FxaaPassRecreatables();

    private:
        void init(App& app);
        void cleanup();

        friend struct FxaaPass;

    };

    struct FxaaPass
    {
        bv::DescriptorSetLayoutPtr descriptor_set_layout = nullptr;
        bv::PipelineLayoutPtr pipeline_layout = nullptr;
        bv::GraphicsPipelinePtr graphics_pipeline = nullptr;

        bv::DescriptorPoolPtr descriptor_pool = nullptr;
        std::vector<bv::DescriptorSetPtr> descriptor_sets;

        FxaaPassRecreatables recreatables;

        FxaaPassFragPushConstants frag_push_constants;

        FxaaPass(App& app);
        ~FxaaPass();

        void recreate(App& app);

    private:
        void recreate_descriptor_sets(App& app);

    };

    class App
    {
    public:
        App() = default;
        void run();

    private:
        static constexpr const char* TITLE =
            "beva demo: deferred lighting";
        static constexpr int INITIAL_WIDTH = 1024;
        static constexpr int INITIAL_HEIGHT = 768;

        static constexpr bool DEBUG_MODE = true;
        static constexpr uint32_t MAX_FRAMES_IN_FLIGHT = 2;

        static constexpr const char* MODEL_PATH =
            "./models/korean_fire_extinguisher_01_mod.obj";

        static constexpr const char* DIFFUSE_TEX_PATH
            = "./textures/korean_fire_extinguisher_01_body_diff_2k.png";
        static constexpr const char* METALLIC_TEX_PATH
            = "./textures/korean_fire_extinguisher_01_body_metal_2k.png";
        static constexpr const char* NORMAL_TEX_PATH
            = "./textures/korean_fire_extinguisher_01_body_nor_gl_2k.png";
        static constexpr const char* ROUGHNESS_TEX_PATH
            = "./textures/korean_fire_extinguisher_01_body_rough_2k.png";

        // alpha = metallic
        static constexpr VkFormat DIFFUSE_METALLIC_TEX_FORMAT =
            VK_FORMAT_R8G8B8A8_UNORM;

        // rg = normal map XY (Z will be calculated), b = roughnees
        static constexpr VkFormat NORMAL_ROUGHNESS_TEX_FORMAT =
            VK_FORMAT_R16G16B16A16_UNORM;

        void init();
        void main_loop();
        void cleanup();

    private:
        GLFWwindow* window = nullptr;
        bv::ContextPtr context = nullptr;
        bv::DebugMessengerPtr debug_messenger = nullptr;
        bv::SurfacePtr surface = nullptr;
        std::optional<bv::PhysicalDevice> physical_device;
        bv::DevicePtr device = nullptr;
        bv::QueuePtr graphics_present_queue = nullptr;
        bv::MemoryBankPtr mem_bank = nullptr;
        bv::CommandPoolPtr cmd_pool = nullptr;
        bv::CommandPoolPtr transient_cmd_pool = nullptr;

        bv::SwapchainPtr swapchain = nullptr;
        std::vector<bv::ImageViewPtr> swapchain_imgviews;

        bv::ImagePtr tex_diffuse_metallic_img = nullptr;
        bv::MemoryChunkPtr tex_diffuse_metallic_mem = nullptr;
        bv::ImageViewPtr tex_diffuse_metallic_view = nullptr;
        bv::SamplerPtr tex_diffuse_metallic_sampler = nullptr;

        bv::ImagePtr tex_normal_roughness_img = nullptr;
        bv::MemoryChunkPtr tex_normal_roughness_mem = nullptr;
        bv::ImageViewPtr tex_normal_roughness_view = nullptr;
        bv::SamplerPtr tex_normal_roughness_sampler = nullptr;

        std::vector<GeometryPassVertex> vertices;
        std::vector<uint32_t> indices;

        bv::BufferPtr vertex_buf = nullptr;
        bv::MemoryChunkPtr vertex_buf_mem = nullptr;

        bv::BufferPtr index_buf = nullptr;
        bv::MemoryChunkPtr index_buf_mem = nullptr;

        bv::BufferPtr quad_vertex_buf = nullptr;
        bv::MemoryChunkPtr quad_vertex_buf_mem = nullptr;

        // geometry pass, lighting pass, and FXAA (+ post processing) pass
        std::shared_ptr<GeometryPass> gpass = nullptr;
        std::shared_ptr<LightingPass> lpass = nullptr;
        std::shared_ptr<FxaaPass> fxaa_pass;

        // "per frame" stuff (as in frames in flight)
        std::vector<bv::CommandBufferPtr> cmd_bufs;
        std::vector<bv::SemaphorePtr> semaphs_image_available;
        std::vector<bv::SemaphorePtr> semaphs_render_finished;
        std::vector<bv::FencePtr> fences_in_flight;

        uint32_t graphics_present_family_idx = 0;

        bool framebuf_resized = false;
        uint32_t frame_idx = 0;
        uint64_t global_frame_idx = 0;

        std::chrono::steady_clock::time_point start_time;
        std::chrono::steady_clock::time_point frame_start_time;
        float scene_time = 0.f;
        float delta_time = 0.f;

        glm::ivec2 cursor_pos{ -1, -1 };
        glm::ivec2 delta_cursor_pos{ -1, -1 };

        bool mouse_down = false;
        bool drag_mode = false;

        glm::vec2 cam_dir_spherical = DEFAULT_CAM_DIR_SPHERICAL;

        void init_window();
        void init_context();
        void setup_debug_messenger();
        void create_surface();
        void pick_physical_device();
        void create_logical_device();
        void create_memory_bank();
        void create_command_pools();
        void create_swapchain();

        void load_textures();
        void load_model();

        void create_vertex_buffer();
        void create_index_buffer();
        void create_quad_vertex_buffer();

        void create_passes();

        void create_command_buffers();
        void create_sync_objects();

        void update_lights();
        void update_camera();
        void draw_frame();

        void cleanup_swapchain();
        void recreate_swapchain();

        // if use_transfer_pool is true, the command buffer will be allocated
        // from transfer_cmd_pool instead of cmd_pool. transfer_cmd_pool has the
        // VK_COMMAND_POOL_CREATE_TRANSIENT_BIT flag enabled which might be of
        // interest.
        bv::CommandBufferPtr begin_single_time_commands(
            bool use_transient_pool
        );

        // end and submit one-time command buffer. if no fence is provided,
        // Queue::wait_idle() will be used. if a fence is provided you'll be in
        // charge of synchronization (like waiting on the fence).
        void end_single_time_commands(
            bv::CommandBufferPtr& cmd_buf,
            const bv::FencePtr fence = nullptr
        );

        uint32_t find_memory_type_idx(
            uint32_t supported_type_bits,
            VkMemoryPropertyFlags required_properties
        );

        VkFormat find_depth_format();

        void create_image(
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
        );

        // when one of old_layout or new_layout is
        // VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, the vertex_shader argument
        // defines whether said shader is the vertex shader or the fragment
        // shader.
        void transition_image_layout(
            const bv::CommandBufferPtr& cmd_buf,
            const bv::ImagePtr& image,
            VkImageLayout old_layout,
            VkImageLayout new_layout,
            uint32_t mip_levels,
            bool vertex_shader = false
        );

        void copy_buffer_to_image(
            const bv::CommandBufferPtr& cmd_buf,
            const bv::BufferPtr& buffer,
            const bv::ImagePtr& image,
            uint32_t width,
            uint32_t height,
            VkDeviceSize buffer_offset = 0
        );

        bv::ImageViewPtr create_image_view(
            const bv::ImagePtr& image,
            VkFormat format,
            VkImageAspectFlags aspect_flags,
            uint32_t mip_levels
        );

        void create_buffer(
            VkDeviceSize size,
            VkBufferUsageFlags usage,
            VkMemoryPropertyFlags memory_properties,
            bv::BufferPtr& out_buffer,
            bv::MemoryChunkPtr& out_memory_chunk
        );

        void copy_buffer(
            const bv::CommandBufferPtr& cmd_buf,
            bv::BufferPtr src,
            bv::BufferPtr dst,
            VkDeviceSize size
        );

        void record_command_buffer(
            const bv::CommandBufferPtr& cmd_buf,
            uint32_t img_idx
        );

        void update_uniform_buffer(uint32_t frame_idx);

        friend void glfw_framebuf_resize_callback(
            GLFWwindow* window, int width, int height
        );
        friend void glfw_key_callback(
            GLFWwindow* window, int key, int scancode, int action, int mods
        );

        friend struct GeometryPassRecreatables;
        friend struct GeometryPass;
        friend struct LightingPassRecreatables;
        friend struct LightingPass;
        friend struct FxaaPassRecreatables;
        friend struct FxaaPass;

    };

}
