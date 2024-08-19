#include "beva.h"

namespace bv
{

    template<size_t size, typename T>
    static std::array<T, size> raw_arr_to_std(const T* raw_arr)
    {
        std::array<T, size> arr;
        std::copy(raw_arr, raw_arr + size, arr.data());
        return arr;
    }

    static std::string cstr_to_std(const char* cstr)
    {
        return (cstr == nullptr) ? std::string() : std::string(cstr);
    }

    // define a derived class named ClassName_public_ctor that lets us use the
    // previously private constructors (actually protected, just go with it) as
    // public ones so that they can be used in std::make_shared() or whatever
    // else.
#define DEFINE_DERIVED_WITH_PUBLIC_CONSTRUCTOR(ClassName) \
    class ClassName##_public_ctor : public ClassName \
    { \
    public: \
        template<typename... Args> ClassName##_public_ctor(Args&&... args) \
            : ClassName(std::forward<Args>(args)...) \
        {} \
    };

    DEFINE_DERIVED_WITH_PUBLIC_CONSTRUCTOR(PhysicalDevice);
    DEFINE_DERIVED_WITH_PUBLIC_CONSTRUCTOR(Context);
    DEFINE_DERIVED_WITH_PUBLIC_CONSTRUCTOR(DebugMessenger);
    DEFINE_DERIVED_WITH_PUBLIC_CONSTRUCTOR(Surface);
    DEFINE_DERIVED_WITH_PUBLIC_CONSTRUCTOR(Queue);
    DEFINE_DERIVED_WITH_PUBLIC_CONSTRUCTOR(Device);
    DEFINE_DERIVED_WITH_PUBLIC_CONSTRUCTOR(Image);
    DEFINE_DERIVED_WITH_PUBLIC_CONSTRUCTOR(Swapchain);
    DEFINE_DERIVED_WITH_PUBLIC_CONSTRUCTOR(ImageView);

#pragma region forward declarations

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
    static VKAPI_ATTR VkBool32 VKAPI_CALL vk_debug_callback(
        VkDebugUtilsMessageSeverityFlagBitsEXT vk_message_severity,
        VkDebugUtilsMessageTypeFlagsEXT vk_message_type_flags,
        const VkDebugUtilsMessengerCallbackDataEXT* p_callback_data,
        void* p_user_data
    );

#pragma endregion

#pragma region Vulkan function loaders for extensions

    static VkResult CreateDebugUtilsMessengerEXT(
        VkInstance instance,
        const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo,
        const VkAllocationCallbacks* pAllocator,
        VkDebugUtilsMessengerEXT* pDebugMessenger
    )
    {
        auto func = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(
            instance,
            "vkCreateDebugUtilsMessengerEXT"
        );
        if (func != nullptr)
        {
            return func(instance, pCreateInfo, pAllocator, pDebugMessenger);
        }
        else
        {
            return VK_ERROR_EXTENSION_NOT_PRESENT;
        }
    }
    static void DestroyDebugUtilsMessengerEXT(
        VkInstance instance,
        VkDebugUtilsMessengerEXT debugMessenger,
        const VkAllocationCallbacks* pAllocator
    )
    {
        auto func = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(
            instance,
            "vkDestroyDebugUtilsMessengerEXT"
        );
        if (func != nullptr)
        {
            func(instance, debugMessenger, pAllocator);
        }
    }

#pragma endregion

    std::string Version::to_string() const
    {
        return std::format("{}.{}.{}.{}", variant, major, minor, patch);
    }

    ApiResult::ApiResult(VkResult vk_result)
        : _undocumented_vk_result(vk_result)
    {
        switch (vk_result)
        {
        case VK_NOT_READY:
            _type = ApiResultType::NotReady;
            break;
        case VK_TIMEOUT:
            _type = ApiResultType::Timeout;
            break;
        case VK_EVENT_SET:
            _type = ApiResultType::EventSet;
            break;
        case VK_EVENT_RESET:
            _type = ApiResultType::EventReset;
            break;
        case VK_INCOMPLETE:
            _type = ApiResultType::Incomplete;
            break;
        case VK_ERROR_OUT_OF_HOST_MEMORY:
            _type = ApiResultType::ErrorOutOfHostMemory;
            break;
        case VK_ERROR_OUT_OF_DEVICE_MEMORY:
            _type = ApiResultType::ErrorOutOfDeviceMemory;
            break;
        case VK_ERROR_INITIALIZATION_FAILED:
            _type = ApiResultType::ErrorInitializationFailed;
            break;
        case VK_ERROR_DEVICE_LOST:
            _type = ApiResultType::ErrorDeviceLost;
            break;
        case VK_ERROR_MEMORY_MAP_FAILED:
            _type = ApiResultType::ErrorMemoryMapFailed;
            break;
        case VK_ERROR_LAYER_NOT_PRESENT:
            _type = ApiResultType::ErrorLayerNotPresent;
            break;
        case VK_ERROR_EXTENSION_NOT_PRESENT:
            _type = ApiResultType::ErrorExtensionNotPresent;
            break;
        case VK_ERROR_FEATURE_NOT_PRESENT:
            _type = ApiResultType::ErrorFeatureNotPresent;
            break;
        case VK_ERROR_INCOMPATIBLE_DRIVER:
            _type = ApiResultType::ErrorIncompatibleDriver;
            break;
        case VK_ERROR_TOO_MANY_OBJECTS:
            _type = ApiResultType::ErrorTooManyObjects;
            break;
        case VK_ERROR_FORMAT_NOT_SUPPORTED:
            _type = ApiResultType::ErrorFormatNotSupported;
            break;
        case VK_ERROR_FRAGMENTED_POOL:
            _type = ApiResultType::ErrorFragmentedPool;
            break;
        case VK_ERROR_UNKNOWN:
            _type = ApiResultType::ErrorUnknown;
            break;
        case VK_ERROR_OUT_OF_POOL_MEMORY:
            _type = ApiResultType::ErrorOutOfPoolMemory;
            break;
        case VK_ERROR_INVALID_EXTERNAL_HANDLE:
            _type = ApiResultType::ErrorInvalidExternalHandle;
            break;
        case VK_ERROR_FRAGMENTATION:
            _type = ApiResultType::ErrorFragmentation;
            break;
        case VK_ERROR_INVALID_OPAQUE_CAPTURE_ADDRESS:
            _type = ApiResultType::ErrorInvalidOpaqueCaptureAddress;
            break;
        case VK_PIPELINE_COMPILE_REQUIRED:
            _type = ApiResultType::PipelineCompileRequired;
            break;
        case VK_ERROR_SURFACE_LOST_KHR:
            _type = ApiResultType::ErrorSurfaceLostKhr;
            break;
        case VK_ERROR_NATIVE_WINDOW_IN_USE_KHR:
            _type = ApiResultType::ErrorNativeWindowInUseKhr;
            break;
        case VK_SUBOPTIMAL_KHR:
            _type = ApiResultType::SuboptimalKhr;
            break;
        case VK_ERROR_OUT_OF_DATE_KHR:
            _type = ApiResultType::ErrorOutOfDateKhr;
            break;
        case VK_ERROR_INCOMPATIBLE_DISPLAY_KHR:
            _type = ApiResultType::ErrorIncompatibleDisplayKhr;
            break;
        case VK_ERROR_VALIDATION_FAILED_EXT:
            _type = ApiResultType::ErrorValidationFailedExt;
            break;
        case VK_ERROR_INVALID_SHADER_NV:
            _type = ApiResultType::ErrorInvalidShaderNv;
            break;
        case VK_ERROR_IMAGE_USAGE_NOT_SUPPORTED_KHR:
            _type = ApiResultType::ErrorImageUsageNotSupportedKhr;
            break;
        case VK_ERROR_VIDEO_PICTURE_LAYOUT_NOT_SUPPORTED_KHR:
            _type =
                ApiResultType::ErrorVideoPictureLayoutNotSupportedKhr;
            break;
        case VK_ERROR_VIDEO_PROFILE_OPERATION_NOT_SUPPORTED_KHR:
            _type =
                ApiResultType::ErrorVideoProfileOperationNotSupportedKhr;
            break;
        case VK_ERROR_VIDEO_PROFILE_FORMAT_NOT_SUPPORTED_KHR:
            _type =
                ApiResultType::ErrorVideoProfileFormatNotSupportedKhr;
            break;
        case VK_ERROR_VIDEO_PROFILE_CODEC_NOT_SUPPORTED_KHR:
            _type = ApiResultType::ErrorVideoProfileCodecNotSupportedKhr;
            break;
        case VK_ERROR_VIDEO_STD_VERSION_NOT_SUPPORTED_KHR:
            _type = ApiResultType::ErrorVideoStdVersionNotSupportedKhr;
            break;
        case VK_ERROR_NOT_PERMITTED_KHR:
            _type = ApiResultType::ErrorNotPermittedKhr;
            break;
        case VK_ERROR_FULL_SCREEN_EXCLUSIVE_MODE_LOST_EXT:
            _type = ApiResultType::ErrorFullScreenExclusiveModeLostExt;
            break;
        case VK_THREAD_IDLE_KHR:
            _type = ApiResultType::ThreadIdleKhr;
            break;
        case VK_THREAD_DONE_KHR:
            _type = ApiResultType::ThreadDoneKhr;
            break;
        case VK_OPERATION_DEFERRED_KHR:
            _type = ApiResultType::OperationDeferredKhr;
            break;
        case VK_OPERATION_NOT_DEFERRED_KHR:
            _type = ApiResultType::OperationNotDeferredKhr;
            break;
        case VK_ERROR_COMPRESSION_EXHAUSTED_EXT:
            _type = ApiResultType::ErrorCompressionExhaustedExt;
            break;
        case VK_ERROR_INCOMPATIBLE_SHADER_BINARY_EXT:
            _type = ApiResultType::ErrorIncompatibleShaderBinaryExt;
            break;
        default:
            _type = ApiResultType::UndocumentedVkResult;
            break;
        }
    }

    std::string ApiResult::to_string() const
    {
        if (_type >= ApiResultType::_)
        {
            throw std::exception(
                "invalid enum value, this should never happen"
            );
        }
        if (_type == ApiResultType::UndocumentedVkResult)
        {
            return std::format(
                "undocumented VkResult \"{}\"",
                string_VkResult(_undocumented_vk_result)
            );
        }
        return ApiResultType_string[(uint8_t)_type];
    }

    AllocationScope AllocationScope_from_vk(
        VkSystemAllocationScope vk_scope
    )
    {
        switch (vk_scope)
        {
        case VK_SYSTEM_ALLOCATION_SCOPE_COMMAND:
            return AllocationScope::Command;
        case VK_SYSTEM_ALLOCATION_SCOPE_OBJECT:
            return AllocationScope::Object;
        case VK_SYSTEM_ALLOCATION_SCOPE_CACHE:
            return AllocationScope::Cache;
        case VK_SYSTEM_ALLOCATION_SCOPE_DEVICE:
            return AllocationScope::Device;
        case VK_SYSTEM_ALLOCATION_SCOPE_INSTANCE:
            return AllocationScope::Instance;
        default:
            return AllocationScope::Unknown;
        }
    }

    InternalAllocationType InternalAllocationType_from_vk(
        VkInternalAllocationType vk_type
    )
    {
        switch (vk_type)
        {
        case VK_INTERNAL_ALLOCATION_TYPE_EXECUTABLE:
            return InternalAllocationType::Executable;
        default:
            return InternalAllocationType::Unknown;
        }
    }

    uint32_t VulkanApiVersion_to_vk(VulkanApiVersion version)
    {
        switch (version)
        {
        case VulkanApiVersion::Vulkan1_0:
            return VK_API_VERSION_1_0;
        case VulkanApiVersion::Vulkan1_1:
            return VK_API_VERSION_1_1;
        case VulkanApiVersion::Vulkan1_2:
            return VK_API_VERSION_1_2;
        case VulkanApiVersion::Vulkan1_3:
            return VK_API_VERSION_1_3;
        default:
            return VK_API_VERSION_1_0;
        }
    }

    PhysicalDeviceType PhysicalDeviceType_from_vk(
        VkPhysicalDeviceType vk_type
    )
    {
        switch (vk_type)
        {
        case VK_PHYSICAL_DEVICE_TYPE_OTHER:
            return PhysicalDeviceType::Other;
        case VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU:
            return PhysicalDeviceType::IntegratedGpu;
        case VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU:
            return PhysicalDeviceType::DiscreteGpu;
        case VK_PHYSICAL_DEVICE_TYPE_VIRTUAL_GPU:
            return PhysicalDeviceType::VirtualGpu;
        case VK_PHYSICAL_DEVICE_TYPE_CPU:
            return PhysicalDeviceType::Cpu;
        default:
            return PhysicalDeviceType::Other;
        }
    }

    SampleCountFlags SampleCountFlags_from_vk(
        VkSampleCountFlags vk_flags
    )
    {
        return SampleCountFlags{
            ._1 = (bool)(vk_flags & VK_SAMPLE_COUNT_1_BIT),
            ._2 = (bool)(vk_flags & VK_SAMPLE_COUNT_2_BIT),
            ._4 = (bool)(vk_flags & VK_SAMPLE_COUNT_4_BIT),
            ._8 = (bool)(vk_flags & VK_SAMPLE_COUNT_8_BIT),
            ._16 = (bool)(vk_flags & VK_SAMPLE_COUNT_16_BIT),
            ._32 = (bool)(vk_flags & VK_SAMPLE_COUNT_32_BIT),
            ._64 = (bool)(vk_flags & VK_SAMPLE_COUNT_64_BIT)
        };
    }

    SampleCount SampleCount_from_vk(VkSampleCountFlagBits vk_sample_count)
    {
        switch (vk_sample_count)
        {
        case VK_SAMPLE_COUNT_1_BIT:
            return SampleCount::_1;
        case VK_SAMPLE_COUNT_2_BIT:
            return SampleCount::_2;
        case VK_SAMPLE_COUNT_4_BIT:
            return SampleCount::_4;
        case VK_SAMPLE_COUNT_8_BIT:
            return SampleCount::_8;
        case VK_SAMPLE_COUNT_16_BIT:
            return SampleCount::_16;
        case VK_SAMPLE_COUNT_32_BIT:
            return SampleCount::_32;
        case VK_SAMPLE_COUNT_64_BIT:
            return SampleCount::_64;
        default:
            return SampleCount::_1;
        }
    }

    VkSampleCountFlagBits SampleCount_to_vk(SampleCount sample_count)
    {
        switch (sample_count)
        {
        case bv::SampleCount::_1:
            return VK_SAMPLE_COUNT_1_BIT;
        case bv::SampleCount::_2:
            return VK_SAMPLE_COUNT_2_BIT;
        case bv::SampleCount::_4:
            return VK_SAMPLE_COUNT_4_BIT;
        case bv::SampleCount::_8:
            return VK_SAMPLE_COUNT_8_BIT;
        case bv::SampleCount::_16:
            return VK_SAMPLE_COUNT_16_BIT;
        case bv::SampleCount::_32:
            return VK_SAMPLE_COUNT_32_BIT;
        case bv::SampleCount::_64:
            return VK_SAMPLE_COUNT_64_BIT;
        default:
            return VK_SAMPLE_COUNT_1_BIT;
        }
    }

    QueueFlags QueueFlags_from_vk(
        const VkQueueFlags& vk_queue_flags,
        VkBool32 vk_presentation_support
    )
    {
        return QueueFlags{
            .graphics = (bool)(vk_queue_flags & VK_QUEUE_GRAPHICS_BIT),

            .presentation = (bool)vk_presentation_support,

            .compute = (bool)(vk_queue_flags & VK_QUEUE_COMPUTE_BIT),

            .transfer = (bool)(vk_queue_flags & VK_QUEUE_TRANSFER_BIT),

            .sparse_binding =
            (bool)(vk_queue_flags & VK_QUEUE_SPARSE_BINDING_BIT),

            .protected_ = (bool)(vk_queue_flags & VK_QUEUE_PROTECTED_BIT),

            .video_decode =
            (bool)(vk_queue_flags & VK_QUEUE_VIDEO_DECODE_BIT_KHR),

            .optical_flow_nv =
            (bool)(vk_queue_flags & VK_QUEUE_OPTICAL_FLOW_BIT_NV)
        };
    }

    ObjectType ObjectType_from_vk(VkObjectType vk_type)
    {
        switch (vk_type)
        {
        case VK_OBJECT_TYPE_UNKNOWN:
            return ObjectType::Unknown;
        case VK_OBJECT_TYPE_INSTANCE:
            return ObjectType::Instance;
        case VK_OBJECT_TYPE_PHYSICAL_DEVICE:
            return ObjectType::PhysicalDevice;
        case VK_OBJECT_TYPE_DEVICE:
            return ObjectType::Device;
        case VK_OBJECT_TYPE_QUEUE:
            return ObjectType::Queue;
        case VK_OBJECT_TYPE_SEMAPHORE:
            return ObjectType::Semaphore;
        case VK_OBJECT_TYPE_COMMAND_BUFFER:
            return ObjectType::CommandBuffer;
        case VK_OBJECT_TYPE_FENCE:
            return ObjectType::Fence;
        case VK_OBJECT_TYPE_DEVICE_MEMORY:
            return ObjectType::DeviceMemory;
        case VK_OBJECT_TYPE_BUFFER:
            return ObjectType::Buffer;
        case VK_OBJECT_TYPE_IMAGE:
            return ObjectType::Image;
        case VK_OBJECT_TYPE_EVENT:
            return ObjectType::Event;
        case VK_OBJECT_TYPE_QUERY_POOL:
            return ObjectType::QueryPool;
        case VK_OBJECT_TYPE_BUFFER_VIEW:
            return ObjectType::BufferView;
        case VK_OBJECT_TYPE_IMAGE_VIEW:
            return ObjectType::ImageView;
        case VK_OBJECT_TYPE_SHADER_MODULE:
            return ObjectType::ShaderModule;
        case VK_OBJECT_TYPE_PIPELINE_CACHE:
            return ObjectType::PipelineCache;
        case VK_OBJECT_TYPE_PIPELINE_LAYOUT:
            return ObjectType::PipelineLayout;
        case VK_OBJECT_TYPE_RENDER_PASS:
            return ObjectType::RenderPass;
        case VK_OBJECT_TYPE_PIPELINE:
            return ObjectType::Pipeline;
        case VK_OBJECT_TYPE_DESCRIPTOR_SET_LAYOUT:
            return ObjectType::DescriptorSetLayout;
        case VK_OBJECT_TYPE_SAMPLER:
            return ObjectType::Sampler;
        case VK_OBJECT_TYPE_DESCRIPTOR_POOL:
            return ObjectType::DescriptorPool;
        case VK_OBJECT_TYPE_DESCRIPTOR_SET:
            return ObjectType::DescriptorSet;
        case VK_OBJECT_TYPE_FRAMEBUFFER:
            return ObjectType::Framebuffer;
        case VK_OBJECT_TYPE_COMMAND_POOL:
            return ObjectType::CommandPool;
        case VK_OBJECT_TYPE_SAMPLER_YCBCR_CONVERSION:
            return ObjectType::SamplerYcbcrConversion;
        case VK_OBJECT_TYPE_DESCRIPTOR_UPDATE_TEMPLATE:
            return ObjectType::DescriptorUpdateTemplate;
        case VK_OBJECT_TYPE_PRIVATE_DATA_SLOT:
            return ObjectType::PrivateDataSlot;
        case VK_OBJECT_TYPE_SURFACE_KHR:
            return ObjectType::Surface;
        case VK_OBJECT_TYPE_SWAPCHAIN_KHR:
            return ObjectType::Swapchain;
        case VK_OBJECT_TYPE_DISPLAY_KHR:
            return ObjectType::Display;
        case VK_OBJECT_TYPE_DISPLAY_MODE_KHR:
            return ObjectType::DisplayMode;
        case VK_OBJECT_TYPE_DEBUG_REPORT_CALLBACK_EXT:
            return ObjectType::DebugReportCallback;
        case VK_OBJECT_TYPE_VIDEO_SESSION_KHR:
            return ObjectType::VideoSession;
        case VK_OBJECT_TYPE_VIDEO_SESSION_PARAMETERS_KHR:
            return ObjectType::VideoSessionParameters;
        case VK_OBJECT_TYPE_CU_MODULE_NVX:
            return ObjectType::CuModuleNvx;
        case VK_OBJECT_TYPE_CU_FUNCTION_NVX:
            return ObjectType::CuFunctionNvx;
        case VK_OBJECT_TYPE_DEBUG_UTILS_MESSENGER_EXT:
            return ObjectType::DebugMessenger;
        case VK_OBJECT_TYPE_ACCELERATION_STRUCTURE_KHR:
            return ObjectType::AccelerationStructure;
        case VK_OBJECT_TYPE_VALIDATION_CACHE_EXT:
            return ObjectType::ValidationCache;
        case VK_OBJECT_TYPE_ACCELERATION_STRUCTURE_NV:
            return ObjectType::AccelerationStructureNv;
        case VK_OBJECT_TYPE_PERFORMANCE_CONFIGURATION_INTEL:
            return ObjectType::PerformanceConfigurationIntel;
        case VK_OBJECT_TYPE_DEFERRED_OPERATION_KHR:
            return ObjectType::DeferredOperation;
        case VK_OBJECT_TYPE_INDIRECT_COMMANDS_LAYOUT_NV:
            return ObjectType::IndirectCommandsLayoutNv;
        case VK_OBJECT_TYPE_BUFFER_COLLECTION_FUCHSIA:
            return ObjectType::BufferCollectionFuchsia;
        case VK_OBJECT_TYPE_MICROMAP_EXT:
            return ObjectType::Micromap;
        case VK_OBJECT_TYPE_OPTICAL_FLOW_SESSION_NV:
            return ObjectType::OpticalFlowSessionNv;
        case VK_OBJECT_TYPE_SHADER_EXT:
            return ObjectType::Shader;
        default:
            return ObjectType::Unknown;
        }
    }

