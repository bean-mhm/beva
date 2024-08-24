#pragma once

#include "vulkan/vulkan.h"
#include "GLFW/glfw3.h"

#include "beva/beva.hpp"

namespace beva_demo
{

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
        bv::Context::ptr context = nullptr;
        bv::DebugMessenger::ptr debug_messenger = nullptr;
        bv::Surface::ptr surface = nullptr;
        bv::PhysicalDevice::ptr physical_device = nullptr;
        bv::Device::ptr device = nullptr;
        bv::Queue::ptr graphics_queue = nullptr;
        bv::Queue::ptr presentation_queue = nullptr;
        bv::Swapchain::ptr swapchain = nullptr;
        std::vector<bv::ImageView::ptr> swapchain_imgviews;
        bv::RenderPass::ptr render_pass = nullptr;
        bv::PipelineLayout::ptr pipeline_layout = nullptr;
        bv::GraphicsPipeline::ptr graphics_pipeline = nullptr;
        std::vector<bv::Framebuffer::ptr> swapchain_framebufs;
        bv::CommandPool::ptr cmd_pool = nullptr;

        // "per frame" stuff (as in frames in flight)
        std::vector<bv::CommandBuffer::ptr> cmd_bufs;
        std::vector<bv::Semaphore::ptr> semaphs_image_available;
        std::vector<bv::Semaphore::ptr> semaphs_render_finished;
        std::vector<bv::Fence::ptr> fences_in_flight;

        uint32_t graphics_family_idx = 0;
        uint32_t presentation_family_idx = 0;

        uint32_t frame_idx = 0;

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
        void create_graphics_pipeline();
        void create_framebuffers();
        void create_command_pool_and_buffers();
        void create_sync_objects();

        void draw_frame();
        void record_command_buffer(
            const bv::CommandBuffer::ptr& cmd_buf,
            uint32_t img_idx
        );

    };

}
