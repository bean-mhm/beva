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
        vk_allocator.pUserData = this;
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

    Result<Context> Context::create(const ContextConfig& config)
    {
        Context c(config);

        // allocation callbacks
        {
            c.vk_allocator.pUserData = &c;
            c.vk_allocator.pfnAllocation = vk_allocation_callback;
            c.vk_allocator.pfnReallocation = vk_reallocation_callback;
            c.vk_allocator.pfnFree = vk_free_callback;
            c.vk_allocator.pfnInternalAllocation =
                vk_internal_allocation_notification;
            c.vk_allocator.pfnInternalFree = vk_internal_free_notification;
        }

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

        VkInstanceCreateInfo create_info{};
        create_info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
        create_info.pApplicationInfo = &app_info;

        // extensions
        std::vector<const char*> required_extensions_cstr;
        {
            required_extensions_cstr.reserve(
                c._config.required_extensions.size()
            );
            for (const auto& ext : c._config.required_extensions)
            {
                required_extensions_cstr.push_back(ext.c_str());
            }

            create_info.enabledExtensionCount =
                (uint32_t)required_extensions_cstr.size();
            create_info.ppEnabledExtensionNames =
                required_extensions_cstr.data();
        }

        create_info.enabledLayerCount = 0;

        if (c._config.will_enumerate_portability)
            create_info.flags |=
            VK_INSTANCE_CREATE_ENUMERATE_PORTABILITY_BIT_KHR;

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

    Result<std::vector<ExtensionProperties>> Context::available_extensions(
        const std::string& layer_name
    )
    {
        const char* layer_name_cstr = nullptr;
        if (!layer_name.empty())
        {
            layer_name_cstr = layer_name.c_str();
        }

        std::vector<VkExtensionProperties> vk_extensions;

        uint32_t count = 0;
        vkEnumerateInstanceExtensionProperties(
            layer_name_cstr,
            &count,
            nullptr
        );
        vk_extensions.resize(count);

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
    }

    Context::~Context()
    {
        vkDestroyInstance(instance, vk_allocator_ptr());
    }

    Context::Context(const ContextConfig& config)
        : _config(config)
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

        Context& c = *(Context*)p_user_data;
        if (c.allocator() == nullptr)
        {
            throw std::runtime_error(
                "Vulkan allocation callback called but the context doesn't "
                "have an allocator"
            );
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

        return c.allocator()->allocate(c, size, alignment, allocation_scope);
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

        Context& c = *(Context*)p_user_data;
        if (c.allocator() == nullptr)
        {
            throw std::runtime_error(
                "Vulkan reallocation callback called but the context doesn't "
                "have an allocator"
            );
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

        return c.allocator()->reallocate(
            c,
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

        Context& c = *(Context*)p_user_data;
        if (c.allocator() == nullptr)
        {
            throw std::runtime_error(
                "Vulkan free callback called but the context doesn't have an "
                "allocator"
            );
        }

        c.allocator()->free(c, p_memory);
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

        Context& c = *(Context*)p_user_data;
        if (c.allocator() == nullptr)
        {
            throw std::runtime_error(
                "Vulkan internal allocation notification called but the "
                "context doesn't have an allocator"
            );
        }

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

        c.allocator()->internal_allocation_notification(
            c,
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

        Context& c = *(Context*)p_user_data;
        if (c.allocator() == nullptr)
        {
            throw std::runtime_error(
                "Vulkan internal free notification called but the context "
                "doesn't have an allocator"
            );
        }

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

        c.allocator()->internal_free_notification(
            c,
            size,
            allocation_type,
            allocation_scope
        );
    }

}