    VkDebugUtilsMessageSeverityFlagsEXT DebugMessageSeverityFlags_to_vk(
        const DebugMessageSeverityFlags& flags
    )
    {
        VkDebugUtilsMessageSeverityFlagsEXT vk_flags = 0;
        if (flags.verbose)
        {
            vk_flags |=
                VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT;
        }
        if (flags.info)
        {
            vk_flags |=
                VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT;
        }
        if (flags.warning)
        {
            vk_flags |=
                VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT;
        }
        if (flags.error)
        {
            vk_flags |=
                VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
        }
        return vk_flags;
    }

    DebugMessageTypeFlags DebugMessageTypeFlags_from_vk(
        VkDebugUtilsMessageTypeFlagsEXT vk_flags
    )
    {
        return DebugMessageTypeFlags{
            .general =
            (bool)(vk_flags & VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT),

            .validation =
            (bool)(vk_flags & VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT),

            .performance =
            (bool)(vk_flags & VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT),

            .device_address_binding =
            (bool)(vk_flags
                & VK_DEBUG_UTILS_MESSAGE_TYPE_DEVICE_ADDRESS_BINDING_BIT_EXT)
        };
    }

    VkDebugUtilsMessageTypeFlagsEXT DebugMessageTypeFlags_to_vk(
        const DebugMessageTypeFlags& flags
    )
    {
        VkDebugUtilsMessageTypeFlagsEXT vk_flags = 0;
        if (flags.general)
        {
            vk_flags |=
                VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT;
        }
        if (flags.validation)
        {
            vk_flags |=
                VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT;
        }
        if (flags.performance)
        {
            vk_flags |=
                VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
        }
        if (flags.device_address_binding)
        {
            vk_flags |=
                VK_DEBUG_UTILS_MESSAGE_TYPE_DEVICE_ADDRESS_BINDING_BIT_EXT;
        }
        return vk_flags;
    }

    DebugMessageSeverity DebugMessageSeverity_from_vk(
        VkDebugUtilsMessageSeverityFlagBitsEXT vk_severity
    )
    {
        if (vk_severity
            & VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT)
            return DebugMessageSeverity::Error;
        else if (vk_severity
            & VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT)
            return DebugMessageSeverity::Warning;
        else if (vk_severity
            & VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT)
            return DebugMessageSeverity::Info;
        else if (vk_severity
            & VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT)
            return DebugMessageSeverity::Verbose;
        else
            return DebugMessageSeverity::Verbose;
    }

    VkDeviceQueueCreateFlags QueueRequestFlags_to_vk(
        const QueueRequestFlags& flags
    )
    {
        VkDeviceQueueCreateFlags vk_flags = 0;
        if (flags.protected_)
        {
            vk_flags |= VK_DEVICE_QUEUE_CREATE_PROTECTED_BIT;
        }
        return vk_flags;
    }

    ExtensionProperties ExtensionProperties_from_vk(
        VkExtensionProperties vk_properties
    )
    {
        return ExtensionProperties{
            .name = cstr_to_std(vk_properties.extensionName),
            .spec_version = vk_properties.specVersion
        };
    }

    LayerProperties LayerProperties_from_vk(
        const VkLayerProperties& vk_properties
    )
    {
        return LayerProperties{
            .name = cstr_to_std(vk_properties.layerName),
            .spec_version = Version(vk_properties.specVersion),
            .implementation_version = vk_properties.implementationVersion,
            .description = cstr_to_std(vk_properties.description)
        };
    }

    PhysicalDeviceLimits PhysicalDeviceLimits_from_vk(
        const VkPhysicalDeviceLimits& vk_limits
    )
    {
        return PhysicalDeviceLimits{
            .max_image_dimension1d =
            vk_limits.maxImageDimension1D,

            .max_image_dimension2d =
            vk_limits.maxImageDimension2D,

            .max_image_dimension3d =
            vk_limits.maxImageDimension3D,

            .max_image_dimension_cube =
            vk_limits.maxImageDimensionCube,

            .max_image_array_layers =
            vk_limits.maxImageArrayLayers,

            .max_texel_buffer_elements =
            vk_limits.maxTexelBufferElements,

            .max_uniform_buffer_range =
            vk_limits.maxUniformBufferRange,

            .max_storage_buffer_range =
            vk_limits.maxStorageBufferRange,

            .max_push_constants_size =
            vk_limits.maxPushConstantsSize,

            .max_memory_allocation_count =
            vk_limits.maxMemoryAllocationCount,

            .max_sampler_allocation_count =
            vk_limits.maxSamplerAllocationCount,

            .buffer_image_granularity =
            vk_limits.bufferImageGranularity,

            .sparse_address_space_size =
            vk_limits.sparseAddressSpaceSize,

            .max_bound_descriptor_sets =
            vk_limits.maxBoundDescriptorSets,

            .max_per_stage_descriptor_samplers =
            vk_limits.maxPerStageDescriptorSamplers,

            .max_per_stage_descriptor_uniform_buffers =
            vk_limits.maxPerStageDescriptorUniformBuffers,

            .max_per_stage_descriptor_storage_buffers =
            vk_limits.maxPerStageDescriptorStorageBuffers,

            .max_per_stage_descriptor_sampled_images =
            vk_limits.maxPerStageDescriptorSampledImages,

            .max_per_stage_descriptor_storage_images =
            vk_limits.maxPerStageDescriptorStorageImages,

            .max_per_stage_descriptor_input_attachments =
            vk_limits.maxPerStageDescriptorInputAttachments,

            .max_per_stage_resources =
            vk_limits.maxPerStageResources,

            .max_descriptor_set_samplers =
            vk_limits.maxDescriptorSetSamplers,

            .max_descriptor_set_uniform_buffers =
            vk_limits.maxDescriptorSetUniformBuffers,

            .max_descriptor_set_uniform_buffers_dynamic =
            vk_limits.maxDescriptorSetUniformBuffersDynamic,

            .max_descriptor_set_storage_buffers =
            vk_limits.maxDescriptorSetStorageBuffers,

            .max_descriptor_set_storage_buffers_dynamic =
            vk_limits.maxDescriptorSetStorageBuffersDynamic,

            .max_descriptor_set_sampled_images =
            vk_limits.maxDescriptorSetSampledImages,

            .max_descriptor_set_storage_images =
            vk_limits.maxDescriptorSetStorageImages,

            .max_descriptor_set_input_attachments =
            vk_limits.maxDescriptorSetInputAttachments,

            .max_vertex_input_attributes =
            vk_limits.maxVertexInputAttributes,

            .max_vertex_input_bindings =
            vk_limits.maxVertexInputBindings,

            .max_vertex_input_attribute_offset =
            vk_limits.maxVertexInputAttributeOffset,

            .max_vertex_input_binding_stride =
            vk_limits.maxVertexInputBindingStride,

            .max_vertex_output_components =
            vk_limits.maxVertexOutputComponents,

            .max_tessellation_generation_level =
            vk_limits.maxTessellationGenerationLevel,

            .max_tessellation_patch_size =
            vk_limits.maxTessellationPatchSize,

            .max_tessellation_control_per_vertex_input_components =
            vk_limits
            .maxTessellationControlPerVertexInputComponents,

            .max_tessellation_control_per_vertex_output_components =
            vk_limits
            .maxTessellationControlPerVertexOutputComponents,

            .max_tessellation_control_per_patch_output_components =
            vk_limits
            .maxTessellationControlPerPatchOutputComponents,

            .max_tessellation_control_total_output_components =
            vk_limits
            .maxTessellationControlTotalOutputComponents,

            .max_tessellation_evaluation_input_components =
            vk_limits.maxTessellationEvaluationInputComponents,

            .max_tessellation_evaluation_output_components =
            vk_limits.maxTessellationEvaluationOutputComponents,

            .max_geometry_shader_invocations =
            vk_limits.maxGeometryShaderInvocations,

            .max_geometry_input_components =
            vk_limits.maxGeometryInputComponents,

            .max_geometry_output_components =
            vk_limits.maxGeometryOutputComponents,

            .max_geometry_output_vertices =
            vk_limits.maxGeometryOutputVertices,

            .max_geometry_total_output_components =
            vk_limits.maxGeometryTotalOutputComponents,

            .max_fragment_input_components =
            vk_limits.maxFragmentInputComponents,

            .max_fragment_output_attachments =
            vk_limits.maxFragmentOutputAttachments,

            .max_fragment_dual_src_attachments =
            vk_limits.maxFragmentDualSrcAttachments,

            .max_fragment_combined_output_resources =
            vk_limits.maxFragmentCombinedOutputResources,

            .max_compute_shared_memory_size =
            vk_limits.maxComputeSharedMemorySize,

            .max_compute_work_group_count = raw_arr_to_std<3>(
                vk_limits.maxComputeWorkGroupCount
            ),

            .max_compute_work_group_invocations =
            vk_limits.maxComputeWorkGroupInvocations,

            .max_compute_work_group_size = raw_arr_to_std<3>(
                vk_limits.maxComputeWorkGroupSize
            ),

            .sub_pixel_precision_bits =
            vk_limits.subPixelPrecisionBits,

            .sub_texel_precision_bits =
            vk_limits.subTexelPrecisionBits,

            .mipmap_precision_bits =
            vk_limits.mipmapPrecisionBits,

            .max_draw_indexed_index_value =
            vk_limits.maxDrawIndexedIndexValue,

            .max_draw_indirect_count =
            vk_limits.maxDrawIndirectCount,

            .max_sampler_lod_bias = vk_limits.maxSamplerLodBias,

            .max_sampler_anisotropy =
            vk_limits.maxSamplerAnisotropy,

            .max_viewports = vk_limits.maxViewports,

            .max_viewport_dimensions = raw_arr_to_std<2>(
                vk_limits.maxViewportDimensions
            ),

            .viewport_bounds_range = raw_arr_to_std<2>(
                vk_limits.viewportBoundsRange
            ),

            .viewport_sub_pixel_bits =
            vk_limits.viewportSubPixelBits,

            .min_memory_map_alignment =
            vk_limits.minMemoryMapAlignment,

            .min_texel_buffer_offset_alignment =
            vk_limits.minTexelBufferOffsetAlignment,

            .min_uniform_buffer_offset_alignment =
            vk_limits.minUniformBufferOffsetAlignment,

            .min_storage_buffer_offset_alignment =
            vk_limits.minStorageBufferOffsetAlignment,

            .min_texel_offset = vk_limits.minTexelOffset,

            .max_texel_offset = vk_limits.maxTexelOffset,

            .min_texel_gather_offset =
            vk_limits.minTexelGatherOffset,

            .max_texel_gather_offset =
            vk_limits.maxTexelGatherOffset,

            .min_interpolation_offset =
            vk_limits.minInterpolationOffset,

            .max_interpolation_offset =
            vk_limits.maxInterpolationOffset,

            .sub_pixel_interpolation_offset_bits =
            vk_limits.subPixelInterpolationOffsetBits,

            .max_framebuffer_width =
            vk_limits.maxFramebufferWidth,

            .max_framebuffer_height =
            vk_limits.maxFramebufferHeight,

            .max_framebuffer_layers =
            vk_limits.maxFramebufferLayers,

            .framebuffer_color_sample_counts = SampleCountFlags_from_vk(
                vk_limits.framebufferColorSampleCounts
            ),

            .framebuffer_depth_sample_counts = SampleCountFlags_from_vk(
                vk_limits.framebufferDepthSampleCounts
            ),

            .framebuffer_stencil_sample_counts = SampleCountFlags_from_vk(
                vk_limits.framebufferStencilSampleCounts
            ),

            .framebuffer_no_attachments_sample_counts =
            SampleCountFlags_from_vk(
                vk_limits.framebufferNoAttachmentsSampleCounts
            ),

            .max_color_attachments =
            vk_limits.maxColorAttachments,

            .sampled_image_color_sample_counts = SampleCountFlags_from_vk(
                vk_limits.sampledImageColorSampleCounts
            ),

            .sampled_image_integer_sample_counts = SampleCountFlags_from_vk(
                vk_limits.sampledImageIntegerSampleCounts
            ),

            .sampled_image_depth_sample_counts = SampleCountFlags_from_vk(
                vk_limits.sampledImageDepthSampleCounts
            ),

            .sampled_image_stencil_sample_counts = SampleCountFlags_from_vk(
                vk_limits.sampledImageStencilSampleCounts
            ),

            .storage_image_sample_counts = SampleCountFlags_from_vk(
                vk_limits.storageImageSampleCounts
            ),

            .max_sample_mask_words =
            vk_limits.maxSampleMaskWords,

            .timestamp_compute_and_graphics =
            (bool)vk_limits.timestampComputeAndGraphics,

            .timestamp_period = vk_limits.timestampPeriod,

            .max_clip_distances = vk_limits.maxClipDistances,

            .max_cull_distances = vk_limits.maxCullDistances,

            .max_combined_clip_and_cull_distances =
            vk_limits.maxCombinedClipAndCullDistances,

            .discrete_queue_priorities =
            vk_limits.discreteQueuePriorities,

            .point_size_range = raw_arr_to_std<2>(
                vk_limits.pointSizeRange
            ),

            .line_width_range = raw_arr_to_std<2>(
                vk_limits.lineWidthRange
            ),

            .point_size_granularity =
            vk_limits.pointSizeGranularity,

            .line_width_granularity =
            vk_limits.lineWidthGranularity,

            .strict_lines = (bool)vk_limits.strictLines,

            .standard_sample_locations =
            (bool)vk_limits.standardSampleLocations,

            .optimal_buffer_copy_offset_alignment =
            vk_limits.optimalBufferCopyOffsetAlignment,

            .optimal_buffer_copy_row_pitch_alignment =
            vk_limits.optimalBufferCopyRowPitchAlignment,

            .non_coherent_atom_size =
            vk_limits.nonCoherentAtomSize
        };
    }

    PhysicalDeviceSparseProperties PhysicalDeviceSparseProperties_from_vk(
        const VkPhysicalDeviceSparseProperties& vk_properties
    )
    {
        return PhysicalDeviceSparseProperties{
            .residency_standard2d_block_shape =
            (bool)vk_properties.residencyStandard2DBlockShape,

            .residency_standard2d_multisample_block_shape =
            (bool)vk_properties.residencyStandard2DMultisampleBlockShape,

            .residency_standard3d_block_shape =
            (bool)vk_properties.residencyStandard3DBlockShape,

            .residency_aligned_mip_size =
            (bool)vk_properties.residencyAlignedMipSize,

            .residency_non_resident_strict =
            (bool)vk_properties.residencyNonResidentStrict
        };
    }

    PhysicalDeviceProperties PhysicalDeviceProperties_from_vk(
        const VkPhysicalDeviceProperties& vk_properties
    )
    {
        return PhysicalDeviceProperties{
            .api_version = Version(vk_properties.apiVersion),
            .driver_version = vk_properties.driverVersion,
            .vendor_id = vk_properties.vendorID,
            .device_id = vk_properties.deviceID,
            .device_type = PhysicalDeviceType_from_vk(
                vk_properties.deviceType
            ),
            .device_name = cstr_to_std(vk_properties.deviceName),
            .pipeline_cache_uuid = raw_arr_to_std<VK_UUID_SIZE>(
                vk_properties.pipelineCacheUUID
            ),
            .limits = PhysicalDeviceLimits_from_vk(vk_properties.limits),
            .sparse_properties = PhysicalDeviceSparseProperties_from_vk(
                vk_properties.sparseProperties
            )
        };
    }

    PhysicalDeviceFeatures PhysicalDeviceFeatures_from_vk(
        const VkPhysicalDeviceFeatures& vk_features
    )
    {
        return PhysicalDeviceFeatures{
            .robust_buffer_access =
            (bool)vk_features.robustBufferAccess,

            .full_draw_index_uint32 =
            (bool)vk_features.fullDrawIndexUint32,

            .image_cube_array =
            (bool)vk_features.imageCubeArray,

            .independent_blend =
            (bool)vk_features.independentBlend,

            .geometry_shader =
            (bool)vk_features.geometryShader,

            .tessellation_shader =
            (bool)vk_features.tessellationShader,

            .sample_rate_shading =
            (bool)vk_features.sampleRateShading,

            .dual_src_blend =
            (bool)vk_features.dualSrcBlend,

            .logic_op =
            (bool)vk_features.logicOp,

            .multi_draw_indirect =
            (bool)vk_features.multiDrawIndirect,

            .draw_indirect_first_instance =
            (bool)vk_features.drawIndirectFirstInstance,

            .depth_clamp =
            (bool)vk_features.depthClamp,

            .depth_bias_clamp =
            (bool)vk_features.depthBiasClamp,

            .fill_mode_non_solid =
            (bool)vk_features.fillModeNonSolid,

            .depth_bounds =
            (bool)vk_features.depthBounds,

            .wide_lines =
            (bool)vk_features.wideLines,

            .large_points =
            (bool)vk_features.largePoints,

            .alpha_to_one =
            (bool)vk_features.alphaToOne,

            .multi_viewport =
            (bool)vk_features.multiViewport,

            .sampler_anisotropy =
            (bool)vk_features.samplerAnisotropy,

            .texture_compression_etc2 =
            (bool)vk_features.textureCompressionETC2,

            .texture_compression_astc_ldr =
            (bool)vk_features.textureCompressionASTC_LDR,

            .texture_compression_bc =
            (bool)vk_features.textureCompressionBC,

            .occlusion_query_precise =
            (bool)vk_features.occlusionQueryPrecise,

            .pipeline_statistics_query =
            (bool)vk_features.pipelineStatisticsQuery,

            .vertex_pipeline_stores_and_atomics =
            (bool)vk_features.vertexPipelineStoresAndAtomics,

            .fragment_stores_and_atomics =
            (bool)vk_features.fragmentStoresAndAtomics,

            .shader_tessellation_and_geometry_point_size =
            (bool)vk_features
            .shaderTessellationAndGeometryPointSize,

            .shader_image_gather_extended =
            (bool)vk_features.shaderImageGatherExtended,

            .shader_storage_image_extended_formats =
            (bool)vk_features
            .shaderStorageImageExtendedFormats,

            .shader_storage_image_multisample =
            (bool)vk_features.shaderStorageImageMultisample,

            .shader_storage_image_read_without_format =
            (bool)vk_features
            .shaderStorageImageReadWithoutFormat,

            .shader_storage_image_write_without_format =
            (bool)vk_features
            .shaderStorageImageWriteWithoutFormat,

            .shader_uniform_buffer_array_dynamic_indexing =
            (bool)vk_features
            .shaderUniformBufferArrayDynamicIndexing,

            .shader_sampled_image_array_dynamic_indexing =
            (bool)vk_features
            .shaderSampledImageArrayDynamicIndexing,

            .shader_storage_buffer_array_dynamic_indexing =
            (bool)vk_features
            .shaderStorageBufferArrayDynamicIndexing,

            .shader_storage_image_array_dynamic_indexing =
            (bool)vk_features
            .shaderStorageImageArrayDynamicIndexing,

            .shader_clip_distance =
            (bool)vk_features.shaderClipDistance,

            .shader_cull_distance =
            (bool)vk_features.shaderCullDistance,

            .shader_float64 =
            (bool)vk_features.shaderFloat64,

            .shader_int64 =
            (bool)vk_features.shaderInt64,

            .shader_int16 =
            (bool)vk_features.shaderInt16,

            .shader_resource_residency =
            (bool)vk_features.shaderResourceResidency,

            .shader_resource_min_lod =
            (bool)vk_features.shaderResourceMinLod,

            .sparse_binding =
            (bool)vk_features.sparseBinding,

            .sparse_residency_buffer =
            (bool)vk_features.sparseResidencyBuffer,

            .sparse_residency_image2d =
            (bool)vk_features.sparseResidencyImage2D,

            .sparse_residency_image3d =
            (bool)vk_features.sparseResidencyImage3D,

            .sparse_residency2_samples =
            (bool)vk_features.sparseResidency2Samples,

            .sparse_residency4_samples =
            (bool)vk_features.sparseResidency4Samples,

            .sparse_residency8_samples =
            (bool)vk_features.sparseResidency8Samples,

            .sparse_residency16_samples =
            (bool)vk_features.sparseResidency16Samples,

            .sparse_residency_aliased =
            (bool)vk_features.sparseResidencyAliased,

            .variable_multisample_rate =
            (bool)vk_features.variableMultisampleRate,

            .inherited_queries =
            (bool)vk_features.inheritedQueries
        };
    }

