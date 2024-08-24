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
        static constexpr const char* title = "beva demo";
        static constexpr int initial_width = 960;
        static constexpr int initial_height = 720;

        static constexpr bool debug_mode = true;

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
        bv::CommandBuffer::ptr cmd_buf = nullptr;
        bv::Semaphore::ptr semaphore_image_available = nullptr;
        bv::Semaphore::ptr semaphore_render_finished = nullptr;
        bv::Fence::ptr fence_in_flight = nullptr;

        uint32_t graphics_family_idx = 0;
        uint32_t presentation_family_idx = 0;

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
        void create_command_pool_and_buffer();
        void create_sync_objects();

        void draw_frame();
        void record_command_buffer(uint32_t img_idx);

    };

}
