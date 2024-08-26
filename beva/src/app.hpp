#pragma once

#include <string>
#include <vector>
#include <array>
#include <chrono>
#include <cstdint>

#include "vulkan/vulkan.h"
#include "GLFW/glfw3.h"

#include "beva/beva.hpp"

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

namespace beva_demo
{

    struct UniformBufferObject
    {
        glm::mat4 model;
        glm::mat4 view;
        glm::mat4 proj;
    };

    struct Vertex
    {
        glm::vec2 pos;
        glm::vec3 col;

        static const bv::VertexInputBindingDescription binding_description;
        static const std::vector<bv::VertexInputAttributeDescription>
            attribute_descriptions;
    };

    static const std::vector<Vertex> vertices{
        Vertex{ .pos = { -.5f, .5f }, .col = { .6f, .4f, .05f } },
        Vertex{ .pos = { -.5f, -.5f }, .col = { .1f, .1f, .1f } },
        Vertex{ .pos = { .5f, -.5f }, .col = { .05f, .2f, .7f } },
        Vertex{ .pos = { .5f, .5f }, .col = { .65f, .65f, .65f } }
    };

    static const std::vector<uint16_t> indices{
        0, 1, 2, 0, 2, 3
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

        GLFWwindow* window;
        bv::ContextPtr context = nullptr;
        bv::DebugMessengerPtr debug_messenger = nullptr;
        bv::SurfacePtr surface = nullptr;
        bv::PhysicalDevicePtr physical_device = nullptr;
        bv::DevicePtr device = nullptr;
        bv::QueuePtr graphics_queue = nullptr;
        bv::QueuePtr presentation_queue = nullptr;
        bv::SwapchainPtr swapchain = nullptr;
        std::vector<bv::ImageViewPtr> swapchain_imgviews;
        bv::RenderPassPtr render_pass = nullptr;
        bv::DescriptorSetLayoutPtr descriptor_set_layout = nullptr;
        bv::PipelineLayoutPtr pipeline_layout = nullptr;
        bv::GraphicsPipelinePtr graphics_pipeline = nullptr;
        std::vector<bv::FramebufferPtr> swapchain_framebufs;
        bv::CommandPoolPtr cmd_pool = nullptr;
        bv::CommandPoolPtr transfer_cmd_pool = nullptr;

        bv::BufferPtr vertex_buf = nullptr;
        bv::DeviceMemoryPtr vertex_buf_mem = nullptr;

        bv::BufferPtr index_buf = nullptr;
        bv::DeviceMemoryPtr index_buf_mem = nullptr;

        std::vector<bv::BufferPtr> uniform_bufs;
        std::vector<bv::DeviceMemoryPtr> uniform_bufs_mem;
        std::vector<void*> uniform_bufs_mapped;

        bv::DescriptorPoolPtr descriptor_pool = nullptr;
        std::vector<bv::DescriptorSetPtr> descriptor_sets;

        // "per frame" stuff (as in frames in flight)
        std::vector<bv::CommandBufferPtr> cmd_bufs;
        std::vector<bv::SemaphorePtr> semaphs_image_available;
        std::vector<bv::SemaphorePtr> semaphs_render_finished;
        std::vector<bv::FencePtr> fences_in_flight;

        uint32_t graphics_family_idx = 0;
        uint32_t presentation_family_idx = 0;

        bool framebuf_resized = false;
        uint32_t frame_idx = 0;

        std::chrono::steady_clock::time_point start_time;

        void init();
        void main_loop();
        void cleanup();

    private:
        void init_window();
        void init_context();
        void setup_debug_messenger();
        void create_surface();
        void pick_physical_device();
        void create_logical_device();
        void create_swapchain();
        void create_render_pass();
        void create_descriptor_set_layout();
        void create_graphics_pipeline();
        void create_framebuffers();
        void create_command_pools();
        void create_vertex_buffer();
        void create_index_buffer();
        void create_uniform_buffers();
        void create_descriptor_pool();
        void create_descriptor_sets();
        void create_command_buffers();
        void create_sync_objects();

        void draw_frame();

        void cleanup_swapchain();
        void recreate_swapchain();

        uint32_t find_memory_type_idx(
            uint32_t supported_type_bits,
            VkMemoryPropertyFlags required_properties
        );

        void create_buffer(
            VkDeviceSize size,
            VkBufferUsageFlags usage,
            VkMemoryPropertyFlags properties,
            bv::BufferPtr& out_buffer,
            bv::DeviceMemoryPtr& out_buffer_memory
        );

        void copy_buffer(
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
            GLFWwindow* window,
            int width,
            int height
        );

    };

}