    VkPhysicalDeviceFeatures PhysicalDeviceFeatures_to_vk(
        const PhysicalDeviceFeatures& features
    )
    {
        return VkPhysicalDeviceFeatures{
            .robustBufferAccess =
            features.robust_buffer_access,

            .fullDrawIndexUint32 =
            features.full_draw_index_uint32,

            .imageCubeArray =
            features.image_cube_array,

            .independentBlend =
            features.independent_blend,

            .geometryShader =
            features.geometry_shader,

            .tessellationShader =
            features.tessellation_shader,

            .sampleRateShading =
            features.sample_rate_shading,

            .dualSrcBlend =
            features.dual_src_blend,

            .logicOp =
            features.logic_op,

            .multiDrawIndirect =
            features.multi_draw_indirect,

            .drawIndirectFirstInstance =
            features.draw_indirect_first_instance,

            .depthClamp =
            features.depth_clamp,

            .depthBiasClamp =
            features.depth_bias_clamp,

            .fillModeNonSolid =
            features.fill_mode_non_solid,

            .depthBounds =
            features.depth_bounds,

            .wideLines =
            features.wide_lines,

            .largePoints =
            features.large_points,

            .alphaToOne =
            features.alpha_to_one,

            .multiViewport =
            features.multi_viewport,

            .samplerAnisotropy =
            features.sampler_anisotropy,

            .textureCompressionETC2 =
            features.texture_compression_etc2,

            .textureCompressionASTC_LDR =
            features.texture_compression_astc_ldr,

            .textureCompressionBC =
            features.texture_compression_bc,

            .occlusionQueryPrecise =
            features.occlusion_query_precise,

            .pipelineStatisticsQuery =
            features.pipeline_statistics_query,

            .vertexPipelineStoresAndAtomics =
            features.vertex_pipeline_stores_and_atomics,

            .fragmentStoresAndAtomics =
            features.fragment_stores_and_atomics,

            .shaderTessellationAndGeometryPointSize = features
            .shader_tessellation_and_geometry_point_size,

            .shaderImageGatherExtended =
            features.shader_image_gather_extended,

            .shaderStorageImageExtendedFormats =
            features.shader_storage_image_extended_formats,

            .shaderStorageImageMultisample =
            features.shader_storage_image_multisample,

            .shaderStorageImageReadWithoutFormat =
            features.shader_storage_image_read_without_format,

            .shaderStorageImageWriteWithoutFormat =
            features.shader_storage_image_write_without_format,

            .shaderUniformBufferArrayDynamicIndexing = features
            .shader_uniform_buffer_array_dynamic_indexing,

            .shaderSampledImageArrayDynamicIndexing = features
            .shader_sampled_image_array_dynamic_indexing,

            .shaderStorageBufferArrayDynamicIndexing = features
            .shader_storage_buffer_array_dynamic_indexing,

            .shaderStorageImageArrayDynamicIndexing = features
            .shader_storage_image_array_dynamic_indexing,

            .shaderClipDistance =
            features.shader_clip_distance,

            .shaderCullDistance =
            features.shader_cull_distance,

            .shaderFloat64 =
            features.shader_float64,

            .shaderInt64 =
            features.shader_int64,

            .shaderInt16 =
            features.shader_int16,

            .shaderResourceResidency =
            features.shader_resource_residency,

            .shaderResourceMinLod =
            features.shader_resource_min_lod,

            .sparseBinding =
            features.sparse_binding,

            .sparseResidencyBuffer =
            features.sparse_residency_buffer,

            .sparseResidencyImage2D =
            features.sparse_residency_image2d,

            .sparseResidencyImage3D =
            features.sparse_residency_image3d,

            .sparseResidency2Samples =
            features.sparse_residency2_samples,

            .sparseResidency4Samples =
            features.sparse_residency4_samples,

            .sparseResidency8Samples =
            features.sparse_residency8_samples,

            .sparseResidency16Samples =
            features.sparse_residency16_samples,

            .sparseResidencyAliased =
            features.sparse_residency_aliased,

            .variableMultisampleRate =
            features.variable_multisample_rate,

            .inheritedQueries =
            features.inherited_queries,
        };
    }

    Extent3d Extent3d_from_vk(const VkExtent3D& vk_extent_3d)
    {
        return Extent3d{
            .width = vk_extent_3d.width,
            .height = vk_extent_3d.height,
            .depth = vk_extent_3d.depth
        };
    }

    VkExtent3D Extent3d_to_vk(const Extent3d& extent_3d)
    {
        return VkExtent3D{
            .width = extent_3d.width,
            .height = extent_3d.height,
            .depth = extent_3d.depth
        };
    }

    Extent2d Extent2d_from_vk(const VkExtent2D& vk_extent_2d)
    {
        return Extent2d{
            .width = vk_extent_2d.width,
            .height = vk_extent_2d.height
        };
    }

    VkExtent2D Extent2d_to_vk(const Extent2d& extent_2d)
    {
        return VkExtent2D{
            .width = extent_2d.width,
            .height = extent_2d.height
        };
    }

    QueueFamily QueueFamily_from_vk(
        const VkQueueFamilyProperties& vk_family,
        VkBool32 vk_presentation_support
    )
    {
        return QueueFamily{
            .queue_flags = QueueFlags_from_vk(
                vk_family.queueFlags,
                vk_presentation_support
            ),
            .queue_count = vk_family.queueCount,
            .timestamp_valid_bits = vk_family.timestampValidBits,
            .min_image_transfer_granularity = Extent3d_from_vk(
                vk_family.minImageTransferGranularity
            )
        };
    }

    SurfaceTransformFlags SurfaceTransformFlags_from_vk(const VkSurfaceTransformFlagsKHR& vk_flags)
    {
        return SurfaceTransformFlags{
            .identity =
            (bool)(vk_flags & VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR),

            .rotate90 =
            (bool)(vk_flags & VK_SURFACE_TRANSFORM_ROTATE_90_BIT_KHR),

            .rotate180 =
            (bool)(vk_flags & VK_SURFACE_TRANSFORM_ROTATE_180_BIT_KHR),

            .rotate270 =
            (bool)(vk_flags & VK_SURFACE_TRANSFORM_ROTATE_270_BIT_KHR),

            .horizontal_mirror =
            (bool)(vk_flags & VK_SURFACE_TRANSFORM_HORIZONTAL_MIRROR_BIT_KHR),

            .horizontal_mirror_rotate90 =
            (bool)(vk_flags
                & VK_SURFACE_TRANSFORM_HORIZONTAL_MIRROR_ROTATE_90_BIT_KHR),

            .horizontal_mirror_rotate180 =
            (bool)(vk_flags
                & VK_SURFACE_TRANSFORM_HORIZONTAL_MIRROR_ROTATE_180_BIT_KHR),

            .horizontal_mirror_rotate270 =
            (bool)(vk_flags
                & VK_SURFACE_TRANSFORM_HORIZONTAL_MIRROR_ROTATE_270_BIT_KHR),

            .inherit =
            (bool)(vk_flags & VK_SURFACE_TRANSFORM_INHERIT_BIT_KHR),
        };
    }

    SurfaceTransform SurfaceTransform_from_vk(
        VkSurfaceTransformFlagBitsKHR vk_transform
    )
    {
        switch (vk_transform)
        {
        case VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR:
            return SurfaceTransform::Identity;
        case VK_SURFACE_TRANSFORM_ROTATE_90_BIT_KHR:
            return SurfaceTransform::Rotate90;
        case VK_SURFACE_TRANSFORM_ROTATE_180_BIT_KHR:
            return SurfaceTransform::Rotate180;
        case VK_SURFACE_TRANSFORM_ROTATE_270_BIT_KHR:
            return SurfaceTransform::Rotate270;
        case VK_SURFACE_TRANSFORM_HORIZONTAL_MIRROR_BIT_KHR:
            return SurfaceTransform::HorizontalMirror;
        case VK_SURFACE_TRANSFORM_HORIZONTAL_MIRROR_ROTATE_90_BIT_KHR:
            return SurfaceTransform::HorizontalMirrorRotate90;
        case VK_SURFACE_TRANSFORM_HORIZONTAL_MIRROR_ROTATE_180_BIT_KHR:
            return SurfaceTransform::HorizontalMirrorRotate180;
        case VK_SURFACE_TRANSFORM_HORIZONTAL_MIRROR_ROTATE_270_BIT_KHR:
            return SurfaceTransform::HorizontalMirrorRotate270;
        case VK_SURFACE_TRANSFORM_INHERIT_BIT_KHR:
            return SurfaceTransform::Inherit;
        default:
            return SurfaceTransform::Identity;
        }
    }

    VkSurfaceTransformFlagBitsKHR SurfaceTransform_to_vk(
        SurfaceTransform transform
    )
    {
        switch (transform)
        {
        case SurfaceTransform::Identity:
            return VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;
        case SurfaceTransform::Rotate90:
            return VK_SURFACE_TRANSFORM_ROTATE_90_BIT_KHR;
        case SurfaceTransform::Rotate180:
            return VK_SURFACE_TRANSFORM_ROTATE_180_BIT_KHR;
        case SurfaceTransform::Rotate270:
            return VK_SURFACE_TRANSFORM_ROTATE_270_BIT_KHR;
        case SurfaceTransform::HorizontalMirror:
            return VK_SURFACE_TRANSFORM_HORIZONTAL_MIRROR_BIT_KHR;
        case SurfaceTransform::HorizontalMirrorRotate90:
            return VK_SURFACE_TRANSFORM_HORIZONTAL_MIRROR_ROTATE_90_BIT_KHR;
        case SurfaceTransform::HorizontalMirrorRotate180:
            return VK_SURFACE_TRANSFORM_HORIZONTAL_MIRROR_ROTATE_180_BIT_KHR;
        case SurfaceTransform::HorizontalMirrorRotate270:
            return VK_SURFACE_TRANSFORM_HORIZONTAL_MIRROR_ROTATE_270_BIT_KHR;
        case SurfaceTransform::Inherit:
            return VK_SURFACE_TRANSFORM_INHERIT_BIT_KHR;
        default:
            return VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;
        }
    }

    CompositeAlphaFlags CompositeAlphaFlags_from_vk(
        VkCompositeAlphaFlagsKHR vk_flags
    )
    {
        return CompositeAlphaFlags{
            .opaque = (bool)(vk_flags & VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR),

            .pre_multiplied =
            (bool)(vk_flags & VK_COMPOSITE_ALPHA_PRE_MULTIPLIED_BIT_KHR),

            .post_multiplied =
            (bool)(vk_flags & VK_COMPOSITE_ALPHA_POST_MULTIPLIED_BIT_KHR),

            .inherit = (bool)(vk_flags & VK_COMPOSITE_ALPHA_INHERIT_BIT_KHR)
        };
    }

    VkCompositeAlphaFlagBitsKHR CompositeAlpha_to_vk(CompositeAlpha alpha)
    {
        switch (alpha)
        {
        case CompositeAlpha::Opaque:
            return VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
        case CompositeAlpha::PreMultiplied:
            return VK_COMPOSITE_ALPHA_PRE_MULTIPLIED_BIT_KHR;
        case CompositeAlpha::PostMultiplied:
            return VK_COMPOSITE_ALPHA_POST_MULTIPLIED_BIT_KHR;
        case CompositeAlpha::Inherit:
            return VK_COMPOSITE_ALPHA_INHERIT_BIT_KHR;
        default:
            return VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
        }
    }

    ImageUsageFlags ImageUsageFlags_from_vk(VkImageUsageFlags vk_flags)
    {
        return ImageUsageFlags{
            .transfer_src =
            (bool)(vk_flags & VK_IMAGE_USAGE_TRANSFER_SRC_BIT),

            .transfer_dst =
            (bool)(vk_flags & VK_IMAGE_USAGE_TRANSFER_DST_BIT),

            .sampled =
            (bool)(vk_flags & VK_IMAGE_USAGE_SAMPLED_BIT),

            .storage =
            (bool)(vk_flags & VK_IMAGE_USAGE_STORAGE_BIT),

            .color_attachment =
            (bool)(vk_flags & VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT),

            .depth_stencil_attachment =
            (bool)(vk_flags & VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT),

            .transient_attachment =
            (bool)(vk_flags & VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT),

            .input_attachment =
            (bool)(vk_flags & VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT),

            .video_decode_dst =
            (bool)(vk_flags & VK_IMAGE_USAGE_VIDEO_DECODE_DST_BIT_KHR),

            .video_decode_src =
            (bool)(vk_flags & VK_IMAGE_USAGE_VIDEO_DECODE_SRC_BIT_KHR),

            .video_decode_dpb =
            (bool)(vk_flags & VK_IMAGE_USAGE_VIDEO_DECODE_DPB_BIT_KHR),

            .fragment_density_map =
            (bool)(vk_flags & VK_IMAGE_USAGE_FRAGMENT_DENSITY_MAP_BIT_EXT),

            .fragment_shading_rate_attachment =
            (bool)(vk_flags
                & VK_IMAGE_USAGE_FRAGMENT_SHADING_RATE_ATTACHMENT_BIT_KHR),

            .attachment_feedback_loop =
            (bool)(vk_flags & VK_IMAGE_USAGE_ATTACHMENT_FEEDBACK_LOOP_BIT_EXT),

            .invocation_mask_huawei =
            (bool)(vk_flags & VK_IMAGE_USAGE_INVOCATION_MASK_BIT_HUAWEI),

            .sample_weight_qcom =
            (bool)(vk_flags & VK_IMAGE_USAGE_SAMPLE_WEIGHT_BIT_QCOM),

            .sample_block_match_qcom =
            (bool)(vk_flags & VK_IMAGE_USAGE_SAMPLE_BLOCK_MATCH_BIT_QCOM)
        };
    }

    VkImageUsageFlags ImageUsageFlags_to_vk(const ImageUsageFlags& flags)
    {
        VkImageUsageFlags vk_flags = 0;
        if (flags.transfer_src)
        {
            vk_flags |= VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
        }
        if (flags.transfer_dst)
        {
            vk_flags |= VK_IMAGE_USAGE_TRANSFER_DST_BIT;
        }
        if (flags.sampled)
        {
            vk_flags |= VK_IMAGE_USAGE_SAMPLED_BIT;
        }
        if (flags.storage)
        {
            vk_flags |= VK_IMAGE_USAGE_STORAGE_BIT;
        }
        if (flags.color_attachment)
        {
            vk_flags |= VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
        }
        if (flags.depth_stencil_attachment)
        {
            vk_flags |= VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
        }
        if (flags.transient_attachment)
        {
            vk_flags |= VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT;
        }
        if (flags.input_attachment)
        {
            vk_flags |= VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT;
        }
        if (flags.video_decode_dst)
        {
            vk_flags |= VK_IMAGE_USAGE_VIDEO_DECODE_DST_BIT_KHR;
        }
        if (flags.video_decode_src)
        {
            vk_flags |= VK_IMAGE_USAGE_VIDEO_DECODE_SRC_BIT_KHR;
        }
        if (flags.video_decode_dpb)
        {
            vk_flags |= VK_IMAGE_USAGE_VIDEO_DECODE_DPB_BIT_KHR;
        }
        if (flags.fragment_density_map)
        {
            vk_flags |= VK_IMAGE_USAGE_FRAGMENT_DENSITY_MAP_BIT_EXT;
        }
        if (flags.fragment_shading_rate_attachment)
        {
            vk_flags |= VK_IMAGE_USAGE_FRAGMENT_SHADING_RATE_ATTACHMENT_BIT_KHR;
        }
        if (flags.attachment_feedback_loop)
        {
            vk_flags |= VK_IMAGE_USAGE_ATTACHMENT_FEEDBACK_LOOP_BIT_EXT;
        }
        if (flags.invocation_mask_huawei)
        {
            vk_flags |= VK_IMAGE_USAGE_INVOCATION_MASK_BIT_HUAWEI;
        }
        if (flags.sample_weight_qcom)
        {
            vk_flags |= VK_IMAGE_USAGE_SAMPLE_WEIGHT_BIT_QCOM;
        }
        if (flags.sample_block_match_qcom)
        {
            vk_flags |= VK_IMAGE_USAGE_SAMPLE_BLOCK_MATCH_BIT_QCOM;
        }
        return vk_flags;
    }

    SurfaceCapabilities SurfaceCapabilities_from_vk(
        const VkSurfaceCapabilitiesKHR& vk_capabilities
    )
    {
        return SurfaceCapabilities{
            .min_image_count = vk_capabilities.minImageCount,
            .max_image_count = vk_capabilities.maxImageCount,
            .current_extent = Extent2d_from_vk(vk_capabilities.currentExtent),
            .min_image_extent = Extent2d_from_vk(
                vk_capabilities.minImageExtent
            ),
            .max_image_extent = Extent2d_from_vk(
                vk_capabilities.maxImageExtent
            ),
            .max_image_array_layers = vk_capabilities.maxImageArrayLayers,
            .supported_transforms = SurfaceTransformFlags_from_vk(
                vk_capabilities.supportedTransforms
            ),
            .current_transform = SurfaceTransform_from_vk(
                vk_capabilities.currentTransform
            ),
            .supported_composite_alpha = CompositeAlphaFlags_from_vk(
                vk_capabilities.supportedCompositeAlpha
            ),
            .supported_usage_flags = ImageUsageFlags_from_vk(
                vk_capabilities.supportedUsageFlags
            )
        };
    }

