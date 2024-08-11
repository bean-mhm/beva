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

#pragma region enums

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

    constexpr const char* AllocationScope_to_string(
        AllocationScope allocation_scope
    )
    {
        if (allocation_scope >= AllocationScope::_)
        {
            throw std::exception(
                "invalid enum value, this should never happen"
            );
        }
        return AllocationScope_string[(uint8_t)allocation_scope];
    }

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

    constexpr const char* InternalAllocationType_to_string(
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

    enum class ApiVersion
    {
        Vulkan1_0,
        Vulkan1_1,
        Vulkan1_2,
        Vulkan1_3
    };

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

    constexpr const char* ObjectType_to_string(ObjectType object_type)
    {
        if (object_type >= ObjectType::_)
        {
            throw std::exception(
                "invalid enum value, this should never happen"
            );
        }
        return ObjectType_string[(uint8_t)object_type];
    }

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

        std::string to_string() const;

    };

    struct ExtensionProperties
    {
        std::string name;
        uint32_t spec_version;
    };

    struct LayerProperties
    {
        std::string name;
        Version spec_version;
        uint32_t implementation_version;
        std::string description;
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

        ApiVersion api_version;

        std::vector<std::string> layers;
        std::vector<std::string> extensions;
    };

    class Context
    {
    public:
        Context() = delete;
        Context(const Context& other) = delete;
        Context(Context&& other);

        // it's best to keep at least one external reference to the allocator
        // so that it doesn't die with the Context because the driver might
        // still use the allocator even after the instance is destroyed and
        // everything is seemingly cleaned up.
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

        // it's best to keep at least one external reference to the allocator
        // so that it doesn't die with the Context because the driver might
        // still use the allocator even after the instance is destroyed and
        // everything is seemingly cleaned up.
        void set_allocator(
            const std::shared_ptr<Allocator>& allocator
        );

        ~Context();

    protected:
        ContextConfig _config;

        std::shared_ptr<Allocator> _allocator = nullptr;
        VkAllocationCallbacks vk_allocator{ 0 };

        VkInstance vk_instance = nullptr;

        Context(
            const ContextConfig& config,
            const std::shared_ptr<Allocator>& allocator
        );

        const VkAllocationCallbacks* vk_allocator_ptr() const;

        friend class DebugMessenger;

    };

    struct DebugMessageSeverityFlags
    {
        // diagnostic message
        bool verbose : 1;

        // informational message like the creation of a resource
        bool info : 1;

        // message about behavior that is not necessarily an error, but very
        // likely a bug in your application
        bool warning : 1;

        // message about behavior that is invalid and may cause crashes
        bool error : 1;
    };

    struct DebugMessageTypeFlags
    {
        // some event has happened that is unrelated to the specification or
        // performance
        bool general : 1;

        // something has happened that violates the specification or indicates a
        // possible mistake
        bool validation : 1;

        // potential non-optimal use of Vulkan
        bool performance : 1;

        // the implementation has modified the set of GPU-visible virtual
        // addresses associated with a Vulkan object
        bool device_address_binding : 1;
    };

    struct DebugLabel
    {
        std::string name;
        std::array<float, 4> color;
    };

    struct DebugObjectInfo
    {
        ObjectType type;
        uint64_t handle;
        std::string name;
    };

    struct DebugMessageData
    {
        std::string message_id_name;
        int32_t message_id_number;
        std::string message;
        std::vector<DebugLabel> queue_labels;
        std::vector<DebugLabel> cmd_buf_labels;
        std::vector<DebugObjectInfo> objects;
    };

    using DebugCallback = std::function<void(
        DebugMessageSeverityFlags,
        DebugMessageTypeFlags,
        DebugMessageData
        )>;

    // requires extension VK_EXT_DEBUG_UTILS_EXTENSION_NAME
    class DebugMessenger
    {
    public:
        DebugMessenger(const DebugMessenger& other) = delete;
        DebugMessenger(DebugMessenger&& other);

        static Result<std::shared_ptr<DebugMessenger>> create(
            const std::shared_ptr<Context>& context,
            DebugMessageSeverityFlags message_severity_flags,
            DebugMessageTypeFlags message_type_flags,
            const DebugCallback& callback
        );

        constexpr const std::shared_ptr<Context>& context() const
        {
            return _context;
        }

        constexpr DebugMessageSeverityFlags message_severity_flags() const
        {
            return _message_severity_flags;
        }

        constexpr DebugMessageTypeFlags message_type_flags() const
        {
            return _message_type_flags;
        }

        constexpr const DebugCallback& callback() const
        {
            return _callback;
        }

        ~DebugMessenger();

    protected:
        std::shared_ptr<Context> _context;
        VkDebugUtilsMessengerEXT vk_debug_messenger;

        DebugMessageSeverityFlags _message_severity_flags;
        DebugMessageTypeFlags _message_type_flags;
        DebugCallback _callback;

        DebugMessenger(
            const std::shared_ptr<Context>& context,
            DebugMessageSeverityFlags message_severity_flags,
            DebugMessageTypeFlags message_type_flags,
            const DebugCallback& callback
        );

    };

}
