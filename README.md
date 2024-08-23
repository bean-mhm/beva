# beva

beva (lowercase) is a thin wrapper over the Vulkan API. It abstracts away very
little functionality from the original API. My main motive was to get rid of
annoying boilerplate like the `sType` and `pNext` fields in creation info
structs and implement RAII wrappers for Vulkan objects, while still keeping the
low-level design of the API.

Here's more things beva does to ease the process of using Vulkan:

- beva provides `Context` for instance management.

- `Context` provides a `fetch_physical_devices()` function that can take an
optional `Surface` as an argument. This function returns a vector of
`PhysicalDevice` objects already containing every detail you need, so you don't
need to fetch them manually. That includes device properties, features, queue
families, and queue family indices, as well as swapchain support details
(supported formats, present modes, etc.) if a surface was provided.

- beva provides `Allocator`, an abstract class to let you implement your own
memory allocator for Vulkan. You can use `Context::set_allocator()` to use an
allocator for that context and every `Device` based on it, and every object
created and destroyed within that `Device`.

- beva provides `Result<T>` and `Error` for error handling. `Error` can be
constructed from a message and an optional `VkResult` and provides a
`to_string()` function with descriptions for every `VkResult` based on the
Vulkan specification.

- beva provides tiny wrappers for Vulkan structs that use STD containers and
types like `std::vector`, `std::array`, and `std::optional` instead of raw
pointers and arrays.

- beva hides away useless fields like flags that are reserved for the future.

- beva provides a `Version` struct with an `encode()` function, a wrapper around
the `VK_MAKE_API_VERSION` macro. It can also be constructed from an encoded
integer using macros like `VK_API_VERSION_XXXX` under the hood.

- beva provides comments containing links to the Khronos manual on top of every
wrapper struct or class for convenience.

- beva only implements a tiny section of the Vulkan API, mostly the parts I
needed. You can call `handle()` on a Vulkan object wrapper to get its raw
handle and directly use the Vulkan API to implement what beva doesn't cover.
You'll see this being done in the demo app as well.

Disclaimer: beva will __not__ try and catch invalid inputs. It's totally
possible to get undefined behavior and crashes with beva, if used incorrectly.
To avoid these situations, read the manual pages linked above structs and
classes to see how to use them properly. For example, whether a
`std::shared_ptr` field can be `nullptr` or must have a value, or in what
conditions a `std::optional` field can actually be `std::nullopt`.

# Including beva

Make a new directory named `beva` somewhere in your include directories and copy
`beva.hpp` and `beva.cpp` into it. Make sure to set up and include the latest
Vulkan SDK as well, such that `#include "vulkan/vulkan.h"` works.
[Here's a tutorial on that.](https://docs.vulkan.org/tutorial/latest/02_Development_environment.html)

# Using beva

Check out `beva/src/app.hpp` and `app.cpp` to see how to render a basic scene
with beva.