    Format Format_from_vk(VkFormat vk_format)
    {
        switch (vk_format)
        {
        case VK_FORMAT_UNDEFINED:
            return Format::Undefined;
        case VK_FORMAT_R4G4_UNORM_PACK8:
            return Format::R4G4_UNORM_PACK8;
        case VK_FORMAT_R4G4B4A4_UNORM_PACK16:
            return Format::R4G4B4A4_UNORM_PACK16;
        case VK_FORMAT_B4G4R4A4_UNORM_PACK16:
            return Format::B4G4R4A4_UNORM_PACK16;
        case VK_FORMAT_R5G6B5_UNORM_PACK16:
            return Format::R5G6B5_UNORM_PACK16;
        case VK_FORMAT_B5G6R5_UNORM_PACK16:
            return Format::B5G6R5_UNORM_PACK16;
        case VK_FORMAT_R5G5B5A1_UNORM_PACK16:
            return Format::R5G5B5A1_UNORM_PACK16;
        case VK_FORMAT_B5G5R5A1_UNORM_PACK16:
            return Format::B5G5R5A1_UNORM_PACK16;
        case VK_FORMAT_A1R5G5B5_UNORM_PACK16:
            return Format::A1R5G5B5_UNORM_PACK16;
        case VK_FORMAT_R8_UNORM:
            return Format::R8_UNORM;
        case VK_FORMAT_R8_SNORM:
            return Format::R8_SNORM;
        case VK_FORMAT_R8_USCALED:
            return Format::R8_USCALED;
        case VK_FORMAT_R8_SSCALED:
            return Format::R8_SSCALED;
        case VK_FORMAT_R8_UINT:
            return Format::R8_UINT;
        case VK_FORMAT_R8_SINT:
            return Format::R8_SINT;
        case VK_FORMAT_R8_SRGB:
            return Format::R8_SRGB;
        case VK_FORMAT_R8G8_UNORM:
            return Format::R8G8_UNORM;
        case VK_FORMAT_R8G8_SNORM:
            return Format::R8G8_SNORM;
        case VK_FORMAT_R8G8_USCALED:
            return Format::R8G8_USCALED;
        case VK_FORMAT_R8G8_SSCALED:
            return Format::R8G8_SSCALED;
        case VK_FORMAT_R8G8_UINT:
            return Format::R8G8_UINT;
        case VK_FORMAT_R8G8_SINT:
            return Format::R8G8_SINT;
        case VK_FORMAT_R8G8_SRGB:
            return Format::R8G8_SRGB;
        case VK_FORMAT_R8G8B8_UNORM:
            return Format::R8G8B8_UNORM;
        case VK_FORMAT_R8G8B8_SNORM:
            return Format::R8G8B8_SNORM;
        case VK_FORMAT_R8G8B8_USCALED:
            return Format::R8G8B8_USCALED;
        case VK_FORMAT_R8G8B8_SSCALED:
            return Format::R8G8B8_SSCALED;
        case VK_FORMAT_R8G8B8_UINT:
            return Format::R8G8B8_UINT;
        case VK_FORMAT_R8G8B8_SINT:
            return Format::R8G8B8_SINT;
        case VK_FORMAT_R8G8B8_SRGB:
            return Format::R8G8B8_SRGB;
        case VK_FORMAT_B8G8R8_UNORM:
            return Format::B8G8R8_UNORM;
        case VK_FORMAT_B8G8R8_SNORM:
            return Format::B8G8R8_SNORM;
        case VK_FORMAT_B8G8R8_USCALED:
            return Format::B8G8R8_USCALED;
        case VK_FORMAT_B8G8R8_SSCALED:
            return Format::B8G8R8_SSCALED;
        case VK_FORMAT_B8G8R8_UINT:
            return Format::B8G8R8_UINT;
        case VK_FORMAT_B8G8R8_SINT:
            return Format::B8G8R8_SINT;
        case VK_FORMAT_B8G8R8_SRGB:
            return Format::B8G8R8_SRGB;
        case VK_FORMAT_R8G8B8A8_UNORM:
            return Format::R8G8B8A8_UNORM;
        case VK_FORMAT_R8G8B8A8_SNORM:
            return Format::R8G8B8A8_SNORM;
        case VK_FORMAT_R8G8B8A8_USCALED:
            return Format::R8G8B8A8_USCALED;
        case VK_FORMAT_R8G8B8A8_SSCALED:
            return Format::R8G8B8A8_SSCALED;
        case VK_FORMAT_R8G8B8A8_UINT:
            return Format::R8G8B8A8_UINT;
        case VK_FORMAT_R8G8B8A8_SINT:
            return Format::R8G8B8A8_SINT;
        case VK_FORMAT_R8G8B8A8_SRGB:
            return Format::R8G8B8A8_SRGB;
        case VK_FORMAT_B8G8R8A8_UNORM:
            return Format::B8G8R8A8_UNORM;
        case VK_FORMAT_B8G8R8A8_SNORM:
            return Format::B8G8R8A8_SNORM;
        case VK_FORMAT_B8G8R8A8_USCALED:
            return Format::B8G8R8A8_USCALED;
        case VK_FORMAT_B8G8R8A8_SSCALED:
            return Format::B8G8R8A8_SSCALED;
        case VK_FORMAT_B8G8R8A8_UINT:
            return Format::B8G8R8A8_UINT;
        case VK_FORMAT_B8G8R8A8_SINT:
            return Format::B8G8R8A8_SINT;
        case VK_FORMAT_B8G8R8A8_SRGB:
            return Format::B8G8R8A8_SRGB;
        case VK_FORMAT_A8B8G8R8_UNORM_PACK32:
            return Format::A8B8G8R8_UNORM_PACK32;
        case VK_FORMAT_A8B8G8R8_SNORM_PACK32:
            return Format::A8B8G8R8_SNORM_PACK32;
        case VK_FORMAT_A8B8G8R8_USCALED_PACK32:
            return Format::A8B8G8R8_USCALED_PACK32;
        case VK_FORMAT_A8B8G8R8_SSCALED_PACK32:
            return Format::A8B8G8R8_SSCALED_PACK32;
        case VK_FORMAT_A8B8G8R8_UINT_PACK32:
            return Format::A8B8G8R8_UINT_PACK32;
        case VK_FORMAT_A8B8G8R8_SINT_PACK32:
            return Format::A8B8G8R8_SINT_PACK32;
        case VK_FORMAT_A8B8G8R8_SRGB_PACK32:
            return Format::A8B8G8R8_SRGB_PACK32;
        case VK_FORMAT_A2R10G10B10_UNORM_PACK32:
            return Format::A2R10G10B10_UNORM_PACK32;
        case VK_FORMAT_A2R10G10B10_SNORM_PACK32:
            return Format::A2R10G10B10_SNORM_PACK32;
        case VK_FORMAT_A2R10G10B10_USCALED_PACK32:
            return Format::A2R10G10B10_USCALED_PACK32;
        case VK_FORMAT_A2R10G10B10_SSCALED_PACK32:
            return Format::A2R10G10B10_SSCALED_PACK32;
        case VK_FORMAT_A2R10G10B10_UINT_PACK32:
            return Format::A2R10G10B10_UINT_PACK32;
        case VK_FORMAT_A2R10G10B10_SINT_PACK32:
            return Format::A2R10G10B10_SINT_PACK32;
        case VK_FORMAT_A2B10G10R10_UNORM_PACK32:
            return Format::A2B10G10R10_UNORM_PACK32;
        case VK_FORMAT_A2B10G10R10_SNORM_PACK32:
            return Format::A2B10G10R10_SNORM_PACK32;
        case VK_FORMAT_A2B10G10R10_USCALED_PACK32:
            return Format::A2B10G10R10_USCALED_PACK32;
        case VK_FORMAT_A2B10G10R10_SSCALED_PACK32:
            return Format::A2B10G10R10_SSCALED_PACK32;
        case VK_FORMAT_A2B10G10R10_UINT_PACK32:
            return Format::A2B10G10R10_UINT_PACK32;
        case VK_FORMAT_A2B10G10R10_SINT_PACK32:
            return Format::A2B10G10R10_SINT_PACK32;
        case VK_FORMAT_R16_UNORM:
            return Format::R16_UNORM;
        case VK_FORMAT_R16_SNORM:
            return Format::R16_SNORM;
        case VK_FORMAT_R16_USCALED:
            return Format::R16_USCALED;
        case VK_FORMAT_R16_SSCALED:
            return Format::R16_SSCALED;
        case VK_FORMAT_R16_UINT:
            return Format::R16_UINT;
        case VK_FORMAT_R16_SINT:
            return Format::R16_SINT;
        case VK_FORMAT_R16_SFLOAT:
            return Format::R16_SFLOAT;
        case VK_FORMAT_R16G16_UNORM:
            return Format::R16G16_UNORM;
        case VK_FORMAT_R16G16_SNORM:
            return Format::R16G16_SNORM;
        case VK_FORMAT_R16G16_USCALED:
            return Format::R16G16_USCALED;
        case VK_FORMAT_R16G16_SSCALED:
            return Format::R16G16_SSCALED;
        case VK_FORMAT_R16G16_UINT:
            return Format::R16G16_UINT;
        case VK_FORMAT_R16G16_SINT:
            return Format::R16G16_SINT;
        case VK_FORMAT_R16G16_SFLOAT:
            return Format::R16G16_SFLOAT;
        case VK_FORMAT_R16G16B16_UNORM:
            return Format::R16G16B16_UNORM;
        case VK_FORMAT_R16G16B16_SNORM:
            return Format::R16G16B16_SNORM;
        case VK_FORMAT_R16G16B16_USCALED:
            return Format::R16G16B16_USCALED;
        case VK_FORMAT_R16G16B16_SSCALED:
            return Format::R16G16B16_SSCALED;
        case VK_FORMAT_R16G16B16_UINT:
            return Format::R16G16B16_UINT;
        case VK_FORMAT_R16G16B16_SINT:
            return Format::R16G16B16_SINT;
        case VK_FORMAT_R16G16B16_SFLOAT:
            return Format::R16G16B16_SFLOAT;
        case VK_FORMAT_R16G16B16A16_UNORM:
            return Format::R16G16B16A16_UNORM;
        case VK_FORMAT_R16G16B16A16_SNORM:
            return Format::R16G16B16A16_SNORM;
        case VK_FORMAT_R16G16B16A16_USCALED:
            return Format::R16G16B16A16_USCALED;
        case VK_FORMAT_R16G16B16A16_SSCALED:
            return Format::R16G16B16A16_SSCALED;
        case VK_FORMAT_R16G16B16A16_UINT:
            return Format::R16G16B16A16_UINT;
        case VK_FORMAT_R16G16B16A16_SINT:
            return Format::R16G16B16A16_SINT;
        case VK_FORMAT_R16G16B16A16_SFLOAT:
            return Format::R16G16B16A16_SFLOAT;
        case VK_FORMAT_R32_UINT:
            return Format::R32_UINT;
        case VK_FORMAT_R32_SINT:
            return Format::R32_SINT;
        case VK_FORMAT_R32_SFLOAT:
            return Format::R32_SFLOAT;
        case VK_FORMAT_R32G32_UINT:
            return Format::R32G32_UINT;
        case VK_FORMAT_R32G32_SINT:
            return Format::R32G32_SINT;
        case VK_FORMAT_R32G32_SFLOAT:
            return Format::R32G32_SFLOAT;
        case VK_FORMAT_R32G32B32_UINT:
            return Format::R32G32B32_UINT;
        case VK_FORMAT_R32G32B32_SINT:
            return Format::R32G32B32_SINT;
        case VK_FORMAT_R32G32B32_SFLOAT:
            return Format::R32G32B32_SFLOAT;
        case VK_FORMAT_R32G32B32A32_UINT:
            return Format::R32G32B32A32_UINT;
        case VK_FORMAT_R32G32B32A32_SINT:
            return Format::R32G32B32A32_SINT;
        case VK_FORMAT_R32G32B32A32_SFLOAT:
            return Format::R32G32B32A32_SFLOAT;
        case VK_FORMAT_R64_UINT:
            return Format::R64_UINT;
        case VK_FORMAT_R64_SINT:
            return Format::R64_SINT;
        case VK_FORMAT_R64_SFLOAT:
            return Format::R64_SFLOAT;
        case VK_FORMAT_R64G64_UINT:
            return Format::R64G64_UINT;
        case VK_FORMAT_R64G64_SINT:
            return Format::R64G64_SINT;
        case VK_FORMAT_R64G64_SFLOAT:
            return Format::R64G64_SFLOAT;
        case VK_FORMAT_R64G64B64_UINT:
            return Format::R64G64B64_UINT;
        case VK_FORMAT_R64G64B64_SINT:
            return Format::R64G64B64_SINT;
        case VK_FORMAT_R64G64B64_SFLOAT:
            return Format::R64G64B64_SFLOAT;
        case VK_FORMAT_R64G64B64A64_UINT:
            return Format::R64G64B64A64_UINT;
        case VK_FORMAT_R64G64B64A64_SINT:
            return Format::R64G64B64A64_SINT;
        case VK_FORMAT_R64G64B64A64_SFLOAT:
            return Format::R64G64B64A64_SFLOAT;
        case VK_FORMAT_B10G11R11_UFLOAT_PACK32:
            return Format::B10G11R11_UFLOAT_PACK32;
        case VK_FORMAT_E5B9G9R9_UFLOAT_PACK32:
            return Format::E5B9G9R9_UFLOAT_PACK32;
        case VK_FORMAT_D16_UNORM:
            return Format::D16_UNORM;
        case VK_FORMAT_X8_D24_UNORM_PACK32:
            return Format::X8_D24_UNORM_PACK32;
        case VK_FORMAT_D32_SFLOAT:
            return Format::D32_SFLOAT;
        case VK_FORMAT_S8_UINT:
            return Format::S8_UINT;
        case VK_FORMAT_D16_UNORM_S8_UINT:
            return Format::D16_UNORM_S8_UINT;
        case VK_FORMAT_D24_UNORM_S8_UINT:
            return Format::D24_UNORM_S8_UINT;
        case VK_FORMAT_D32_SFLOAT_S8_UINT:
            return Format::D32_SFLOAT_S8_UINT;
        case VK_FORMAT_BC1_RGB_UNORM_BLOCK:
            return Format::BC1_RGB_UNORM_BLOCK;
        case VK_FORMAT_BC1_RGB_SRGB_BLOCK:
            return Format::BC1_RGB_SRGB_BLOCK;
        case VK_FORMAT_BC1_RGBA_UNORM_BLOCK:
            return Format::BC1_RGBA_UNORM_BLOCK;
        case VK_FORMAT_BC1_RGBA_SRGB_BLOCK:
            return Format::BC1_RGBA_SRGB_BLOCK;
        case VK_FORMAT_BC2_UNORM_BLOCK:
            return Format::BC2_UNORM_BLOCK;
        case VK_FORMAT_BC2_SRGB_BLOCK:
            return Format::BC2_SRGB_BLOCK;
        case VK_FORMAT_BC3_UNORM_BLOCK:
            return Format::BC3_UNORM_BLOCK;
        case VK_FORMAT_BC3_SRGB_BLOCK:
            return Format::BC3_SRGB_BLOCK;
        case VK_FORMAT_BC4_UNORM_BLOCK:
            return Format::BC4_UNORM_BLOCK;
        case VK_FORMAT_BC4_SNORM_BLOCK:
            return Format::BC4_SNORM_BLOCK;
        case VK_FORMAT_BC5_UNORM_BLOCK:
            return Format::BC5_UNORM_BLOCK;
        case VK_FORMAT_BC5_SNORM_BLOCK:
            return Format::BC5_SNORM_BLOCK;
        case VK_FORMAT_BC6H_UFLOAT_BLOCK:
            return Format::BC6H_UFLOAT_BLOCK;
        case VK_FORMAT_BC6H_SFLOAT_BLOCK:
            return Format::BC6H_SFLOAT_BLOCK;
        case VK_FORMAT_BC7_UNORM_BLOCK:
            return Format::BC7_UNORM_BLOCK;
        case VK_FORMAT_BC7_SRGB_BLOCK:
            return Format::BC7_SRGB_BLOCK;
        case VK_FORMAT_ETC2_R8G8B8_UNORM_BLOCK:
            return Format::ETC2_R8G8B8_UNORM_BLOCK;
        case VK_FORMAT_ETC2_R8G8B8_SRGB_BLOCK:
            return Format::ETC2_R8G8B8_SRGB_BLOCK;
        case VK_FORMAT_ETC2_R8G8B8A1_UNORM_BLOCK:
            return Format::ETC2_R8G8B8A1_UNORM_BLOCK;
        case VK_FORMAT_ETC2_R8G8B8A1_SRGB_BLOCK:
            return Format::ETC2_R8G8B8A1_SRGB_BLOCK;
        case VK_FORMAT_ETC2_R8G8B8A8_UNORM_BLOCK:
            return Format::ETC2_R8G8B8A8_UNORM_BLOCK;
        case VK_FORMAT_ETC2_R8G8B8A8_SRGB_BLOCK:
            return Format::ETC2_R8G8B8A8_SRGB_BLOCK;
        case VK_FORMAT_EAC_R11_UNORM_BLOCK:
            return Format::EAC_R11_UNORM_BLOCK;
        case VK_FORMAT_EAC_R11_SNORM_BLOCK:
            return Format::EAC_R11_SNORM_BLOCK;
        case VK_FORMAT_EAC_R11G11_UNORM_BLOCK:
            return Format::EAC_R11G11_UNORM_BLOCK;
        case VK_FORMAT_EAC_R11G11_SNORM_BLOCK:
            return Format::EAC_R11G11_SNORM_BLOCK;
        case VK_FORMAT_ASTC_4x4_UNORM_BLOCK:
            return Format::ASTC_4x4_UNORM_BLOCK;
        case VK_FORMAT_ASTC_4x4_SRGB_BLOCK:
            return Format::ASTC_4x4_SRGB_BLOCK;
        case VK_FORMAT_ASTC_5x4_UNORM_BLOCK:
            return Format::ASTC_5x4_UNORM_BLOCK;
        case VK_FORMAT_ASTC_5x4_SRGB_BLOCK:
            return Format::ASTC_5x4_SRGB_BLOCK;
        case VK_FORMAT_ASTC_5x5_UNORM_BLOCK:
            return Format::ASTC_5x5_UNORM_BLOCK;
        case VK_FORMAT_ASTC_5x5_SRGB_BLOCK:
            return Format::ASTC_5x5_SRGB_BLOCK;
        case VK_FORMAT_ASTC_6x5_UNORM_BLOCK:
            return Format::ASTC_6x5_UNORM_BLOCK;
        case VK_FORMAT_ASTC_6x5_SRGB_BLOCK:
            return Format::ASTC_6x5_SRGB_BLOCK;
        case VK_FORMAT_ASTC_6x6_UNORM_BLOCK:
            return Format::ASTC_6x6_UNORM_BLOCK;
        case VK_FORMAT_ASTC_6x6_SRGB_BLOCK:
            return Format::ASTC_6x6_SRGB_BLOCK;
        case VK_FORMAT_ASTC_8x5_UNORM_BLOCK:
            return Format::ASTC_8x5_UNORM_BLOCK;
        case VK_FORMAT_ASTC_8x5_SRGB_BLOCK:
            return Format::ASTC_8x5_SRGB_BLOCK;
        case VK_FORMAT_ASTC_8x6_UNORM_BLOCK:
            return Format::ASTC_8x6_UNORM_BLOCK;
        case VK_FORMAT_ASTC_8x6_SRGB_BLOCK:
            return Format::ASTC_8x6_SRGB_BLOCK;
        case VK_FORMAT_ASTC_8x8_UNORM_BLOCK:
            return Format::ASTC_8x8_UNORM_BLOCK;
        case VK_FORMAT_ASTC_8x8_SRGB_BLOCK:
            return Format::ASTC_8x8_SRGB_BLOCK;
        case VK_FORMAT_ASTC_10x5_UNORM_BLOCK:
            return Format::ASTC_10x5_UNORM_BLOCK;
        case VK_FORMAT_ASTC_10x5_SRGB_BLOCK:
            return Format::ASTC_10x5_SRGB_BLOCK;
        case VK_FORMAT_ASTC_10x6_UNORM_BLOCK:
            return Format::ASTC_10x6_UNORM_BLOCK;
        case VK_FORMAT_ASTC_10x6_SRGB_BLOCK:
            return Format::ASTC_10x6_SRGB_BLOCK;
        case VK_FORMAT_ASTC_10x8_UNORM_BLOCK:
            return Format::ASTC_10x8_UNORM_BLOCK;
        case VK_FORMAT_ASTC_10x8_SRGB_BLOCK:
            return Format::ASTC_10x8_SRGB_BLOCK;
        case VK_FORMAT_ASTC_10x10_UNORM_BLOCK:
            return Format::ASTC_10x10_UNORM_BLOCK;
        case VK_FORMAT_ASTC_10x10_SRGB_BLOCK:
            return Format::ASTC_10x10_SRGB_BLOCK;
        case VK_FORMAT_ASTC_12x10_UNORM_BLOCK:
            return Format::ASTC_12x10_UNORM_BLOCK;
        case VK_FORMAT_ASTC_12x10_SRGB_BLOCK:
            return Format::ASTC_12x10_SRGB_BLOCK;
        case VK_FORMAT_ASTC_12x12_UNORM_BLOCK:
            return Format::ASTC_12x12_UNORM_BLOCK;
        case VK_FORMAT_ASTC_12x12_SRGB_BLOCK:
            return Format::ASTC_12x12_SRGB_BLOCK;
        case VK_FORMAT_G8B8G8R8_422_UNORM:
            return Format::G8B8G8R8_422_UNORM;
        case VK_FORMAT_B8G8R8G8_422_UNORM:
            return Format::B8G8R8G8_422_UNORM;
        case VK_FORMAT_G8_B8_R8_3PLANE_420_UNORM:
            return Format::G8_B8_R8_3PLANE_420_UNORM;
        case VK_FORMAT_G8_B8R8_2PLANE_420_UNORM:
            return Format::G8_B8R8_2PLANE_420_UNORM;
        case VK_FORMAT_G8_B8_R8_3PLANE_422_UNORM:
            return Format::G8_B8_R8_3PLANE_422_UNORM;
        case VK_FORMAT_G8_B8R8_2PLANE_422_UNORM:
            return Format::G8_B8R8_2PLANE_422_UNORM;
        case VK_FORMAT_G8_B8_R8_3PLANE_444_UNORM:
            return Format::G8_B8_R8_3PLANE_444_UNORM;
        case VK_FORMAT_R10X6_UNORM_PACK16:
            return Format::R10X6_UNORM_PACK16;
        case VK_FORMAT_R10X6G10X6_UNORM_2PACK16:
            return Format::R10X6G10X6_UNORM_2PACK16;
        case VK_FORMAT_R10X6G10X6B10X6A10X6_UNORM_4PACK16:
            return Format::R10X6G10X6B10X6A10X6_UNORM_4PACK16;
        case VK_FORMAT_G10X6B10X6G10X6R10X6_422_UNORM_4PACK16:
            return Format::G10X6B10X6G10X6R10X6_422_UNORM_4PACK16;
        case VK_FORMAT_B10X6G10X6R10X6G10X6_422_UNORM_4PACK16:
            return Format::B10X6G10X6R10X6G10X6_422_UNORM_4PACK16;
        case VK_FORMAT_G10X6_B10X6_R10X6_3PLANE_420_UNORM_3PACK16:
            return Format::G10X6_B10X6_R10X6_3PLANE_420_UNORM_3PACK16;
        case VK_FORMAT_G10X6_B10X6R10X6_2PLANE_420_UNORM_3PACK16:
            return Format::G10X6_B10X6R10X6_2PLANE_420_UNORM_3PACK16;
        case VK_FORMAT_G10X6_B10X6_R10X6_3PLANE_422_UNORM_3PACK16:
            return Format::G10X6_B10X6_R10X6_3PLANE_422_UNORM_3PACK16;
        case VK_FORMAT_G10X6_B10X6R10X6_2PLANE_422_UNORM_3PACK16:
            return Format::G10X6_B10X6R10X6_2PLANE_422_UNORM_3PACK16;
        case VK_FORMAT_G10X6_B10X6_R10X6_3PLANE_444_UNORM_3PACK16:
            return Format::G10X6_B10X6_R10X6_3PLANE_444_UNORM_3PACK16;
        case VK_FORMAT_R12X4_UNORM_PACK16:
            return Format::R12X4_UNORM_PACK16;
        case VK_FORMAT_R12X4G12X4_UNORM_2PACK16:
            return Format::R12X4G12X4_UNORM_2PACK16;
        case VK_FORMAT_R12X4G12X4B12X4A12X4_UNORM_4PACK16:
            return Format::R12X4G12X4B12X4A12X4_UNORM_4PACK16;
        case VK_FORMAT_G12X4B12X4G12X4R12X4_422_UNORM_4PACK16:
            return Format::G12X4B12X4G12X4R12X4_422_UNORM_4PACK16;
        case VK_FORMAT_B12X4G12X4R12X4G12X4_422_UNORM_4PACK16:
            return Format::B12X4G12X4R12X4G12X4_422_UNORM_4PACK16;
        case VK_FORMAT_G12X4_B12X4_R12X4_3PLANE_420_UNORM_3PACK16:
            return Format::G12X4_B12X4_R12X4_3PLANE_420_UNORM_3PACK16;
        case VK_FORMAT_G12X4_B12X4R12X4_2PLANE_420_UNORM_3PACK16:
            return Format::G12X4_B12X4R12X4_2PLANE_420_UNORM_3PACK16;
        case VK_FORMAT_G12X4_B12X4_R12X4_3PLANE_422_UNORM_3PACK16:
            return Format::G12X4_B12X4_R12X4_3PLANE_422_UNORM_3PACK16;
        case VK_FORMAT_G12X4_B12X4R12X4_2PLANE_422_UNORM_3PACK16:
            return Format::G12X4_B12X4R12X4_2PLANE_422_UNORM_3PACK16;
        case VK_FORMAT_G12X4_B12X4_R12X4_3PLANE_444_UNORM_3PACK16:
            return Format::G12X4_B12X4_R12X4_3PLANE_444_UNORM_3PACK16;
        case VK_FORMAT_G16B16G16R16_422_UNORM:
            return Format::G16B16G16R16_422_UNORM;
        case VK_FORMAT_B16G16R16G16_422_UNORM:
            return Format::B16G16R16G16_422_UNORM;
        case VK_FORMAT_G16_B16_R16_3PLANE_420_UNORM:
            return Format::G16_B16_R16_3PLANE_420_UNORM;
        case VK_FORMAT_G16_B16R16_2PLANE_420_UNORM:
            return Format::G16_B16R16_2PLANE_420_UNORM;
        case VK_FORMAT_G16_B16_R16_3PLANE_422_UNORM:
            return Format::G16_B16_R16_3PLANE_422_UNORM;
        case VK_FORMAT_G16_B16R16_2PLANE_422_UNORM:
            return Format::G16_B16R16_2PLANE_422_UNORM;
        case VK_FORMAT_G16_B16_R16_3PLANE_444_UNORM:
            return Format::G16_B16_R16_3PLANE_444_UNORM;
        case VK_FORMAT_G8_B8R8_2PLANE_444_UNORM:
            return Format::G8_B8R8_2PLANE_444_UNORM;
        case VK_FORMAT_G10X6_B10X6R10X6_2PLANE_444_UNORM_3PACK16:
            return Format::G10X6_B10X6R10X6_2PLANE_444_UNORM_3PACK16;
        case VK_FORMAT_G12X4_B12X4R12X4_2PLANE_444_UNORM_3PACK16:
            return Format::G12X4_B12X4R12X4_2PLANE_444_UNORM_3PACK16;
        case VK_FORMAT_G16_B16R16_2PLANE_444_UNORM:
            return Format::G16_B16R16_2PLANE_444_UNORM;
        case VK_FORMAT_A4R4G4B4_UNORM_PACK16:
            return Format::A4R4G4B4_UNORM_PACK16;
        case VK_FORMAT_A4B4G4R4_UNORM_PACK16:
            return Format::A4B4G4R4_UNORM_PACK16;
        case VK_FORMAT_ASTC_4x4_SFLOAT_BLOCK:
            return Format::ASTC_4x4_SFLOAT_BLOCK;
        case VK_FORMAT_ASTC_5x4_SFLOAT_BLOCK:
            return Format::ASTC_5x4_SFLOAT_BLOCK;
        case VK_FORMAT_ASTC_5x5_SFLOAT_BLOCK:
            return Format::ASTC_5x5_SFLOAT_BLOCK;
        case VK_FORMAT_ASTC_6x5_SFLOAT_BLOCK:
            return Format::ASTC_6x5_SFLOAT_BLOCK;
        case VK_FORMAT_ASTC_6x6_SFLOAT_BLOCK:
            return Format::ASTC_6x6_SFLOAT_BLOCK;
        case VK_FORMAT_ASTC_8x5_SFLOAT_BLOCK:
            return Format::ASTC_8x5_SFLOAT_BLOCK;
        case VK_FORMAT_ASTC_8x6_SFLOAT_BLOCK:
            return Format::ASTC_8x6_SFLOAT_BLOCK;
        case VK_FORMAT_ASTC_8x8_SFLOAT_BLOCK:
            return Format::ASTC_8x8_SFLOAT_BLOCK;
        case VK_FORMAT_ASTC_10x5_SFLOAT_BLOCK:
            return Format::ASTC_10x5_SFLOAT_BLOCK;
        case VK_FORMAT_ASTC_10x6_SFLOAT_BLOCK:
            return Format::ASTC_10x6_SFLOAT_BLOCK;
        case VK_FORMAT_ASTC_10x8_SFLOAT_BLOCK:
            return Format::ASTC_10x8_SFLOAT_BLOCK;
        case VK_FORMAT_ASTC_10x10_SFLOAT_BLOCK:
            return Format::ASTC_10x10_SFLOAT_BLOCK;
        case VK_FORMAT_ASTC_12x10_SFLOAT_BLOCK:
            return Format::ASTC_12x10_SFLOAT_BLOCK;
        case VK_FORMAT_ASTC_12x12_SFLOAT_BLOCK:
            return Format::ASTC_12x12_SFLOAT_BLOCK;
        case VK_FORMAT_PVRTC1_2BPP_UNORM_BLOCK_IMG:
            return Format::PVRTC1_2BPP_UNORM_BLOCK_IMG;
        case VK_FORMAT_PVRTC1_4BPP_UNORM_BLOCK_IMG:
            return Format::PVRTC1_4BPP_UNORM_BLOCK_IMG;
        case VK_FORMAT_PVRTC2_2BPP_UNORM_BLOCK_IMG:
            return Format::PVRTC2_2BPP_UNORM_BLOCK_IMG;
        case VK_FORMAT_PVRTC2_4BPP_UNORM_BLOCK_IMG:
            return Format::PVRTC2_4BPP_UNORM_BLOCK_IMG;
        case VK_FORMAT_PVRTC1_2BPP_SRGB_BLOCK_IMG:
            return Format::PVRTC1_2BPP_SRGB_BLOCK_IMG;
        case VK_FORMAT_PVRTC1_4BPP_SRGB_BLOCK_IMG:
            return Format::PVRTC1_4BPP_SRGB_BLOCK_IMG;
        case VK_FORMAT_PVRTC2_2BPP_SRGB_BLOCK_IMG:
            return Format::PVRTC2_2BPP_SRGB_BLOCK_IMG;
        case VK_FORMAT_PVRTC2_4BPP_SRGB_BLOCK_IMG:
            return Format::PVRTC2_4BPP_SRGB_BLOCK_IMG;
        case VK_FORMAT_R16G16_S10_5_NV:
            return Format::R16G16_S10_5_NV;
        default:
            return Format::Undefined;
        }
    }

