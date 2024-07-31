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
#include <stdexcept>
#include <cstdint>

#include "vulkan/vulkan.h"
#include "vulkan/vk_enum_string_helper.h"

namespace beva
{

#pragma region errors and results

    enum class VulkanResultType : uint8_t
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
    static constexpr size_t VulkanResultType_size =
        (uint8_t)VulkanResultType::_;

    static constexpr const char* VulkanResultType_string[]
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

    class VulkanResult
    {
    public:
        constexpr VulkanResult(VkResult vk_result)
        {
            _undocumented_vk_result = vk_result;
            switch (vk_result)
            {
            case VK_NOT_READY:
                _type = VulkanResultType::NotReady;
                break;
            case VK_TIMEOUT:
                _type = VulkanResultType::Timeout;
                break;
            case VK_EVENT_SET:
                _type = VulkanResultType::EventSet;
                break;
            case VK_EVENT_RESET:
                _type = VulkanResultType::EventReset;
                break;
            case VK_INCOMPLETE:
                _type = VulkanResultType::Incomplete;
                break;
            case VK_ERROR_OUT_OF_HOST_MEMORY:
                _type = VulkanResultType::ErrorOutOfHostMemory;
                break;
            case VK_ERROR_OUT_OF_DEVICE_MEMORY:
                _type = VulkanResultType::ErrorOutOfDeviceMemory;
                break;
            case VK_ERROR_INITIALIZATION_FAILED:
                _type = VulkanResultType::ErrorInitializationFailed;
                break;
            case VK_ERROR_DEVICE_LOST:
                _type = VulkanResultType::ErrorDeviceLost;
                break;
            case VK_ERROR_MEMORY_MAP_FAILED:
                _type = VulkanResultType::ErrorMemoryMapFailed;
                break;
            case VK_ERROR_LAYER_NOT_PRESENT:
                _type = VulkanResultType::ErrorLayerNotPresent;
                break;
            case VK_ERROR_EXTENSION_NOT_PRESENT:
                _type = VulkanResultType::ErrorExtensionNotPresent;
                break;
            case VK_ERROR_FEATURE_NOT_PRESENT:
                _type = VulkanResultType::ErrorFeatureNotPresent;
                break;
            case VK_ERROR_INCOMPATIBLE_DRIVER:
                _type = VulkanResultType::ErrorIncompatibleDriver;
                break;
            case VK_ERROR_TOO_MANY_OBJECTS:
                _type = VulkanResultType::ErrorTooManyObjects;
                break;
            case VK_ERROR_FORMAT_NOT_SUPPORTED:
                _type = VulkanResultType::ErrorFormatNotSupported;
                break;
            case VK_ERROR_FRAGMENTED_POOL:
                _type = VulkanResultType::ErrorFragmentedPool;
                break;
            case VK_ERROR_UNKNOWN:
                _type = VulkanResultType::ErrorUnknown;
                break;
            case VK_ERROR_OUT_OF_POOL_MEMORY:
                _type = VulkanResultType::ErrorOutOfPoolMemory;
                break;
            case VK_ERROR_INVALID_EXTERNAL_HANDLE:
                _type = VulkanResultType::ErrorInvalidExternalHandle;
                break;
            case VK_ERROR_FRAGMENTATION:
                _type = VulkanResultType::ErrorFragmentation;
                break;
            case VK_ERROR_INVALID_OPAQUE_CAPTURE_ADDRESS:
                _type = VulkanResultType::ErrorInvalidOpaqueCaptureAddress;
                break;
            case VK_PIPELINE_COMPILE_REQUIRED:
                _type = VulkanResultType::PipelineCompileRequired;
                break;
            case VK_ERROR_SURFACE_LOST_KHR:
                _type = VulkanResultType::ErrorSurfaceLostKhr;
                break;
            case VK_ERROR_NATIVE_WINDOW_IN_USE_KHR:
                _type = VulkanResultType::ErrorNativeWindowInUseKhr;
                break;
            case VK_SUBOPTIMAL_KHR:
                _type = VulkanResultType::SuboptimalKhr;
                break;
            case VK_ERROR_OUT_OF_DATE_KHR:
                _type = VulkanResultType::ErrorOutOfDateKhr;
                break;
            case VK_ERROR_INCOMPATIBLE_DISPLAY_KHR:
                _type = VulkanResultType::ErrorIncompatibleDisplayKhr;
                break;
            case VK_ERROR_VALIDATION_FAILED_EXT:
                _type = VulkanResultType::ErrorValidationFailedExt;
                break;
            case VK_ERROR_INVALID_SHADER_NV:
                _type = VulkanResultType::ErrorInvalidShaderNv;
                break;
            case VK_ERROR_IMAGE_USAGE_NOT_SUPPORTED_KHR:
                _type = VulkanResultType::ErrorImageUsageNotSupportedKhr;
                break;
            case VK_ERROR_VIDEO_PICTURE_LAYOUT_NOT_SUPPORTED_KHR:
                _type =
                    VulkanResultType::ErrorVideoPictureLayoutNotSupportedKhr;
                break;
            case VK_ERROR_VIDEO_PROFILE_OPERATION_NOT_SUPPORTED_KHR:
                _type =
                    VulkanResultType::ErrorVideoProfileOperationNotSupportedKhr;
                break;
            case VK_ERROR_VIDEO_PROFILE_FORMAT_NOT_SUPPORTED_KHR:
                _type =
                    VulkanResultType::ErrorVideoProfileFormatNotSupportedKhr;
                break;
            case VK_ERROR_VIDEO_PROFILE_CODEC_NOT_SUPPORTED_KHR:
                _type = VulkanResultType::ErrorVideoProfileCodecNotSupportedKhr;
                break;
            case VK_ERROR_VIDEO_STD_VERSION_NOT_SUPPORTED_KHR:
                _type = VulkanResultType::ErrorVideoStdVersionNotSupportedKhr;
                break;
            case VK_ERROR_NOT_PERMITTED_KHR:
                _type = VulkanResultType::ErrorNotPermittedKhr;
                break;
            case VK_ERROR_FULL_SCREEN_EXCLUSIVE_MODE_LOST_EXT:
                _type = VulkanResultType::ErrorFullScreenExclusiveModeLostExt;
                break;
            case VK_THREAD_IDLE_KHR:
                _type = VulkanResultType::ThreadIdleKhr;
                break;
            case VK_THREAD_DONE_KHR:
                _type = VulkanResultType::ThreadDoneKhr;
                break;
            case VK_OPERATION_DEFERRED_KHR:
                _type = VulkanResultType::OperationDeferredKhr;
                break;
            case VK_OPERATION_NOT_DEFERRED_KHR:
                _type = VulkanResultType::OperationNotDeferredKhr;
                break;
            case VK_ERROR_COMPRESSION_EXHAUSTED_EXT:
                _type = VulkanResultType::ErrorCompressionExhaustedExt;
                break;
            case VK_ERROR_INCOMPATIBLE_SHADER_BINARY_EXT:
                _type = VulkanResultType::ErrorIncompatibleShaderBinaryExt;
                break;
            default:
                _type = VulkanResultType::UndocumentedVkResult;
                break;
            }
        }

