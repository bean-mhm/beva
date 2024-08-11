#pragma once

#include <memory>

#include "vulkan/vulkan.h"
#include "GLFW/glfw3.h"

#include "beva/beva.h"

namespace beva_demo
{

    class App
    {
    public:
        App() = default;
        void run();

    private:
        static constexpr const char* initial_title = "beva demo";
        static constexpr int initial_width = 960;
        static constexpr int initial_height = 720;

        static constexpr bool debug_mode = true;

        GLFWwindow* window;
        std::shared_ptr<beva::Context> context = nullptr;
        std::shared_ptr<beva::DebugMessenger> debug_messenger = nullptr;

        void init_window();
        void init_context();
        void setup_debug_messenger();
        void main_loop();
        void cleanup();

    };

}