    VkFormat Format_to_vk(Format format)
    {
        switch (format)
        {
        case Format::Undefined:
            return VK_FORMAT_UNDEFINED;
        case Format::R4G4_UNORM_PACK8:
            return VK_FORMAT_R4G4_UNORM_PACK8;
        case Format::R4G4B4A4_UNORM_PACK16:
            return VK_FORMAT_R4G4B4A4_UNORM_PACK16;
        case Format::B4G4R4A4_UNORM_PACK16:
            return VK_FORMAT_B4G4R4A4_UNORM_PACK16;
        case Format::R5G6B5_UNORM_PACK16:
            return VK_FORMAT_R5G6B5_UNORM_PACK16;
        case Format::B5G6R5_UNORM_PACK16:
            return VK_FORMAT_B5G6R5_UNORM_PACK16;
        case Format::R5G5B5A1_UNORM_PACK16:
            return VK_FORMAT_R5G5B5A1_UNORM_PACK16;
        case Format::B5G5R5A1_UNORM_PACK16:
            return VK_FORMAT_B5G5R5A1_UNORM_PACK16;
        case Format::A1R5G5B5_UNORM_PACK16:
            return VK_FORMAT_A1R5G5B5_UNORM_PACK16;
        case Format::R8_UNORM:
            return VK_FORMAT_R8_UNORM;
        case Format::R8_SNORM:
            return VK_FORMAT_R8_SNORM;
        case Format::R8_USCALED:
            return VK_FORMAT_R8_USCALED;
        case Format::R8_SSCALED:
            return VK_FORMAT_R8_SSCALED;
        case Format::R8_UINT:
            return VK_FORMAT_R8_UINT;
        case Format::R8_SINT:
            return VK_FORMAT_R8_SINT;
        case Format::R8_SRGB:
            return VK_FORMAT_R8_SRGB;
        case Format::R8G8_UNORM:
            return VK_FORMAT_R8G8_UNORM;
        case Format::R8G8_SNORM:
            return VK_FORMAT_R8G8_SNORM;
        case Format::R8G8_USCALED:
            return VK_FORMAT_R8G8_USCALED;
        case Format::R8G8_SSCALED:
            return VK_FORMAT_R8G8_SSCALED;
        case Format::R8G8_UINT:
            return VK_FORMAT_R8G8_UINT;
        case Format::R8G8_SINT:
            return VK_FORMAT_R8G8_SINT;
        case Format::R8G8_SRGB:
            return VK_FORMAT_R8G8_SRGB;
        case Format::R8G8B8_UNORM:
            return VK_FORMAT_R8G8B8_UNORM;
        case Format::R8G8B8_SNORM:
            return VK_FORMAT_R8G8B8_SNORM;
        case Format::R8G8B8_USCALED:
            return VK_FORMAT_R8G8B8_USCALED;
        case Format::R8G8B8_SSCALED:
            return VK_FORMAT_R8G8B8_SSCALED;
        case Format::R8G8B8_UINT:
            return VK_FORMAT_R8G8B8_UINT;
        case Format::R8G8B8_SINT:
            return VK_FORMAT_R8G8B8_SINT;
        case Format::R8G8B8_SRGB:
            return VK_FORMAT_R8G8B8_SRGB;
        case Format::B8G8R8_UNORM:
            return VK_FORMAT_B8G8R8_UNORM;
        case Format::B8G8R8_SNORM:
            return VK_FORMAT_B8G8R8_SNORM;
        case Format::B8G8R8_USCALED:
            return VK_FORMAT_B8G8R8_USCALED;
        case Format::B8G8R8_SSCALED:
            return VK_FORMAT_B8G8R8_SSCALED;
        case Format::B8G8R8_UINT:
            return VK_FORMAT_B8G8R8_UINT;
        case Format::B8G8R8_SINT:
            return VK_FORMAT_B8G8R8_SINT;
        case Format::B8G8R8_SRGB:
            return VK_FORMAT_B8G8R8_SRGB;
        case Format::R8G8B8A8_UNORM:
            return VK_FORMAT_R8G8B8A8_UNORM;
        case Format::R8G8B8A8_SNORM:
            return VK_FORMAT_R8G8B8A8_SNORM;
        case Format::R8G8B8A8_USCALED:
            return VK_FORMAT_R8G8B8A8_USCALED;
        case Format::R8G8B8A8_SSCALED:
            return VK_FORMAT_R8G8B8A8_SSCALED;
        case Format::R8G8B8A8_UINT:
            return VK_FORMAT_R8G8B8A8_UINT;
        case Format::R8G8B8A8_SINT:
            return VK_FORMAT_R8G8B8A8_SINT;
        case Format::R8G8B8A8_SRGB:
            return VK_FORMAT_R8G8B8A8_SRGB;
        case Format::B8G8R8A8_UNORM:
            return VK_FORMAT_B8G8R8A8_UNORM;
        case Format::B8G8R8A8_SNORM:
            return VK_FORMAT_B8G8R8A8_SNORM;
        case Format::B8G8R8A8_USCALED:
            return VK_FORMAT_B8G8R8A8_USCALED;
        case Format::B8G8R8A8_SSCALED:
            return VK_FORMAT_B8G8R8A8_SSCALED;
        case Format::B8G8R8A8_UINT:
            return VK_FORMAT_B8G8R8A8_UINT;
        case Format::B8G8R8A8_SINT:
            return VK_FORMAT_B8G8R8A8_SINT;
        case Format::B8G8R8A8_SRGB:
            return VK_FORMAT_B8G8R8A8_SRGB;
        case Format::A8B8G8R8_UNORM_PACK32:
            return VK_FORMAT_A8B8G8R8_UNORM_PACK32;
        case Format::A8B8G8R8_SNORM_PACK32:
            return VK_FORMAT_A8B8G8R8_SNORM_PACK32;
        case Format::A8B8G8R8_USCALED_PACK32:
            return VK_FORMAT_A8B8G8R8_USCALED_PACK32;
        case Format::A8B8G8R8_SSCALED_PACK32:
            return VK_FORMAT_A8B8G8R8_SSCALED_PACK32;
        case Format::A8B8G8R8_UINT_PACK32:
            return VK_FORMAT_A8B8G8R8_UINT_PACK32;
        case Format::A8B8G8R8_SINT_PACK32:
            return VK_FORMAT_A8B8G8R8_SINT_PACK32;
        case Format::A8B8G8R8_SRGB_PACK32:
            return VK_FORMAT_A8B8G8R8_SRGB_PACK32;
        case Format::A2R10G10B10_UNORM_PACK32:
            return VK_FORMAT_A2R10G10B10_UNORM_PACK32;
        case Format::A2R10G10B10_SNORM_PACK32:
            return VK_FORMAT_A2R10G10B10_SNORM_PACK32;
        case Format::A2R10G10B10_USCALED_PACK32:
            return VK_FORMAT_A2R10G10B10_USCALED_PACK32;
        case Format::A2R10G10B10_SSCALED_PACK32:
            return VK_FORMAT_A2R10G10B10_SSCALED_PACK32;
        case Format::A2R10G10B10_UINT_PACK32:
            return VK_FORMAT_A2R10G10B10_UINT_PACK32;
        case Format::A2R10G10B10_SINT_PACK32:
            return VK_FORMAT_A2R10G10B10_SINT_PACK32;
        case Format::A2B10G10R10_UNORM_PACK32:
            return VK_FORMAT_A2B10G10R10_UNORM_PACK32;
        case Format::A2B10G10R10_SNORM_PACK32:
            return VK_FORMAT_A2B10G10R10_SNORM_PACK32;
        case Format::A2B10G10R10_USCALED_PACK32:
            return VK_FORMAT_A2B10G10R10_USCALED_PACK32;
        case Format::A2B10G10R10_SSCALED_PACK32:
            return VK_FORMAT_A2B10G10R10_SSCALED_PACK32;
        case Format::A2B10G10R10_UINT_PACK32:
            return VK_FORMAT_A2B10G10R10_UINT_PACK32;
        case Format::A2B10G10R10_SINT_PACK32:
            return VK_FORMAT_A2B10G10R10_SINT_PACK32;
        case Format::R16_UNORM:
            return VK_FORMAT_R16_UNORM;
        case Format::R16_SNORM:
            return VK_FORMAT_R16_SNORM;
        case Format::R16_USCALED:
            return VK_FORMAT_R16_USCALED;
        case Format::R16_SSCALED:
            return VK_FORMAT_R16_SSCALED;
        case Format::R16_UINT:
            return VK_FORMAT_R16_UINT;
        case Format::R16_SINT:
            return VK_FORMAT_R16_SINT;
        case Format::R16_SFLOAT:
            return VK_FORMAT_R16_SFLOAT;
        case Format::R16G16_UNORM:
            return VK_FORMAT_R16G16_UNORM;
        case Format::R16G16_SNORM:
            return VK_FORMAT_R16G16_SNORM;
        case Format::R16G16_USCALED:
            return VK_FORMAT_R16G16_USCALED;
        case Format::R16G16_SSCALED:
            return VK_FORMAT_R16G16_SSCALED;
        case Format::R16G16_UINT:
            return VK_FORMAT_R16G16_UINT;
        case Format::R16G16_SINT:
            return VK_FORMAT_R16G16_SINT;
        case Format::R16G16_SFLOAT:
            return VK_FORMAT_R16G16_SFLOAT;
        case Format::R16G16B16_UNORM:
            return VK_FORMAT_R16G16B16_UNORM;
        case Format::R16G16B16_SNORM:
            return VK_FORMAT_R16G16B16_SNORM;
        case Format::R16G16B16_USCALED:
            return VK_FORMAT_R16G16B16_USCALED;
        case Format::R16G16B16_SSCALED:
            return VK_FORMAT_R16G16B16_SSCALED;
        case Format::R16G16B16_UINT:
            return VK_FORMAT_R16G16B16_UINT;
        case Format::R16G16B16_SINT:
            return VK_FORMAT_R16G16B16_SINT;
        case Format::R16G16B16_SFLOAT:
            return VK_FORMAT_R16G16B16_SFLOAT;
        case Format::R16G16B16A16_UNORM:
            return VK_FORMAT_R16G16B16A16_UNORM;
        case Format::R16G16B16A16_SNORM:
            return VK_FORMAT_R16G16B16A16_SNORM;
        case Format::R16G16B16A16_USCALED:
            return VK_FORMAT_R16G16B16A16_USCALED;
        case Format::R16G16B16A16_SSCALED:
            return VK_FORMAT_R16G16B16A16_SSCALED;
        case Format::R16G16B16A16_UINT:
            return VK_FORMAT_R16G16B16A16_UINT;
        case Format::R16G16B16A16_SINT:
            return VK_FORMAT_R16G16B16A16_SINT;
        case Format::R16G16B16A16_SFLOAT:
            return VK_FORMAT_R16G16B16A16_SFLOAT;
        case Format::R32_UINT:
            return VK_FORMAT_R32_UINT;
        case Format::R32_SINT:
            return VK_FORMAT_R32_SINT;
        case Format::R32_SFLOAT:
            return VK_FORMAT_R32_SFLOAT;
        case Format::R32G32_UINT:
            return VK_FORMAT_R32G32_UINT;
        case Format::R32G32_SINT:
            return VK_FORMAT_R32G32_SINT;
        case Format::R32G32_SFLOAT:
            return VK_FORMAT_R32G32_SFLOAT;
        case Format::R32G32B32_UINT:
            return VK_FORMAT_R32G32B32_UINT;
        case Format::R32G32B32_SINT:
            return VK_FORMAT_R32G32B32_SINT;
        case Format::R32G32B32_SFLOAT:
            return VK_FORMAT_R32G32B32_SFLOAT;
        case Format::R32G32B32A32_UINT:
            return VK_FORMAT_R32G32B32A32_UINT;
        case Format::R32G32B32A32_SINT:
            return VK_FORMAT_R32G32B32A32_SINT;
        case Format::R32G32B32A32_SFLOAT:
            return VK_FORMAT_R32G32B32A32_SFLOAT;
        case Format::R64_UINT:
            return VK_FORMAT_R64_UINT;
        case Format::R64_SINT:
            return VK_FORMAT_R64_SINT;
        case Format::R64_SFLOAT:
            return VK_FORMAT_R64_SFLOAT;
        case Format::R64G64_UINT:
            return VK_FORMAT_R64G64_UINT;
        case Format::R64G64_SINT:
            return VK_FORMAT_R64G64_SINT;
        case Format::R64G64_SFLOAT:
            return VK_FORMAT_R64G64_SFLOAT;
        case Format::R64G64B64_UINT:
            return VK_FORMAT_R64G64B64_UINT;
        case Format::R64G64B64_SINT:
            return VK_FORMAT_R64G64B64_SINT;
        case Format::R64G64B64_SFLOAT:
            return VK_FORMAT_R64G64B64_SFLOAT;
        case Format::R64G64B64A64_UINT:
            return VK_FORMAT_R64G64B64A64_UINT;
        case Format::R64G64B64A64_SINT:
            return VK_FORMAT_R64G64B64A64_SINT;
        case Format::R64G64B64A64_SFLOAT:
            return VK_FORMAT_R64G64B64A64_SFLOAT;
        case Format::B10G11R11_UFLOAT_PACK32:
            return VK_FORMAT_B10G11R11_UFLOAT_PACK32;
        case Format::E5B9G9R9_UFLOAT_PACK32:
            return VK_FORMAT_E5B9G9R9_UFLOAT_PACK32;
        case Format::D16_UNORM:
            return VK_FORMAT_D16_UNORM;
        case Format::X8_D24_UNORM_PACK32:
            return VK_FORMAT_X8_D24_UNORM_PACK32;
        case Format::D32_SFLOAT:
            return VK_FORMAT_D32_SFLOAT;
        case Format::S8_UINT:
            return VK_FORMAT_S8_UINT;
        case Format::D16_UNORM_S8_UINT:
            return VK_FORMAT_D16_UNORM_S8_UINT;
        case Format::D24_UNORM_S8_UINT:
            return VK_FORMAT_D24_UNORM_S8_UINT;
        case Format::D32_SFLOAT_S8_UINT:
            return VK_FORMAT_D32_SFLOAT_S8_UINT;
        case Format::BC1_RGB_UNORM_BLOCK:
            return VK_FORMAT_BC1_RGB_UNORM_BLOCK;
        case Format::BC1_RGB_SRGB_BLOCK:
            return VK_FORMAT_BC1_RGB_SRGB_BLOCK;
        case Format::BC1_RGBA_UNORM_BLOCK:
            return VK_FORMAT_BC1_RGBA_UNORM_BLOCK;
        case Format::BC1_RGBA_SRGB_BLOCK:
            return VK_FORMAT_BC1_RGBA_SRGB_BLOCK;
        case Format::BC2_UNORM_BLOCK:
            return VK_FORMAT_BC2_UNORM_BLOCK;
        case Format::BC2_SRGB_BLOCK:
            return VK_FORMAT_BC2_SRGB_BLOCK;
        case Format::BC3_UNORM_BLOCK:
            return VK_FORMAT_BC3_UNORM_BLOCK;
        case Format::BC3_SRGB_BLOCK:
            return VK_FORMAT_BC3_SRGB_BLOCK;
        case Format::BC4_UNORM_BLOCK:
            return VK_FORMAT_BC4_UNORM_BLOCK;
        case Format::BC4_SNORM_BLOCK:
            return VK_FORMAT_BC4_SNORM_BLOCK;
        case Format::BC5_UNORM_BLOCK:
            return VK_FORMAT_BC5_UNORM_BLOCK;
        case Format::BC5_SNORM_BLOCK:
            return VK_FORMAT_BC5_SNORM_BLOCK;
        case Format::BC6H_UFLOAT_BLOCK:
            return VK_FORMAT_BC6H_UFLOAT_BLOCK;
        case Format::BC6H_SFLOAT_BLOCK:
            return VK_FORMAT_BC6H_SFLOAT_BLOCK;
        case Format::BC7_UNORM_BLOCK:
            return VK_FORMAT_BC7_UNORM_BLOCK;
        case Format::BC7_SRGB_BLOCK:
            return VK_FORMAT_BC7_SRGB_BLOCK;
        case Format::ETC2_R8G8B8_UNORM_BLOCK:
            return VK_FORMAT_ETC2_R8G8B8_UNORM_BLOCK;
        case Format::ETC2_R8G8B8_SRGB_BLOCK:
            return VK_FORMAT_ETC2_R8G8B8_SRGB_BLOCK;
        case Format::ETC2_R8G8B8A1_UNORM_BLOCK:
            return VK_FORMAT_ETC2_R8G8B8A1_UNORM_BLOCK;
        case Format::ETC2_R8G8B8A1_SRGB_BLOCK:
            return VK_FORMAT_ETC2_R8G8B8A1_SRGB_BLOCK;
        case Format::ETC2_R8G8B8A8_UNORM_BLOCK:
            return VK_FORMAT_ETC2_R8G8B8A8_UNORM_BLOCK;
        case Format::ETC2_R8G8B8A8_SRGB_BLOCK:
            return VK_FORMAT_ETC2_R8G8B8A8_SRGB_BLOCK;
        case Format::EAC_R11_UNORM_BLOCK:
            return VK_FORMAT_EAC_R11_UNORM_BLOCK;
        case Format::EAC_R11_SNORM_BLOCK:
            return VK_FORMAT_EAC_R11_SNORM_BLOCK;
        case Format::EAC_R11G11_UNORM_BLOCK:
            return VK_FORMAT_EAC_R11G11_UNORM_BLOCK;
        case Format::EAC_R11G11_SNORM_BLOCK:
            return VK_FORMAT_EAC_R11G11_SNORM_BLOCK;
        case Format::ASTC_4x4_UNORM_BLOCK:
            return VK_FORMAT_ASTC_4x4_UNORM_BLOCK;
        case Format::ASTC_4x4_SRGB_BLOCK:
            return VK_FORMAT_ASTC_4x4_SRGB_BLOCK;
        case Format::ASTC_5x4_UNORM_BLOCK:
            return VK_FORMAT_ASTC_5x4_UNORM_BLOCK;
        case Format::ASTC_5x4_SRGB_BLOCK:
            return VK_FORMAT_ASTC_5x4_SRGB_BLOCK;
        case Format::ASTC_5x5_UNORM_BLOCK:
            return VK_FORMAT_ASTC_5x5_UNORM_BLOCK;
        case Format::ASTC_5x5_SRGB_BLOCK:
            return VK_FORMAT_ASTC_5x5_SRGB_BLOCK;
        case Format::ASTC_6x5_UNORM_BLOCK:
            return VK_FORMAT_ASTC_6x5_UNORM_BLOCK;
        case Format::ASTC_6x5_SRGB_BLOCK:
            return VK_FORMAT_ASTC_6x5_SRGB_BLOCK;
        case Format::ASTC_6x6_UNORM_BLOCK:
            return VK_FORMAT_ASTC_6x6_UNORM_BLOCK;
        case Format::ASTC_6x6_SRGB_BLOCK:
            return VK_FORMAT_ASTC_6x6_SRGB_BLOCK;
        case Format::ASTC_8x5_UNORM_BLOCK:
            return VK_FORMAT_ASTC_8x5_UNORM_BLOCK;
        case Format::ASTC_8x5_SRGB_BLOCK:
            return VK_FORMAT_ASTC_8x5_SRGB_BLOCK;
        case Format::ASTC_8x6_UNORM_BLOCK:
            return VK_FORMAT_ASTC_8x6_UNORM_BLOCK;
        case Format::ASTC_8x6_SRGB_BLOCK:
            return VK_FORMAT_ASTC_8x6_SRGB_BLOCK;
        case Format::ASTC_8x8_UNORM_BLOCK:
            return VK_FORMAT_ASTC_8x8_UNORM_BLOCK;
        case Format::ASTC_8x8_SRGB_BLOCK:
            return VK_FORMAT_ASTC_8x8_SRGB_BLOCK;
        case Format::ASTC_10x5_UNORM_BLOCK:
            return VK_FORMAT_ASTC_10x5_UNORM_BLOCK;
        case Format::ASTC_10x5_SRGB_BLOCK:
            return VK_FORMAT_ASTC_10x5_SRGB_BLOCK;
        case Format::ASTC_10x6_UNORM_BLOCK:
            return VK_FORMAT_ASTC_10x6_UNORM_BLOCK;
        case Format::ASTC_10x6_SRGB_BLOCK:
            return VK_FORMAT_ASTC_10x6_SRGB_BLOCK;
        case Format::ASTC_10x8_UNORM_BLOCK:
            return VK_FORMAT_ASTC_10x8_UNORM_BLOCK;
        case Format::ASTC_10x8_SRGB_BLOCK:
            return VK_FORMAT_ASTC_10x8_SRGB_BLOCK;
        case Format::ASTC_10x10_UNORM_BLOCK:
            return VK_FORMAT_ASTC_10x10_UNORM_BLOCK;
        case Format::ASTC_10x10_SRGB_BLOCK:
            return VK_FORMAT_ASTC_10x10_SRGB_BLOCK;
        case Format::ASTC_12x10_UNORM_BLOCK:
            return VK_FORMAT_ASTC_12x10_UNORM_BLOCK;
        case Format::ASTC_12x10_SRGB_BLOCK:
            return VK_FORMAT_ASTC_12x10_SRGB_BLOCK;
        case Format::ASTC_12x12_UNORM_BLOCK:
            return VK_FORMAT_ASTC_12x12_UNORM_BLOCK;
        case Format::ASTC_12x12_SRGB_BLOCK:
            return VK_FORMAT_ASTC_12x12_SRGB_BLOCK;
        case Format::G8B8G8R8_422_UNORM:
            return VK_FORMAT_G8B8G8R8_422_UNORM;
        case Format::B8G8R8G8_422_UNORM:
            return VK_FORMAT_B8G8R8G8_422_UNORM;
        case Format::G8_B8_R8_3PLANE_420_UNORM:
            return VK_FORMAT_G8_B8_R8_3PLANE_420_UNORM;
        case Format::G8_B8R8_2PLANE_420_UNORM:
            return VK_FORMAT_G8_B8R8_2PLANE_420_UNORM;
        case Format::G8_B8_R8_3PLANE_422_UNORM:
            return VK_FORMAT_G8_B8_R8_3PLANE_422_UNORM;
        case Format::G8_B8R8_2PLANE_422_UNORM:
            return VK_FORMAT_G8_B8R8_2PLANE_422_UNORM;
        case Format::G8_B8_R8_3PLANE_444_UNORM:
            return VK_FORMAT_G8_B8_R8_3PLANE_444_UNORM;
        case Format::R10X6_UNORM_PACK16:
            return VK_FORMAT_R10X6_UNORM_PACK16;
        case Format::R10X6G10X6_UNORM_2PACK16:
            return VK_FORMAT_R10X6G10X6_UNORM_2PACK16;
        case Format::R10X6G10X6B10X6A10X6_UNORM_4PACK16:
            return VK_FORMAT_R10X6G10X6B10X6A10X6_UNORM_4PACK16;
        case Format::G10X6B10X6G10X6R10X6_422_UNORM_4PACK16:
            return VK_FORMAT_G10X6B10X6G10X6R10X6_422_UNORM_4PACK16;
        case Format::B10X6G10X6R10X6G10X6_422_UNORM_4PACK16:
            return VK_FORMAT_B10X6G10X6R10X6G10X6_422_UNORM_4PACK16;
        case Format::G10X6_B10X6_R10X6_3PLANE_420_UNORM_3PACK16:
            return VK_FORMAT_G10X6_B10X6_R10X6_3PLANE_420_UNORM_3PACK16;
        case Format::G10X6_B10X6R10X6_2PLANE_420_UNORM_3PACK16:
            return VK_FORMAT_G10X6_B10X6R10X6_2PLANE_420_UNORM_3PACK16;
        case Format::G10X6_B10X6_R10X6_3PLANE_422_UNORM_3PACK16:
            return VK_FORMAT_G10X6_B10X6_R10X6_3PLANE_422_UNORM_3PACK16;
        case Format::G10X6_B10X6R10X6_2PLANE_422_UNORM_3PACK16:
            return VK_FORMAT_G10X6_B10X6R10X6_2PLANE_422_UNORM_3PACK16;
        case Format::G10X6_B10X6_R10X6_3PLANE_444_UNORM_3PACK16:
            return VK_FORMAT_G10X6_B10X6_R10X6_3PLANE_444_UNORM_3PACK16;
        case Format::R12X4_UNORM_PACK16:
            return VK_FORMAT_R12X4_UNORM_PACK16;
        case Format::R12X4G12X4_UNORM_2PACK16:
            return VK_FORMAT_R12X4G12X4_UNORM_2PACK16;
        case Format::R12X4G12X4B12X4A12X4_UNORM_4PACK16:
            return VK_FORMAT_R12X4G12X4B12X4A12X4_UNORM_4PACK16;
        case Format::G12X4B12X4G12X4R12X4_422_UNORM_4PACK16:
            return VK_FORMAT_G12X4B12X4G12X4R12X4_422_UNORM_4PACK16;
        case Format::B12X4G12X4R12X4G12X4_422_UNORM_4PACK16:
            return VK_FORMAT_B12X4G12X4R12X4G12X4_422_UNORM_4PACK16;
        case Format::G12X4_B12X4_R12X4_3PLANE_420_UNORM_3PACK16:
            return VK_FORMAT_G12X4_B12X4_R12X4_3PLANE_420_UNORM_3PACK16;
        case Format::G12X4_B12X4R12X4_2PLANE_420_UNORM_3PACK16:
            return VK_FORMAT_G12X4_B12X4R12X4_2PLANE_420_UNORM_3PACK16;
        case Format::G12X4_B12X4_R12X4_3PLANE_422_UNORM_3PACK16:
            return VK_FORMAT_G12X4_B12X4_R12X4_3PLANE_422_UNORM_3PACK16;
        case Format::G12X4_B12X4R12X4_2PLANE_422_UNORM_3PACK16:
            return VK_FORMAT_G12X4_B12X4R12X4_2PLANE_422_UNORM_3PACK16;
        case Format::G12X4_B12X4_R12X4_3PLANE_444_UNORM_3PACK16:
            return VK_FORMAT_G12X4_B12X4_R12X4_3PLANE_444_UNORM_3PACK16;
        case Format::G16B16G16R16_422_UNORM:
            return VK_FORMAT_G16B16G16R16_422_UNORM;
        case Format::B16G16R16G16_422_UNORM:
            return VK_FORMAT_B16G16R16G16_422_UNORM;
        case Format::G16_B16_R16_3PLANE_420_UNORM:
            return VK_FORMAT_G16_B16_R16_3PLANE_420_UNORM;
        case Format::G16_B16R16_2PLANE_420_UNORM:
            return VK_FORMAT_G16_B16R16_2PLANE_420_UNORM;
        case Format::G16_B16_R16_3PLANE_422_UNORM:
            return VK_FORMAT_G16_B16_R16_3PLANE_422_UNORM;
        case Format::G16_B16R16_2PLANE_422_UNORM:
            return VK_FORMAT_G16_B16R16_2PLANE_422_UNORM;
        case Format::G16_B16_R16_3PLANE_444_UNORM:
            return VK_FORMAT_G16_B16_R16_3PLANE_444_UNORM;
        case Format::G8_B8R8_2PLANE_444_UNORM:
            return VK_FORMAT_G8_B8R8_2PLANE_444_UNORM;
        case Format::G10X6_B10X6R10X6_2PLANE_444_UNORM_3PACK16:
            return VK_FORMAT_G10X6_B10X6R10X6_2PLANE_444_UNORM_3PACK16;
        case Format::G12X4_B12X4R12X4_2PLANE_444_UNORM_3PACK16:
            return VK_FORMAT_G12X4_B12X4R12X4_2PLANE_444_UNORM_3PACK16;
        case Format::G16_B16R16_2PLANE_444_UNORM:
            return VK_FORMAT_G16_B16R16_2PLANE_444_UNORM;
        case Format::A4R4G4B4_UNORM_PACK16:
            return VK_FORMAT_A4R4G4B4_UNORM_PACK16;
        case Format::A4B4G4R4_UNORM_PACK16:
            return VK_FORMAT_A4B4G4R4_UNORM_PACK16;
        case Format::ASTC_4x4_SFLOAT_BLOCK:
            return VK_FORMAT_ASTC_4x4_SFLOAT_BLOCK;
        case Format::ASTC_5x4_SFLOAT_BLOCK:
            return VK_FORMAT_ASTC_5x4_SFLOAT_BLOCK;
        case Format::ASTC_5x5_SFLOAT_BLOCK:
            return VK_FORMAT_ASTC_5x5_SFLOAT_BLOCK;
        case Format::ASTC_6x5_SFLOAT_BLOCK:
            return VK_FORMAT_ASTC_6x5_SFLOAT_BLOCK;
        case Format::ASTC_6x6_SFLOAT_BLOCK:
            return VK_FORMAT_ASTC_6x6_SFLOAT_BLOCK;
        case Format::ASTC_8x5_SFLOAT_BLOCK:
            return VK_FORMAT_ASTC_8x5_SFLOAT_BLOCK;
        case Format::ASTC_8x6_SFLOAT_BLOCK:
            return VK_FORMAT_ASTC_8x6_SFLOAT_BLOCK;
        case Format::ASTC_8x8_SFLOAT_BLOCK:
            return VK_FORMAT_ASTC_8x8_SFLOAT_BLOCK;
        case Format::ASTC_10x5_SFLOAT_BLOCK:
            return VK_FORMAT_ASTC_10x5_SFLOAT_BLOCK;
        case Format::ASTC_10x6_SFLOAT_BLOCK:
            return VK_FORMAT_ASTC_10x6_SFLOAT_BLOCK;
        case Format::ASTC_10x8_SFLOAT_BLOCK:
            return VK_FORMAT_ASTC_10x8_SFLOAT_BLOCK;
        case Format::ASTC_10x10_SFLOAT_BLOCK:
            return VK_FORMAT_ASTC_10x10_SFLOAT_BLOCK;
        case Format::ASTC_12x10_SFLOAT_BLOCK:
            return VK_FORMAT_ASTC_12x10_SFLOAT_BLOCK;
        case Format::ASTC_12x12_SFLOAT_BLOCK:
            return VK_FORMAT_ASTC_12x12_SFLOAT_BLOCK;
        case Format::PVRTC1_2BPP_UNORM_BLOCK_IMG:
            return VK_FORMAT_PVRTC1_2BPP_UNORM_BLOCK_IMG;
        case Format::PVRTC1_4BPP_UNORM_BLOCK_IMG:
            return VK_FORMAT_PVRTC1_4BPP_UNORM_BLOCK_IMG;
        case Format::PVRTC2_2BPP_UNORM_BLOCK_IMG:
            return VK_FORMAT_PVRTC2_2BPP_UNORM_BLOCK_IMG;
        case Format::PVRTC2_4BPP_UNORM_BLOCK_IMG:
            return VK_FORMAT_PVRTC2_4BPP_UNORM_BLOCK_IMG;
        case Format::PVRTC1_2BPP_SRGB_BLOCK_IMG:
            return VK_FORMAT_PVRTC1_2BPP_SRGB_BLOCK_IMG;
        case Format::PVRTC1_4BPP_SRGB_BLOCK_IMG:
            return VK_FORMAT_PVRTC1_4BPP_SRGB_BLOCK_IMG;
        case Format::PVRTC2_2BPP_SRGB_BLOCK_IMG:
            return VK_FORMAT_PVRTC2_2BPP_SRGB_BLOCK_IMG;
        case Format::PVRTC2_4BPP_SRGB_BLOCK_IMG:
            return VK_FORMAT_PVRTC2_4BPP_SRGB_BLOCK_IMG;
        case Format::R16G16_S10_5_NV:
            return VK_FORMAT_R16G16_S10_5_NV;
        default:
            return VK_FORMAT_UNDEFINED;
        }
    }

