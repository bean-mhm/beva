#include "beva.h"

namespace beva
{

    std::string VulkanResult::to_string() const
    {
        if (_type >= VulkanResultType::_)
        {
            throw std::exception(
                "invalid enum value, this should never happen"
            );
        }
        if (_type == VulkanResultType::UndocumentedVkResult)
        {
            return string_VkResult(_undocumented_vk_result);
        }
        return VulkanResultType_string[(uint8_t)_type];
    }

    std::string Error::to_string() const
    {
        return _vulkan_result.to_string();
    }

    const char* AllocationScope_to_string(AllocationScope allocation_scope)
    {
        if (allocation_scope >= AllocationScope::_)
        {
            throw std::exception(
                "invalid enum value, this should never happen"
            );
        }
        return AllocationScope_string[(uint8_t)allocation_scope];
    }

    const char* InternalAllocationType_to_string(
        InternalAllocationType allocation_type
    )
    {
        if (allocation_type >= InternalAllocationType::_)
        {
            throw std::exception(
                "invalid enum value, this should never happen"
            );
        }
        return InternalAllocationType_string[(uint8_t)allocation_type];
    }

    std::string Version::to_string() const
    {
        return std::format("{}.{}.{}.{}", variant, major, minor, patch);
    }

    Context::Context(Context&& other)
        : _config(std::move(other._config))
    {
        _config = other._config;
        other._config = {};

        _allocator = other._allocator;
        other._allocator = nullptr;

        vk_allocator = other.vk_allocator;
        vk_allocator.pUserData = _allocator.get();
        other.vk_allocator = VkAllocationCallbacks{ 0 };

        instance = other.instance;
        other.instance = nullptr;
    }

    // forward declarations
    static void* vk_allocation_callback(
        void* p_user_data,
        size_t size,
        size_t alignment,
        VkSystemAllocationScope vk_allocation_scope
    );
    static void* vk_reallocation_callback(
        void* p_user_data,
        void* p_original,
        size_t size,
        size_t alignment,
        VkSystemAllocationScope vk_allocation_scope
    );
    static void vk_free_callback(
        void* p_user_data,
        void* p_memory
    );
    static void vk_internal_allocation_notification(
        void* p_user_data,
        size_t size,
        VkInternalAllocationType vk_allocation_type,
        VkSystemAllocationScope vk_allocation_scope
    );
    static void vk_internal_free_notification(
        void* p_user_data,
        size_t size,
        VkInternalAllocationType vk_allocation_type,
        VkSystemAllocationScope vk_allocation_scope
    );

    Result<Context> Context::create(
        const ContextConfig& config,
        const std::shared_ptr<Allocator>& allocator
    )
    {
        Context c(config, allocator);

        // allocation callbacks
        {
            c.vk_allocator.pUserData = c._allocator.get();
            c.vk_allocator.pfnAllocation = vk_allocation_callback;
            c.vk_allocator.pfnReallocation = vk_reallocation_callback;
            c.vk_allocator.pfnFree = vk_free_callback;
            c.vk_allocator.pfnInternalAllocation =
                vk_internal_allocation_notification;
            c.vk_allocator.pfnInternalFree = vk_internal_free_notification;
        }

        VkInstanceCreateInfo create_info{};
        create_info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;

        if (c._config.will_enumerate_portability)
            create_info.flags |=
            VK_INSTANCE_CREATE_ENUMERATE_PORTABILITY_BIT_KHR;

        VkApplicationInfo app_info{};
        {
            app_info.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;

            app_info.pApplicationName = c._config.app_name.c_str();
            app_info.applicationVersion = VK_MAKE_API_VERSION(
                c._config.app_version.variant,
                c._config.app_version.major,
                c._config.app_version.minor,
                c._config.app_version.patch
            );

            app_info.pEngineName = c._config.engine_name.c_str();
            app_info.engineVersion = VK_MAKE_API_VERSION(
                c._config.engine_version.variant,
                c._config.engine_version.major,
                c._config.engine_version.minor,
                c._config.engine_version.patch
            );

            switch (c._config.api_version)
            {
            case ApiVersion::Vulkan1_0:
                app_info.apiVersion = VK_API_VERSION_1_0;
                break;
            case ApiVersion::Vulkan1_1:
                app_info.apiVersion = VK_API_VERSION_1_1;
                break;
            case ApiVersion::Vulkan1_2:
                app_info.apiVersion = VK_API_VERSION_1_2;
                break;
            case ApiVersion::Vulkan1_3:
                app_info.apiVersion = VK_API_VERSION_1_3;
                break;
            default:
                break;
            }
        }
        create_info.pApplicationInfo = &app_info;

        // layers
        std::vector<const char*> layers_cstr;
        {
            layers_cstr.reserve(
                c._config.layers.size()
            );
            for (const auto& layer : c._config.layers)
            {
                layers_cstr.push_back(layer.c_str());
            }

            create_info.enabledLayerCount =
                (uint32_t)layers_cstr.size();
            create_info.ppEnabledLayerNames =
                layers_cstr.data();
        }

        // extensions
        std::vector<const char*> extensions_cstr;
        {
            extensions_cstr.reserve(
                c._config.extensions.size()
            );
            for (const auto& ext : c._config.extensions)
            {
                extensions_cstr.push_back(ext.c_str());
            }

            create_info.enabledExtensionCount =
                (uint32_t)extensions_cstr.size();
            create_info.ppEnabledExtensionNames =
                extensions_cstr.data();
        }

        // create instance
        VkResult vk_result = vkCreateInstance(
            &create_info,
            c.vk_allocator_ptr(),
            &c.instance
        );
        if (vk_result != VK_SUCCESS)
        {
            return Error(vk_result);
        }
        return std::move(c);
    }

