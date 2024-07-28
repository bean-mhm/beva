#include <iostream>

#include "app.h"

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