    ColorSpace ColorSpace_from_vk(VkColorSpaceKHR vk_space)
    {
        switch (vk_space)
        {
        case VK_COLOR_SPACE_SRGB_NONLINEAR_KHR:
            return ColorSpace::SrgbNonlinear;
        case VK_COLOR_SPACE_DISPLAY_P3_NONLINEAR_EXT:
            return ColorSpace::DisplayP3Nonlinear;
        case VK_COLOR_SPACE_EXTENDED_SRGB_LINEAR_EXT:
            return ColorSpace::ExtendedSrgbLinear;
        case VK_COLOR_SPACE_DISPLAY_P3_LINEAR_EXT:
            return ColorSpace::DisplayP3Linear;
        case VK_COLOR_SPACE_DCI_P3_NONLINEAR_EXT:
            return ColorSpace::DciP3Nonlinear;
        case VK_COLOR_SPACE_BT709_LINEAR_EXT:
            return ColorSpace::Bt709Linear;
        case VK_COLOR_SPACE_BT709_NONLINEAR_EXT:
            return ColorSpace::Bt709Nonlinear;
        case VK_COLOR_SPACE_BT2020_LINEAR_EXT:
            return ColorSpace::Bt2020Linear;
        case VK_COLOR_SPACE_HDR10_ST2084_EXT:
            return ColorSpace::Hdr10St2084;
        case VK_COLOR_SPACE_DOLBYVISION_EXT:
            return ColorSpace::DolbyVision;
        case VK_COLOR_SPACE_HDR10_HLG_EXT:
            return ColorSpace::Hdr10Hlg;
        case VK_COLOR_SPACE_ADOBERGB_LINEAR_EXT:
            return ColorSpace::AdobeRgbLinear;
        case VK_COLOR_SPACE_ADOBERGB_NONLINEAR_EXT:
            return ColorSpace::AdobeRgbNonlinear;
        case VK_COLOR_SPACE_PASS_THROUGH_EXT:
            return ColorSpace::PassThrough;
        case VK_COLOR_SPACE_EXTENDED_SRGB_NONLINEAR_EXT:
            return ColorSpace::ExtendedSrgbNonlinear;
        case VK_COLOR_SPACE_DISPLAY_NATIVE_AMD:
            return ColorSpace::DisplayNativeAmd;
        default:
            return ColorSpace::SrgbNonlinear;
        }
    }

