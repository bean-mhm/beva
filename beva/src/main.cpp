#include <iostream>
#include <vector>
#include <string>

#include "demos/01_first_triangle.hpp"
#include "demos/02_textured_model.hpp"

static const std::vector<std::string> demos{
    "first triangle",

    "OBJ models, uniforms, textures, depth buffer, mipmaps, multisampling, "
    "instanced rendering, push constants"
};

int main()
{
    try
    {
        std::cout << "pick a demo to run by entering its index:\n";
        for (size_t i = 0; i < demos.size(); i++)
        {
            std::cout << std::format(
                "{}: {}\n",
                i,
                demos[i]
            );
        }

        int32_t idx = 0;
        while (true)
        {
            std::string s_idx;
            std::getline(std::cin, s_idx);
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
        std::cout << '\n';

        switch (idx)
        {
        case 0:
        {
            beva_demo_01_first_triangle::App app{};
            app.run();
            break;
        }
        case 1:
        {
            beva_demo_02_textured_model::App app{};
            app.run();
            break;
        }
        default:
            throw std::runtime_error("invalid demo index");
        }
    }
    catch (const std::exception& e)
    {
        std::cerr << e.what() << '\n';
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
