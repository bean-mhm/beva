#pragma once

#include <string>
#include <vector>
#include <array>
#include <cstdint>

#include "vulkan/vulkan.h"
#include "GLFW/glfw3.h"

#include "beva/beva.hpp"

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include "glm/glm.hpp"

namespace beva_demo_01_first_triangle
{

    struct Vertex
    {
        glm::vec2 pos;
        glm::vec3 col;

        static const bv::VertexInputBindingDescription binding_description;
        static const std::vector<bv::VertexInputAttributeDescription>
            attribute_descriptions;
    };

    class App
    {
    public:
        App() = default;
        void run();

    private:
        static constexpr const char* TITLE = "beva demo";
        static constexpr int INITIAL_WIDTH = 960;
        static constexpr int INITIAL_HEIGHT = 720;

        static constexpr bool DEBUG_MODE = true;
        static constexpr uint32_t MAX_FRAMES_IN_FLIGHT = 2;

        void init();
        void main_loop();
        void cleanup();

    private:
        GLFWwindow* window;
        bv::ContextPtr context = nullptr;
        bv::DebugMessengerPtr debug_messenger = nullptr;
        bv::SurfacePtr surface = nullptr;
        std::optional<bv::PhysicalDevice> physical_device;
        bv::DevicePtr device = nullptr;
        bv::QueuePtr graphics_queue = nullptr;
        bv::QueuePtr presentation_queue = nullptr;
        bv::SwapchainPtr swapchain = nullptr;
        std::vector<bv::ImageViewPtr> swapchain_imgviews;
        bv::RenderPassPtr render_pass = nullptr;
        bv::PipelineLayoutPtr pipeline_layout = nullptr;
        bv::GraphicsPipelinePtr graphics_pipeline = nullptr;
        std::vector<bv::FramebufferPtr> swapchain_framebufs;

        bv::CommandPoolPtr cmd_pool = nullptr;
        bv::CommandPoolPtr transient_cmd_pool = nullptr;

        bv::BufferPtr vertex_buf = nullptr;
        bv::DeviceMemoryPtr vertex_buf_mem = nullptr;

        // "per frame" stuff (as in frames in flight)
        std::vector<bv::CommandBufferPtr> cmd_bufs;
        std::vector<bv::SemaphorePtr> semaphs_image_available;
        std::vector<bv::SemaphorePtr> semaphs_render_finished;
        std::vector<bv::FencePtr> fences_in_flight;

        uint32_t graphics_family_idx = 0;
        uint32_t presentation_family_idx = 0;

        bool framebuf_resized = false;
        uint32_t frame_idx = 0;

        void init_window();
        void init_context();
        void setup_debug_messenger();
        void create_surface();
        void pick_physical_device();
        void create_logical_device();
        void create_swapchain();
        void create_render_pass();
        void create_swapchain_framebuffers();
        void create_graphics_pipeline();
        void create_command_pools();
        void create_vertex_buffer();
        void create_command_buffers();
        void create_sync_objects();

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

        void create_buffer(
            VkDeviceSize size,
            VkBufferUsageFlags usage,
            VkMemoryPropertyFlags memory_properties,
            bv::BufferPtr& out_buffer,
            bv::DeviceMemoryPtr& out_buffer_memory
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

        friend void glfw_framebuf_resize_callback(
            GLFWwindow* window,
            int width,
            int height
        );

    };

}