    VkColorSpaceKHR ColorSpace_to_vk(ColorSpace space)
    {
        switch (space)
        {
        case ColorSpace::SrgbNonlinear:
            return VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;
        case ColorSpace::DisplayP3Nonlinear:
            return VK_COLOR_SPACE_DISPLAY_P3_NONLINEAR_EXT;
        case ColorSpace::ExtendedSrgbLinear:
            return VK_COLOR_SPACE_EXTENDED_SRGB_LINEAR_EXT;
        case ColorSpace::DisplayP3Linear:
            return VK_COLOR_SPACE_DISPLAY_P3_LINEAR_EXT;
        case ColorSpace::DciP3Nonlinear:
            return VK_COLOR_SPACE_DCI_P3_NONLINEAR_EXT;
        case ColorSpace::Bt709Linear:
            return VK_COLOR_SPACE_BT709_LINEAR_EXT;
        case ColorSpace::Bt709Nonlinear:
            return VK_COLOR_SPACE_BT709_NONLINEAR_EXT;
        case ColorSpace::Bt2020Linear:
            return VK_COLOR_SPACE_BT2020_LINEAR_EXT;
        case ColorSpace::Hdr10St2084:
            return VK_COLOR_SPACE_HDR10_ST2084_EXT;
        case ColorSpace::DolbyVision:
            return VK_COLOR_SPACE_DOLBYVISION_EXT;
        case ColorSpace::Hdr10Hlg:
            return VK_COLOR_SPACE_HDR10_HLG_EXT;
        case ColorSpace::AdobeRgbLinear:
            return VK_COLOR_SPACE_ADOBERGB_LINEAR_EXT;
        case ColorSpace::AdobeRgbNonlinear:
            return VK_COLOR_SPACE_ADOBERGB_NONLINEAR_EXT;
        case ColorSpace::PassThrough:
            return VK_COLOR_SPACE_PASS_THROUGH_EXT;
        case ColorSpace::ExtendedSrgbNonlinear:
            return VK_COLOR_SPACE_EXTENDED_SRGB_NONLINEAR_EXT;
        case ColorSpace::DisplayNativeAmd:
            return VK_COLOR_SPACE_DISPLAY_NATIVE_AMD;
        default:
            return VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;
        }
    }

    SurfaceFormat SurfaceFormat_from_vk(
        const VkSurfaceFormatKHR& vk_surface_format
    )
    {
        return SurfaceFormat{
            .format = Format_from_vk(vk_surface_format.format),
            .color_space = ColorSpace_from_vk(vk_surface_format.colorSpace)
        };
    }

    PresentMode PresentMode_from_vk(VkPresentModeKHR vk_mode)
    {
        switch (vk_mode)
        {
        case VK_PRESENT_MODE_IMMEDIATE_KHR:
            return PresentMode::Immediate;
        case VK_PRESENT_MODE_MAILBOX_KHR:
            return PresentMode::Mailbox;
        case VK_PRESENT_MODE_FIFO_KHR:
            return PresentMode::Fifo;
        case VK_PRESENT_MODE_FIFO_RELAXED_KHR:
            return PresentMode::FifoRelaxed;
        case VK_PRESENT_MODE_SHARED_DEMAND_REFRESH_KHR:
            return PresentMode::SharedDemandRefresh;
        case VK_PRESENT_MODE_SHARED_CONTINUOUS_REFRESH_KHR:
            return PresentMode::SharedContinuousRefresh;
        default:
            return PresentMode::Immediate;
        }
    }

    VkPresentModeKHR PresentMode_to_vk(PresentMode mode)
    {
        switch (mode)
        {
        case bv::PresentMode::Immediate:
            return VK_PRESENT_MODE_IMMEDIATE_KHR;
        case bv::PresentMode::Mailbox:
            return VK_PRESENT_MODE_MAILBOX_KHR;
        case bv::PresentMode::Fifo:
            return VK_PRESENT_MODE_FIFO_KHR;
        case bv::PresentMode::FifoRelaxed:
            return VK_PRESENT_MODE_FIFO_RELAXED_KHR;
        case bv::PresentMode::SharedDemandRefresh:
            return VK_PRESENT_MODE_SHARED_DEMAND_REFRESH_KHR;
        case bv::PresentMode::SharedContinuousRefresh:
            return VK_PRESENT_MODE_SHARED_CONTINUOUS_REFRESH_KHR;
        default:
            return VK_PRESENT_MODE_IMMEDIATE_KHR;
        }
    }

    DebugLabel DebugLabel_from_vk(const VkDebugUtilsLabelEXT& vk_label)
    {
        return DebugLabel{
            .name = cstr_to_std(vk_label.pLabelName),
            .color = raw_arr_to_std<4>(vk_label.color)
        };
    }

    DebugObjectInfo DebugObjectInfo_from_vk(
        const VkDebugUtilsObjectNameInfoEXT& vk_info
    )
    {
        return DebugObjectInfo{
            .type = ObjectType_from_vk(vk_info.objectType),
            .handle = vk_info.objectHandle,
            .name = cstr_to_std(vk_info.pObjectName)
        };
    }

    DebugMessageData DebugMessageData_from_vk(
        const VkDebugUtilsMessengerCallbackDataEXT& vk_data
    )
    {
        std::vector<DebugLabel> queue_labels;
        for (size_t i = 0; i < vk_data.queueLabelCount; i++)
        {
            queue_labels.push_back(
                DebugLabel_from_vk(vk_data.pQueueLabels[i])
            );
        }

        std::vector<DebugLabel> cmd_buf_labels;
        for (size_t i = 0; i < vk_data.cmdBufLabelCount; i++)
        {
            cmd_buf_labels.push_back(
                DebugLabel_from_vk(vk_data.pCmdBufLabels[i])
            );
        }

        std::vector<DebugObjectInfo> objects;
        for (size_t i = 0; i < vk_data.objectCount; i++)
        {
            objects.push_back(
                DebugObjectInfo_from_vk(vk_data.pObjects[i])
            );
        }

        return DebugMessageData{
            .message_id_name = cstr_to_std(vk_data.pMessageIdName),
            .message_id_number = vk_data.messageIdNumber,
            .message = cstr_to_std(vk_data.pMessage),
            .queue_labels = queue_labels,
            .cmd_buf_labels = cmd_buf_labels,
            .objects = objects
        };;
    }

    VkDeviceQueueCreateInfo QueueRequest_to_vk(
        const QueueRequest& request
    )
    {
        return VkDeviceQueueCreateInfo{
            .sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
            .pNext = nullptr,
            .flags = QueueRequestFlags_to_vk(request.flags),
            .queueFamilyIndex = request.queue_family_index,
            .queueCount = request.num_queues_to_create,
            .pQueuePriorities = request.priorities.data()
        };
    }

    VkSwapchainCreateFlagsKHR SwapchainFlags_to_vk(const SwapchainFlags& flags)
    {
        VkSwapchainCreateFlagsKHR vk_flags = 0;
        if (flags.split_instance_bind_regions)
        {
            vk_flags |= VK_SWAPCHAIN_CREATE_SPLIT_INSTANCE_BIND_REGIONS_BIT_KHR;
        }
        if (flags.protected_)
        {
            vk_flags |= VK_SWAPCHAIN_CREATE_PROTECTED_BIT_KHR;
        }
        if (flags.mutable_format)
        {
            vk_flags |= VK_SWAPCHAIN_CREATE_MUTABLE_FORMAT_BIT_KHR;
        }
        if (flags.deferred_memory_allocation)
        {
            vk_flags |= VK_SWAPCHAIN_CREATE_DEFERRED_MEMORY_ALLOCATION_BIT_EXT;
        }
        return vk_flags;
    }

    VkSharingMode SharingMode_to_vk(SharingMode mode)
    {
        switch (mode)
        {
        case SharingMode::Exclusive:
            return VK_SHARING_MODE_EXCLUSIVE;
        case SharingMode::Concurrent:
            return VK_SHARING_MODE_CONCURRENT;
        default:
            return VK_SHARING_MODE_EXCLUSIVE;
        }
    }

    VkImageViewCreateFlags ImageViewFlags_to_vk(const ImageViewFlags& flags)
    {
        VkImageViewCreateFlags vk_flags = 0;
        if (flags.fragment_density_map_dynamic)
        {
            vk_flags |=
                VK_IMAGE_VIEW_CREATE_FRAGMENT_DENSITY_MAP_DYNAMIC_BIT_EXT;
        }
        if (flags.descriptor_buffer_capture_replay)
        {
            vk_flags |=
                VK_IMAGE_VIEW_CREATE_DESCRIPTOR_BUFFER_CAPTURE_REPLAY_BIT_EXT;
        }
        if (flags.fragment_density_map_deferred)
        {
            vk_flags |=
                VK_IMAGE_VIEW_CREATE_FRAGMENT_DENSITY_MAP_DEFERRED_BIT_EXT;
        }
        return vk_flags;
    }

    VkImageViewType ImageViewType_to_vk(ImageViewType type)
    {
        switch (type)
        {
        case bv::ImageViewType::_1d:
            return VK_IMAGE_VIEW_TYPE_1D;
        case bv::ImageViewType::_2d:
            return VK_IMAGE_VIEW_TYPE_2D;
        case bv::ImageViewType::_3d:
            return VK_IMAGE_VIEW_TYPE_3D;
        case bv::ImageViewType::Cube:
            return VK_IMAGE_VIEW_TYPE_CUBE;
        case bv::ImageViewType::_1dArray:
            return VK_IMAGE_VIEW_TYPE_1D_ARRAY;
        case bv::ImageViewType::_2dArray:
            return VK_IMAGE_VIEW_TYPE_2D_ARRAY;
        case bv::ImageViewType::CubeArray:
            return VK_IMAGE_VIEW_TYPE_CUBE_ARRAY;
        default:
            return VK_IMAGE_VIEW_TYPE_1D;
        }
    }

    VkComponentSwizzle ComponentSwizzle_to_vk(ComponentSwizzle swizzle)
    {
        switch (swizzle)
        {
        case bv::ComponentSwizzle::Identity:
            return VK_COMPONENT_SWIZZLE_IDENTITY;
        case bv::ComponentSwizzle::Zero:
            return VK_COMPONENT_SWIZZLE_ZERO;
        case bv::ComponentSwizzle::One:
            return VK_COMPONENT_SWIZZLE_ONE;
        case bv::ComponentSwizzle::R:
            return VK_COMPONENT_SWIZZLE_R;
        case bv::ComponentSwizzle::G:
            return VK_COMPONENT_SWIZZLE_G;
        case bv::ComponentSwizzle::B:
            return VK_COMPONENT_SWIZZLE_B;
        case bv::ComponentSwizzle::A:
            return VK_COMPONENT_SWIZZLE_A;
        default:
            return VK_COMPONENT_SWIZZLE_IDENTITY;
        }
    }

    VkComponentMapping ComponentMapping_to_vk(const ComponentMapping& mapping)
    {
        return VkComponentMapping{
            .r = ComponentSwizzle_to_vk(mapping.r),
            .g = ComponentSwizzle_to_vk(mapping.g),
            .b = ComponentSwizzle_to_vk(mapping.b),
            .a = ComponentSwizzle_to_vk(mapping.a)
        };
    }

    VkImageAspectFlags ImageAspectFlags_to_vk(const ImageAspectFlags& flags)
    {
        VkImageAspectFlags vk_flags = 0;
        if (flags.color)
        {
            vk_flags |= VK_IMAGE_ASPECT_COLOR_BIT;
        }
        if (flags.depth)
        {
            vk_flags |= VK_IMAGE_ASPECT_DEPTH_BIT;
        }
        if (flags.stencil)
        {
            vk_flags |= VK_IMAGE_ASPECT_STENCIL_BIT;
        }
        if (flags.metadata)
        {
            vk_flags |= VK_IMAGE_ASPECT_METADATA_BIT;
        }
        if (flags.plane_0)
        {
            vk_flags |= VK_IMAGE_ASPECT_PLANE_0_BIT;
        }
        if (flags.plane_1)
        {
            vk_flags |= VK_IMAGE_ASPECT_PLANE_1_BIT;
        }
        if (flags.plane_2)
        {
            vk_flags |= VK_IMAGE_ASPECT_PLANE_2_BIT;
        }
        if (flags.none)
        {
            vk_flags |= VK_IMAGE_ASPECT_NONE;
        }
        if (flags.memory_plane_0)
        {
            vk_flags |= VK_IMAGE_ASPECT_MEMORY_PLANE_0_BIT_EXT;
        }
        if (flags.memory_plane_1)
        {
            vk_flags |= VK_IMAGE_ASPECT_MEMORY_PLANE_1_BIT_EXT;
        }
        if (flags.memory_plane_2)
        {
            vk_flags |= VK_IMAGE_ASPECT_MEMORY_PLANE_2_BIT_EXT;
        }
        if (flags.memory_plane_3)
        {
            vk_flags |= VK_IMAGE_ASPECT_MEMORY_PLANE_3_BIT_EXT;
        }
        return vk_flags;
    }

    VkImageSubresourceRange ImageSubresourceRange_to_vk(
        const ImageSubresourceRange& range
    )
    {
        return VkImageSubresourceRange{
            .aspectMask = ImageAspectFlags_to_vk(range.aspect_mask),
            .baseMipLevel = range.base_mip_level,
            .levelCount = range.level_count,
            .baseArrayLayer = range.base_array_layer,
            .layerCount = range.layer_count
        };
    }

    Error::Error()
        : message("no error information provided"),
        api_result(std::nullopt)
    {}

    Error::Error(std::string message)
        : message(std::move(message)),
        api_result(std::nullopt)
    {}

    Error::Error(ApiResult api_result)
        : message(),
        api_result(api_result)
    {}

    Error::Error(std::string message, ApiResult api_result)
        : message(std::move(message)),
        api_result(api_result)
    {}

    std::string Error::to_string() const
    {
        std::string s = message;
        if (api_result.has_value())
        {
            if (!message.empty())
            {
                s += ": ";
            }
            s += api_result.value().to_string();
        }
        return s;
    }

    Result<std::vector<ExtensionProperties>>
        PhysicalDevice::fetch_available_extensions(
            const std::string& layer_name
        )
    {
        const char* layer_name_cstr = nullptr;
        if (!layer_name.empty())
        {
            layer_name_cstr = layer_name.c_str();
        }

        uint32_t count = 0;
        vkEnumerateDeviceExtensionProperties(
            vk_physical_device,
            layer_name_cstr,
            &count,
            nullptr
        );

        std::vector<VkExtensionProperties> vk_extensions(count);
        VkResult vk_result = vkEnumerateDeviceExtensionProperties(
            vk_physical_device,
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
        for (const auto& vk_ext : vk_extensions)
        {
            extensions.push_back(ExtensionProperties_from_vk(vk_ext));
        }
        return extensions;
    }

    PhysicalDevice::PhysicalDevice(
        VkPhysicalDevice vk_physical_device,
        const PhysicalDeviceProperties& properties,
        const PhysicalDeviceFeatures& features,
        const std::vector<QueueFamily>& queue_families,
        const QueueFamilyIndices& queue_family_indices
    )
        : vk_physical_device(vk_physical_device),
        _properties(properties),
        _features(features),
        _queue_families(queue_families),
        _queue_family_indices(queue_family_indices)
    {}

    Result<> PhysicalDevice::check_swapchain_support(
        const std::shared_ptr<Surface>& surface
    )
    {
        _swapchain_support = std::nullopt;

        if (surface == nullptr)
        {
            return Result();
        }

        // check for extension
        {
            auto available_extensions_result = fetch_available_extensions();
            if (!available_extensions_result.ok())
            {
                return available_extensions_result.error();
            }
            auto available_extensions = available_extensions_result.value();

            bool has_extension = false;
            for (const auto& ext : available_extensions)
            {
                if (ext.name == VK_KHR_SWAPCHAIN_EXTENSION_NAME)
                {
                    has_extension = true;
                    break;
                }
            }
            if (!has_extension)
            {
                return Result();
            }
        }

        VkSurfaceCapabilitiesKHR vk_capabilities;
        VkResult vk_result = vkGetPhysicalDeviceSurfaceCapabilitiesKHR(
            vk_physical_device,
            surface->vk_surface,
            &vk_capabilities
        );
        if (vk_result != VK_SUCCESS)
        {
            return Error(vk_result);
        }

        std::vector<SurfaceFormat> surface_formats;
        {
            uint32_t surface_format_count;
            vkGetPhysicalDeviceSurfaceFormatsKHR(
                vk_physical_device,
                surface->vk_surface,
                &surface_format_count,
                nullptr
            );

            std::vector<VkSurfaceFormatKHR> vk_surface_formats(
                surface_format_count
            );
            vk_result = vkGetPhysicalDeviceSurfaceFormatsKHR(
                vk_physical_device,
                surface->vk_surface,
                &surface_format_count,
                vk_surface_formats.data()
            );
            if (vk_result != VK_SUCCESS && vk_result != VK_INCOMPLETE)
            {
                return Error(vk_result);
            }

            surface_formats.reserve(vk_surface_formats.size());
            for (const auto& vk_surface_format : vk_surface_formats)
            {
                surface_formats.push_back(
                    SurfaceFormat_from_vk(vk_surface_format)
                );
            }
        }

        std::vector<PresentMode> present_modes;
        {
            uint32_t present_mode_count;
            vkGetPhysicalDeviceSurfacePresentModesKHR(
                vk_physical_device,
                surface->vk_surface,
                &present_mode_count,
                nullptr
            );

            std::vector<VkPresentModeKHR> vk_present_modes(present_mode_count);
            vk_result = vkGetPhysicalDeviceSurfacePresentModesKHR(
                vk_physical_device,
                surface->vk_surface,
                &present_mode_count,
                vk_present_modes.data()
            );
            if (vk_result != VK_SUCCESS && vk_result != VK_INCOMPLETE)
            {
                return Error(vk_result);
            }

            present_modes.reserve(vk_present_modes.size());
            for (const auto& vk_present_mode : vk_present_modes)
            {
                present_modes.push_back(PresentMode_from_vk(vk_present_mode));
            }
        }

        _swapchain_support = SwapchainSupport{
            .capabilities = SurfaceCapabilities_from_vk(vk_capabilities),
            .surface_formats = surface_formats,
            .present_modes = present_modes
        };

        return Result();
    }

    Context::Context(Context&& other)
        : _config(std::move(other._config))
    {
        _config = other._config;
        other._config = {};

        _allocator = other._allocator;
        other._allocator = nullptr;

        _vk_allocator = other._vk_allocator;
        _vk_allocator.pUserData = _allocator.get();
        other._vk_allocator = VkAllocationCallbacks{};

        _vk_instance = other._vk_instance;
        other._vk_instance = nullptr;
    }

    Result<Context::ptr> Context::create(
        const ContextConfig& config,
        const Allocator::ptr& allocator
    )
    {
        Context::ptr c = std::make_shared<Context_public_ctor>(
            config,
            allocator
        );

        // allocation callbacks
        {
            c->_vk_allocator.pUserData = c->allocator().get();
            c->_vk_allocator.pfnAllocation = vk_allocation_callback;
            c->_vk_allocator.pfnReallocation = vk_reallocation_callback;
            c->_vk_allocator.pfnFree = vk_free_callback;
            c->_vk_allocator.pfnInternalAllocation =
                vk_internal_allocation_notification;
            c->_vk_allocator.pfnInternalFree = vk_internal_free_notification;
        }

        VkInstanceCreateInfo create_info{};
        create_info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;

        if (c->config().will_enumerate_portability)
            create_info.flags |=
            VK_INSTANCE_CREATE_ENUMERATE_PORTABILITY_BIT_KHR;

        VkApplicationInfo app_info{};
        {
            app_info.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;

            app_info.pApplicationName = c->config().app_name.c_str();
            app_info.applicationVersion = c->config().app_version.encode();

            app_info.pEngineName = c->config().engine_name.c_str();
            app_info.engineVersion = c->config().engine_version.encode();

            app_info.apiVersion = VulkanApiVersion_to_vk(
                c->config().vulkan_api_version
            );
        }
        create_info.pApplicationInfo = &app_info;

        // layers
        std::vector<const char*> layers_cstr;
        {
            layers_cstr.reserve(
                c->config().layers.size()
            );
            for (const auto& layer : c->config().layers)
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
                c->config().extensions.size()
            );
            for (const auto& ext : c->config().extensions)
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
            c->vk_allocator_ptr(),
            &c->_vk_instance
        );
        if (vk_result != VK_SUCCESS)
        {
            return Error(vk_result);
        }
        return c;
    }