    Result<std::vector<LayerProperties>> Context::available_layers()
    {
        uint32_t count = 0;
        vkEnumerateInstanceLayerProperties(
            &count,
            nullptr
        );

        std::vector<VkLayerProperties> vk_layers(count);
        VkResult vk_result = vkEnumerateInstanceLayerProperties(
            &count,
            vk_layers.data()
        );
        if (vk_result != VK_SUCCESS && vk_result != VK_INCOMPLETE)
        {
            return Error(vk_result);
        }

        std::vector<LayerProperties> layers;
        layers.reserve(vk_layers.size());
        for (const auto& layer : vk_layers)
        {
            layers.push_back({
                .name = layer.layerName,
                .spec_version = Version(
                    VK_API_VERSION_VARIANT(layer.specVersion),
                    VK_API_VERSION_MAJOR(layer.specVersion),
                    VK_API_VERSION_MINOR(layer.specVersion),
                    VK_API_VERSION_PATCH(layer.specVersion)
                ),
                .implementation_version = layer.implementationVersion,
                .description = layer.description
                });
        }
        return layers;
    }

    Result<std::vector<ExtensionProperties>> Context::available_extensions(
        const std::string& layer_name
    )
    {
        const char* layer_name_cstr = nullptr;
        if (!layer_name.empty())
        {
            layer_name_cstr = layer_name.c_str();
        }

        uint32_t count = 0;
        vkEnumerateInstanceExtensionProperties(
            layer_name_cstr,
            &count,
            nullptr
        );

        std::vector<VkExtensionProperties> vk_extensions(count);
        VkResult vk_result = vkEnumerateInstanceExtensionProperties(
            layer_name_cstr,
            &count,
            vk_extensions.data()
        );
        if (vk_result != VK_SUCCESS && vk_result != VK_INCOMPLETE)
        {
            return Error(vk_result);
        }

        std::vector<ExtensionProperties> extensions;
        extensions.reserve(vk_extensions.size());
        for (const auto& ext : vk_extensions)
        {
            extensions.push_back({
                .name = ext.extensionName,
                .spec_version = ext.specVersion
                });
        }
        return extensions;
    }

    void Context::set_allocator(const std::shared_ptr<Allocator>& allocator)
    {
        _allocator = allocator;
        vk_allocator.pUserData = _allocator.get();
    }

    Context::~Context()
    {
        vkDestroyInstance(instance, vk_allocator_ptr());
    }

    Context::Context(
        const ContextConfig& config,
        const std::shared_ptr<Allocator>& allocator
    )
        : _config(config), _allocator(allocator)
    {}

    const VkAllocationCallbacks* Context::vk_allocator_ptr() const
    {
        if (_allocator == nullptr)
        {
            return nullptr;
        }
        return &vk_allocator;
    }

    static void* vk_allocation_callback(
        void* p_user_data,
        size_t size,
        size_t alignment,
        VkSystemAllocationScope vk_allocation_scope
    )
    {
        if (!p_user_data)
        {
            throw std::runtime_error(
                "Vulkan allocation callback called with no user data"
            );
        }

        Allocator* allocator = (Allocator*)p_user_data;

        AllocationScope allocation_scope;
        switch (vk_allocation_scope)
        {
        case VK_SYSTEM_ALLOCATION_SCOPE_COMMAND:
            allocation_scope = AllocationScope::Command;
            break;
        case VK_SYSTEM_ALLOCATION_SCOPE_OBJECT:
            allocation_scope = AllocationScope::Object;
            break;
        case VK_SYSTEM_ALLOCATION_SCOPE_CACHE:
            allocation_scope = AllocationScope::Cache;
            break;
        case VK_SYSTEM_ALLOCATION_SCOPE_DEVICE:
            allocation_scope = AllocationScope::Device;
            break;
        case VK_SYSTEM_ALLOCATION_SCOPE_INSTANCE:
            allocation_scope = AllocationScope::Instance;
            break;
        default:
            allocation_scope = AllocationScope::Unknown;
            break;
        }

        return allocator->allocate(size, alignment, allocation_scope);
    }

