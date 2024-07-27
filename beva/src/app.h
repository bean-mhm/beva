#pragma once

struct GLFWwindow;

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

        GLFWwindow* window;

        void init_window();
        void init_vulkan();
        void main_loop();
        void cleanup();

    };

}