    Result<std::vector<LayerProperties>> Context::fetch_available_layers()
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
        for (const auto& vk_layer : vk_layers)
        {
            layers.push_back(LayerProperties_from_vk(vk_layer));
        }
        return layers;
    }

    Result<std::vector<ExtensionProperties>>
        Context::fetch_available_extensions(
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
        for (const auto& vk_ext : vk_extensions)
        {
            extensions.push_back(ExtensionProperties_from_vk(vk_ext));
        }
        return extensions;
    }

    void Context::set_allocator(const Allocator::ptr& allocator)
    {
        _allocator = allocator;
        _vk_allocator.pUserData = _allocator.get();
    }

    const VkAllocationCallbacks* Context::vk_allocator_ptr() const
    {
        if (_allocator == nullptr)
        {
            return nullptr;
        }
        return &_vk_allocator;
    }

    Result<std::vector<PhysicalDevice::ptr>> Context::fetch_physical_devices(
        const std::shared_ptr<Surface>& surface
    )
    {
        uint32_t count = 0;
        vkEnumeratePhysicalDevices(_vk_instance, &count, nullptr);

        std::vector<VkPhysicalDevice> vk_physical_devices(count);
        VkResult vk_result = vkEnumeratePhysicalDevices(
            _vk_instance,
            &count,
            vk_physical_devices.data()
        );
        if (vk_result != VK_SUCCESS && vk_result != VK_INCOMPLETE)
        {
            return Error(vk_result);
        }

        std::vector<PhysicalDevice::ptr> physical_devices;
        physical_devices.reserve(vk_physical_devices.size());
        for (const auto& vk_physical_device : vk_physical_devices)
        {
            VkPhysicalDeviceProperties vk_properties;
            vkGetPhysicalDeviceProperties(
                vk_physical_device,
                &vk_properties
            );

            auto properties = PhysicalDeviceProperties_from_vk(vk_properties);

            VkPhysicalDeviceFeatures vk_features;
            vkGetPhysicalDeviceFeatures(vk_physical_device, &vk_features);

            auto features = PhysicalDeviceFeatures_from_vk(vk_features);

            uint32_t queue_family_count = 0;
            vkGetPhysicalDeviceQueueFamilyProperties(
                vk_physical_device,
                &queue_family_count,
                nullptr
            );

            std::vector<VkQueueFamilyProperties> vk_queue_families(
                queue_family_count
            );
            vkGetPhysicalDeviceQueueFamilyProperties(
                vk_physical_device,
                &queue_family_count,
                vk_queue_families.data()
            );

            QueueFamilyIndices queue_family_indices;

            std::vector<QueueFamily> queue_families;
            queue_families.reserve(vk_queue_families.size());
            for (size_t i = 0; i < vk_queue_families.size(); i++)
            {
                const VkQueueFamilyProperties& vk_queue_family =
                    vk_queue_families[i];

                VkBool32 vk_present_support = VK_FALSE;
                if (surface != nullptr)
                {
                    vk_result = vkGetPhysicalDeviceSurfaceSupportKHR(
                        vk_physical_device,
                        i,
                        surface->vk_surface,
                        &vk_present_support
                    );
                    if (vk_result != VK_SUCCESS)
                    {
                        return Error(
                            "failed to check surface support",
                            vk_result
                        );
                    }
                }

                auto queue_family = QueueFamily_from_vk(
                    vk_queue_family,
                    vk_present_support
                );

                if (queue_family.queue_flags.graphics &&
                    !queue_family_indices.graphics.has_value())
                {
                    queue_family_indices.graphics = i;
                }
                if (queue_family.queue_flags.presentation &&
                    !queue_family_indices.presentation.has_value())
                {
                    queue_family_indices.presentation = i;
                }
                if (queue_family.queue_flags.compute &&
                    !queue_family_indices.compute.has_value())
                {
                    queue_family_indices.compute = i;
                }
                if (queue_family.queue_flags.transfer &&
                    !queue_family_indices.transfer.has_value())
                {
                    queue_family_indices.transfer = i;
                }
                if (queue_family.queue_flags.sparse_binding &&
                    !queue_family_indices.sparse_binding.has_value())
                {
                    queue_family_indices.sparse_binding = i;
                }
                if (queue_family.queue_flags.protected_ &&
                    !queue_family_indices.protected_.has_value())
                {
                    queue_family_indices.protected_ = i;
                }
                if (queue_family.queue_flags.video_decode &&
                    !queue_family_indices.video_decode.has_value())
                {
                    queue_family_indices.video_decode = i;
                }
                if (queue_family.queue_flags.optical_flow_nv &&
                    !queue_family_indices.optical_flow_nv.has_value())
                {
                    queue_family_indices.optical_flow_nv = i;
                }

                bool supports_both_graphics_and_presentation =
                    queue_family.queue_flags.graphics
                    && queue_family.queue_flags.presentation;

                bool already_have_shared_graphics_and_presentation_indices =
                    (
                        queue_family_indices.graphics.has_value()
                        && queue_family_indices.presentation.has_value()
                        )
                    && (
                        queue_family_indices.graphics.value()
                        == queue_family_indices.presentation.value()
                        );

                if (supports_both_graphics_and_presentation &&
                    !already_have_shared_graphics_and_presentation_indices)
                {
                    queue_family_indices.graphics = i;
                    queue_family_indices.presentation = i;
                }

                queue_families.push_back(queue_family);
            }

            physical_devices.push_back(
                std::make_shared<PhysicalDevice_public_ctor>(
                    vk_physical_device,
                    properties,
                    features,
                    queue_families,
                    queue_family_indices
                )
            );

            auto check_swapchain_support_result =
                physical_devices.back()->check_swapchain_support(surface);
            if (!check_swapchain_support_result.ok())
            {
                return Error(
                    "failed to check swapchain support for a physical device: "
                    + check_swapchain_support_result.error().to_string()
                );
            }
        }

        return physical_devices;
    }

    Context::~Context()
    {
        vkDestroyInstance(_vk_instance, vk_allocator_ptr());
    }

    Context::Context(
        const ContextConfig& config,
        const Allocator::ptr& allocator
    )
        : _config(config), _allocator(allocator)
    {}

    DebugMessenger::DebugMessenger(DebugMessenger&& other)
    {
        _context = other._context;
        other._context = nullptr;

        vk_debug_messenger = other.vk_debug_messenger;
        other.vk_debug_messenger = nullptr;

        _message_severity_filter = other._message_severity_filter;
        other._message_severity_filter = DebugMessageSeverityFlags{};

        _message_type_filter = other._message_type_filter;
        other._message_type_filter = DebugMessageTypeFlags{};

        _callback = other._callback;
        other._callback = nullptr;
    }

    Result<DebugMessenger::ptr> DebugMessenger::create(
        const Context::ptr& context,
        DebugMessageSeverityFlags message_severity_filter,
        DebugMessageTypeFlags message_type_filter,
        const DebugCallback& callback
    )
    {
        DebugMessenger::ptr messenger =
            std::make_shared<DebugMessenger_public_ctor>(
                context,
                message_severity_filter,
                message_type_filter,
                callback
            );

        VkDebugUtilsMessengerCreateInfoEXT create_info{
            .sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT,
            .pNext = nullptr,
            .flags = 0,
            .messageSeverity = DebugMessageSeverityFlags_to_vk(
                messenger->message_severity_filter()
            ),
            .messageType = DebugMessageTypeFlags_to_vk(
                messenger->message_type_filter()
            ),
            .pfnUserCallback = vk_debug_callback,
            .pUserData = messenger.get()
        };

        VkResult vk_result = CreateDebugUtilsMessengerEXT(
            messenger->context()->vk_instance(),
            &create_info,
            messenger->context()->vk_allocator_ptr(),
            &messenger->vk_debug_messenger
        );
        if (vk_result != VK_SUCCESS)
        {
            return Error(vk_result);
        }
        return messenger;
    }

    DebugMessenger::~DebugMessenger()
    {
        DestroyDebugUtilsMessengerEXT(
            context()->vk_instance(),
            vk_debug_messenger,
            context()->vk_allocator_ptr()
        );
    }

    DebugMessenger::DebugMessenger(
        const Context::ptr& context,
        const DebugMessageSeverityFlags& message_severity_filter,
        const DebugMessageTypeFlags& message_type_filter,
        const DebugCallback& callback
    )
        : _context(context),
        _message_severity_filter(message_severity_filter),
        _message_type_filter(message_type_filter),
        _callback(callback)
    {}

    Surface::Surface(Surface&& other)
    {
        vk_surface = other.vk_surface;
        other.vk_surface = nullptr;
    }

    Surface::ptr Surface::create(
        const Context::ptr& context,
        VkSurfaceKHR vk_surface
    )
    {
        return std::make_shared<Surface_public_ctor>(context, vk_surface);
    }

    Surface::~Surface()
    {
        vkDestroySurfaceKHR(
            _context->vk_instance(),
            vk_surface,
            _context->vk_allocator_ptr()
        );
    }

    Surface::Surface(
        const Context::ptr& context,
        VkSurfaceKHR vk_surface
    )
        : _context(context), vk_surface(vk_surface)
    {}

    Queue::Queue(Queue&& other)
    {
        vk_queue = other.vk_queue;
        other.vk_queue = nullptr;
    }

    Queue::Queue(VkQueue vk_queue)
        : vk_queue(vk_queue)
    {}

    Device::Device(Device&& other)
    {
        _context = other._context;
        other._context = nullptr;

        _physical_device = other._physical_device;
        other._physical_device = nullptr;

        _config = other._config;
        other._config = DeviceConfig{};

        vk_device = other.vk_device;
        other.vk_device = nullptr;
    }

    Result<Device::ptr>Device::create(
        const Context::ptr& context,
        const PhysicalDevice::ptr& physical_device,
        const DeviceConfig& config
    )
    {
        Device::ptr device = std::make_shared<Device_public_ctor>(
            context,
            physical_device,
            config
        );

        std::vector<VkDeviceQueueCreateInfo> vk_queue_requests;
        for (const auto& queue_request : device->config().queue_requests)
        {
            if (queue_request.priorities.size()
                != queue_request.num_queues_to_create)
            {
                return Error(
                    "there should be the same number of queue priorities as "
                    "the number of queues to create"
                );
            }

            vk_queue_requests.push_back(QueueRequest_to_vk(queue_request));
        }

        std::vector<const char*> layers_cstr;
        {
            layers_cstr.reserve(
                device->context()->config().layers.size()
            );
            for (const auto& layer : device->context()->config().layers)
            {
                layers_cstr.push_back(layer.c_str());
            }
        }

        std::vector<const char*> extensions_cstr;
        {
            extensions_cstr.reserve(
                device->config().extensions.size()
            );
            for (const auto& ext : device->config().extensions)
            {
                extensions_cstr.push_back(ext.c_str());
            }
        }

        VkPhysicalDeviceFeatures vk_enabled_features =
            PhysicalDeviceFeatures_to_vk(
                device->config().enabled_features
            );

        VkDeviceCreateInfo create_info{
            .sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
            .pNext = nullptr,
            .flags = 0,
            .queueCreateInfoCount = (uint32_t)vk_queue_requests.size(),
            .pQueueCreateInfos = vk_queue_requests.data(),
            .enabledLayerCount = (uint32_t)layers_cstr.size(),
            .ppEnabledLayerNames = layers_cstr.data(),
            .enabledExtensionCount = (uint32_t)extensions_cstr.size(),
            .ppEnabledExtensionNames = extensions_cstr.data(),
            .pEnabledFeatures = &vk_enabled_features
        };

        VkResult vk_result = vkCreateDevice(
            device->physical_device()->vk_physical_device,
            &create_info,
            device->context()->vk_allocator_ptr(),
            &device->vk_device
        );
        if (vk_result != VK_SUCCESS)
        {
            return Error(vk_result);
        }
        return device;
    }

    Queue::ptr Device::retrieve_queue(
        uint32_t queue_family_index,
        uint32_t queue_index
    )
    {
        VkQueue vk_queue;
        vkGetDeviceQueue(
            vk_device,
            queue_family_index,
            queue_index,
            &vk_queue
        );
        return std::make_shared<Queue_public_ctor>(vk_queue);
    }

    Device::~Device()
    {
        vkDestroyDevice(vk_device, context()->vk_allocator_ptr());
    }

    Device::Device(
        const Context::ptr& context,
        const PhysicalDevice::ptr& physical_device,
        const DeviceConfig& config
    )
        : _context(context),
        _physical_device(physical_device),
        _config(config)
    {}

    Image::Image(Image&& other)
    {
        vk_image = other.vk_image;
        other.vk_image = nullptr;
    }

    Image::Image(VkImage vk_image)
        : vk_image(vk_image)
    {}

    Swapchain::Swapchain(Swapchain&& other)
    {
        _device = other._device;
        other._device = nullptr;

        _surface = other._surface;
        other._surface = nullptr;

        _config = other._config;
        other._config = SwapchainConfig{};

        _old_swapchain = other._old_swapchain;
        other._old_swapchain = nullptr;

        vk_swapchain = other.vk_swapchain;
        other.vk_swapchain = nullptr;
    }

    Result<Swapchain::ptr> Swapchain::create(
        const Device::ptr& device,
        const Surface::ptr& surface,
        const SwapchainConfig& config,
        const Swapchain::ptr& old_swapchain
    )
    {
        Swapchain::ptr sc = std::make_shared<Swapchain_public_ctor>(
            device,
            surface,
            config,
            old_swapchain
        );

        VkSwapchainKHR vk_old_swapchain = nullptr;
        if (sc->old_swapchain() != nullptr)
        {
            vk_old_swapchain = sc->old_swapchain()->vk_swapchain;
        }

        VkSwapchainCreateInfoKHR create_info{
            .sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
            .pNext = nullptr,
            .flags = SwapchainFlags_to_vk(sc->config().flags),
            .surface = sc->surface()->vk_surface,
            .minImageCount = sc->config().min_image_count,
            .imageFormat = Format_to_vk(sc->config().image_format),
            .imageColorSpace = ColorSpace_to_vk(sc->config().image_color_space),
            .imageExtent = Extent2d_to_vk(sc->config().image_extent),
            .imageArrayLayers = sc->config().image_array_layers,
            .imageUsage = ImageUsageFlags_to_vk(sc->config().image_usage),
            .imageSharingMode = SharingMode_to_vk(
                sc->config().image_sharing_mode
            ),

            .queueFamilyIndexCount =
            (uint32_t)sc->config().queue_family_indices.size(),

            .pQueueFamilyIndices = sc->config().queue_family_indices.data(),
            .preTransform = SurfaceTransform_to_vk(sc->config().pre_transform),
            .compositeAlpha = CompositeAlpha_to_vk(
                sc->config().composite_alpha
            ),
            .presentMode = PresentMode_to_vk(sc->config().present_mode),
            .clipped = sc->config().clipped,
            .oldSwapchain = vk_old_swapchain
        };

        VkResult vk_result = vkCreateSwapchainKHR(
            sc->device()->vk_device,
            &create_info,
            sc->device()->context()->vk_allocator_ptr(),
            &sc->vk_swapchain
        );
        if (vk_result != VK_SUCCESS)
        {
            return Error(vk_result);
        }

        uint32_t actual_image_count;
        vkGetSwapchainImagesKHR(
            sc->device()->vk_device,
            sc->vk_swapchain,
            &actual_image_count,
            nullptr
        );

        std::vector<VkImage> vk_images(actual_image_count);
        vk_result = vkGetSwapchainImagesKHR(
            sc->device()->vk_device,
            sc->vk_swapchain,
            &actual_image_count,
            vk_images.data()
        );
        if (vk_result != VK_SUCCESS && vk_result != VK_INCOMPLETE)
        {
            return Error("failed to retrieve images", vk_result);
        }

        sc->_images.reserve(actual_image_count);
        for (auto vk_image : vk_images)
        {
            sc->_images.push_back(
                std::make_shared<Image_public_ctor>(vk_image)
            );
        }

        return sc;
    }

    Swapchain::~Swapchain()
    {
        vkDestroySwapchainKHR(
            device()->vk_device,
            vk_swapchain,
            device()->context()->vk_allocator_ptr()
        );
    }

    Swapchain::Swapchain(
        const Device::ptr& device,
        const Surface::ptr& surface,
        const SwapchainConfig& config,
        const Swapchain::ptr& old_swapchain
    )
        : _device(device),
        _surface(surface),
        _config(config),
        _old_swapchain(old_swapchain)
    {}

    ImageView::ImageView(ImageView&& other)
    {
        _device = other._device;
        other._device = nullptr;

        _image = other._image;
        other._image = nullptr;

        _config = other._config;
        other._config = ImageViewConfig{};

        vk_image_view = other.vk_image_view;
        other.vk_image_view = nullptr;
    }

    Result<ImageView::ptr> ImageView::create(
        const Device::ptr& device,
        const Image::ptr& image,
        const ImageViewConfig& config
    )
    {
        ImageView::ptr view = std::make_shared<ImageView_public_ctor>(
            device,
            image,
            config
        );

        VkImageViewCreateInfo create_info{
            .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
            .pNext = nullptr,
            .flags = ImageViewFlags_to_vk(view->config().flags),
            .image = view->image()->vk_image,
            .viewType = ImageViewType_to_vk(view->config().view_type),
            .format = Format_to_vk(view->config().format),
            .components = ComponentMapping_to_vk(view->config().components),
            .subresourceRange = ImageSubresourceRange_to_vk(
                view->config().subresource_range
            )
        };

        VkResult vk_result = vkCreateImageView(
            view->device()->vk_device,
            &create_info,
            view->device()->context()->vk_allocator_ptr(),
            &view->vk_image_view
        );
        if (vk_result != VK_SUCCESS)
        {
            return Error(vk_result);
        }
        return view;
    }

    ImageView::~ImageView()
    {
        vkDestroyImageView(
            device()->vk_device,
            vk_image_view,
            device()->context()->vk_allocator_ptr()
        );
    }

    ImageView::ImageView(
        const Device::ptr& device,
        const Image::ptr& image,
        const ImageViewConfig& config
    )
        : _device(device),
        _image(image),
        _config(config)
    {}

#pragma region Vulkan callbacks

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
        return allocator->allocate(
            size,
            alignment,
            AllocationScope_from_vk(vk_allocation_scope)
        );
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
        return allocator->reallocate(
            p_original,
            size,
            alignment,
            AllocationScope_from_vk(vk_allocation_scope)
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
        allocator->internal_allocation_notification(
            size,
            InternalAllocationType_from_vk(vk_allocation_type),
            AllocationScope_from_vk(vk_allocation_scope)
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
        allocator->internal_free_notification(
            size,
            InternalAllocationType_from_vk(vk_allocation_type),
            AllocationScope_from_vk(vk_allocation_scope)
        );
    }

    static VKAPI_ATTR VkBool32 VKAPI_CALL vk_debug_callback(
        VkDebugUtilsMessageSeverityFlagBitsEXT vk_message_severity,
        VkDebugUtilsMessageTypeFlagsEXT vk_message_type_flags,
        const VkDebugUtilsMessengerCallbackDataEXT* p_callback_data,
        void* p_user_data
    )
    {
        if (!p_user_data)
        {
            throw std::runtime_error(
                "Vulkan debug callback called with no user data"
            );
        }

        DebugMessenger* messenger = (DebugMessenger*)p_user_data;
        messenger->callback()(
            DebugMessageSeverity_from_vk(vk_message_severity),
            DebugMessageTypeFlags_from_vk(vk_message_type_flags),
            DebugMessageData_from_vk(*p_callback_data)
            );

        return VK_FALSE;
    }

#pragma endregion

}