    static void* vk_reallocation_callback(
        void* p_user_data,
        void* p_original,
        size_t size,
        size_t alignment,
        VkSystemAllocationScope vk_allocation_scope
    )
    {
        if (!p_user_data)
        {
            throw std::runtime_error(
                "Vulkan reallocation callback called with no user data"
            );
        }

        Allocator* allocator = (Allocator*)p_user_data;

        AllocationScope allocation_scope;
        switch (vk_allocation_scope)
        {
        case VK_SYSTEM_ALLOCATION_SCOPE_COMMAND:
            allocation_scope = AllocationScope::Command;
            break;
        case VK_SYSTEM_ALLOCATION_SCOPE_OBJECT:
            allocation_scope = AllocationScope::Object;
            break;
        case VK_SYSTEM_ALLOCATION_SCOPE_CACHE:
            allocation_scope = AllocationScope::Cache;
            break;
        case VK_SYSTEM_ALLOCATION_SCOPE_DEVICE:
            allocation_scope = AllocationScope::Device;
            break;
        case VK_SYSTEM_ALLOCATION_SCOPE_INSTANCE:
            allocation_scope = AllocationScope::Instance;
            break;
        default:
            allocation_scope = AllocationScope::Unknown;
            break;
        }

        return allocator->reallocate(
            p_original,
            size,
            alignment,
            allocation_scope
        );
    }

    static void vk_free_callback(
        void* p_user_data,
        void* p_memory
    )
    {
        if (!p_user_data)
        {
            throw std::runtime_error(
                "Vulkan free callback called with no user data"
            );
        }

        Allocator* allocator = (Allocator*)p_user_data;
        allocator->free(p_memory);
    }

    static void vk_internal_allocation_notification(
        void* p_user_data,
        size_t size,
        VkInternalAllocationType vk_allocation_type,
        VkSystemAllocationScope vk_allocation_scope
    )
    {
        if (!p_user_data)
        {
            throw std::runtime_error(
                "Vulkan internal allocation notification called with no user "
                "data"
            );
        }

        Allocator* allocator = (Allocator*)p_user_data;

        InternalAllocationType allocation_type;
        switch (vk_allocation_type)
        {
        case VK_INTERNAL_ALLOCATION_TYPE_EXECUTABLE:
            allocation_type = InternalAllocationType::Executable;
            break;
        default:
            allocation_type = InternalAllocationType::Unknown;
            break;
        }

        AllocationScope allocation_scope;
        switch (vk_allocation_scope)
        {
        case VK_SYSTEM_ALLOCATION_SCOPE_COMMAND:
            allocation_scope = AllocationScope::Command;
            break;
        case VK_SYSTEM_ALLOCATION_SCOPE_OBJECT:
            allocation_scope = AllocationScope::Object;
            break;
        case VK_SYSTEM_ALLOCATION_SCOPE_CACHE:
            allocation_scope = AllocationScope::Cache;
            break;
        case VK_SYSTEM_ALLOCATION_SCOPE_DEVICE:
            allocation_scope = AllocationScope::Device;
            break;
        case VK_SYSTEM_ALLOCATION_SCOPE_INSTANCE:
            allocation_scope = AllocationScope::Instance;
            break;
        default:
            allocation_scope = AllocationScope::Unknown;
            break;
        }

        allocator->internal_allocation_notification(
            size,
            allocation_type,
            allocation_scope
        );
    }

    static void vk_internal_free_notification(
        void* p_user_data,
        size_t size,
        VkInternalAllocationType vk_allocation_type,
        VkSystemAllocationScope vk_allocation_scope
    )
    {
        if (!p_user_data)
        {
            throw std::runtime_error(
                "Vulkan internal free notification called with no user data"
            );
        }

        Allocator* allocator = (Allocator*)p_user_data;

        InternalAllocationType allocation_type;
        switch (vk_allocation_type)
        {
        case VK_INTERNAL_ALLOCATION_TYPE_EXECUTABLE:
            allocation_type = InternalAllocationType::Executable;
            break;
        default:
            allocation_type = InternalAllocationType::Unknown;
            break;
        }

        AllocationScope allocation_scope;
        switch (vk_allocation_scope)
        {
        case VK_SYSTEM_ALLOCATION_SCOPE_COMMAND:
            allocation_scope = AllocationScope::Command;
            break;
        case VK_SYSTEM_ALLOCATION_SCOPE_OBJECT:
            allocation_scope = AllocationScope::Object;
            break;
        case VK_SYSTEM_ALLOCATION_SCOPE_CACHE:
            allocation_scope = AllocationScope::Cache;
            break;
        case VK_SYSTEM_ALLOCATION_SCOPE_DEVICE:
            allocation_scope = AllocationScope::Device;
            break;
        case VK_SYSTEM_ALLOCATION_SCOPE_INSTANCE:
            allocation_scope = AllocationScope::Instance;
            break;
        default:
            allocation_scope = AllocationScope::Unknown;
            break;
        }

        allocator->internal_free_notification(
            size,
            allocation_type,
            allocation_scope
        );
    }

}
