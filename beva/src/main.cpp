#include <iostream>
#include <vector>
#include <string>
#include <cstdint>

#include "demos/00_first_triangle.hpp"
#include "demos/01_textured_model.hpp"
#include "demos/02_compute_shader.hpp"
#include "demos/03_deferred_rendering.hpp"

static const std::vector<std::string> demos{
    "first triangle",

    "textured model (baked lighting): OBJ, uniforms, textures, depth, mipmaps, "
    "multisampling, instanced rendering, push constants",

    "wave simulation: compute shader, storage image, specialization constants",

    "deferred rendering: G-buffer, SSBO lights, PBR, filmic color transform"
};

void run_demo(int32_t idx)
{
    switch (idx)
    {
    case 0:
    {
        beva_demo_00_first_triangle::App app{};
        app.run();
        break;
    }
    case 1:
    {
        beva_demo_01_textured_model::App app{};
        app.run();
        break;
    }
    case 2:
    {
        beva_demo_02_compute_shader::App app{};
        app.run();
        break;
    }
    case 3:
    {
        beva_demo_03_deferred_rendering::App app{};
        app.run();
        break;
    }
    default:
        throw std::runtime_error("invalid demo index");
    }
}

int main()
{
    try
    {
        while (true)
        {
            std::cout <<
                "-------------------------------------------------------------"
                "\n\npick a demo by entering its index (q to quit):\n\n";
            for (size_t i = 0; i < demos.size(); i++)
            {
                std::cout << std::format(
                    "{}: {}\n\n",
                    i,
                    demos[i]
                );
            }

            int32_t idx = 0;
            while (true)
            {
                std::string s_idx;
                std::getline(std::cin, s_idx);

                if (s_idx == "q")
                {
                    idx = -1;
                    break;
                }

                try
                {
                    idx = std::stoi(s_idx);
                    if (idx < 0 || idx >= demos.size())
                    {
                        throw std::exception();
                    }
                    break;
                }
                catch (const std::exception&)
                {
                    std::cout << "enter a valid demo index\n";
                }
            }
            if (idx == -1)
            {
                break;
            }

            std::cout << '\n';
            run_demo(idx);
            std::cout << '\n';
        }
    }
    catch (const std::exception& e)
    {
        std::cerr << e.what() << '\n';
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