        constexpr VulkanResultType type() const
        {
            return _type;
        }

        constexpr VkResult undocumented_vk_result() const
        {
            return _undocumented_vk_result;
        }

        std::string to_string() const;

    private:
        VulkanResultType _type;
        VkResult _undocumented_vk_result;

    };

    class Error
    {
    public:
        constexpr Error()
            : _vulkan_result(VK_SUCCESS)
        {}

        constexpr Error(VulkanResult vulkan_result)
            : _vulkan_result(vulkan_result)
        {}

        constexpr VulkanResult vulkan_result() const
        {
            return _vulkan_result;
        }

        std::string to_string() const;

    private:
        VulkanResult _vulkan_result;

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

    class Context;

    enum class AllocationScope : uint8_t
    {
        Command,
        Object,
        Cache,
        Device,
        Instance,
        Unknown
    };

    enum class InternalAllocationType : uint8_t
    {
        Executable,
        Unknown
    };

    class Allocator
    {
    public:
        virtual void* allocate(
            Context& context,
            size_t size,
            size_t alignment,
            AllocationScope allocation_scope
        ) = 0;

        virtual void* reallocate(
            Context& context,
            void* original,
            size_t size,
            size_t alignment,
            AllocationScope allocation_scope
        ) = 0;

        virtual void free(Context& context, void* memory) = 0;

        virtual void internal_allocation_notification(
            Context& context,
            size_t size,
            InternalAllocationType allocation_type,
            AllocationScope allocation_scope
        ) = 0;

        virtual void internal_free_notification(
            Context& context,
            size_t size,
            InternalAllocationType allocation_type,
            AllocationScope allocation_scope
        ) = 0;

    };

    struct ExtensionProperties
    {
        std::string name;
        uint32_t spec_version;
    };

    struct Version
    {
        uint8_t variant : 8;
        uint8_t major : 8;
        uint8_t minor : 8;
        uint8_t patch : 8;

        constexpr Version(
            uint8_t variant = 0,
            uint8_t major = 0,
            uint8_t minor = 0,
            uint8_t patch = 0
        )
            : variant(variant), major(major), minor(minor), patch(patch)
        {}

        std::string to_string() const;

    };

    enum class ApiVersion
    {
        Vulkan1_0,
        Vulkan1_1,
        Vulkan1_2,
        Vulkan1_3
    };

    struct ContextConfig
    {
        std::string app_name;
        Version app_version;

        std::string engine_name;
        Version engine_version;

        ApiVersion api_version;
        std::vector<std::string> required_extensions;

        // enables the VK_INSTANCE_CREATE_ENUMERATE_PORTABILITY_BIT_KHR flag
        // which specifies that the instance will enumerate available Vulkan
        // Portability-compliant physical devices and groups in addition to the
        // Vulkan physical devices and groups that are enumerated by default.
        bool will_enumerate_portability = false;
    };

    class Context
    {
    public:
        Context() = delete;
        Context(Context&& other);

        static Result<Context> create(const ContextConfig& config);
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

        void set_allocator(
            const std::shared_ptr<Allocator>& allocator
        );

        ~Context();

    private:
        ContextConfig _config;

        std::shared_ptr<Allocator> _allocator = nullptr;
        VkAllocationCallbacks vk_allocator{ 0 };

        VkInstance instance = nullptr;

        Context(const ContextConfig& config);

        const VkAllocationCallbacks* vk_allocator_ptr() const;

    };

}
