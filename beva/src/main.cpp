#include <iostream>

#include "app.hpp"

int main()
{
    try
    {
        beva_demo::App app{};
        app.run();
    }
    catch (const std::exception& e)
    {
        std::cerr << e.what() << '\n';
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
