#pragma once

#include <vector>
#include <string>
#include <format>
#include <array>
#include <memory>
#include <any>
#include <optional>
#include <variant>
#include <type_traits>
#include <functional>
#include <stdexcept>
#include <cstdint>

#include "vulkan/vulkan.h"
#include "vulkan/vk_enum_string_helper.h"

namespace beva
{

    template<size_t size, typename T>
    std::array<T, size> raw_arr_to_std(T* raw_arr)
    {
        std::array<T, size> arr;
        std::copy(raw_arr, raw_arr + size, arr.data());
        return arr;
    }

#pragma region enums and flags

    // defines a string conversion function for an enum that meets the following
    // requirements:
    // - enum must have a value named _ at the end
    // - enum must be based on uint8_t
    // - there must be a static constexpr const char* array named
    //   EnumName_string representing the enum values as strings.
#define BEVA_DEFINE_ENUM_TO_STRING_FUNCTION(EnumName) \
    constexpr const char* EnumName##_to_string( \
        EnumName v \
    ) \
    { \
        if (v >= EnumName::_) \
        { \
            throw std::exception( \
                "invalid enum value, this should never happen" \
            ); \
        } \
        return EnumName##_string[(uint8_t)v]; \
    }

    // https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VkResult.html
    enum class ApiResultType : uint8_t
    {
        Success,
        NotReady,
        Timeout,
        EventSet,
        EventReset,
        Incomplete,
        ErrorOutOfHostMemory,
        ErrorOutOfDeviceMemory,
        ErrorInitializationFailed,
        ErrorDeviceLost,
        ErrorMemoryMapFailed,
        ErrorLayerNotPresent,
        ErrorExtensionNotPresent,
        ErrorFeatureNotPresent,
        ErrorIncompatibleDriver,
        ErrorTooManyObjects,
        ErrorFormatNotSupported,
        ErrorFragmentedPool,
        ErrorUnknown,
        ErrorOutOfPoolMemory,
        ErrorInvalidExternalHandle,
        ErrorFragmentation,
        ErrorInvalidOpaqueCaptureAddress,
        PipelineCompileRequired,
        ErrorSurfaceLostKhr,
        ErrorNativeWindowInUseKhr,
        SuboptimalKhr,
        ErrorOutOfDateKhr,
        ErrorIncompatibleDisplayKhr,
        ErrorValidationFailedExt,
        ErrorInvalidShaderNv,
        ErrorImageUsageNotSupportedKhr,
        ErrorVideoPictureLayoutNotSupportedKhr,
        ErrorVideoProfileOperationNotSupportedKhr,
        ErrorVideoProfileFormatNotSupportedKhr,
        ErrorVideoProfileCodecNotSupportedKhr,
        ErrorVideoStdVersionNotSupportedKhr,
        ErrorNotPermittedKhr,
        ErrorFullScreenExclusiveModeLostExt,
        ThreadIdleKhr,
        ThreadDoneKhr,
        OperationDeferredKhr,
        OperationNotDeferredKhr,
        ErrorCompressionExhaustedExt,
        ErrorIncompatibleShaderBinaryExt,
        UndocumentedVkResult,
        _
    };

    static constexpr const char* ApiResultType_string[]
    {
        "Success: command successfully completed",
        "NotReady: a fence or query has not yet completed",
        "Timeout: a wait operation has not completed in the specified time",
        "EventSet: an event is signaled",
        "EventReset: an event is unsignaled",
        "Incomplete: a return array was too small for the result",
        "ErrorOutOfHostMemory: a host memory allocation has failed.",
        "ErrorOutOfDeviceMemory: a device memory allocation has failed.",

        "ErrorInitializationFailed: initialization of an object could not be "
        "completed for implementation-specific reasons.",

        "ErrorDeviceLost: the logical or physical device has been lost.",
        "ErrorMemoryMapFailed: mapping of a memory object has failed.",

        "ErrorLayerNotPresent: a requested layer is not present or could not "
        "be loaded.",

        "ErrorExtensionNotPresent: a requested extension is not supported.",
        "ErrorFeatureNotPresent: a requested feature is not supported.",

        "ErrorIncompatibleDriver: the requested version of Vulkan is not "
        "supported by the driver or is otherwise incompatible for "
        "implementation-specific reasons.",

        "ErrorTooManyObjects: too many objects of the type have already been "
        "created.",

        "ErrorFormatNotSupported: a requested format is not supported on this "
        "device.",

        "ErrorFragmentedPool: a pool allocation has failed due to "
        "fragmentation of the pool’s memory. this must only be returned if no "
        "attempt to allocate host or device memory was made to accommodate the "
        "new allocation. this should be returned in preference to "
        "VK_ERROR_OUT_OF_POOL_MEMORY, but only if the implementation is "
        "certain that the pool allocation failure was due to fragmentation.",

        "ErrorUnknown: an unknown error has occurred; either the application "
        "has provided invalid input, or an implementation failure has "
        "occurred.",

        "ErrorOutOfPoolMemory: a pool memory allocation has failed. this must "
        "only be returned if no attempt to allocate host or device memory was "
        "made to accommodate the new allocation. if the failure was definitely "
        "due to fragmentation of the pool, VK_ERROR_FRAGMENTED_POOL should be "
        "returned instead.",

        "ErrorInvalidExternalHandle: an external handle is not a valid handle "
        "of the specified type.",

        "ErrorFragmentation: a descriptor pool creation has failed due to "
        "fragmentation.",

        "ErrorInvalidOpaqueCaptureAddress: a buffer creation or memory "
        "allocation failed because the requested address is not available. a "
        "shader group handle assignment failed because the requested shader "
        "group handle information is no longer valid.",

        "PipelineCompileRequired: a requested pipeline creation would have "
        "required compilation, but the application requested compilation to "
        "not be performed.",

        "ErrorSurfaceLostKhr: a surface is no longer available.",

        "ErrorNativeWindowInUseKhr: the requested window is already in use by "
        "Vulkan or another API in a manner which prevents it from being used "
        "again.",

        "SuboptimalKhr: a swapchain no longer matches the surface properties "
        "exactly, but can still be used to present to the surface "
        "successfully.",

        "ErrorOutOfDateKhr: a surface has changed in such a way that it is no "
        "longer compatible with the swapchain, and further presentation "
        "requests using the swapchain will fail. applications must query the "
        "new surface properties and recreate their swapchain if they wish to "
        "continue presenting to the surface.",

        "ErrorIncompatibleDisplayKhr: the display used by a swapchain does not "
        "use the same presentable image layout, or is incompatible in a way "
        "that prevents sharing an image.",

        "ErrorValidationFailedExt: a command failed because invalid usage was "
        "detected by the implementation or a validation-layer.",

        "ErrorInvalidShaderNv: one or more shaders failed to compile or link. "
        "more details are reported back to the application via "
        "VK_EXT_debug_report if enabled.",

        "ErrorImageUsageNotSupportedKhr: the requested VkImageUsageFlags are "
        "not supported.",

        "ErrorVideoPictureLayoutNotSupportedKhr: the requested video picture "
        "layout is not supported.",

        "ErrorVideoProfileOperationNotSupportedKhr: a video profile operation "
        "specified via VkVideoProfileInfoKHR::videoCodecOperation is not "
        "supported.",

        "ErrorVideoProfileFormatNotSupportedKhr: format parameters in a "
        "requested VkVideoProfileInfoKHR chain are not supported.",

        "ErrorVideoProfileCodecNotSupportedKhr: codec-specific parameters in a "
        "requested VkVideoProfileInfoKHR chain are not supported.",

        "ErrorVideoStdVersionNotSupportedKhr: the specified video Std header "
        "version is not supported.",

        "ErrorNotPermittedKhr: the driver implementation has denied a request "
        "to acquire a priority above the default priority "
        "(VK_QUEUE_GLOBAL_PRIORITY_MEDIUM_EXT) because the application does "
        "not have sufficient privileges.",

        "ErrorFullScreenExclusiveModeLostExt: an operation on a swapchain "
        "created with VK_FULL_SCREEN_EXCLUSIVE_APPLICATION_CONTROLLED_EXT "
        "failed as it did not have exclusive full-screen access. this may "
        "occur due to implementation-dependent reasons, outside of the "
        "application’s control.",

        "ThreadIdleKhr: a deferred operation is not complete but there is "
        "currently no work for this thread to do at the time of this call.",

        "ThreadDoneKhr: a deferred operation is not complete but there is no "
        "work remaining to assign to additional threads.",

        "OperationDeferredKhr: a deferred operation was requested and at least "
        "some of the work was deferred.",

        "OperationNotDeferredKhr: a deferred operation was requested and no "
        "operations were deferred.",

        "ErrorCompressionExhaustedExt: an image creation failed because "
        "internal resources required for compression are exhausted. this must "
        "only be returned when fixed-rate compression is requested.",

        "ErrorIncompatibleShaderBinaryExt: the provided binary shader code is "
        "not compatible with this device.",
    };

    class ApiResult
    {
    public:
        constexpr ApiResult(VkResult vk_result)
        {
            _undocumented_vk_result = vk_result;
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

        constexpr ApiResultType type() const
        {
            return _type;
        }

        constexpr VkResult undocumented_vk_result() const
        {
            return _undocumented_vk_result;
        }

        std::string to_string() const;

    private:
        ApiResultType _type;
        VkResult _undocumented_vk_result;

    };

    // https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VkSystemAllocationScope.html
    enum class AllocationScope : uint8_t
    {
        Command,
        Object,
        Cache,
        Device,
        Instance,
        Unknown,
        _
    };

    static constexpr const char* AllocationScope_string[]
    {
        "Command",
        "Object",
        "Cache",
        "Device",
        "Instance",
        "Unknown"
    };

    BEVA_DEFINE_ENUM_TO_STRING_FUNCTION(AllocationScope);

    // https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VkInternalAllocationType.html
    enum class InternalAllocationType : uint8_t
    {
        Executable,
        Unknown,
        _
    };

    static constexpr const char* InternalAllocationType_string[]
    {
        "Executable",
        "Unknown"
    };

    BEVA_DEFINE_ENUM_TO_STRING_FUNCTION(InternalAllocationType);

    enum class VulkanApiVersion
    {
        Vulkan1_0,
        Vulkan1_1,
        Vulkan1_2,
        Vulkan1_3,
        _
    };

    static constexpr const char* VulkanApiVersion_string[]
    {
        "Vulkan 1.0",
        "Vulkan 1.1",
        "Vulkan 1.2",
        "Vulkan 1.3"
    };

    BEVA_DEFINE_ENUM_TO_STRING_FUNCTION(VulkanApiVersion);

    // https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VkPhysicalDeviceType.html
    enum class PhysicalDeviceType : uint8_t
    {
        Other,
        IntegratedGpu,
        DiscreteGpu,
        VirtualGpu,
        Cpu,
        _
    };

    static constexpr const char* PhysicalDeviceType_string[]
    {
        "Other",
        "Integrated GPU",
        "Discrete GPU",
        "Virtual GPU",
        "CPU"
    };

    BEVA_DEFINE_ENUM_TO_STRING_FUNCTION(PhysicalDeviceType);

    constexpr PhysicalDeviceType PhysicalDeviceType_from_VkPhysicalDeviceType(
        VkPhysicalDeviceType vk_physical_device_type
    )
    {
        switch (vk_physical_device_type)
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

    // https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VkSampleCountFlagBits.html
    struct SampleCountFlags
    {
        bool _1 : 1;
        bool _2 : 1;
        bool _4 : 1;
        bool _8 : 1;
        bool _16 : 1;
        bool _32 : 1;
        bool _64 : 1;

        SampleCountFlags() = default;

    private:
        constexpr SampleCountFlags(VkSampleCountFlags vk_flags)
            : _1(vk_flags& VK_SAMPLE_COUNT_1_BIT),
            _2(vk_flags& VK_SAMPLE_COUNT_2_BIT),
            _4(vk_flags& VK_SAMPLE_COUNT_4_BIT),
            _8(vk_flags& VK_SAMPLE_COUNT_8_BIT),
            _16(vk_flags& VK_SAMPLE_COUNT_16_BIT),
            _32(vk_flags& VK_SAMPLE_COUNT_32_BIT),
            _64(vk_flags& VK_SAMPLE_COUNT_64_BIT)
        {}

        friend class Context;

    };

    // https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VkSampleCountFlagBits.html
    enum class SampleCount : uint8_t
    {
        _1,
        _2,
        _4,
        _8,
        _16,
        _32,
        _64,
        _
    };

    static constexpr const char* SampleCount_string[]
    {
        "1",
        "2",
        "4",
        "8",
        "16",
        "32",
        "64"
    };

    BEVA_DEFINE_ENUM_TO_STRING_FUNCTION(SampleCount);

    // https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VkQueueFlagBits.html
    struct QueueFlags
    {
        bool graphics : 1;
        bool compute : 1;
        bool transfer : 1;
        bool sparse_binding : 1;
        bool protected_ : 1;
        bool video_decode : 1;
        bool optical_flow_nv : 1;
    };

    // https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VkObjectType.html
    enum class ObjectType : uint8_t
    {
        Unknown,
        Instance,
        PhysicalDevice,
        Device,
        Queue,
        Semaphore,
        CommandBuffer,
        Fence,
        DeviceMemory,
        Buffer,
        Image,
        Event,
        QueryPool,
        BufferView,
        ImageView,
        ShaderModule,
        PipelineCache,
        PipelineLayout,
        RenderPass,
        Pipeline,
        DescriptorSetLayout,
        Sampler,
        DescriptorPool,
        DescriptorSet,
        Framebuffer,
        CommandPool,
        SamplerYcbcrConversion,
        DescriptorUpdateTemplate,
        PrivateDataSlot,
        Surface,
        Swapchain,
        Display,
        DisplayMode,
        DebugReportCallback,
        VideoSession,
        VideoSessionParameters,
        CuModuleNvx,
        CuFunctionNvx,
        DebugMessenger,
        AccelerationStructure,
        ValidationCache,
        AccelerationStructureNv,
        PerformanceConfigurationIntel,
        DeferredOperation,
        IndirectCommandsLayoutNv,
        BufferCollectionFuchsia,
        Micromap,
        OpticalFlowSessionNv,
        Shader,
        _
    };

    static constexpr const char* ObjectType_string[]
    {
        "Unknown",
        "Instance",
        "PhysicalDevice",
        "Device",
        "Queue",
        "Semaphore",
        "CommandBuffer",
        "Fence",
        "DeviceMemory",
        "Buffer",
        "Image",
        "Event",
        "QueryPool",
        "BufferView",
        "ImageView",
        "ShaderModule",
        "PipelineCache",
        "PipelineLayout",
        "RenderPass",
        "Pipeline",
        "DescriptorSetLayout",
        "Sampler",
        "DescriptorPool",
        "DescriptorSet",
        "Framebuffer",
        "CommandPool",
        "SamplerYcbcrConversion",
        "DescriptorUpdateTemplate",
        "PrivateDataSlot",
        "Surface",
        "Swapchain",
        "Display",
        "DisplayMode",
        "DebugReportCallback",
        "VideoSession",
        "VideoSessionParameters",
        "CuModuleNvx",
        "CuFunctionNvx",
        "DebugMessenger",
        "AccelerationStructure",
        "ValidationCache",
        "AccelerationStructureNv",
        "PerformanceConfigurationIntel",
        "DeferredOperation",
        "IndirectCommandsLayoutNv",
        "BufferCollectionFuchsia",
        "Micromap",
        "OpticalFlowSessionNv",
        "Shader"
    };

    BEVA_DEFINE_ENUM_TO_STRING_FUNCTION(ObjectType);

    constexpr ObjectType ObjectType_from_VkObjectType(
        VkObjectType vk_object_type
    )
    {
        switch (vk_object_type)
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

    // https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VkDebugUtilsMessageSeverityFlagBitsEXT.html
    struct DebugMessageSeverityFilter
    {
        bool verbose : 1;
        bool info : 1;
        bool warning : 1;
        bool error : 1;
    };

    // // https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VkDebugUtilsMessageTypeFlagBitsEXT.html
    struct DebugMessageTypeFilter
    {
        bool general : 1;
        bool validation : 1;
        bool performance : 1;
        bool device_address_binding : 1;
    };

    // https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VkDebugUtilsMessageSeverityFlagBitsEXT.html
    enum class DebugMessageSeverity : uint8_t
    {
        Verbose,
        Info,
        Warning,
        Error,
        _
    };

    static constexpr const char* DebugMessageSeverity_string[]
    {
        "Verbose",
        "Info",
        "Warning",
        "Error"
    };

    BEVA_DEFINE_ENUM_TO_STRING_FUNCTION(DebugMessageSeverity);

    // https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VkDebugUtilsMessageTypeFlagBitsEXT.html
    enum class DebugMessageType : uint8_t
    {
        General,
        Validation,
        Performance,
        DeviceAddressBinding,
        _
    };

    static constexpr const char* DebugMessageType_string[]
    {
        "General",
        "Validation",
        "Performance",
        "Device Address Binding"
    };

    BEVA_DEFINE_ENUM_TO_STRING_FUNCTION(DebugMessageType);

#pragma endregion

#pragma region error handling

    class Error
    {
    public:
        constexpr Error()
            : _api_result(VK_SUCCESS)
        {}

        constexpr Error(ApiResult api_result)
            : _api_result(api_result)
        {}

        constexpr ApiResult api_result() const
        {
            return _api_result;
        }

        std::string to_string() const;

    private:
        ApiResult _api_result;

    };

    template<typename T = void>
    class Result
    {
    public:
        Result() = delete;

        Result(const T& value)
            : data(value)
        {}

        Result(T&& value)
            : data(std::move(value))
        {}

        Result(const Error& error)
            : data(error)
        {}

        constexpr bool ok() const
        {
            return std::holds_alternative<T>(data);
        }

        constexpr const T& value() const
        {
            if (!ok())
            {
                throw std::exception(
                    "Result::value() called while there's an error"
                );
            }
            return std::get<T>(data);
        }

        constexpr T& value()
        {
            if (!ok())
            {
                throw std::exception(
                    "Result::value() called while there's an error"
                );
            }
            return std::get<T>(data);
        }

        constexpr const Error& error() const
        {
            if (ok())
            {
                throw std::exception(
                    "Result::error() called while there's no error"
                );
            }
            return std::get<Error>(data);
        }

    private:
        std::variant<T, Error> data;

    };

    template<>
    class Result<void>
    {
    public:
        Result(const Result&) = default;
        Result& operator=(const Result&) = default;

        Result()
            : _error(std::nullopt)
        {}

        Result(const Error& error)
            : _error(error)
        {}

        constexpr bool ok() const
        {
            return !_error.has_value();
        }

        constexpr const Error& error() const
        {
            if (!_error.has_value())
            {
                throw std::exception(
                    "Result::error() called while there's no error"
                );
            }
            return _error.value();
        }

    private:
        std::optional<Error> _error;

    };

#pragma endregion

    // https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VkAllocationCallbacks.html
    class Allocator
    {
    public:
        virtual void* allocate(
            size_t size,
            size_t alignment,
            AllocationScope allocation_scope
        ) = 0;

        virtual void* reallocate(
            void* original,
            size_t size,
            size_t alignment,
            AllocationScope allocation_scope
        ) = 0;

        virtual void free(void* memory) = 0;

        virtual void internal_allocation_notification(
            size_t size,
            InternalAllocationType allocation_type,
            AllocationScope allocation_scope
        ) = 0;

        virtual void internal_free_notification(
            size_t size,
            InternalAllocationType allocation_type,
            AllocationScope allocation_scope
        ) = 0;

    };

    // https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VK_MAKE_API_VERSION.html
    struct Version
    {
        uint8_t variant : 8 = 0;
        uint8_t major : 8 = 0;
        uint8_t minor : 8 = 0;
        uint8_t patch : 8 = 0;

        Version() = default;

        constexpr Version(
            uint8_t variant,
            uint8_t major,
            uint8_t minor,
            uint8_t patch
        )
            : variant(variant), major(major), minor(minor), patch(patch)
        {}

        constexpr Version(uint32_t encoded)
            : variant(VK_API_VERSION_VARIANT(encoded)),
            major(VK_API_VERSION_MAJOR(encoded)),
            minor(VK_API_VERSION_MINOR(encoded)),
            patch(VK_API_VERSION_PATCH(encoded))
        {}

        std::string to_string() const;

    };

    // https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VkExtensionProperties.html
    struct ExtensionProperties
    {
        std::string name;
        uint32_t spec_version;
    };

    // https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VkLayerProperties.html
    struct LayerProperties
    {
        std::string name;
        Version spec_version;
        uint32_t implementation_version;
        std::string description;
    };

    // https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VkPhysicalDeviceLimits.html
    struct PhysicalDeviceLimits
    {
        uint32_t max_image_dimension1d;
        uint32_t max_image_dimension2d;
        uint32_t max_image_dimension3d;
        uint32_t max_image_dimension_cube;
        uint32_t max_image_array_layers;
        uint32_t max_texel_buffer_elements;
        uint32_t max_uniform_buffer_range;
        uint32_t max_storage_buffer_range;
        uint32_t max_push_constants_size;
        uint32_t max_memory_allocation_count;
        uint32_t max_sampler_allocation_count;
        uint64_t buffer_image_granularity;
        uint64_t sparse_address_space_size;
        uint32_t max_bound_descriptor_sets;
        uint32_t max_per_stage_descriptor_samplers;
        uint32_t max_per_stage_descriptor_uniform_buffers;
        uint32_t max_per_stage_descriptor_storage_buffers;
        uint32_t max_per_stage_descriptor_sampled_images;
        uint32_t max_per_stage_descriptor_storage_images;
        uint32_t max_per_stage_descriptor_input_attachments;
        uint32_t max_per_stage_resources;
        uint32_t max_descriptor_set_samplers;
        uint32_t max_descriptor_set_uniform_buffers;
        uint32_t max_descriptor_set_uniform_buffers_dynamic;
        uint32_t max_descriptor_set_storage_buffers;
        uint32_t max_descriptor_set_storage_buffers_dynamic;
        uint32_t max_descriptor_set_sampled_images;
        uint32_t max_descriptor_set_storage_images;
        uint32_t max_descriptor_set_input_attachments;
        uint32_t max_vertex_input_attributes;
        uint32_t max_vertex_input_bindings;
        uint32_t max_vertex_input_attribute_offset;
        uint32_t max_vertex_input_binding_stride;
        uint32_t max_vertex_output_components;
        uint32_t max_tessellation_generation_level;
        uint32_t max_tessellation_patch_size;
        uint32_t max_tessellation_control_per_vertex_input_components;
        uint32_t max_tessellation_control_per_vertex_output_components;
        uint32_t max_tessellation_control_per_patch_output_components;
        uint32_t max_tessellation_control_total_output_components;
        uint32_t max_tessellation_evaluation_input_components;
        uint32_t max_tessellation_evaluation_output_components;
        uint32_t max_geometry_shader_invocations;
        uint32_t max_geometry_input_components;
        uint32_t max_geometry_output_components;
        uint32_t max_geometry_output_vertices;
        uint32_t max_geometry_total_output_components;
        uint32_t max_fragment_input_components;
        uint32_t max_fragment_output_attachments;
        uint32_t max_fragment_dual_src_attachments;
        uint32_t max_fragment_combined_output_resources;
        uint32_t max_compute_shared_memory_size;
        std::array<uint32_t, 3> max_compute_work_group_count;
        uint32_t max_compute_work_group_invocations;
        std::array<uint32_t, 3> max_compute_work_group_size;
        uint32_t sub_pixel_precision_bits;
        uint32_t sub_texel_precision_bits;
        uint32_t mipmap_precision_bits;
        uint32_t max_draw_indexed_index_value;
        uint32_t max_draw_indirect_count;
        float max_sampler_lod_bias;
        float max_sampler_anisotropy;
        uint32_t max_viewports;
        std::array<uint32_t, 2> max_viewport_dimensions;
        std::array<float, 2> viewport_bounds_range;
        uint32_t viewport_sub_pixel_bits;
        size_t min_memory_map_alignment;
        uint64_t min_texel_buffer_offset_alignment;
        uint64_t min_uniform_buffer_offset_alignment;
        uint64_t min_storage_buffer_offset_alignment;
        int32_t min_texel_offset;
        uint32_t max_texel_offset;
        int32_t min_texel_gather_offset;
        uint32_t max_texel_gather_offset;
        float min_interpolation_offset;
        float max_interpolation_offset;
        uint32_t sub_pixel_interpolation_offset_bits;
        uint32_t max_framebuffer_width;
        uint32_t max_framebuffer_height;
        uint32_t max_framebuffer_layers;
        SampleCountFlags framebuffer_color_sample_counts;
        SampleCountFlags framebuffer_depth_sample_counts;
        SampleCountFlags framebuffer_stencil_sample_counts;
        SampleCountFlags framebuffer_no_attachments_sample_counts;
        uint32_t max_color_attachments;
        SampleCountFlags sampled_image_color_sample_counts;
        SampleCountFlags sampled_image_integer_sample_counts;
        SampleCountFlags sampled_image_depth_sample_counts;
        SampleCountFlags sampled_image_stencil_sample_counts;
        SampleCountFlags storage_image_sample_counts;
        uint32_t max_sample_mask_words;
        bool timestamp_compute_and_graphics;
        float timestamp_period;
        uint32_t max_clip_distances;
        uint32_t max_cull_distances;
        uint32_t max_combined_clip_and_cull_distances;
        uint32_t discrete_queue_priorities;
        std::array<float, 2> point_size_range;
        std::array<float, 2> line_width_range;
        float point_size_granularity;
        float line_width_granularity;
        bool strict_lines;
        bool standard_sample_locations;
        uint64_t optimal_buffer_copy_offset_alignment;
        uint64_t optimal_buffer_copy_row_pitch_alignment;
        uint64_t non_coherent_atom_size;
    };

    // https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VkPhysicalDeviceSparseProperties.html
    struct PhysicalDeviceSparseProperties
    {
        bool residency_standard2d_block_shape : 1;
        bool residency_standard2d_multisample_block_shape : 1;
        bool residency_standard3d_block_shape : 1;
        bool residency_aligned_mip_size : 1;
        bool residency_non_resident_strict : 1;
    };

    // https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VkPhysicalDeviceProperties.html
    struct PhysicalDeviceProperties
    {
        Version api_version;
        uint32_t driver_version;
        uint32_t vendor_id;
        uint32_t device_id;
        PhysicalDeviceType device_type;
        std::string device_name;
        std::array<uint8_t, VK_UUID_SIZE> pipeline_cache_uuid;
        PhysicalDeviceLimits limits;
        PhysicalDeviceSparseProperties sparse_properties;
    };

    // https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VkPhysicalDeviceFeatures.html
    struct PhysicalDeviceFeatures
    {
        bool robust_buffer_access : 1 = false;
        bool full_draw_index_uint32 : 1 = false;
        bool image_cube_array : 1 = false;
        bool independent_blend : 1 = false;
        bool geometry_shader : 1 = false;
        bool tessellation_shader : 1 = false;
        bool sample_rate_shading : 1 = false;
        bool dual_src_blend : 1 = false;
        bool logic_op : 1 = false;
        bool multi_draw_indirect : 1 = false;
        bool draw_indirect_first_instance : 1 = false;
        bool depth_clamp : 1 = false;
        bool depth_bias_clamp : 1 = false;
        bool fill_mode_non_solid : 1 = false;
        bool depth_bounds : 1 = false;
        bool wide_lines : 1 = false;
        bool large_points : 1 = false;
        bool alpha_to_one : 1 = false;
        bool multi_viewport : 1 = false;
        bool sampler_anisotropy : 1 = false;
        bool texture_compression_etc2 : 1 = false;
        bool texture_compression_astc_ldr : 1 = false;
        bool texture_compression_bc : 1 = false;
        bool occlusion_query_precise : 1 = false;
        bool pipeline_statistics_query : 1 = false;
        bool vertex_pipeline_stores_and_atomics : 1 = false;
        bool fragment_stores_and_atomics : 1 = false;
        bool shader_tessellation_and_geometry_point_size : 1 = false;
        bool shader_image_gather_extended : 1 = false;
        bool shader_storage_image_extended_formats : 1 = false;
        bool shader_storage_image_multisample : 1 = false;
        bool shader_storage_image_read_without_format : 1 = false;
        bool shader_storage_image_write_without_format : 1 = false;
        bool shader_uniform_buffer_array_dynamic_indexing : 1 = false;
        bool shader_sampled_image_array_dynamic_indexing : 1 = false;
        bool shader_storage_buffer_array_dynamic_indexing : 1 = false;
        bool shader_storage_image_array_dynamic_indexing : 1 = false;
        bool shader_clip_distance : 1 = false;
        bool shader_cull_distance : 1 = false;
        bool shader_float64 : 1 = false;
        bool shader_int64 : 1 = false;
        bool shader_int16 : 1 = false;
        bool shader_resource_residency : 1 = false;
        bool shader_resource_min_lod : 1 = false;
        bool sparse_binding : 1 = false;
        bool sparse_residency_buffer : 1 = false;
        bool sparse_residency_image2d : 1 = false;
        bool sparse_residency_image3d : 1 = false;
        bool sparse_residency2_samples : 1 = false;
        bool sparse_residency4_samples : 1 = false;
        bool sparse_residency8_samples : 1 = false;
        bool sparse_residency16_samples : 1 = false;
        bool sparse_residency_aliased : 1 = false;
        bool variable_multisample_rate : 1 = false;
        bool inherited_queries : 1 = false;
    };

    // https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VkExtent3D.html
    struct Extent3D
    {
        uint32_t width;
        uint32_t height;
        uint32_t depth;

        Extent3D() = default;

    private:
        constexpr Extent3D(const VkExtent3D& vk_extent_3d)
            : width(vk_extent_3d.width),
            height(vk_extent_3d.height),
            depth(vk_extent_3d.depth)
        {}

        friend class Context;

    };

    // https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VkQueueFamilyProperties.html
    struct QueueFamily
    {
        QueueFlags queue_flags;
        uint32_t queue_count;
        uint32_t timestamp_valid_bits;
        Extent3D min_image_transfer_granularity;
    };

    // index of the first queue family that supports the corresponding flag
    struct QueueFamilyIndices
    {
        std::optional<uint32_t> graphics;
        std::optional<uint32_t> compute;
        std::optional<uint32_t> transfer;
        std::optional<uint32_t> sparse_binding;
        std::optional<uint32_t> protected_;
        std::optional<uint32_t> video_decode;
        std::optional<uint32_t> optical_flow_nv;
    };

    // https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VkPhysicalDevice.html
    class PhysicalDevice
    {
    public:
        PhysicalDevice() = delete;

        constexpr const PhysicalDeviceProperties& properties() const
        {
            return _properties;
        }

        constexpr const PhysicalDeviceFeatures& features() const
        {
            return _features;
        }

        constexpr const std::vector<QueueFamily>& queue_families() const
        {
            return _queue_families;
        }

        constexpr const QueueFamilyIndices& queue_family_indices() const
        {
            return _queue_family_indices;
        }

    private:
        VkPhysicalDevice vk_physical_device;

        PhysicalDeviceProperties _properties;
        PhysicalDeviceFeatures _features;
        std::vector<QueueFamily> _queue_families;
        QueueFamilyIndices _queue_family_indices;

        PhysicalDevice(
            VkPhysicalDevice vk_physical_device,
            const PhysicalDeviceProperties& properties,
            const PhysicalDeviceFeatures& features,
            const std::vector<QueueFamily>& queue_families,
            const QueueFamilyIndices& queue_family_indices
        );

        friend class Context;

    };

    struct ContextConfig
    {
        // enables the VK_INSTANCE_CREATE_ENUMERATE_PORTABILITY_BIT_KHR flag
        // which specifies that the instance will enumerate available Vulkan
        // Portability-compliant physical devices and groups in addition to the
        // Vulkan physical devices and groups that are enumerated by default.
        // you might want to enable the
        // VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME extension when using
        // this.
        bool will_enumerate_portability = false;

        std::string app_name;
        Version app_version;

        std::string engine_name;
        Version engine_version;

        VulkanApiVersion vulkan_api_version;

        std::vector<std::string> layers;
        std::vector<std::string> extensions;
    };

    // manages a VkInstance and custom allocators, provides utility functions,
    // and is used by other classes
    // https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VkInstance.html
    class Context
    {
    public:
        Context() = delete;
        Context(const Context& other) = delete;
        Context(Context&& other);

        // * it's best to keep at least one external reference to the allocator
        //   so that it doesn't die with the Context because the driver might
        //   still use the allocator even after the instance is destroyed.
        static Result<std::shared_ptr<Context>> create(
            const ContextConfig& config,
            const std::shared_ptr<Allocator>& allocator = nullptr
        );

        static Result<std::vector<LayerProperties>> available_layers();

        static Result<std::vector<ExtensionProperties>> available_extensions(
            const std::string& layer_name = ""
        );

        constexpr const ContextConfig& config() const
        {
            return _config;
        }

        constexpr const std::shared_ptr<Allocator>& allocator() const
        {
            return _allocator;
        }

        // * it's best to keep at least one external reference to the allocator
        //   so that it doesn't die with the Context because the driver might
        //   still use the allocator even after the instance is destroyed.
        void set_allocator(
            const std::shared_ptr<Allocator>& allocator
        );

        constexpr const std::vector<PhysicalDevice>& physical_devices() const
        {
            return _physical_devices;
        }

        ~Context();

    protected:
        ContextConfig _config;

        std::shared_ptr<Allocator> _allocator = nullptr;
        VkAllocationCallbacks vk_allocator{ 0 };

        VkInstance vk_instance = nullptr;

        std::vector<PhysicalDevice> _physical_devices;

        Context(
            const ContextConfig& config,
            const std::shared_ptr<Allocator>& allocator
        );

        const VkAllocationCallbacks* vk_allocator_ptr() const;

        Result<> add_physical_devices();

        friend class DebugMessenger;

    };

    // https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VkDebugUtilsLabelEXT.html
    struct DebugLabel
    {
        std::string name;
        std::array<float, 4> color;
    };

    // https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VkDebugUtilsObjectNameInfoEXT.html
    struct DebugObjectInfo
    {
        ObjectType type;
        uint64_t handle;
        std::string name;
    };

    // https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VkDebugUtilsMessengerCallbackDataEXT.html
    struct DebugMessageData
    {
        std::string message_id_name;
        int32_t message_id_number;
        std::string message;
        std::vector<DebugLabel> queue_labels;
        std::vector<DebugLabel> cmd_buf_labels;
        std::vector<DebugObjectInfo> objects;
    };

    // https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/PFN_vkDebugUtilsMessengerCallbackEXT.html
    using DebugCallback = std::function<void(
        DebugMessageSeverity,
        DebugMessageType,
        const beva::DebugMessageData&
        )>;

    // * requires extension VK_EXT_DEBUG_UTILS_EXTENSION_NAME
    // https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VkDebugUtilsMessengerEXT.html
    class DebugMessenger
    {
    public:
        DebugMessenger(const DebugMessenger& other) = delete;
        DebugMessenger(DebugMessenger&& other);

        static Result<std::shared_ptr<DebugMessenger>> create(
            const std::shared_ptr<Context>& context,
            DebugMessageSeverityFilter message_severity_filter,
            DebugMessageTypeFilter message_type_filter,
            const DebugCallback& callback
        );

        constexpr const std::shared_ptr<Context>& context() const
        {
            return _context;
        }

        constexpr DebugMessageSeverityFilter message_severity_filter() const
        {
            return _message_severity_filter;
        }

        constexpr DebugMessageTypeFilter message_type_filter() const
        {
            return _message_type_filter;
        }

        constexpr const DebugCallback& callback() const
        {
            return _callback;
        }

        ~DebugMessenger();

    protected:
        std::shared_ptr<Context> _context;
        VkDebugUtilsMessengerEXT vk_debug_messenger;

        DebugMessageSeverityFilter _message_severity_filter;
        DebugMessageTypeFilter _message_type_filter;
        DebugCallback _callback;

        DebugMessenger(
            const std::shared_ptr<Context>& context,
            DebugMessageSeverityFilter message_severity_filter,
            DebugMessageTypeFilter message_type_filter,
            const DebugCallback& callback
        );

    };

}
