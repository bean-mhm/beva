# beva

beva (lowercase) is a thin wrapper over the Vulkan API. It abstracts away very
little functionality from the original API and keeps the low-level design.

Here's what beva does and doesn't do:

- beva provides comments containing links to the Khronos manual on top of every
wrapper struct, class, or function.

- beva provides `Context` for instance management.

- `Context` provides a `fetch_physical_devices()` function that can take an
optional `Surface` as an argument. This function returns a vector of
`PhysicalDevice` objects already containing information such as device
properties, features, queue families, and queue family indices, as well as
swapchain support details (supported formats, present modes, etc.) if a surface
was provided.

- beva provides `Result<T>` and `Error` for error handling. `Error` can be
constructed from a message and an optional `VkResult` and provides a
`to_string()` function with descriptions for every `VkResult` based on the
Vulkan specification.

- beva provides tiny wrappers for Vulkan structs that use STD containers and
types like `std::vector`, `std::array`, `std::string`, and `std::optional`
instead of raw pointers and arrays.

- beva hides away useless fields like flags that are reserved for the future and
the _usually_ redundant `sType` and `pNext` fields (how often do you actually
use extensions?).

- beva provides `Allocator`, an abstract class to let you implement your own
memory allocator for Vulkan. You can use `Context::set_allocator()` to use an
allocator for that context and every `Device` based on it, and every object
created and destroyed within that `Device`.

- beva only implements a tiny section of the Vulkan API, mostly the parts
needed for traditional rasterized rendering. You can call `handle()` on an
object wrapper to get its raw handle and directly use the Vulkan API to
implement what beva doesn't cover.

- beva does __not__ use RAII. I did initially implement RAII but it wasn't
working _too_ well given how the Vulkan API is designed, so I switched to
explicit `destroy()` functions on object wrappers and some helper functions to
destroy all objects in a vector.

- beva will __not__ try and catch invalid input. It's totally possible to get
undefined behavior and crashes with beva if used incorrectly. To avoid these
situations, read the Khronos manual pages linked above structs, classes, or
functions to see how to use them properly. For example, whether a
`std::shared_ptr` field can be `nullptr` or must have a value, or in what
conditions a `std::optional` field can actually be `std::nullopt`.

# Including beva

Make a new directory named `beva` somewhere in your include directories and copy
`beva.hpp` and `beva.cpp` into it. Make sure your compiler is recognizing and
actually compiling `beva.cpp`. And of course, make sure to set up and include
the latest Vulkan SDK such that `#include "vulkan/vulkan.h"` works.
[Here's a tutorial on that.](https://docs.vulkan.org/tutorial/latest/02_Development_environment.html)

# Using beva

Check out `beva/src/app.hpp` and `app.cpp` to see how to render a basic scene
with beva.
