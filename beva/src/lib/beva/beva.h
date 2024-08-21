#pragma once

#include <vector>
#include <string>
#include <format>
#include <array>
#include <unordered_map>
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

namespace bv
{

#pragma region data-only structs, enums, and type aliases

    using Flags = uint32_t;

    template<typename Enum>
    using EnumStrMap = const std::unordered_map<Enum, std::string>;

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

        constexpr uint32_t encode() const
        {
            return VK_MAKE_API_VERSION(variant, major, minor, patch);
        }

    };

    // https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VkResult.html
    enum class ApiResult : int32_t
    {
        Success = 0,
        NotReady = 1,
        Timeout = 2,
        EventSet = 3,
        EventReset = 4,
        Incomplete = 5,
        ErrorOutOfHostMemory = -1,
        ErrorOutOfDeviceMemory = -2,
        ErrorInitializationFailed = -3,
        ErrorDeviceLost = -4,
        ErrorMemoryMapFailed = -5,
        ErrorLayerNotPresent = -6,
        ErrorExtensionNotPresent = -7,
        ErrorFeatureNotPresent = -8,
        ErrorIncompatibleDriver = -9,
        ErrorTooManyObjects = -10,
        ErrorFormatNotSupported = -11,
        ErrorFragmentedPool = -12,
        ErrorUnknown = -13,

        // provided by VK_VERSION_1_1
        ErrorOutOfPoolMemory = -1000069000,

        // provided by VK_VERSION_1_1
        ErrorInvalidExternalHandle = -1000072003,

        // provided by VK_VERSION_1_2
        ErrorFragmentation = -1000161000,

        // provided by VK_VERSION_1_2
        ErrorInvalidOpaqueCaptureAddress = -1000257000,

        // provided by VK_VERSION_1_3
        PipelineCompileRequired = 1000297000,

        // provided by VK_KHR_surface
        ErrorSurfaceLostKhr = -1000000000,

        // provided by VK_KHR_surface
        ErrorNativeWindowInUseKhr = -1000000001,

        // provided by VK_KHR_swapchain
        SuboptimalKhr = 1000001003,

        // provided by VK_KHR_swapchain
        ErrorOutOfDateKhr = -1000001004,

        // provided by VK_KHR_display_swapchain
        ErrorIncompatibleDisplayKhr = -1000003001,

        // provided by VK_EXT_debug_report
        ErrorValidationFailedExt = -1000011001,

        // provided by VK_NV_glsl_shader
        ErrorInvalidShaderNv = -1000012000,

        // provided by VK_KHR_video_queue
        ErrorImageUsageNotSupportedKhr = -1000023000,

        // provided by VK_KHR_video_queue
        ErrorVideoPictureLayoutNotSupportedKhr = -1000023001,

        // provided by VK_KHR_video_queue
        ErrorVideoProfileOperationNotSupportedKhr = -1000023002,

        // provided by VK_KHR_video_queue
        ErrorVideoProfileFormatNotSupportedKhr = -1000023003,

        // provided by VK_KHR_video_queue
        ErrorVideoProfileCodecNotSupportedKhr = -1000023004,

        // provided by VK_KHR_video_queue
        ErrorVideoStdVersionNotSupportedKhr = -1000023005,

        // provided by VK_EXT_image_drm_format_modifier
        ErrorInvalidDrmFormatModifierPlaneLayoutExt = -1000158000,

        // provided by VK_KHR_global_priority
        ErrorNotPermittedKhr = -1000174001,

        // provided by VK_EXT_full_screen_exclusive
        ErrorFullScreenExclusiveModeLostExt = -1000255000,

        // provided by VK_KHR_deferred_host_operations
        ThreadIdleKhr = 1000268000,

        // provided by VK_KHR_deferred_host_operations
        ThreadDoneKhr = 1000268001,

        // provided by VK_KHR_deferred_host_operations
        OperationDeferredKhr = 1000268002,

        // provided by VK_KHR_deferred_host_operations
        OperationNotDeferredKhr = 1000268003,

        // provided by VK_KHR_video_encode_queue
        ErrorInvalidVideoStdParametersKhr = -1000299000,

        // provided by VK_EXT_image_compression_control
        ErrorCompressionExhaustedExt = -1000338000,

        // provided by VK_EXT_shader_object
        IncompatibleShaderBinaryExt = 1000482000,

        // provided by VK_KHR_maintenance1
        ErrorOutOfPoolMemoryKhr = ErrorOutOfPoolMemory,

        // provided by VK_KHR_external_memory
        ErrorInvalidExternalHandleKhr = ErrorInvalidExternalHandle,

        // provided by VK_EXT_descriptor_indexing
        ErrorFragmentationExt = ErrorFragmentation,

        // provided by VK_EXT_global_priority
        ErrorNotPermittedExt = ErrorNotPermittedKhr,

        // provided by VK_EXT_buffer_device_address
        ErrorInvalidDeviceAddressExt = ErrorInvalidOpaqueCaptureAddress,

        // provided by VK_KHR_buffer_device_address
        ErrorInvalidOpaqueCaptureAddressKhr = ErrorInvalidOpaqueCaptureAddress,

        // provided by VK_EXT_pipeline_creation_cache_control
        PipelineCompileRequiredExt = PipelineCompileRequired,

        // provided by VK_EXT_pipeline_creation_cache_control
        ErrorPipelineCompileRequiredExt = PipelineCompileRequired,

        // provided by VK_EXT_shader_object
        // VK_ERROR_INCOMPATIBLE_SHADER_BINARY_EXT is a deprecated alias
        ErrorIncompatibleShaderBinaryExt = IncompatibleShaderBinaryExt
    };

    static EnumStrMap<ApiResult> ApiResult_strmap{
        {
            ApiResult::Success,
            "Success: command successfully completed"
        },
{
    ApiResult::NotReady,
    "NotReady: a fence or query has not yet completed"
},
{
    ApiResult::Timeout,
    "Timeout: a wait operation has not completed in the specified time"
},
{
    ApiResult::EventSet,
    "EventSet: an event is signaled"
},
{
    ApiResult::EventReset,
    "EventReset: an event is unsignaled"
},
{
    ApiResult::Incomplete,
    "Incomplete: a return array was too small for the result"
},
{
    ApiResult::ErrorOutOfHostMemory,
    "ErrorOutOfHostMemory: a host memory allocation has failed"
},
{
    ApiResult::ErrorOutOfDeviceMemory,
    "ErrorOutOfDeviceMemory: a device memory allocation has failed"
},
{
    ApiResult::ErrorInitializationFailed,
    "ErrorInitializationFailed: initialization of an object could not be "
    "completed for implementation-specific reasons."
},
{
    ApiResult::ErrorDeviceLost,
    "ErrorDeviceLost: the logical or physical device has been lost"
},
{
    ApiResult::ErrorMemoryMapFailed,
    "ErrorMemoryMapFailed: mapping of a memory object has failed"
},
{
    ApiResult::ErrorLayerNotPresent,
    "ErrorLayerNotPresent: a requested layer is not present or could not be "
    "loaded"
},
{
    ApiResult::ErrorExtensionNotPresent,
    "ErrorExtensionNotPresent: a requested extension is not supported"
},
{
    ApiResult::ErrorFeatureNotPresent,
    "ErrorFeatureNotPresent: a requested feature is not supported"
},
{
    ApiResult::ErrorIncompatibleDriver,
    "ErrorIncompatibleDriver: the requested version of Vulkan is not supported "
    "by the driver or is otherwise incompatible for implementation-specific "
        "reasons."
},
{
    ApiResult::ErrorTooManyObjects,
    "ErrorTooManyObjects: too many objects of the type have already been "
    "created"
},
{
    ApiResult::ErrorFormatNotSupported,
    "ErrorFormatNotSupported: a requested format is not supported on this "
    "device"
},
{
    ApiResult::ErrorFragmentedPool,
    "ErrorFragmentedPool: a pool allocation has failed due to fragmentation of "
    "the pool’s memory. this must only be returned if no attempt to allocate "
        "host or device memory was made to accommodate the new allocation. "
        "this should be returned in preference to VK_ERROR_OUT_OF_POOL_MEMORY, "
        "but only if the implementation is certain that the pool allocation "
        "failure was due to fragmentation."
},
{
    ApiResult::ErrorUnknown,
    "ErrorUnknown: an unknown error has occurred; either the application has "
    "provided invalid input, or an implementation failure has occurred."
},
{
    ApiResult::ErrorOutOfPoolMemory,
    "ErrorOutOfPoolMemory: a pool memory allocation has failed. this must only "
    "be returned if no attempt to allocate host or device memory was made to "
        "accommodate the new allocation. if the failure was definitely due to "
        "fragmentation of the pool, VK_ERROR_FRAGMENTED_POOL should be "
        "returned instead."
},
{
    ApiResult::ErrorInvalidExternalHandle,
    "ErrorInvalidExternalHandle: an external handle is not a valid handle of "
    "the specified type."
},
{
    ApiResult::ErrorFragmentation,
    "ErrorFragmentation: a descriptor pool creation has failed due to "
    "fragmentation"
},
{
    ApiResult::ErrorInvalidOpaqueCaptureAddress,
    "ErrorInvalidOpaqueCaptureAddress: a buffer creation or memory allocation "
    "failed because the requested address is not available. a shader group "
        "handle assignment failed because the requested shader group handle "
        "information is no longer valid."
},
{
    ApiResult::PipelineCompileRequired,
    "PipelineCompileRequired: a requested pipeline creation would have "
    "required compilation, but the application requested compilation to not be "
        "performed."
},
{
    ApiResult::ErrorSurfaceLostKhr,
    "ErrorSurfaceLostKhr: a surface is no longer available"
},
{
    ApiResult::ErrorNativeWindowInUseKhr,
    "ErrorNativeWindowInUseKhr: the requested window is already in use by "
    "Vulkan or another API in a manner which prevents it from being used again."
},
{
    ApiResult::SuboptimalKhr,
    "SuboptimalKhr: a swapchain no longer matches the surface properties "
    "exactly, but can still be used to present to the surface successfully."
},
{
    ApiResult::ErrorOutOfDateKhr,
    "ErrorOutOfDateKhr: a surface has changed in such a way that it is no "
    "longer compatible with the swapchain, and further presentation requests "
        "using the swapchain will fail. applications must query the new surface "
        "properties and recreate their swapchain if they wish to continue "
        "presenting to the surface."
},
{
    ApiResult::ErrorIncompatibleDisplayKhr,
    "ErrorIncompatibleDisplayKhr: the display used by a swapchain does not use "
    "the same presentable image layout, or is incompatible in a way that "
        "prevents sharing an image."
},
{
    ApiResult::ErrorValidationFailedExt,
    "ErrorValidationFailedExt: a command failed because invalid usage was "
    "detected by the implementation or a validation-layer."
},
{
    ApiResult::ErrorInvalidShaderNv,
    "ErrorInvalidShaderNv: one or more shaders failed to compile or link. more "
    "details are reported back to the application via VK_EXT_debug_report if "
        "enabled."
},
{
    ApiResult::ErrorImageUsageNotSupportedKhr,
    "ErrorImageUsageNotSupportedKhr: the requested VkImageUsageFlags are not "
    "supported"
},
{
    ApiResult::ErrorVideoPictureLayoutNotSupportedKhr,
    "ErrorVideoPictureLayoutNotSupportedKhr: the requested video picture "
    "layout is not supported."
},
{
    ApiResult::ErrorVideoProfileOperationNotSupportedKhr,
    "ErrorVideoProfileOperationNotSupportedKhr: a video profile operation "
    "specified via VkVideoProfileInfoKHR::videoCodecOperation is not supported."
},
{
    ApiResult::ErrorVideoProfileFormatNotSupportedKhr,
    "ErrorVideoProfileFormatNotSupportedKhr: format parameters in a requested "
    "VkVideoProfileInfoKHR chain are not supported."
},
{
    ApiResult::ErrorVideoProfileCodecNotSupportedKhr,
    "ErrorVideoProfileCodecNotSupportedKhr: codec-specific parameters in a "
    "requested VkVideoProfileInfoKHR chain are not supported."
},
{
    ApiResult::ErrorVideoStdVersionNotSupportedKhr,
    "ErrorVideoStdVersionNotSupportedKhr: the specified video Std header "
    "version is not supported."
},
{
    ApiResult::ErrorInvalidDrmFormatModifierPlaneLayoutExt,
    "ErrorInvalidDrmFormatModifierPlaneLayoutExt"
},
{
    ApiResult::ErrorNotPermittedKhr,
    "ErrorNotPermittedKhr: the driver implementation has denied a request to "
    "acquire a priority above the default priority "
        "(VK_QUEUE_GLOBAL_PRIORITY_MEDIUM_EXT) because the application does not "
        "have sufficient privileges."
},
{
    ApiResult::ErrorFullScreenExclusiveModeLostExt,
    "ErrorFullScreenExclusiveModeLostExt: an operation on a swapchain created "
    "with VK_FULL_SCREEN_EXCLUSIVE_APPLICATION_CONTROLLED_EXT failed as it did "
        "not have exclusive full-screen access. this may occur due to "
        "implementation-dependent reasons, outside of the application’s control."
},
{
    ApiResult::ThreadIdleKhr,
    "ThreadIdleKhr: a deferred operation is not complete but there is "
    "currently no work for this thread to do at the time of this call."
},
{
    ApiResult::ThreadDoneKhr,
    "ThreadDoneKhr: a deferred operation is not complete but there is no work "
    "remaining to assign to additional threads."
},
{
    ApiResult::OperationDeferredKhr,
    "OperationDeferredKhr: a deferred operation was requested and at least "
    "some of the work was deferred."
},
{
    ApiResult::OperationNotDeferredKhr,
    "OperationNotDeferredKhr: a deferred operation was requested and no "
    "operations were deferred."
},
{
    ApiResult::ErrorInvalidVideoStdParametersKhr,
    "ErrorInvalidVideoStdParametersKhr: the specified Video Std parameters do "
    "not adhere to the syntactic or semantic requirements of the used video "
        "compression standard, or values derived from parameters according to the "
        "rules defined by the used video compression standard do not adhere to the "
        "capabilities of the video compression standard or the implementation."
},
{
    ApiResult::ErrorCompressionExhaustedExt,
    "ErrorCompressionExhaustedExt: an image creation failed because internal "
    "resources required for compression are exhausted. this must only be "
        "returned when fixed-rate compression is requested."
},
{
    ApiResult::IncompatibleShaderBinaryExt,
    "IncompatibleShaderBinaryExt: the provided binary shader code is not "
    "compatible with this device."
}
    };

    std::string ApiResult_to_string(ApiResult result);

    // https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VkSystemAllocationScope.html
    enum class AllocationScope : int32_t
    {
        Command = 0,
        Object = 1,
        Cache = 2,
        Device = 3,
        Instance = 4
    };

    static EnumStrMap<AllocationScope> AllocationScope_strmap{
        { AllocationScope::Command, "Command" },
        { AllocationScope::Object, "Object" },
        { AllocationScope::Cache, "Cache" },
        { AllocationScope::Device, "Device" },
        { AllocationScope::Instance, "Instance" }
    };

    // https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VkInternalAllocationType.html
    enum class InternalAllocationType : int32_t
    {
        Executable = 0
    };

    static EnumStrMap<InternalAllocationType> InternalAllocationType_strmap{
        { InternalAllocationType::Executable, "Executable" }
    };

    enum class VulkanApiVersion : int32_t
    {
        Vulkan1_0,
        Vulkan1_1,
        Vulkan1_2,
        Vulkan1_3
    };

    static EnumStrMap<VulkanApiVersion> VulkanApiVersion_strmap{
        { VulkanApiVersion::Vulkan1_0, "Vulkan 1.0" },
        { VulkanApiVersion::Vulkan1_1, "Vulkan 1.1" },
        { VulkanApiVersion::Vulkan1_2, "Vulkan 1.2" },
        { VulkanApiVersion::Vulkan1_3, "Vulkan 1.3" }
    };

    uint32_t VulkanApiVersion_encode(VulkanApiVersion version);

    // https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VkPhysicalDeviceType.html
    enum class PhysicalDeviceType : int32_t
    {
        Other = 0,
        IntegratedGpu = 1,
        DiscreteGpu = 2,
        VirtualGpu = 3,
        Cpu = 4
    };

    static EnumStrMap<PhysicalDeviceType> PhysicalDeviceType_strmap{
        { PhysicalDeviceType::Other, "Other" },
        { PhysicalDeviceType::IntegratedGpu, "Integrated GPU" },
        { PhysicalDeviceType::DiscreteGpu, "Discrete GPU" },
        { PhysicalDeviceType::VirtualGpu, "Virtual GPU" },
        { PhysicalDeviceType::Cpu, "CPU" }
    };

    // https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VkSampleCountFlagBits.html
    struct SampleCountFlags
    {
        bool _1 : 1 = false;
        bool _2 : 1 = false;
        bool _4 : 1 = false;
        bool _8 : 1 = false;
        bool _16 : 1 = false;
        bool _32 : 1 = false;
        bool _64 : 1 = false;
    };

    // https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VkQueueFlagBits.html
    struct QueueFlags
    {
        bool graphics : 1 = false;
        bool compute : 1 = false;
        bool transfer : 1 = false;
        bool sparse_binding : 1 = false;
        bool protected_ : 1 = false;
        bool video_decode : 1 = false;
        bool optical_flow_nv : 1 = false;
    };

    // https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VkObjectType.html
    enum class ObjectType : int32_t
    {
        Unknown = 0,
        Instance = 1,
        PhysicalDevice = 2,
        Device = 3,
        Queue = 4,
        Semaphore = 5,
        CommandBuffer = 6,
        Fence = 7,
        DeviceMemory = 8,
        Buffer = 9,
        Image = 10,
        Event = 11,
        QueryPool = 12,
        BufferView = 13,
        ImageView = 14,
        ShaderModule = 15,
        PipelineCache = 16,
        PipelineLayout = 17,
        RenderPass = 18,
        Pipeline = 19,
        DescriptorSetLayout = 20,
        Sampler = 21,
        DescriptorPool = 22,
        DescriptorSet = 23,
        Framebuffer = 24,
        CommandPool = 25,

        // provided by VK_VERSION_1_1
        SamplerYcbcrConversion = 1000156000,

        // provided by VK_VERSION_1_1
        DescriptorUpdateTemplate = 1000085000,

        // provided by VK_VERSION_1_3
        PrivateDataSlot = 1000295000,

        // provided by VK_KHR_surface
        SurfaceKhr = 1000000000,

        // provided by VK_KHR_swapchain
        SwapchainKhr = 1000001000,

        // provided by VK_KHR_display
        DisplayKhr = 1000002000,

        // provided by VK_KHR_display
        DisplayModeKhr = 1000002001,

        // provided by VK_EXT_debug_report
        DebugReportCallbackExt = 1000011000,

        // provided by VK_KHR_video_queue
        VideoSessionKhr = 1000023000,

        // provided by VK_KHR_video_queue
        VideoSessionParametersKhr = 1000023001,

        // provided by VK_NVX_binary_import
        CuModuleNvx = 1000029000,

        // provided by VK_NVX_binary_import
        CuFunctionNvx = 1000029001,

        // provided by VK_EXT_debug_utils
        DebugUtilsMessengerExt = 1000128000,

        // provided by VK_KHR_acceleration_structure
        AccelerationStructureKhr = 1000150000,

        // provided by VK_EXT_validation_cache
        ValidationCacheExt = 1000160000,

        // provided by VK_NV_ray_tracing
        AccelerationStructureNv = 1000165000,

        // provided by VK_INTEL_performance_query
        PerformanceConfigurationIntel = 1000210000,

        // provided by VK_KHR_deferred_host_operations
        DeferredOperationKhr = 1000268000,

        // provided by VK_NV_device_generated_commands
        IndirectCommandsLayoutNv = 1000277000,

        // provided by VK_NV_cuda_kernel_launch
        CudaModuleNv = 1000307000,

        // provided by VK_NV_cuda_kernel_launch
        CudaFunctionNv = 1000307001,

        // provided by VK_FUCHSIA_buffer_collection
        BufferCollectionFuchsia = 1000366000,

        // provided by VK_EXT_opacity_micromap
        MicromapExt = 1000396000,

        // provided by VK_NV_optical_flow
        OpticalFlowSessionNv = 1000464000,

        // provided by VK_EXT_shader_object
        ShaderExt = 1000482000,

        // provided by VK_KHR_descriptor_update_template
        DescriptorUpdateTemplateKhr = DescriptorUpdateTemplate,

        // provided by VK_KHR_sampler_ycbcr_conversion
        SamplerYcbcrConversionKhr = SamplerYcbcrConversion,

        // provided by VK_EXT_private_data
        PrivateDataSlotExt = PrivateDataSlot
    };

    static EnumStrMap<ObjectType> ObjectType_strmap
    {
        { ObjectType::Unknown, "Unknown" },
        { ObjectType::Instance, "Instance" },
        { ObjectType::PhysicalDevice, "PhysicalDevice" },
        { ObjectType::Device, "Device" },
        { ObjectType::Queue, "Queue" },
        { ObjectType::Semaphore, "Semaphore" },
        { ObjectType::CommandBuffer, "CommandBuffer" },
        { ObjectType::Fence, "Fence" },
        { ObjectType::DeviceMemory, "DeviceMemory" },
        { ObjectType::Buffer, "Buffer" },
        { ObjectType::Image, "Image" },
        { ObjectType::Event, "Event" },
        { ObjectType::QueryPool, "QueryPool" },
        { ObjectType::BufferView, "BufferView" },
        { ObjectType::ImageView, "ImageView" },
        { ObjectType::ShaderModule, "ShaderModule" },
        { ObjectType::PipelineCache, "PipelineCache" },
        { ObjectType::PipelineLayout, "PipelineLayout" },
        { ObjectType::RenderPass, "RenderPass" },
        { ObjectType::Pipeline, "Pipeline" },
        { ObjectType::DescriptorSetLayout, "DescriptorSetLayout" },
        { ObjectType::Sampler, "Sampler" },
        { ObjectType::DescriptorPool, "DescriptorPool" },
        { ObjectType::DescriptorSet, "DescriptorSet" },
        { ObjectType::Framebuffer, "Framebuffer" },
        { ObjectType::CommandPool, "CommandPool" },
        { ObjectType::SamplerYcbcrConversion, "SamplerYcbcrConversion" },
        { ObjectType::DescriptorUpdateTemplate, "DescriptorUpdateTemplate" },
        { ObjectType::PrivateDataSlot, "PrivateDataSlot" },
        { ObjectType::SurfaceKhr, "SurfaceKhr" },
        { ObjectType::SwapchainKhr, "SwapchainKhr" },
        { ObjectType::DisplayKhr, "DisplayKhr" },
        { ObjectType::DisplayModeKhr, "DisplayModeKhr" },
        { ObjectType::DebugReportCallbackExt, "DebugReportCallbackExt" },
        { ObjectType::VideoSessionKhr, "VideoSessionKhr" },
        { ObjectType::VideoSessionParametersKhr, "VideoSessionParametersKhr" },
        { ObjectType::CuModuleNvx, "CuModuleNvx" },
        { ObjectType::CuFunctionNvx, "CuFunctionNvx" },
        { ObjectType::DebugUtilsMessengerExt, "DebugUtilsMessengerExt" },
        { ObjectType::AccelerationStructureKhr, "AccelerationStructureKhr" },
        { ObjectType::ValidationCacheExt, "ValidationCacheExt" },
        { ObjectType::AccelerationStructureNv, "AccelerationStructureNv" },
        {
            ObjectType::PerformanceConfigurationIntel,
            "PerformanceConfigurationIntel"
        },
        { ObjectType::DeferredOperationKhr, "DeferredOperationKhr" },
        { ObjectType::IndirectCommandsLayoutNv, "IndirectCommandsLayoutNv" },
        { ObjectType::CudaModuleNv, "CudaModuleNv" },
        { ObjectType::CudaFunctionNv, "CudaFunctionNv" },
        { ObjectType::BufferCollectionFuchsia, "BufferCollectionFuchsia" },
        { ObjectType::MicromapExt, "MicromapExt" },
        { ObjectType::OpticalFlowSessionNv, "OpticalFlowSessionNv" },
        { ObjectType::ShaderExt, "ShaderExt" },
        {
            ObjectType::DescriptorUpdateTemplateKhr,
            "DescriptorUpdateTemplateKhr"
        },
        { ObjectType::SamplerYcbcrConversionKhr, "SamplerYcbcrConversionKhr" },
        { ObjectType::PrivateDataSlotExt, "PrivateDataSlotExt" }
    };

    // provided by VK_EXT_debug_utils
    // https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VkDebugUtilsMessageSeverityFlagBitsEXT.html
    struct DebugMessageSeverityFlags
    {
        bool verbose : 1 = false;
        bool info : 1 = false;
        bool warning : 1 = false;
        bool error : 1 = false;
    };

    // provided by VK_EXT_debug_utils
    // https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VkDebugUtilsMessageTypeFlagBitsEXT.html
    struct DebugMessageTypeFlags
    {
        bool general : 1 = false;
        bool validation : 1 = false;
        bool performance : 1 = false;
        bool device_address_binding : 1 = false;
    };

    // https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VkDeviceQueueCreateFlagBits.html
    struct QueueRequestFlags
    {
        // provided by VK_VERSION_1_1
        bool protected_ : 1 = false;
    };

    // https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VkExtensionProperties.html
    struct ExtensionProperties
    {
        std::string name;
        uint32_t spec_version;
    };

    ExtensionProperties ExtensionProperties_from_vk(
        VkExtensionProperties vk_properties
    );

    // https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VkLayerProperties.html
    struct LayerProperties
    {
        std::string name;
        Version spec_version;
        uint32_t implementation_version;
        std::string description;
    };

    LayerProperties LayerProperties_from_vk(
        const VkLayerProperties& vk_properties
    );

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

    PhysicalDeviceLimits PhysicalDeviceLimits_from_vk(
        const VkPhysicalDeviceLimits& vk_limits
    );

    // https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VkPhysicalDeviceSparseProperties.html
    struct PhysicalDeviceSparseProperties
    {
        bool residency_standard2d_block_shape : 1;
        bool residency_standard2d_multisample_block_shape : 1;
        bool residency_standard3d_block_shape : 1;
        bool residency_aligned_mip_size : 1;
        bool residency_non_resident_strict : 1;
    };

    PhysicalDeviceSparseProperties PhysicalDeviceSparseProperties_from_vk(
        const VkPhysicalDeviceSparseProperties& vk_properties
    );

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

    PhysicalDeviceProperties PhysicalDeviceProperties_from_vk(
        const VkPhysicalDeviceProperties& vk_properties
    );

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

    PhysicalDeviceFeatures PhysicalDeviceFeatures_from_vk(
        const VkPhysicalDeviceFeatures& vk_features
    );

    VkPhysicalDeviceFeatures PhysicalDeviceFeatures_to_vk(
        const PhysicalDeviceFeatures& features
    );

    // https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VkExtent3D.html
    struct Extent3d
    {
        uint32_t width;
        uint32_t height;
        uint32_t depth;
    };

    Extent3d Extent3d_from_vk(const VkExtent3D& vk_extent_3d);
    VkExtent3D Extent3d_to_vk(const Extent3d& extent_3d);

    // https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VkExtent2D.html
    struct Extent2d
    {
        uint32_t width;
        uint32_t height;
    };

    Extent2d Extent2d_from_vk(const VkExtent2D& vk_extent_2d);
    VkExtent2D Extent2d_to_vk(const Extent2d& extent_2d);

    // https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VkQueueFamilyProperties.html
    struct QueueFamily
    {
        QueueFlags queue_flags;
        uint32_t queue_count;
        uint32_t timestamp_valid_bits;
        Extent3d min_image_transfer_granularity;
    };

    QueueFamily QueueFamily_from_vk(
        const VkQueueFamilyProperties& vk_family,
        VkBool32 vk_presentation_support
    );

    // index of the first queue family that supports the corresponding set of
    // operations
    struct QueueFamilyIndices
    {
        std::optional<uint32_t> graphics;
        std::optional<uint32_t> presentation;
        std::optional<uint32_t> compute;
        std::optional<uint32_t> transfer;
        std::optional<uint32_t> sparse_binding;
        std::optional<uint32_t> protected_;
        std::optional<uint32_t> video_decode;
        std::optional<uint32_t> optical_flow_nv;
    };

    // provided by VK_KHR_surface
    // https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VkSurfaceTransformFlagBitsKHR.html
    struct SurfaceTransformFlags
    {
        bool identity : 1 = false;
        bool rotate90 : 1 = false;
        bool rotate180 : 1 = false;
        bool rotate270 : 1 = false;
        bool horizontal_mirror : 1 = false;
        bool horizontal_mirror_rotate90 : 1 = false;
        bool horizontal_mirror_rotate180 : 1 = false;
        bool horizontal_mirror_rotate270 : 1 = false;
        bool inherit : 1 = false;
    };

    // provided by VK_KHR_surface
    // https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VkCompositeAlphaFlagBitsKHR.html
    struct CompositeAlphaFlags
    {
        bool opaque : 1 = false;
        bool pre_multiplied : 1 = false;
        bool post_multiplied : 1 = false;
        bool inherit : 1 = false;
    };

    // https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VkImageUsageFlagBits.html
    struct ImageUsageFlags
    {
        bool transfer_src : 1 = false;
        bool transfer_dst : 1 = false;
        bool sampled : 1 = false;
        bool storage : 1 = false;
        bool color_attachment : 1 = false;
        bool depth_stencil_attachment : 1 = false;
        bool transient_attachment : 1 = false;
        bool input_attachment : 1 = false;
        bool video_decode_dst : 1 = false;
        bool video_decode_src : 1 = false;
        bool video_decode_dpb : 1 = false;
        bool fragment_density_map : 1 = false;
        bool fragment_shading_rate_attachment : 1 = false;
        bool attachment_feedback_loop : 1 = false;
        bool invocation_mask_huawei : 1 = false;
        bool sample_weight_qcom : 1 = false;
        bool sample_block_match_qcom : 1 = false;
    };

    // provided by VK_KHR_surface
    // https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VkSurfaceCapabilitiesKHR.html
    struct SurfaceCapabilities
    {
        uint32_t min_image_count;
        uint32_t max_image_count;
        Extent2d current_extent;
        Extent2d min_image_extent;
        Extent2d max_image_extent;
        uint32_t max_image_array_layers;
        SurfaceTransformFlags supported_transforms;
        SurfaceTransform current_transform;
        CompositeAlphaFlags supported_composite_alpha;
        ImageUsageFlags supported_usage_flags;
    };

    SurfaceCapabilities SurfaceCapabilities_from_vk(
        const VkSurfaceCapabilitiesKHR& vk_capabilities
    );

    // https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VkFormat.html
    enum class Format : int32_t
    {
        Undefined = 0,
        R4G4_UNORM_PACK8 = 1,
        R4G4B4A4_UNORM_PACK16 = 2,
        B4G4R4A4_UNORM_PACK16 = 3,
        R5G6B5_UNORM_PACK16 = 4,
        B5G6R5_UNORM_PACK16 = 5,
        R5G5B5A1_UNORM_PACK16 = 6,
        B5G5R5A1_UNORM_PACK16 = 7,
        A1R5G5B5_UNORM_PACK16 = 8,
        R8_UNORM = 9,
        R8_SNORM = 10,
        R8_USCALED = 11,
        R8_SSCALED = 12,
        R8_UINT = 13,
        R8_SINT = 14,
        R8_SRGB = 15,
        R8G8_UNORM = 16,
        R8G8_SNORM = 17,
        R8G8_USCALED = 18,
        R8G8_SSCALED = 19,
        R8G8_UINT = 20,
        R8G8_SINT = 21,
        R8G8_SRGB = 22,
        R8G8B8_UNORM = 23,
        R8G8B8_SNORM = 24,
        R8G8B8_USCALED = 25,
        R8G8B8_SSCALED = 26,
        R8G8B8_UINT = 27,
        R8G8B8_SINT = 28,
        R8G8B8_SRGB = 29,
        B8G8R8_UNORM = 30,
        B8G8R8_SNORM = 31,
        B8G8R8_USCALED = 32,
        B8G8R8_SSCALED = 33,
        B8G8R8_UINT = 34,
        B8G8R8_SINT = 35,
        B8G8R8_SRGB = 36,
        R8G8B8A8_UNORM = 37,
        R8G8B8A8_SNORM = 38,
        R8G8B8A8_USCALED = 39,
        R8G8B8A8_SSCALED = 40,
        R8G8B8A8_UINT = 41,
        R8G8B8A8_SINT = 42,
        R8G8B8A8_SRGB = 43,
        B8G8R8A8_UNORM = 44,
        B8G8R8A8_SNORM = 45,
        B8G8R8A8_USCALED = 46,
        B8G8R8A8_SSCALED = 47,
        B8G8R8A8_UINT = 48,
        B8G8R8A8_SINT = 49,
        B8G8R8A8_SRGB = 50,
        A8B8G8R8_UNORM_PACK32 = 51,
        A8B8G8R8_SNORM_PACK32 = 52,
        A8B8G8R8_USCALED_PACK32 = 53,
        A8B8G8R8_SSCALED_PACK32 = 54,
        A8B8G8R8_UINT_PACK32 = 55,
        A8B8G8R8_SINT_PACK32 = 56,
        A8B8G8R8_SRGB_PACK32 = 57,
        A2R10G10B10_UNORM_PACK32 = 58,
        A2R10G10B10_SNORM_PACK32 = 59,
        A2R10G10B10_USCALED_PACK32 = 60,
        A2R10G10B10_SSCALED_PACK32 = 61,
        A2R10G10B10_UINT_PACK32 = 62,
        A2R10G10B10_SINT_PACK32 = 63,
        A2B10G10R10_UNORM_PACK32 = 64,
        A2B10G10R10_SNORM_PACK32 = 65,
        A2B10G10R10_USCALED_PACK32 = 66,
        A2B10G10R10_SSCALED_PACK32 = 67,
        A2B10G10R10_UINT_PACK32 = 68,
        A2B10G10R10_SINT_PACK32 = 69,
        R16_UNORM = 70,
        R16_SNORM = 71,
        R16_USCALED = 72,
        R16_SSCALED = 73,
        R16_UINT = 74,
        R16_SINT = 75,
        R16_SFLOAT = 76,
        R16G16_UNORM = 77,
        R16G16_SNORM = 78,
        R16G16_USCALED = 79,
        R16G16_SSCALED = 80,
        R16G16_UINT = 81,
        R16G16_SINT = 82,
        R16G16_SFLOAT = 83,
        R16G16B16_UNORM = 84,
        R16G16B16_SNORM = 85,
        R16G16B16_USCALED = 86,
        R16G16B16_SSCALED = 87,
        R16G16B16_UINT = 88,
        R16G16B16_SINT = 89,
        R16G16B16_SFLOAT = 90,
        R16G16B16A16_UNORM = 91,
        R16G16B16A16_SNORM = 92,
        R16G16B16A16_USCALED = 93,
        R16G16B16A16_SSCALED = 94,
        R16G16B16A16_UINT = 95,
        R16G16B16A16_SINT = 96,
        R16G16B16A16_SFLOAT = 97,
        R32_UINT = 98,
        R32_SINT = 99,
        R32_SFLOAT = 100,
        R32G32_UINT = 101,
        R32G32_SINT = 102,
        R32G32_SFLOAT = 103,
        R32G32B32_UINT = 104,
        R32G32B32_SINT = 105,
        R32G32B32_SFLOAT = 106,
        R32G32B32A32_UINT = 107,
        R32G32B32A32_SINT = 108,
        R32G32B32A32_SFLOAT = 109,
        R64_UINT = 110,
        R64_SINT = 111,
        R64_SFLOAT = 112,
        R64G64_UINT = 113,
        R64G64_SINT = 114,
        R64G64_SFLOAT = 115,
        R64G64B64_UINT = 116,
        R64G64B64_SINT = 117,
        R64G64B64_SFLOAT = 118,
        R64G64B64A64_UINT = 119,
        R64G64B64A64_SINT = 120,
        R64G64B64A64_SFLOAT = 121,
        B10G11R11_UFLOAT_PACK32 = 122,
        E5B9G9R9_UFLOAT_PACK32 = 123,
        D16_UNORM = 124,
        X8_D24_UNORM_PACK32 = 125,
        D32_SFLOAT = 126,
        S8_UINT = 127,
        D16_UNORM_S8_UINT = 128,
        D24_UNORM_S8_UINT = 129,
        D32_SFLOAT_S8_UINT = 130,
        BC1_RGB_UNORM_BLOCK = 131,
        BC1_RGB_SRGB_BLOCK = 132,
        BC1_RGBA_UNORM_BLOCK = 133,
        BC1_RGBA_SRGB_BLOCK = 134,
        BC2_UNORM_BLOCK = 135,
        BC2_SRGB_BLOCK = 136,
        BC3_UNORM_BLOCK = 137,
        BC3_SRGB_BLOCK = 138,
        BC4_UNORM_BLOCK = 139,
        BC4_SNORM_BLOCK = 140,
        BC5_UNORM_BLOCK = 141,
        BC5_SNORM_BLOCK = 142,
        BC6H_UFLOAT_BLOCK = 143,
        BC6H_SFLOAT_BLOCK = 144,
        BC7_UNORM_BLOCK = 145,
        BC7_SRGB_BLOCK = 146,
        ETC2_R8G8B8_UNORM_BLOCK = 147,
        ETC2_R8G8B8_SRGB_BLOCK = 148,
        ETC2_R8G8B8A1_UNORM_BLOCK = 149,
        ETC2_R8G8B8A1_SRGB_BLOCK = 150,
        ETC2_R8G8B8A8_UNORM_BLOCK = 151,
        ETC2_R8G8B8A8_SRGB_BLOCK = 152,
        EAC_R11_UNORM_BLOCK = 153,
        EAC_R11_SNORM_BLOCK = 154,
        EAC_R11G11_UNORM_BLOCK = 155,
        EAC_R11G11_SNORM_BLOCK = 156,
        ASTC_4x4_UNORM_BLOCK = 157,
        ASTC_4x4_SRGB_BLOCK = 158,
        ASTC_5x4_UNORM_BLOCK = 159,
        ASTC_5x4_SRGB_BLOCK = 160,
        ASTC_5x5_UNORM_BLOCK = 161,
        ASTC_5x5_SRGB_BLOCK = 162,
        ASTC_6x5_UNORM_BLOCK = 163,
        ASTC_6x5_SRGB_BLOCK = 164,
        ASTC_6x6_UNORM_BLOCK = 165,
        ASTC_6x6_SRGB_BLOCK = 166,
        ASTC_8x5_UNORM_BLOCK = 167,
        ASTC_8x5_SRGB_BLOCK = 168,
        ASTC_8x6_UNORM_BLOCK = 169,
        ASTC_8x6_SRGB_BLOCK = 170,
        ASTC_8x8_UNORM_BLOCK = 171,
        ASTC_8x8_SRGB_BLOCK = 172,
        ASTC_10x5_UNORM_BLOCK = 173,
        ASTC_10x5_SRGB_BLOCK = 174,
        ASTC_10x6_UNORM_BLOCK = 175,
        ASTC_10x6_SRGB_BLOCK = 176,
        ASTC_10x8_UNORM_BLOCK = 177,
        ASTC_10x8_SRGB_BLOCK = 178,
        ASTC_10x10_UNORM_BLOCK = 179,
        ASTC_10x10_SRGB_BLOCK = 180,
        ASTC_12x10_UNORM_BLOCK = 181,
        ASTC_12x10_SRGB_BLOCK = 182,
        ASTC_12x12_UNORM_BLOCK = 183,
        ASTC_12x12_SRGB_BLOCK = 184,

        // provided by VK_VERSION_1_1
        G8B8G8R8_422_UNORM = 1000156000,

        // provided by VK_VERSION_1_1
        B8G8R8G8_422_UNORM = 1000156001,

        // provided by VK_VERSION_1_1
        G8_B8_R8_3PLANE_420_UNORM = 1000156002,

        // provided by VK_VERSION_1_1
        G8_B8R8_2PLANE_420_UNORM = 1000156003,

        // provided by VK_VERSION_1_1
        G8_B8_R8_3PLANE_422_UNORM = 1000156004,

        // provided by VK_VERSION_1_1
        G8_B8R8_2PLANE_422_UNORM = 1000156005,

        // provided by VK_VERSION_1_1
        G8_B8_R8_3PLANE_444_UNORM = 1000156006,

        // provided by VK_VERSION_1_1
        R10X6_UNORM_PACK16 = 1000156007,

        // provided by VK_VERSION_1_1
        R10X6G10X6_UNORM_2PACK16 = 1000156008,

        // provided by VK_VERSION_1_1
        R10X6G10X6B10X6A10X6_UNORM_4PACK16 = 1000156009,

        // provided by VK_VERSION_1_1
        G10X6B10X6G10X6R10X6_422_UNORM_4PACK16 = 1000156010,

        // provided by VK_VERSION_1_1
        B10X6G10X6R10X6G10X6_422_UNORM_4PACK16 = 1000156011,

        // provided by VK_VERSION_1_1
        G10X6_B10X6_R10X6_3PLANE_420_UNORM_3PACK16 = 1000156012,

        // provided by VK_VERSION_1_1
        G10X6_B10X6R10X6_2PLANE_420_UNORM_3PACK16 = 1000156013,

        // provided by VK_VERSION_1_1
        G10X6_B10X6_R10X6_3PLANE_422_UNORM_3PACK16 = 1000156014,

        // provided by VK_VERSION_1_1
        G10X6_B10X6R10X6_2PLANE_422_UNORM_3PACK16 = 1000156015,

        // provided by VK_VERSION_1_1
        G10X6_B10X6_R10X6_3PLANE_444_UNORM_3PACK16 = 1000156016,

        // provided by VK_VERSION_1_1
        R12X4_UNORM_PACK16 = 1000156017,

        // provided by VK_VERSION_1_1
        R12X4G12X4_UNORM_2PACK16 = 1000156018,

        // provided by VK_VERSION_1_1
        R12X4G12X4B12X4A12X4_UNORM_4PACK16 = 1000156019,

        // provided by VK_VERSION_1_1
        G12X4B12X4G12X4R12X4_422_UNORM_4PACK16 = 1000156020,

        // provided by VK_VERSION_1_1
        B12X4G12X4R12X4G12X4_422_UNORM_4PACK16 = 1000156021,

        // provided by VK_VERSION_1_1
        G12X4_B12X4_R12X4_3PLANE_420_UNORM_3PACK16 = 1000156022,

        // provided by VK_VERSION_1_1
        G12X4_B12X4R12X4_2PLANE_420_UNORM_3PACK16 = 1000156023,

        // provided by VK_VERSION_1_1
        G12X4_B12X4_R12X4_3PLANE_422_UNORM_3PACK16 = 1000156024,

        // provided by VK_VERSION_1_1
        G12X4_B12X4R12X4_2PLANE_422_UNORM_3PACK16 = 1000156025,

        // provided by VK_VERSION_1_1
        G12X4_B12X4_R12X4_3PLANE_444_UNORM_3PACK16 = 1000156026,

        // provided by VK_VERSION_1_1
        G16B16G16R16_422_UNORM = 1000156027,

        // provided by VK_VERSION_1_1
        B16G16R16G16_422_UNORM = 1000156028,

        // provided by VK_VERSION_1_1
        G16_B16_R16_3PLANE_420_UNORM = 1000156029,

        // provided by VK_VERSION_1_1
        G16_B16R16_2PLANE_420_UNORM = 1000156030,

        // provided by VK_VERSION_1_1
        G16_B16_R16_3PLANE_422_UNORM = 1000156031,

        // provided by VK_VERSION_1_1
        G16_B16R16_2PLANE_422_UNORM = 1000156032,

        // provided by VK_VERSION_1_1
        G16_B16_R16_3PLANE_444_UNORM = 1000156033,

        // provided by VK_VERSION_1_3
        G8_B8R8_2PLANE_444_UNORM = 1000330000,

        // provided by VK_VERSION_1_3
        G10X6_B10X6R10X6_2PLANE_444_UNORM_3PACK16 = 1000330001,

        // provided by VK_VERSION_1_3
        G12X4_B12X4R12X4_2PLANE_444_UNORM_3PACK16 = 1000330002,

        // provided by VK_VERSION_1_3
        G16_B16R16_2PLANE_444_UNORM = 1000330003,

        // provided by VK_VERSION_1_3
        A4R4G4B4_UNORM_PACK16 = 1000340000,

        // provided by VK_VERSION_1_3
        A4B4G4R4_UNORM_PACK16 = 1000340001,

        // provided by VK_VERSION_1_3
        ASTC_4x4_SFLOAT_BLOCK = 1000066000,

        // provided by VK_VERSION_1_3
        ASTC_5x4_SFLOAT_BLOCK = 1000066001,

        // provided by VK_VERSION_1_3
        ASTC_5x5_SFLOAT_BLOCK = 1000066002,

        // provided by VK_VERSION_1_3
        ASTC_6x5_SFLOAT_BLOCK = 1000066003,

        // provided by VK_VERSION_1_3
        ASTC_6x6_SFLOAT_BLOCK = 1000066004,

        // provided by VK_VERSION_1_3
        ASTC_8x5_SFLOAT_BLOCK = 1000066005,

        // provided by VK_VERSION_1_3
        ASTC_8x6_SFLOAT_BLOCK = 1000066006,

        // provided by VK_VERSION_1_3
        ASTC_8x8_SFLOAT_BLOCK = 1000066007,

        // provided by VK_VERSION_1_3
        ASTC_10x5_SFLOAT_BLOCK = 1000066008,

        // provided by VK_VERSION_1_3
        ASTC_10x6_SFLOAT_BLOCK = 1000066009,

        // provided by VK_VERSION_1_3
        ASTC_10x8_SFLOAT_BLOCK = 1000066010,

        // provided by VK_VERSION_1_3
        ASTC_10x10_SFLOAT_BLOCK = 1000066011,

        // provided by VK_VERSION_1_3
        ASTC_12x10_SFLOAT_BLOCK = 1000066012,

        // provided by VK_VERSION_1_3
        ASTC_12x12_SFLOAT_BLOCK = 1000066013,

        // provided by VK_IMG_format_pvrtc
        PVRTC1_2BPP_UNORM_BLOCK_IMG = 1000054000,

        // provided by VK_IMG_format_pvrtc
        PVRTC1_4BPP_UNORM_BLOCK_IMG = 1000054001,

        // provided by VK_IMG_format_pvrtc
        PVRTC2_2BPP_UNORM_BLOCK_IMG = 1000054002,

        // provided by VK_IMG_format_pvrtc
        PVRTC2_4BPP_UNORM_BLOCK_IMG = 1000054003,

        // provided by VK_IMG_format_pvrtc
        PVRTC1_2BPP_SRGB_BLOCK_IMG = 1000054004,

        // provided by VK_IMG_format_pvrtc
        PVRTC1_4BPP_SRGB_BLOCK_IMG = 1000054005,

        // provided by VK_IMG_format_pvrtc
        PVRTC2_2BPP_SRGB_BLOCK_IMG = 1000054006,

        // provided by VK_IMG_format_pvrtc
        PVRTC2_4BPP_SRGB_BLOCK_IMG = 1000054007,

        // provided by VK_NV_optical_flow
        R16G16_SFIXED5_NV = 1000464000,

        // provided by VK_KHR_maintenance5
        A1B5G5R5_UNORM_PACK16_KHR = 1000470000,

        // provided by VK_KHR_maintenance5
        A8_UNORM_KHR = 1000470001,

        // provided by VK_EXT_texture_compression_astc_hdr
        ASTC_4x4_SFLOAT_BLOCK_EXT = ASTC_4x4_SFLOAT_BLOCK,

        // provided by VK_EXT_texture_compression_astc_hdr
        ASTC_5x4_SFLOAT_BLOCK_EXT = ASTC_5x4_SFLOAT_BLOCK,

        // provided by VK_EXT_texture_compression_astc_hdr
        ASTC_5x5_SFLOAT_BLOCK_EXT = ASTC_5x5_SFLOAT_BLOCK,

        // provided by VK_EXT_texture_compression_astc_hdr
        ASTC_6x5_SFLOAT_BLOCK_EXT = ASTC_6x5_SFLOAT_BLOCK,

        // provided by VK_EXT_texture_compression_astc_hdr
        ASTC_6x6_SFLOAT_BLOCK_EXT = ASTC_6x6_SFLOAT_BLOCK,

        // provided by VK_EXT_texture_compression_astc_hdr
        ASTC_8x5_SFLOAT_BLOCK_EXT = ASTC_8x5_SFLOAT_BLOCK,

        // provided by VK_EXT_texture_compression_astc_hdr
        ASTC_8x6_SFLOAT_BLOCK_EXT = ASTC_8x6_SFLOAT_BLOCK,

        // provided by VK_EXT_texture_compression_astc_hdr
        ASTC_8x8_SFLOAT_BLOCK_EXT = ASTC_8x8_SFLOAT_BLOCK,

        // provided by VK_EXT_texture_compression_astc_hdr
        ASTC_10x5_SFLOAT_BLOCK_EXT = ASTC_10x5_SFLOAT_BLOCK,

        // provided by VK_EXT_texture_compression_astc_hdr
        ASTC_10x6_SFLOAT_BLOCK_EXT = ASTC_10x6_SFLOAT_BLOCK,

        // provided by VK_EXT_texture_compression_astc_hdr
        ASTC_10x8_SFLOAT_BLOCK_EXT = ASTC_10x8_SFLOAT_BLOCK,

        // provided by VK_EXT_texture_compression_astc_hdr
        ASTC_10x10_SFLOAT_BLOCK_EXT = ASTC_10x10_SFLOAT_BLOCK,

        // provided by VK_EXT_texture_compression_astc_hdr
        ASTC_12x10_SFLOAT_BLOCK_EXT = ASTC_12x10_SFLOAT_BLOCK,

        // provided by VK_EXT_texture_compression_astc_hdr
        ASTC_12x12_SFLOAT_BLOCK_EXT = ASTC_12x12_SFLOAT_BLOCK,

        // provided by VK_KHR_sampler_ycbcr_conversion
        G8B8G8R8_422_UNORM_KHR = G8B8G8R8_422_UNORM,

        // provided by VK_KHR_sampler_ycbcr_conversion
        B8G8R8G8_422_UNORM_KHR = B8G8R8G8_422_UNORM,

        // provided by VK_KHR_sampler_ycbcr_conversion
        G8_B8_R8_3PLANE_420_UNORM_KHR = G8_B8_R8_3PLANE_420_UNORM,

        // provided by VK_KHR_sampler_ycbcr_conversion
        G8_B8R8_2PLANE_420_UNORM_KHR = G8_B8R8_2PLANE_420_UNORM,

        // provided by VK_KHR_sampler_ycbcr_conversion
        G8_B8_R8_3PLANE_422_UNORM_KHR = G8_B8_R8_3PLANE_422_UNORM,

        // provided by VK_KHR_sampler_ycbcr_conversion
        G8_B8R8_2PLANE_422_UNORM_KHR = G8_B8R8_2PLANE_422_UNORM,

        // provided by VK_KHR_sampler_ycbcr_conversion
        G8_B8_R8_3PLANE_444_UNORM_KHR = G8_B8_R8_3PLANE_444_UNORM,

        // provided by VK_KHR_sampler_ycbcr_conversion
        R10X6_UNORM_PACK16_KHR = R10X6_UNORM_PACK16,

        // provided by VK_KHR_sampler_ycbcr_conversion
        R10X6G10X6_UNORM_2PACK16_KHR = R10X6G10X6_UNORM_2PACK16,

        // provided by VK_KHR_sampler_ycbcr_conversion
        R10X6G10X6B10X6A10X6_UNORM_4PACK16_KHR = R10X6G10X6B10X6A10X6_UNORM_4PACK16,

        // provided by VK_KHR_sampler_ycbcr_conversion
        G10X6B10X6G10X6R10X6_422_UNORM_4PACK16_KHR = G10X6B10X6G10X6R10X6_422_UNORM_4PACK16,

        // provided by VK_KHR_sampler_ycbcr_conversion
        B10X6G10X6R10X6G10X6_422_UNORM_4PACK16_KHR = B10X6G10X6R10X6G10X6_422_UNORM_4PACK16,

        // provided by VK_KHR_sampler_ycbcr_conversion
        G10X6_B10X6_R10X6_3PLANE_420_UNORM_3PACK16_KHR = G10X6_B10X6_R10X6_3PLANE_420_UNORM_3PACK16,

        // provided by VK_KHR_sampler_ycbcr_conversion
        G10X6_B10X6R10X6_2PLANE_420_UNORM_3PACK16_KHR = G10X6_B10X6R10X6_2PLANE_420_UNORM_3PACK16,

        // provided by VK_KHR_sampler_ycbcr_conversion
        G10X6_B10X6_R10X6_3PLANE_422_UNORM_3PACK16_KHR = G10X6_B10X6_R10X6_3PLANE_422_UNORM_3PACK16,

        // provided by VK_KHR_sampler_ycbcr_conversion
        G10X6_B10X6R10X6_2PLANE_422_UNORM_3PACK16_KHR = G10X6_B10X6R10X6_2PLANE_422_UNORM_3PACK16,

        // provided by VK_KHR_sampler_ycbcr_conversion
        G10X6_B10X6_R10X6_3PLANE_444_UNORM_3PACK16_KHR = G10X6_B10X6_R10X6_3PLANE_444_UNORM_3PACK16,

        // provided by VK_KHR_sampler_ycbcr_conversion
        R12X4_UNORM_PACK16_KHR = R12X4_UNORM_PACK16,

        // provided by VK_KHR_sampler_ycbcr_conversion
        R12X4G12X4_UNORM_2PACK16_KHR = R12X4G12X4_UNORM_2PACK16,

        // provided by VK_KHR_sampler_ycbcr_conversion
        R12X4G12X4B12X4A12X4_UNORM_4PACK16_KHR = R12X4G12X4B12X4A12X4_UNORM_4PACK16,

        // provided by VK_KHR_sampler_ycbcr_conversion
        G12X4B12X4G12X4R12X4_422_UNORM_4PACK16_KHR = G12X4B12X4G12X4R12X4_422_UNORM_4PACK16,

        // provided by VK_KHR_sampler_ycbcr_conversion
        B12X4G12X4R12X4G12X4_422_UNORM_4PACK16_KHR = B12X4G12X4R12X4G12X4_422_UNORM_4PACK16,

        // provided by VK_KHR_sampler_ycbcr_conversion
        G12X4_B12X4_R12X4_3PLANE_420_UNORM_3PACK16_KHR = G12X4_B12X4_R12X4_3PLANE_420_UNORM_3PACK16,

        // provided by VK_KHR_sampler_ycbcr_conversion
        G12X4_B12X4R12X4_2PLANE_420_UNORM_3PACK16_KHR = G12X4_B12X4R12X4_2PLANE_420_UNORM_3PACK16,

        // provided by VK_KHR_sampler_ycbcr_conversion
        G12X4_B12X4_R12X4_3PLANE_422_UNORM_3PACK16_KHR = G12X4_B12X4_R12X4_3PLANE_422_UNORM_3PACK16,

        // provided by VK_KHR_sampler_ycbcr_conversion
        G12X4_B12X4R12X4_2PLANE_422_UNORM_3PACK16_KHR = G12X4_B12X4R12X4_2PLANE_422_UNORM_3PACK16,

        // provided by VK_KHR_sampler_ycbcr_conversion
        G12X4_B12X4_R12X4_3PLANE_444_UNORM_3PACK16_KHR = G12X4_B12X4_R12X4_3PLANE_444_UNORM_3PACK16,

        // provided by VK_KHR_sampler_ycbcr_conversion
        G16B16G16R16_422_UNORM_KHR = G16B16G16R16_422_UNORM,

        // provided by VK_KHR_sampler_ycbcr_conversion
        B16G16R16G16_422_UNORM_KHR = B16G16R16G16_422_UNORM,

        // provided by VK_KHR_sampler_ycbcr_conversion
        G16_B16_R16_3PLANE_420_UNORM_KHR = G16_B16_R16_3PLANE_420_UNORM,

        // provided by VK_KHR_sampler_ycbcr_conversion
        G16_B16R16_2PLANE_420_UNORM_KHR = G16_B16R16_2PLANE_420_UNORM,

        // provided by VK_KHR_sampler_ycbcr_conversion
        G16_B16_R16_3PLANE_422_UNORM_KHR = G16_B16_R16_3PLANE_422_UNORM,

        // provided by VK_KHR_sampler_ycbcr_conversion
        G16_B16R16_2PLANE_422_UNORM_KHR = G16_B16R16_2PLANE_422_UNORM,

        // provided by VK_KHR_sampler_ycbcr_conversion
        G16_B16_R16_3PLANE_444_UNORM_KHR = G16_B16_R16_3PLANE_444_UNORM,

        // provided by VK_EXT_ycbcr_2plane_444_formats
        G8_B8R8_2PLANE_444_UNORM_EXT = G8_B8R8_2PLANE_444_UNORM,

        // provided by VK_EXT_ycbcr_2plane_444_formats
        G10X6_B10X6R10X6_2PLANE_444_UNORM_3PACK16_EXT = G10X6_B10X6R10X6_2PLANE_444_UNORM_3PACK16,

        // provided by VK_EXT_ycbcr_2plane_444_formats
        G12X4_B12X4R12X4_2PLANE_444_UNORM_3PACK16_EXT = G12X4_B12X4R12X4_2PLANE_444_UNORM_3PACK16,

        // provided by VK_EXT_ycbcr_2plane_444_formats
        G16_B16R16_2PLANE_444_UNORM_EXT = G16_B16R16_2PLANE_444_UNORM,

        // provided by VK_EXT_4444_formats
        A4R4G4B4_UNORM_PACK16_EXT = A4R4G4B4_UNORM_PACK16,

        // provided by VK_EXT_4444_formats
        A4B4G4R4_UNORM_PACK16_EXT = A4B4G4R4_UNORM_PACK16,

        // provided by VK_NV_optical_flow
        // VK_FORMAT_R16G16_S10_5_NV  is a deprecated alias
        R16G16_S10_5_NV = R16G16_SFIXED5_NV
    };

    static EnumStrMap<Format> Format_strmap{
        { Format::Undefined, "Undefined" },
        { Format::R4G4_UNORM_PACK8, "R4G4_UNORM_PACK8" },
        { Format::R4G4B4A4_UNORM_PACK16, "R4G4B4A4_UNORM_PACK16" },
        { Format::B4G4R4A4_UNORM_PACK16, "B4G4R4A4_UNORM_PACK16" },
        { Format::R5G6B5_UNORM_PACK16, "R5G6B5_UNORM_PACK16" },
        { Format::B5G6R5_UNORM_PACK16, "B5G6R5_UNORM_PACK16" },
        { Format::R5G5B5A1_UNORM_PACK16, "R5G5B5A1_UNORM_PACK16" },
        { Format::B5G5R5A1_UNORM_PACK16, "B5G5R5A1_UNORM_PACK16" },
        { Format::A1R5G5B5_UNORM_PACK16, "A1R5G5B5_UNORM_PACK16" },
        { Format::R8_UNORM, "R8_UNORM" },
        { Format::R8_SNORM, "R8_SNORM" },
        { Format::R8_USCALED, "R8_USCALED" },
        { Format::R8_SSCALED, "R8_SSCALED" },
        { Format::R8_UINT, "R8_UINT" },
        { Format::R8_SINT, "R8_SINT" },
        { Format::R8_SRGB, "R8_SRGB" },
        { Format::R8G8_UNORM, "R8G8_UNORM" },
        { Format::R8G8_SNORM, "R8G8_SNORM" },
        { Format::R8G8_USCALED, "R8G8_USCALED" },
        { Format::R8G8_SSCALED, "R8G8_SSCALED" },
        { Format::R8G8_UINT, "R8G8_UINT" },
        { Format::R8G8_SINT, "R8G8_SINT" },
        { Format::R8G8_SRGB, "R8G8_SRGB" },
        { Format::R8G8B8_UNORM, "R8G8B8_UNORM" },
        { Format::R8G8B8_SNORM, "R8G8B8_SNORM" },
        { Format::R8G8B8_USCALED, "R8G8B8_USCALED" },
        { Format::R8G8B8_SSCALED, "R8G8B8_SSCALED" },
        { Format::R8G8B8_UINT, "R8G8B8_UINT" },
        { Format::R8G8B8_SINT, "R8G8B8_SINT" },
        { Format::R8G8B8_SRGB, "R8G8B8_SRGB" },
        { Format::B8G8R8_UNORM, "B8G8R8_UNORM" },
        { Format::B8G8R8_SNORM, "B8G8R8_SNORM" },
        { Format::B8G8R8_USCALED, "B8G8R8_USCALED" },
        { Format::B8G8R8_SSCALED, "B8G8R8_SSCALED" },
        { Format::B8G8R8_UINT, "B8G8R8_UINT" },
        { Format::B8G8R8_SINT, "B8G8R8_SINT" },
        { Format::B8G8R8_SRGB, "B8G8R8_SRGB" },
        { Format::R8G8B8A8_UNORM, "R8G8B8A8_UNORM" },
        { Format::R8G8B8A8_SNORM, "R8G8B8A8_SNORM" },
        { Format::R8G8B8A8_USCALED, "R8G8B8A8_USCALED" },
        { Format::R8G8B8A8_SSCALED, "R8G8B8A8_SSCALED" },
        { Format::R8G8B8A8_UINT, "R8G8B8A8_UINT" },
        { Format::R8G8B8A8_SINT, "R8G8B8A8_SINT" },
        { Format::R8G8B8A8_SRGB, "R8G8B8A8_SRGB" },
        { Format::B8G8R8A8_UNORM, "B8G8R8A8_UNORM" },
        { Format::B8G8R8A8_SNORM, "B8G8R8A8_SNORM" },
        { Format::B8G8R8A8_USCALED, "B8G8R8A8_USCALED" },
        { Format::B8G8R8A8_SSCALED, "B8G8R8A8_SSCALED" },
        { Format::B8G8R8A8_UINT, "B8G8R8A8_UINT" },
        { Format::B8G8R8A8_SINT, "B8G8R8A8_SINT" },
        { Format::B8G8R8A8_SRGB, "B8G8R8A8_SRGB" },
        { Format::A8B8G8R8_UNORM_PACK32, "A8B8G8R8_UNORM_PACK32" },
        { Format::A8B8G8R8_SNORM_PACK32, "A8B8G8R8_SNORM_PACK32" },
        { Format::A8B8G8R8_USCALED_PACK32, "A8B8G8R8_USCALED_PACK32" },
        { Format::A8B8G8R8_SSCALED_PACK32, "A8B8G8R8_SSCALED_PACK32" },
        { Format::A8B8G8R8_UINT_PACK32, "A8B8G8R8_UINT_PACK32" },
        { Format::A8B8G8R8_SINT_PACK32, "A8B8G8R8_SINT_PACK32" },
        { Format::A8B8G8R8_SRGB_PACK32, "A8B8G8R8_SRGB_PACK32" },
        { Format::A2R10G10B10_UNORM_PACK32, "A2R10G10B10_UNORM_PACK32" },
        { Format::A2R10G10B10_SNORM_PACK32, "A2R10G10B10_SNORM_PACK32" },
        { Format::A2R10G10B10_USCALED_PACK32, "A2R10G10B10_USCALED_PACK32" },
        { Format::A2R10G10B10_SSCALED_PACK32, "A2R10G10B10_SSCALED_PACK32" },
        { Format::A2R10G10B10_UINT_PACK32, "A2R10G10B10_UINT_PACK32" },
        { Format::A2R10G10B10_SINT_PACK32, "A2R10G10B10_SINT_PACK32" },
        { Format::A2B10G10R10_UNORM_PACK32, "A2B10G10R10_UNORM_PACK32" },
        { Format::A2B10G10R10_SNORM_PACK32, "A2B10G10R10_SNORM_PACK32" },
        { Format::A2B10G10R10_USCALED_PACK32, "A2B10G10R10_USCALED_PACK32" },
        { Format::A2B10G10R10_SSCALED_PACK32, "A2B10G10R10_SSCALED_PACK32" },
        { Format::A2B10G10R10_UINT_PACK32, "A2B10G10R10_UINT_PACK32" },
        { Format::A2B10G10R10_SINT_PACK32, "A2B10G10R10_SINT_PACK32" },
        { Format::R16_UNORM, "R16_UNORM" },
        { Format::R16_SNORM, "R16_SNORM" },
        { Format::R16_USCALED, "R16_USCALED" },
        { Format::R16_SSCALED, "R16_SSCALED" },
        { Format::R16_UINT, "R16_UINT" },
        { Format::R16_SINT, "R16_SINT" },
        { Format::R16_SFLOAT, "R16_SFLOAT" },
        { Format::R16G16_UNORM, "R16G16_UNORM" },
        { Format::R16G16_SNORM, "R16G16_SNORM" },
        { Format::R16G16_USCALED, "R16G16_USCALED" },
        { Format::R16G16_SSCALED, "R16G16_SSCALED" },
        { Format::R16G16_UINT, "R16G16_UINT" },
        { Format::R16G16_SINT, "R16G16_SINT" },
        { Format::R16G16_SFLOAT, "R16G16_SFLOAT" },
        { Format::R16G16B16_UNORM, "R16G16B16_UNORM" },
        { Format::R16G16B16_SNORM, "R16G16B16_SNORM" },
        { Format::R16G16B16_USCALED, "R16G16B16_USCALED" },
        { Format::R16G16B16_SSCALED, "R16G16B16_SSCALED" },
        { Format::R16G16B16_UINT, "R16G16B16_UINT" },
        { Format::R16G16B16_SINT, "R16G16B16_SINT" },
        { Format::R16G16B16_SFLOAT, "R16G16B16_SFLOAT" },
        { Format::R16G16B16A16_UNORM, "R16G16B16A16_UNORM" },
        { Format::R16G16B16A16_SNORM, "R16G16B16A16_SNORM" },
        { Format::R16G16B16A16_USCALED, "R16G16B16A16_USCALED" },
        { Format::R16G16B16A16_SSCALED, "R16G16B16A16_SSCALED" },
        { Format::R16G16B16A16_UINT, "R16G16B16A16_UINT" },
        { Format::R16G16B16A16_SINT, "R16G16B16A16_SINT" },
        { Format::R16G16B16A16_SFLOAT, "R16G16B16A16_SFLOAT" },
        { Format::R32_UINT, "R32_UINT" },
        { Format::R32_SINT, "R32_SINT" },
        { Format::R32_SFLOAT, "R32_SFLOAT" },
        { Format::R32G32_UINT, "R32G32_UINT" },
        { Format::R32G32_SINT, "R32G32_SINT" },
        { Format::R32G32_SFLOAT, "R32G32_SFLOAT" },
        { Format::R32G32B32_UINT, "R32G32B32_UINT" },
        { Format::R32G32B32_SINT, "R32G32B32_SINT" },
        { Format::R32G32B32_SFLOAT, "R32G32B32_SFLOAT" },
        { Format::R32G32B32A32_UINT, "R32G32B32A32_UINT" },
        { Format::R32G32B32A32_SINT, "R32G32B32A32_SINT" },
        { Format::R32G32B32A32_SFLOAT, "R32G32B32A32_SFLOAT" },
        { Format::R64_UINT, "R64_UINT" },
        { Format::R64_SINT, "R64_SINT" },
        { Format::R64_SFLOAT, "R64_SFLOAT" },
        { Format::R64G64_UINT, "R64G64_UINT" },
        { Format::R64G64_SINT, "R64G64_SINT" },
        { Format::R64G64_SFLOAT, "R64G64_SFLOAT" },
        { Format::R64G64B64_UINT, "R64G64B64_UINT" },
        { Format::R64G64B64_SINT, "R64G64B64_SINT" },
        { Format::R64G64B64_SFLOAT, "R64G64B64_SFLOAT" },
        { Format::R64G64B64A64_UINT, "R64G64B64A64_UINT" },
        { Format::R64G64B64A64_SINT, "R64G64B64A64_SINT" },
        { Format::R64G64B64A64_SFLOAT, "R64G64B64A64_SFLOAT" },
        { Format::B10G11R11_UFLOAT_PACK32, "B10G11R11_UFLOAT_PACK32" },
        { Format::E5B9G9R9_UFLOAT_PACK32, "E5B9G9R9_UFLOAT_PACK32" },
        { Format::D16_UNORM, "D16_UNORM" },
        { Format::X8_D24_UNORM_PACK32, "X8_D24_UNORM_PACK32" },
        { Format::D32_SFLOAT, "D32_SFLOAT" },
        { Format::S8_UINT, "S8_UINT" },
        { Format::D16_UNORM_S8_UINT, "D16_UNORM_S8_UINT" },
        { Format::D24_UNORM_S8_UINT, "D24_UNORM_S8_UINT" },
        { Format::D32_SFLOAT_S8_UINT, "D32_SFLOAT_S8_UINT" },
        { Format::BC1_RGB_UNORM_BLOCK, "BC1_RGB_UNORM_BLOCK" },
        { Format::BC1_RGB_SRGB_BLOCK, "BC1_RGB_SRGB_BLOCK" },
        { Format::BC1_RGBA_UNORM_BLOCK, "BC1_RGBA_UNORM_BLOCK" },
        { Format::BC1_RGBA_SRGB_BLOCK, "BC1_RGBA_SRGB_BLOCK" },
        { Format::BC2_UNORM_BLOCK, "BC2_UNORM_BLOCK" },
        { Format::BC2_SRGB_BLOCK, "BC2_SRGB_BLOCK" },
        { Format::BC3_UNORM_BLOCK, "BC3_UNORM_BLOCK" },
        { Format::BC3_SRGB_BLOCK, "BC3_SRGB_BLOCK" },
        { Format::BC4_UNORM_BLOCK, "BC4_UNORM_BLOCK" },
        { Format::BC4_SNORM_BLOCK, "BC4_SNORM_BLOCK" },
        { Format::BC5_UNORM_BLOCK, "BC5_UNORM_BLOCK" },
        { Format::BC5_SNORM_BLOCK, "BC5_SNORM_BLOCK" },
        { Format::BC6H_UFLOAT_BLOCK, "BC6H_UFLOAT_BLOCK" },
        { Format::BC6H_SFLOAT_BLOCK, "BC6H_SFLOAT_BLOCK" },
        { Format::BC7_UNORM_BLOCK, "BC7_UNORM_BLOCK" },
        { Format::BC7_SRGB_BLOCK, "BC7_SRGB_BLOCK" },
        { Format::ETC2_R8G8B8_UNORM_BLOCK, "ETC2_R8G8B8_UNORM_BLOCK" },
        { Format::ETC2_R8G8B8_SRGB_BLOCK, "ETC2_R8G8B8_SRGB_BLOCK" },
        { Format::ETC2_R8G8B8A1_UNORM_BLOCK, "ETC2_R8G8B8A1_UNORM_BLOCK" },
        { Format::ETC2_R8G8B8A1_SRGB_BLOCK, "ETC2_R8G8B8A1_SRGB_BLOCK" },
        { Format::ETC2_R8G8B8A8_UNORM_BLOCK, "ETC2_R8G8B8A8_UNORM_BLOCK" },
        { Format::ETC2_R8G8B8A8_SRGB_BLOCK, "ETC2_R8G8B8A8_SRGB_BLOCK" },
        { Format::EAC_R11_UNORM_BLOCK, "EAC_R11_UNORM_BLOCK" },
        { Format::EAC_R11_SNORM_BLOCK, "EAC_R11_SNORM_BLOCK" },
        { Format::EAC_R11G11_UNORM_BLOCK, "EAC_R11G11_UNORM_BLOCK" },
        { Format::EAC_R11G11_SNORM_BLOCK, "EAC_R11G11_SNORM_BLOCK" },
        { Format::ASTC_4x4_UNORM_BLOCK, "ASTC_4x4_UNORM_BLOCK" },
        { Format::ASTC_4x4_SRGB_BLOCK, "ASTC_4x4_SRGB_BLOCK" },
        { Format::ASTC_5x4_UNORM_BLOCK, "ASTC_5x4_UNORM_BLOCK" },
        { Format::ASTC_5x4_SRGB_BLOCK, "ASTC_5x4_SRGB_BLOCK" },
        { Format::ASTC_5x5_UNORM_BLOCK, "ASTC_5x5_UNORM_BLOCK" },
        { Format::ASTC_5x5_SRGB_BLOCK, "ASTC_5x5_SRGB_BLOCK" },
        { Format::ASTC_6x5_UNORM_BLOCK, "ASTC_6x5_UNORM_BLOCK" },
        { Format::ASTC_6x5_SRGB_BLOCK, "ASTC_6x5_SRGB_BLOCK" },
        { Format::ASTC_6x6_UNORM_BLOCK, "ASTC_6x6_UNORM_BLOCK" },
        { Format::ASTC_6x6_SRGB_BLOCK, "ASTC_6x6_SRGB_BLOCK" },
        { Format::ASTC_8x5_UNORM_BLOCK, "ASTC_8x5_UNORM_BLOCK" },
        { Format::ASTC_8x5_SRGB_BLOCK, "ASTC_8x5_SRGB_BLOCK" },
        { Format::ASTC_8x6_UNORM_BLOCK, "ASTC_8x6_UNORM_BLOCK" },
        { Format::ASTC_8x6_SRGB_BLOCK, "ASTC_8x6_SRGB_BLOCK" },
        { Format::ASTC_8x8_UNORM_BLOCK, "ASTC_8x8_UNORM_BLOCK" },
        { Format::ASTC_8x8_SRGB_BLOCK, "ASTC_8x8_SRGB_BLOCK" },
        { Format::ASTC_10x5_UNORM_BLOCK, "ASTC_10x5_UNORM_BLOCK" },
        { Format::ASTC_10x5_SRGB_BLOCK, "ASTC_10x5_SRGB_BLOCK" },
        { Format::ASTC_10x6_UNORM_BLOCK, "ASTC_10x6_UNORM_BLOCK" },
        { Format::ASTC_10x6_SRGB_BLOCK, "ASTC_10x6_SRGB_BLOCK" },
        { Format::ASTC_10x8_UNORM_BLOCK, "ASTC_10x8_UNORM_BLOCK" },
        { Format::ASTC_10x8_SRGB_BLOCK, "ASTC_10x8_SRGB_BLOCK" },
        { Format::ASTC_10x10_UNORM_BLOCK, "ASTC_10x10_UNORM_BLOCK" },
        { Format::ASTC_10x10_SRGB_BLOCK, "ASTC_10x10_SRGB_BLOCK" },
        { Format::ASTC_12x10_UNORM_BLOCK, "ASTC_12x10_UNORM_BLOCK" },
        { Format::ASTC_12x10_SRGB_BLOCK, "ASTC_12x10_SRGB_BLOCK" },
        { Format::ASTC_12x12_UNORM_BLOCK, "ASTC_12x12_UNORM_BLOCK" },
        { Format::ASTC_12x12_SRGB_BLOCK, "ASTC_12x12_SRGB_BLOCK" },
        { Format::G8B8G8R8_422_UNORM, "G8B8G8R8_422_UNORM" },
        { Format::B8G8R8G8_422_UNORM, "B8G8R8G8_422_UNORM" },
        { Format::G8_B8_R8_3PLANE_420_UNORM, "G8_B8_R8_3PLANE_420_UNORM" },
        { Format::G8_B8R8_2PLANE_420_UNORM, "G8_B8R8_2PLANE_420_UNORM" },
        { Format::G8_B8_R8_3PLANE_422_UNORM, "G8_B8_R8_3PLANE_422_UNORM" },
        { Format::G8_B8R8_2PLANE_422_UNORM, "G8_B8R8_2PLANE_422_UNORM" },
        { Format::G8_B8_R8_3PLANE_444_UNORM, "G8_B8_R8_3PLANE_444_UNORM" },
        { Format::R10X6_UNORM_PACK16, "R10X6_UNORM_PACK16" },
        { Format::R10X6G10X6_UNORM_2PACK16, "R10X6G10X6_UNORM_2PACK16" },
        {
            Format::R10X6G10X6B10X6A10X6_UNORM_4PACK16,
            "R10X6G10X6B10X6A10X6_UNORM_4PACK16"
        },
        {
            Format::G10X6B10X6G10X6R10X6_422_UNORM_4PACK16,
            "G10X6B10X6G10X6R10X6_422_UNORM_4PACK16"
        },
        {
            Format::B10X6G10X6R10X6G10X6_422_UNORM_4PACK16,
            "B10X6G10X6R10X6G10X6_422_UNORM_4PACK16"
        },
        {
            Format::G10X6_B10X6_R10X6_3PLANE_420_UNORM_3PACK16,
            "G10X6_B10X6_R10X6_3PLANE_420_UNORM_3PACK16"
        },
        {
            Format::G10X6_B10X6R10X6_2PLANE_420_UNORM_3PACK16,
            "G10X6_B10X6R10X6_2PLANE_420_UNORM_3PACK16"
        },
        {
            Format::G10X6_B10X6_R10X6_3PLANE_422_UNORM_3PACK16,
            "G10X6_B10X6_R10X6_3PLANE_422_UNORM_3PACK16"
        },
        {
            Format::G10X6_B10X6R10X6_2PLANE_422_UNORM_3PACK16,
            "G10X6_B10X6R10X6_2PLANE_422_UNORM_3PACK16"
        },
        {
            Format::G10X6_B10X6_R10X6_3PLANE_444_UNORM_3PACK16,
            "G10X6_B10X6_R10X6_3PLANE_444_UNORM_3PACK16"
        },
        { Format::R12X4_UNORM_PACK16, "R12X4_UNORM_PACK16" },
        { Format::R12X4G12X4_UNORM_2PACK16, "R12X4G12X4_UNORM_2PACK16" },
        {
            Format::R12X4G12X4B12X4A12X4_UNORM_4PACK16,
            "R12X4G12X4B12X4A12X4_UNORM_4PACK16"
        },
        {
            Format::G12X4B12X4G12X4R12X4_422_UNORM_4PACK16,
            "G12X4B12X4G12X4R12X4_422_UNORM_4PACK16"
        },
        {
            Format::B12X4G12X4R12X4G12X4_422_UNORM_4PACK16,
            "B12X4G12X4R12X4G12X4_422_UNORM_4PACK16"
        },
        {
            Format::G12X4_B12X4_R12X4_3PLANE_420_UNORM_3PACK16,
            "G12X4_B12X4_R12X4_3PLANE_420_UNORM_3PACK16"
        },
        {
            Format::G12X4_B12X4R12X4_2PLANE_420_UNORM_3PACK16,
            "G12X4_B12X4R12X4_2PLANE_420_UNORM_3PACK16"
        },
        {
            Format::G12X4_B12X4_R12X4_3PLANE_422_UNORM_3PACK16,
            "G12X4_B12X4_R12X4_3PLANE_422_UNORM_3PACK16"
        },
        {
            Format::G12X4_B12X4R12X4_2PLANE_422_UNORM_3PACK16,
            "G12X4_B12X4R12X4_2PLANE_422_UNORM_3PACK16"
        },
        {
            Format::G12X4_B12X4_R12X4_3PLANE_444_UNORM_3PACK16,
            "G12X4_B12X4_R12X4_3PLANE_444_UNORM_3PACK16"
        },
        { Format::G16B16G16R16_422_UNORM, "G16B16G16R16_422_UNORM" },
        { Format::B16G16R16G16_422_UNORM, "B16G16R16G16_422_UNORM" },
        { Format::G16_B16_R16_3PLANE_420_UNORM, "G16_B16_R16_3PLANE_420_UNORM" },
        { Format::G16_B16R16_2PLANE_420_UNORM, "G16_B16R16_2PLANE_420_UNORM" },
        { Format::G16_B16_R16_3PLANE_422_UNORM, "G16_B16_R16_3PLANE_422_UNORM" },
        { Format::G16_B16R16_2PLANE_422_UNORM, "G16_B16R16_2PLANE_422_UNORM" },
        { Format::G16_B16_R16_3PLANE_444_UNORM, "G16_B16_R16_3PLANE_444_UNORM" },
        { Format::G8_B8R8_2PLANE_444_UNORM, "G8_B8R8_2PLANE_444_UNORM" },
        {
            Format::G10X6_B10X6R10X6_2PLANE_444_UNORM_3PACK16,
            "G10X6_B10X6R10X6_2PLANE_444_UNORM_3PACK16"
        },
        {
            Format::G12X4_B12X4R12X4_2PLANE_444_UNORM_3PACK16,
            "G12X4_B12X4R12X4_2PLANE_444_UNORM_3PACK16"
        },
        { Format::G16_B16R16_2PLANE_444_UNORM, "G16_B16R16_2PLANE_444_UNORM" },
        { Format::A4R4G4B4_UNORM_PACK16, "A4R4G4B4_UNORM_PACK16" },
        { Format::A4B4G4R4_UNORM_PACK16, "A4B4G4R4_UNORM_PACK16" },
        { Format::ASTC_4x4_SFLOAT_BLOCK, "ASTC_4x4_SFLOAT_BLOCK" },
        { Format::ASTC_5x4_SFLOAT_BLOCK, "ASTC_5x4_SFLOAT_BLOCK" },
        { Format::ASTC_5x5_SFLOAT_BLOCK, "ASTC_5x5_SFLOAT_BLOCK" },
        { Format::ASTC_6x5_SFLOAT_BLOCK, "ASTC_6x5_SFLOAT_BLOCK" },
        { Format::ASTC_6x6_SFLOAT_BLOCK, "ASTC_6x6_SFLOAT_BLOCK" },
        { Format::ASTC_8x5_SFLOAT_BLOCK, "ASTC_8x5_SFLOAT_BLOCK" },
        { Format::ASTC_8x6_SFLOAT_BLOCK, "ASTC_8x6_SFLOAT_BLOCK" },
        { Format::ASTC_8x8_SFLOAT_BLOCK, "ASTC_8x8_SFLOAT_BLOCK" },
        { Format::ASTC_10x5_SFLOAT_BLOCK, "ASTC_10x5_SFLOAT_BLOCK" },
        { Format::ASTC_10x6_SFLOAT_BLOCK, "ASTC_10x6_SFLOAT_BLOCK" },
        { Format::ASTC_10x8_SFLOAT_BLOCK, "ASTC_10x8_SFLOAT_BLOCK" },
        { Format::ASTC_10x10_SFLOAT_BLOCK, "ASTC_10x10_SFLOAT_BLOCK" },
        { Format::ASTC_12x10_SFLOAT_BLOCK, "ASTC_12x10_SFLOAT_BLOCK" },
        { Format::ASTC_12x12_SFLOAT_BLOCK, "ASTC_12x12_SFLOAT_BLOCK" },
        { Format::PVRTC1_2BPP_UNORM_BLOCK_IMG, "PVRTC1_2BPP_UNORM_BLOCK_IMG" },
        { Format::PVRTC1_4BPP_UNORM_BLOCK_IMG, "PVRTC1_4BPP_UNORM_BLOCK_IMG" },
        { Format::PVRTC2_2BPP_UNORM_BLOCK_IMG, "PVRTC2_2BPP_UNORM_BLOCK_IMG" },
        { Format::PVRTC2_4BPP_UNORM_BLOCK_IMG, "PVRTC2_4BPP_UNORM_BLOCK_IMG" },
        { Format::PVRTC1_2BPP_SRGB_BLOCK_IMG, "PVRTC1_2BPP_SRGB_BLOCK_IMG" },
        { Format::PVRTC1_4BPP_SRGB_BLOCK_IMG, "PVRTC1_4BPP_SRGB_BLOCK_IMG" },
        { Format::PVRTC2_2BPP_SRGB_BLOCK_IMG, "PVRTC2_2BPP_SRGB_BLOCK_IMG" },
        { Format::PVRTC2_4BPP_SRGB_BLOCK_IMG, "PVRTC2_4BPP_SRGB_BLOCK_IMG" },
        { Format::R16G16_SFIXED5_NV, "R16G16_SFIXED5_NV" },
        { Format::A1B5G5R5_UNORM_PACK16_KHR, "A1B5G5R5_UNORM_PACK16_KHR" },
        { Format::A8_UNORM_KHR, "A8_UNORM_KHR" }
    };

    // provided by VK_KHR_surface
    // https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VkColorSpaceKHR.html
    enum class ColorSpace : int32_t
    {
        SrgbNonlinearKhr = 0,

        // provided by VK_EXT_swapchain_colorspace
        DisplayP3NonlinearExt = 1000104001,

        // provided by VK_EXT_swapchain_colorspace
        ExtendedSrgbLinearExt = 1000104002,

        // provided by VK_EXT_swapchain_colorspace
        DisplayP3LinearExt = 1000104003,

        // provided by VK_EXT_swapchain_colorspace
        DciP3NonlinearExt = 1000104004,

        // provided by VK_EXT_swapchain_colorspace
        Bt709LinearExt = 1000104005,

        // provided by VK_EXT_swapchain_colorspace
        Bt709NonlinearExt = 1000104006,

        // provided by VK_EXT_swapchain_colorspace
        Bt2020LinearExt = 1000104007,

        // provided by VK_EXT_swapchain_colorspace
        Hdr10St2084Ext = 1000104008,

        // provided by VK_EXT_swapchain_colorspace
        DolbyVisionExt = 1000104009,

        // provided by VK_EXT_swapchain_colorspace
        Hdr10HlgExt = 1000104010,

        // provided by VK_EXT_swapchain_colorspace
        AdobeRgbLinearExt = 1000104011,

        // provided by VK_EXT_swapchain_colorspace
        AdobeRgbNonlinearExt = 1000104012,

        // provided by VK_EXT_swapchain_colorspace
        PassThroughExt = 1000104013,

        // provided by VK_EXT_swapchain_colorspace
        ExtendedSrgbNonlinearExt = 1000104014,

        // provided by VK_AMD_display_native_hdr
        DisplayNativeAmd = 1000213000,

        // VK_COLORSPACE_SRGB_NONLINEAR_KHR is a deprecated alias
        RgbNonlinearKhr = SrgbNonlinearKhr,

        // provided by VK_EXT_swapchain_colorspace
        // VK_COLOR_SPACE_DCI_P3_LINEAR_EXT is a deprecated alias
        DciP3LinearExt = DisplayP3LinearExt
    };

    static EnumStrMap<ColorSpace> ColorSpace_strmap{
        { ColorSpace::SrgbNonlinearKhr, "sRGB Nonlinear KHR" },
        { ColorSpace::DisplayP3NonlinearExt, "Display P3 Nonlinear EXT" },
        { ColorSpace::ExtendedSrgbLinearExt, "Extended sRGB Linear EXT" },
        { ColorSpace::DisplayP3LinearExt, "Display P3 Linear EXT" },
        { ColorSpace::DciP3NonlinearExt, "DCI-P3 Nonlinear EXT" },
        { ColorSpace::Bt709LinearExt, "BT.709 Linear EXT" },
        { ColorSpace::Bt709NonlinearExt, "BT.709 Nonlinear EXT" },
        { ColorSpace::Bt2020LinearExt, "BT.2020 Linear EXT" },
        { ColorSpace::Hdr10St2084Ext, "HDR10 ST.2084 EXT" },
        { ColorSpace::DolbyVisionExt, "DolbyVision EXT" },
        { ColorSpace::Hdr10HlgExt, "HDR10 HLG EXT" },
        { ColorSpace::AdobeRgbLinearExt, "Adobe RGB Linear EXT" },
        { ColorSpace::AdobeRgbNonlinearExt, "Adobe RGB Nonlinear EXT" },
        { ColorSpace::PassThroughExt, "Pass Through EXT" },
        { ColorSpace::ExtendedSrgbNonlinearExt, "Extended sRGB Nonlinear EXT" },
        { ColorSpace::DisplayNativeAmd, "Display Native AMD" }
    };

    // provided by VK_KHR_surface
    // https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VkSurfaceFormatKHR.html
    struct SurfaceFormat
    {
        Format format;
        ColorSpace color_space;
    };

    SurfaceFormat SurfaceFormat_from_vk(
        const VkSurfaceFormatKHR& vk_surface_format
    );

    // provided by VK_KHR_surface
    // https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VkPresentModeKHR.html
    enum class PresentMode : int32_t
    {
        ImmediateKhr = 0,
        MailboxKhr = 1,
        FifoKhr = 2,
        FifoRelaxedKhr = 3,

        // provided by VK_KHR_shared_presentable_image
        SharedDemandRefreshKhr = 1000111000,

        // provided by VK_KHR_shared_presentable_image
        SharedContinuousRefreshKhr = 1000111001
    };

    static EnumStrMap<PresentMode> PresentMode_strmap{
        { PresentMode::ImmediateKhr, "Immediate KHR" },
        { PresentMode::MailboxKhr, "Mailbox KHR" },
        { PresentMode::FifoKhr, "FIFO KHR" },
        { PresentMode::FifoRelaxedKhr, "FIFO Relaxed KHR" },
        { PresentMode::SharedDemandRefreshKhr, "Shared Demand Refresh KHR" },
        {
            PresentMode::SharedContinuousRefreshKhr,
            "Shared Continuous Refresh KHR"
        }
    };

    // provided by VK_KHR_surface
    // https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/vkGetPhysicalDeviceSurfaceCapabilitiesKHR.html
    // https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/vkGetPhysicalDeviceSurfaceFormatsKHR.html
    // https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/vkGetPhysicalDeviceSurfacePresentModesKHR.html
    struct SwapchainSupport
    {
        SurfaceCapabilities capabilities;
        std::vector<SurfaceFormat> surface_formats;
        std::vector<PresentMode> present_modes;
    };

    // https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VkInstanceCreateInfo.html
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

    // provided by VK_EXT_debug_utils
    // https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VkDebugUtilsLabelEXT.html
    struct DebugLabel
    {
        std::string name;
        std::array<float, 4> color;
    };

    DebugLabel DebugLabel_from_vk(const VkDebugUtilsLabelEXT& vk_label);

    // provided by VK_EXT_debug_utils
    // https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VkDebugUtilsObjectNameInfoEXT.html
    struct DebugObjectInfo
    {
        ObjectType type;
        uint64_t handle;
        std::string name;
    };

    DebugObjectInfo DebugObjectInfo_from_vk(
        const VkDebugUtilsObjectNameInfoEXT& vk_info
    );

    // provided by VK_EXT_debug_utils
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

    DebugMessageData DebugMessageData_from_vk(
        const VkDebugUtilsMessengerCallbackDataEXT& vk_data
    );

    // provided by VK_EXT_debug_utils
    // https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/PFN_vkDebugUtilsMessengerCallbackEXT.html
    using DebugCallback = std::function<void(
        DebugMessageSeverity,
        DebugMessageTypeFlags,
        const DebugMessageData&
        )>;

    // https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VkDeviceQueueCreateInfo.html
    struct QueueRequest
    {
        QueueRequestFlags flags;
        uint32_t queue_family_index;
        uint32_t num_queues_to_create;
        std::vector<float> priorities; // * same size as num_queues_to_create
    };

    VkDeviceQueueCreateInfo QueueRequest_to_vk(
        const QueueRequest& request,
        std::vector<float>& waste_priorities
    );

    // https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VkDeviceCreateInfo.html
    struct DeviceConfig
    {
        std::vector<QueueRequest> queue_requests;
        std::vector<std::string> extensions;
        PhysicalDeviceFeatures enabled_features;
    };

    // provided by VK_KHR_swapchain
    // https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VkSwapchainCreateFlagBitsKHR.html
    struct SwapchainFlags
    {
        bool split_instance_bind_regions : 1 = false;
        bool protected_ : 1 = false;
        bool mutable_format : 1 = false;
        bool deferred_memory_allocation : 1 = false;
    };

    // https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VkSharingMode.html
    enum class SharingMode : int32_t
    {
        Exclusive = 0,
        Concurrent = 1
    };

    static EnumStrMap<SharingMode> SharingMode_strmap{
        { SharingMode::Exclusive, "Exclusive" },
        { SharingMode::Concurrent, "Concurrent" }
    };

    // provided by VK_KHR_swapchain
    // https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VkSwapchainCreateInfoKHR.html
    struct SwapchainConfig
    {
        SwapchainFlags flags;
        uint32_t min_image_count;
        Format image_format;
        ColorSpace image_color_space;
        Extent2d image_extent;
        uint32_t image_array_layers;
        ImageUsageFlags image_usage;
        SharingMode image_sharing_mode;
        std::vector<uint32_t> queue_family_indices;
        SurfaceTransform pre_transform;
        CompositeAlpha composite_alpha;
        PresentMode present_mode;
        bool clipped;
    };

    // https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VkImageViewCreateFlagBits.html
    struct ImageViewFlags
    {
        bool fragment_density_map_dynamic : 1 = false;
        bool descriptor_buffer_capture_replay : 1 = false;
        bool fragment_density_map_deferred : 1 = false;
    };

    // https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VkImageViewType.html
    enum class ImageViewType : int32_t
    {
        _1d = 0,
        _2d = 1,
        _3d = 2,
        Cube = 3,
        _1dArray = 4,
        _2dArray = 5,
        CubeArray = 6
    };

    static EnumStrMap<ImageViewType> ImageViewType_strmap{
        { ImageViewType::_1d, "1D" },
        { ImageViewType::_2d, "2D" },
        { ImageViewType::_3d, "3D" },
        { ImageViewType::Cube, "Cube" },
        { ImageViewType::_1dArray, "1D Array" },
        { ImageViewType::_2dArray, "2D Array" },
        { ImageViewType::CubeArray, "Cube Array" }
    };

    // https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VkComponentSwizzle.html
    enum class ComponentSwizzle : int32_t
    {
        Identity = 0,
        Zero = 1,
        One = 2,
        R = 3,
        G = 4,
        B = 5,
        A = 6
    };

    static EnumStrMap<ComponentSwizzle> ComponentSwizzle_strmap{
        { ComponentSwizzle::Identity, "Identity" },
        { ComponentSwizzle::Zero, "Zero" },
        { ComponentSwizzle::One, "One" },
        { ComponentSwizzle::R, "R" },
        { ComponentSwizzle::G, "G" },
        { ComponentSwizzle::B, "B" },
        { ComponentSwizzle::A, "A" }
    };

    // https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VkComponentMapping.html
    struct ComponentMapping
    {
        ComponentSwizzle r = ComponentSwizzle::Identity;
        ComponentSwizzle g = ComponentSwizzle::Identity;
        ComponentSwizzle b = ComponentSwizzle::Identity;
        ComponentSwizzle a = ComponentSwizzle::Identity;
    };

    VkComponentMapping ComponentMapping_to_vk(const ComponentMapping& mapping);

    // https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VkImageAspectFlagBits.html
    struct ImageAspectFlags
    {
        bool color : 1 = false;
        bool depth : 1 = false;
        bool stencil : 1 = false;
        bool metadata : 1 = false;
        bool plane_0 : 1 = false;
        bool plane_1 : 1 = false;
        bool plane_2 : 1 = false;
        bool none : 1 = false;
        bool memory_plane_0 : 1 = false;
        bool memory_plane_1 : 1 = false;
        bool memory_plane_2 : 1 = false;
        bool memory_plane_3 : 1 = false;
    };

    // https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VkImageSubresourceRange.html
    struct ImageSubresourceRange
    {
        ImageAspectFlags aspect_mask;
        uint32_t base_mip_level;
        uint32_t level_count;
        uint32_t base_array_layer;
        uint32_t layer_count;
    };

    VkImageSubresourceRange ImageSubresourceRange_to_vk(
        const ImageSubresourceRange& range
    );

    // https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VkImageViewCreateInfo.html
    struct ImageViewConfig
    {
        ImageViewFlags flags;
        ImageViewType view_type;
        Format format;
        ComponentMapping components;
        ImageSubresourceRange subresource_range;
    };

    // https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VkPipelineShaderStageCreateFlagBits.html
    struct FakeShaderStageFlagsCREATEFLAGS
    {
        bool allow_varying_subgroup_size : 1 = false;
        bool require_full_subgroups : 1 = false;
    };

    // https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VkShaderStageFlagBits.html
    using ActualShaderStageFlags = TODO();

    // https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VkSpecializationMapEntry.html
    struct SpecializationMapEntry
    {
        uint32_t constant_id;
        uint32_t offset;
        size_t size;
    };

    VkSpecializationMapEntry SpecializationMapEntry_to_vk(
        const SpecializationMapEntry& entry
    );

    // https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VkSpecializationInfo.html
    struct SpecializationInfo
    {
        std::vector<SpecializationMapEntry> map_entries;

        // std::copy your data here (and use reinterpret_cast)
        std::vector<uint8_t> data;
    };

    VkSpecializationInfo SpecializationInfo_to_vk(
        const SpecializationInfo& info,
        std::vector<VkSpecializationMapEntry>& waste_vk_map_entries,
        std::vector<uint8_t>& waste_data
    );

    class ShaderModule;

    // https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VkPipelineShaderStageCreateInfo.html
    struct ShaderStage
    {
        ShaderStageFlags flags;
        ShaderStageType type;
        std::shared_ptr<ShaderModule> module;
        std::string entry_point;
        std::optional<SpecializationInfo> specialization_info;
    };

    VkPipelineShaderStageCreateInfo ShaderStage_to_vk(
        const ShaderStage& stage,
        VkSpecializationInfo& waste_vk_specialization_info,
        std::vector<VkSpecializationMapEntry>& waste_vk_map_entries,
        std::vector<uint8_t>& waste_data
    );

    // https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VkDynamicState.html
    enum class DynamicState : int32_t
    {
        Viewport = 0,
        Scissor = 1,
        LineWidth = 2,
        DepthBias = 3,
        BlendConstants = 4,
        DepthBounds = 5,
        StencilCompareMask = 6,
        StencilWriteMask = 7,
        StencilReference = 8,

        // provided by VK_VERSION_1_3
        CullMode = 1000267000,

        // provided by VK_VERSION_1_3
        FrontFace = 1000267001,

        // provided by VK_VERSION_1_3
        PrimitiveTopology = 1000267002,

        // provided by VK_VERSION_1_3
        ViewportWithCount = 1000267003,

        // provided by VK_VERSION_1_3
        ScissorWithCount = 1000267004,

        // provided by VK_VERSION_1_3
        VertexInputBindingStride = 1000267005,

        // provided by VK_VERSION_1_3
        DepthTestEnable = 1000267006,

        // provided by VK_VERSION_1_3
        DepthWriteEnable = 1000267007,

        // provided by VK_VERSION_1_3
        DepthCompareOp = 1000267008,

        // provided by VK_VERSION_1_3
        DepthBoundsTestEnable = 1000267009,

        // provided by VK_VERSION_1_3
        StencilTestEnable = 1000267010,

        // provided by VK_VERSION_1_3
        StencilOp = 1000267011,

        // provided by VK_VERSION_1_3
        RasterizerDiscardEnable = 1000377001,

        // provided by VK_VERSION_1_3
        DepthBiasEnable = 1000377002,

        // provided by VK_VERSION_1_3
        PrimitiveRestartEnable = 1000377004,

        // provided by VK_NV_clip_space_w_scaling
        ViewportWScalingNv = 1000087000,

        // provided by VK_EXT_discard_rectangles
        DiscardRectangleExt = 1000099000,

        // provided by VK_EXT_discard_rectangles
        DiscardRectangleEnableExt = 1000099001,

        // provided by VK_EXT_discard_rectangles
        DiscardRectangleModeExt = 1000099002,

        // provided by VK_EXT_sample_locations
        SampleLocationsExt = 1000143000,

        // provided by VK_KHR_ray_tracing_pipeline
        RayTracingPipelineStackSizeKhr = 1000347000,

        // provided by VK_NV_shading_rate_image
        ViewportShadingRatePaletteNv = 1000164004,

        // provided by VK_NV_shading_rate_image
        ViewportCoarseSampleOrderNv = 1000164006,

        // provided by VK_NV_scissor_exclusive
        ExclusiveScissorEnableNv = 1000205000,

        // provided by VK_NV_scissor_exclusive
        ExclusiveScissorNv = 1000205001,

        // provided by VK_KHR_fragment_shading_rate
        FragmentShadingRateKhr = 1000226000,

        // provided by VK_EXT_vertex_input_dynamic_state
        VertexInputExt = 1000352000,

        // provided by VK_EXT_extended_dynamic_state2
        PatchControlPointsExt = 1000377000,

        // provided by VK_EXT_extended_dynamic_state2
        LogicOpExt = 1000377003,

        // provided by VK_EXT_color_write_enable
        ColorWriteEnableExt = 1000381000,

        // provided by VK_EXT_extended_dynamic_state3
        DepthClampEnableExt = 1000455003,

        // provided by VK_EXT_extended_dynamic_state3
        PolygonModeExt = 1000455004,

        // provided by VK_EXT_extended_dynamic_state3
        RasterizationSamplesExt = 1000455005,

        // provided by VK_EXT_extended_dynamic_state3
        SampleMaskExt = 1000455006,

        // provided by VK_EXT_extended_dynamic_state3
        AlphaToCoverageEnableExt = 1000455007,

        // provided by VK_EXT_extended_dynamic_state3
        AlphaToOneEnableExt = 1000455008,

        // provided by VK_EXT_extended_dynamic_state3
        LogicOpEnableExt = 1000455009,

        // provided by VK_EXT_extended_dynamic_state3
        ColorBlendEnableExt = 1000455010,

        // provided by VK_EXT_extended_dynamic_state3
        ColorBlendEquationExt = 1000455011,

        // provided by VK_EXT_extended_dynamic_state3
        ColorWriteMaskExt = 1000455012,

        // provided by VK_EXT_extended_dynamic_state3 with VK_KHR_maintenance2
        // or VK_VERSION_1_1
        TessellationDomainOriginExt = 1000455002,

        // provided by VK_EXT_extended_dynamic_state3 with
        // VK_EXT_transform_feedback
        RasterizationStreamExt = 1000455013,

        // provided by VK_EXT_conservative_rasterization with
        // VK_EXT_extended_dynamic_state3
        ConservativeRasterizationModeExt = 1000455014,

        // provided by VK_EXT_conservative_rasterization with
        // VK_EXT_extended_dynamic_state3
        ExtraPrimitiveOverestimationSizeExt = 1000455015,

        // provided by VK_EXT_depth_clip_enable with
        // VK_EXT_extended_dynamic_state3
        DepthClipEnableExt = 1000455016,

        // provided by VK_EXT_extended_dynamic_state3 with
        // VK_EXT_sample_locations
        SampleLocationsEnableExt = 1000455017,

        // provided by VK_EXT_blend_operation_advanced with
        // VK_EXT_extended_dynamic_state3
        ColorBlendAdvancedExt = 1000455018,

        // provided by VK_EXT_extended_dynamic_state3 with
        // VK_EXT_provoking_vertex
        ProvokingVertexModeExt = 1000455019,

        // provided by VK_EXT_extended_dynamic_state3 with
        // VK_EXT_line_rasterization
        LineRasterizationModeExt = 1000455020,

        // provided by VK_EXT_extended_dynamic_state3 with
        // VK_EXT_line_rasterization
        LineStippleEnableExt = 1000455021,

        // provided by VK_EXT_depth_clip_control with
        // VK_EXT_extended_dynamic_state3
        DepthClipNegativeOneToOneExt = 1000455022,

        // provided by VK_EXT_extended_dynamic_state3 with
        // VK_NV_clip_space_w_scaling
        ViewportWScalingEnableNv = 1000455023,

        // provided by VK_EXT_extended_dynamic_state3 with
        // VK_NV_viewport_swizzle
        ViewportSwizzleNv = 1000455024,

        // provided by VK_EXT_extended_dynamic_state3 with
        // VK_NV_fragment_coverage_to_color
        CoverageToColorEnableNv = 1000455025,

        // provided by VK_EXT_extended_dynamic_state3 with
        // VK_NV_fragment_coverage_to_color
        CoverageToColorLocationNv = 1000455026,

        // provided by VK_EXT_extended_dynamic_state3 with
        // VK_NV_framebuffer_mixed_samples
        CoverageModulationModeNv = 1000455027,

        // provided by VK_EXT_extended_dynamic_state3 with
        // VK_NV_framebuffer_mixed_samples
        CoverageModulationTableEnableNv = 1000455028,

        // provided by VK_EXT_extended_dynamic_state3 with
        // VK_NV_framebuffer_mixed_samples
        CoverageModulationTableNv = 1000455029,

        // provided by VK_EXT_extended_dynamic_state3 with
        // VK_NV_shading_rate_image
        ShadingRateImageEnableNv = 1000455030,

        // provided by VK_EXT_extended_dynamic_state3 with
        // VK_NV_representative_fragment_test
        RepresentativeFragmentTestEnableNv = 1000455031,

        // provided by VK_EXT_extended_dynamic_state3 with
        // VK_NV_coverage_reduction_mode
        CoverageReductionModeNv = 1000455032,

        // provided by VK_EXT_attachment_feedback_loop_dynamic_state
        AttachmentFeedbackLoopEnableExt = 1000524000,

        // provided by VK_KHR_line_rasterization
        LineStippleKhr = 1000259000,

        // provided by VK_EXT_line_rasterization
        LineStippleExt = LineStippleKhr,

        // provided by VK_EXT_extended_dynamic_state
        CullModeExt = CullMode,

        // provided by VK_EXT_extended_dynamic_state
        FrontFaceExt = FrontFace,

        // provided by VK_EXT_extended_dynamic_state
        PrimitiveTopologyExt = PrimitiveTopology,

        // provided by VK_EXT_extended_dynamic_state
        ViewportWithCountExt = ViewportWithCount,

        // provided by VK_EXT_extended_dynamic_state
        ScissorWithCountExt = ScissorWithCount,

        // provided by VK_EXT_extended_dynamic_state
        VertexInputBindingStrideExt = VertexInputBindingStride,

        // provided by VK_EXT_extended_dynamic_state
        DepthTestEnableExt = DepthTestEnable,

        // provided by VK_EXT_extended_dynamic_state
        DepthWriteEnableExt = DepthWriteEnable,

        // provided by VK_EXT_extended_dynamic_state
        DepthCompareOpExt = DepthCompareOp,

        // provided by VK_EXT_extended_dynamic_state
        DepthBoundsTestEnableExt = DepthBoundsTestEnable,

        // provided by VK_EXT_extended_dynamic_state
        StencilTestEnableExt = StencilTestEnable,

        // provided by VK_EXT_extended_dynamic_state
        StencilOpExt = StencilOp,

        // provided by VK_EXT_extended_dynamic_state2
        RasterizerDiscardEnableExt = RasterizerDiscardEnable,

        // provided by VK_EXT_extended_dynamic_state2
        DepthBiasEnableExt = DepthBiasEnable,

        // provided by VK_EXT_extended_dynamic_state2
        PrimitiveRestartEnableExt = PrimitiveRestartEnable
    };

    static EnumStrMap<DynamicState> DynamicState_strmap{
        {
            DynamicState::Viewport,
            "Viewport"
        },
    {
        DynamicState::Scissor,
        "Scissor"
    },
    {
        DynamicState::LineWidth,
        "LineWidth"
    },
    {
        DynamicState::DepthBias,
        "DepthBias"
    },
    {
        DynamicState::BlendConstants,
        "BlendConstants"
    },
    {
        DynamicState::DepthBounds,
        "DepthBounds"
    },
    {
        DynamicState::StencilCompareMask,
        "StencilCompareMask"
    },
    {
        DynamicState::StencilWriteMask,
        "StencilWriteMask"
    },
    {
        DynamicState::StencilReference,
        "StencilReference"
    },
    {
        DynamicState::CullMode,
        "CullMode"
    },
    {
        DynamicState::FrontFace,
        "FrontFace"
    },
    {
        DynamicState::PrimitiveTopology,
        "PrimitiveTopology"
    },
    {
        DynamicState::ViewportWithCount,
        "ViewportWithCount"
    },
    {
        DynamicState::ScissorWithCount,
        "ScissorWithCount"
    },
    {
        DynamicState::VertexInputBindingStride,
        "VertexInputBindingStride"
    },
    {
        DynamicState::DepthTestEnable,
        "DepthTestEnable"
    },
    {
        DynamicState::DepthWriteEnable,
        "DepthWriteEnable"
    },
    {
        DynamicState::DepthCompareOp,
        "DepthCompareOp"
    },
    {
        DynamicState::DepthBoundsTestEnable,
        "DepthBoundsTestEnable"
    },
    {
        DynamicState::StencilTestEnable,
        "StencilTestEnable"
    },
    {
        DynamicState::StencilOp,
        "StencilOp"
    },
    {
        DynamicState::RasterizerDiscardEnable,
        "RasterizerDiscardEnable"
    },
    {
        DynamicState::DepthBiasEnable,
        "DepthBiasEnable"
    },
    {
        DynamicState::PrimitiveRestartEnable,
        "PrimitiveRestartEnable"
    },
    {
        DynamicState::ViewportWScalingNv,
        "ViewportWScalingNv"
    },
    {
        DynamicState::DiscardRectangleExt,
        "DiscardRectangleExt"
    },
    {
        DynamicState::DiscardRectangleEnableExt,
        "DiscardRectangleEnableExt"
    },
    {
        DynamicState::DiscardRectangleModeExt,
        "DiscardRectangleModeExt"
    },
    {
        DynamicState::SampleLocationsExt,
        "SampleLocationsExt"
    },
    {
        DynamicState::RayTracingPipelineStackSizeKhr,
        "RayTracingPipelineStackSizeKhr"
    },
    {
        DynamicState::ViewportShadingRatePaletteNv,
        "ViewportShadingRatePaletteNv"
    },
    {
        DynamicState::ViewportCoarseSampleOrderNv,
        "ViewportCoarseSampleOrderNv"
    },
    {
        DynamicState::ExclusiveScissorEnableNv,
        "ExclusiveScissorEnableNv"
    },
    {
        DynamicState::ExclusiveScissorNv,
        "ExclusiveScissorNv"
    },
    {
        DynamicState::FragmentShadingRateKhr,
        "FragmentShadingRateKhr"
    },
    {
        DynamicState::VertexInputExt,
        "VertexInputExt"
    },
    {
        DynamicState::PatchControlPointsExt,
        "PatchControlPointsExt"
    },
    {
        DynamicState::LogicOpExt,
        "LogicOpExt"
    },
    {
        DynamicState::ColorWriteEnableExt,
        "ColorWriteEnableExt"
    },
    {
        DynamicState::DepthClampEnableExt,
        "DepthClampEnableExt"
    },
    {
        DynamicState::PolygonModeExt,
        "PolygonModeExt"
    },
    {
        DynamicState::RasterizationSamplesExt,
        "RasterizationSamplesExt"
    },
    {
        DynamicState::SampleMaskExt,
        "SampleMaskExt"
    },
    {
        DynamicState::AlphaToCoverageEnableExt,
        "AlphaToCoverageEnableExt"
    },
    {
        DynamicState::AlphaToOneEnableExt,
        "AlphaToOneEnableExt"
    },
    {
        DynamicState::LogicOpEnableExt,
        "LogicOpEnableExt"
    },
    {
        DynamicState::ColorBlendEnableExt,
        "ColorBlendEnableExt"
    },
    {
        DynamicState::ColorBlendEquationExt,
        "ColorBlendEquationExt"
    },
    {
        DynamicState::ColorWriteMaskExt,
        "ColorWriteMaskExt"
    },
    {
        DynamicState::TessellationDomainOriginExt,
        "TessellationDomainOriginExt"
    },
    {
        DynamicState::RasterizationStreamExt,
        "RasterizationStreamExt"
    },
    {
        DynamicState::ConservativeRasterizationModeExt,
        "ConservativeRasterizationModeExt"
    },
    {
        DynamicState::ExtraPrimitiveOverestimationSizeExt,
        "ExtraPrimitiveOverestimationSizeExt"
    },
    {
        DynamicState::DepthClipEnableExt,
        "DepthClipEnableExt"
    },
    {
        DynamicState::SampleLocationsEnableExt,
        "SampleLocationsEnableExt"
    },
    {
        DynamicState::ColorBlendAdvancedExt,
        "ColorBlendAdvancedExt"
    },
    {
        DynamicState::ProvokingVertexModeExt,
        "ProvokingVertexModeExt"
    },
    {
        DynamicState::LineRasterizationModeExt,
        "LineRasterizationModeExt"
    },
    {
        DynamicState::LineStippleEnableExt,
        "LineStippleEnableExt"
    },
    {
        DynamicState::DepthClipNegativeOneToOneExt,
        "DepthClipNegativeOneToOneExt"
    },
    {
        DynamicState::ViewportWScalingEnableNv,
        "ViewportWScalingEnableNv"
    },
    {
        DynamicState::ViewportSwizzleNv,
        "ViewportSwizzleNv"
    },
    {
        DynamicState::CoverageToColorEnableNv,
        "CoverageToColorEnableNv"
    },
    {
        DynamicState::CoverageToColorLocationNv,
        "CoverageToColorLocationNv"
    },
    {
        DynamicState::CoverageModulationModeNv,
        "CoverageModulationModeNv"
    },
    {
        DynamicState::CoverageModulationTableEnableNv,
        "CoverageModulationTableEnableNv"
    },
    {
        DynamicState::CoverageModulationTableNv,
        "CoverageModulationTableNv"
    },
    {
        DynamicState::ShadingRateImageEnableNv,
        "ShadingRateImageEnableNv"
    },
    {
        DynamicState::RepresentativeFragmentTestEnableNv,
        "RepresentativeFragmentTestEnableNv"
    },
    {
        DynamicState::CoverageReductionModeNv,
        "CoverageReductionModeNv"
    },
    {
        DynamicState::AttachmentFeedbackLoopEnableExt,
        "AttachmentFeedbackLoopEnableExt"
    },
    {
        DynamicState::LineStippleKhr,
        "LineStippleKhr"
    }
    };

#pragma endregion

#pragma region error handling

    class Error
    {
    public:
        Error();
        Error(std::string message);
        Error(ApiResult api_result);
        Error(std::string message, ApiResult api_result);

        std::string to_string() const;

    private:
        std::string message;
        std::optional<ApiResult> api_result;

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
        using ptr = std::shared_ptr<Allocator>;

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

    class Surface;

    // https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VkPhysicalDevice.html
    class PhysicalDevice
    {
    public:
        using ptr = std::shared_ptr<PhysicalDevice>;

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

        // this will only have a value if:
        // - this extension is available: VK_KHR_SWAPCHAIN_EXTENSION_NAME
        // - a surface was provided to Context::fetch_physical_devices()
        const std::optional<SwapchainSupport>& swapchain_support() const
        {
            return _swapchain_support;
        }

        Result<std::vector<ExtensionProperties>> fetch_available_extensions(
            const std::string& layer_name = ""
        );

    protected:
        VkPhysicalDevice vk_physical_device = nullptr;

        PhysicalDeviceProperties _properties;
        PhysicalDeviceFeatures _features;
        std::vector<QueueFamily> _queue_families;
        QueueFamilyIndices _queue_family_indices;
        std::optional<SwapchainSupport> _swapchain_support;

        PhysicalDevice(
            VkPhysicalDevice vk_physical_device,
            const PhysicalDeviceProperties& properties,
            const PhysicalDeviceFeatures& features,
            const std::vector<QueueFamily>& queue_families,
            const QueueFamilyIndices& queue_family_indices
        );

        Result<> check_swapchain_support(
            const std::shared_ptr<Surface>& surface
        );

        friend class Context;
        friend class Device;

    };

    // manages a VkInstance and custom allocators, provides utility functions,
    // and is used by other classes
    // https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VkInstance.html
    class Context
    {
    public:
        using ptr = std::shared_ptr<Context>;

        Context() = delete;
        Context(const Context& other) = delete;
        Context(Context&& other) noexcept;

        // * it's best to keep at least one external reference to the allocator
        //   so that it doesn't die with the Context because the driver might
        //   still use the allocator even after the instance is destroyed.
        static Result<Context::ptr> create(
            const ContextConfig& config,
            const Allocator::ptr& allocator = nullptr
        );

        static Result<std::vector<LayerProperties>> fetch_available_layers();

        static Result<std::vector<ExtensionProperties>>
            fetch_available_extensions(
                const std::string& layer_name = ""
            );

        constexpr const ContextConfig& config() const
        {
            return _config;
        }

        constexpr const Allocator::ptr& allocator() const
        {
            return _allocator;
        }

        // * it's best to keep at least one external reference to the allocator
        //   so that it doesn't die with the Context because the driver might
        //   still use the allocator even after the instance is destroyed.
        void set_allocator(
            const Allocator::ptr& allocator
        );

        // get the VkInstance handle
        // * avoid using this as much as possible. it's generally supposed to be
        //   used internally. the only reason it's public is for surface
        //   creation which is handed off to the user to write their own little
        //   implementation based on the platform or the windowing library
        //   they're using.
        constexpr VkInstance vk_instance() const
        {
            return _vk_instance;
        }

        // get allocation callbacks pointer
        // * avoid using this as much as possible. it's generally supposed to be
        //   used internally. the only reason it's public is for surface
        //   creation which is handed off to the user to write their own little
        //   implementation based on the platform or the windowing library
        //   they're using.
        const VkAllocationCallbacks* vk_allocator_ptr() const;

        Result<std::vector<PhysicalDevice::ptr>> fetch_physical_devices(
            const std::shared_ptr<Surface>& surface = nullptr
        );

        ~Context();

    protected:
        ContextConfig _config;

        Allocator::ptr _allocator;
        VkAllocationCallbacks _vk_allocator{};

        VkInstance _vk_instance = nullptr;

        Context(
            const ContextConfig& config,
            const Allocator::ptr& allocator
        );

    };

    // provided by VK_EXT_debug_utils
    // https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VkDebugUtilsMessengerEXT.html
    class DebugMessenger
    {
    public:
        using ptr = std::shared_ptr<DebugMessenger>;

        DebugMessenger(const DebugMessenger& other) = delete;
        DebugMessenger(DebugMessenger&& other) = default;

        static Result<DebugMessenger::ptr> create(
            const Context::ptr& context,
            DebugMessageSeverityFlags message_severity_filter,
            DebugMessageTypeFlags message_type_filter,
            const DebugCallback& callback
        );

        constexpr const Context::ptr& context() const
        {
            return _context;
        }

        constexpr const DebugMessageSeverityFlags&
            message_severity_filter() const
        {
            return _message_severity_filter;
        }

        constexpr const DebugMessageTypeFlags& message_type_filter() const
        {
            return _message_type_filter;
        }

        constexpr const DebugCallback& callback() const
        {
            return _callback;
        }

        ~DebugMessenger();

    protected:
        Context::ptr _context;
        DebugMessageSeverityFlags _message_severity_filter;
        DebugMessageTypeFlags _message_type_filter;
        DebugCallback _callback;

        VkDebugUtilsMessengerEXT vk_debug_messenger = nullptr;

        DebugMessenger(
            const Context::ptr& context,
            const DebugMessageSeverityFlags& message_severity_filter,
            const DebugMessageTypeFlags& message_type_filter,
            const DebugCallback& callback
        );

    };

    // provided by VK_KHR_surface
    // https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VkSurfaceKHR.html
    class Surface
    {
    public:
        using ptr = std::shared_ptr<Surface>;

        Surface() = delete;
        Surface(const Surface& other) = delete;
        Surface(Surface&& other) = default;

        // create a surface based on a user-provided handle. this lets the user
        // write their own little surface creation implementation based on the
        // platform or the windowing library they're using.
        // * make sure to enable the required extensions for surfaces. some
        //   windowing libraries (like GLFW) provide the list for you.
        static Surface::ptr create(
            const Context::ptr& context,
            VkSurfaceKHR vk_surface
        );

        constexpr const Context::ptr& context() const
        {
            return _context;
        }

        ~Surface();

    protected:
        Context::ptr _context;

        VkSurfaceKHR vk_surface;

        Surface(
            const Context::ptr& context,
            VkSurfaceKHR vk_surface
        );

        friend class Context;
        friend class PhysicalDevice;
        friend class Swapchain;

    };

    // https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VkQueue.html
    class Queue
    {
    public:
        using ptr = std::shared_ptr<Queue>;

        Queue() = delete;
        Queue(const Queue& other) = delete;
        Queue(Queue&& other) = default;

    protected:
        VkQueue vk_queue;

        Queue(VkQueue vk_queue);

    };

    // https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VkDevice.html
    class Device
    {
    public:
        using ptr = std::shared_ptr<Device>;

        Device() = delete;
        Device(const Device& other) = delete;
        Device(Device&& other) = default;

        static Result<Device::ptr> create(
            const Context::ptr& context,
            const PhysicalDevice::ptr& physical_device,
            const DeviceConfig& config
        );

        constexpr const Context::ptr& context() const
        {
            return _context;
        }

        constexpr const PhysicalDevice::ptr& physical_device() const
        {
            return _physical_device;
        }

        constexpr const DeviceConfig& config() const
        {
            return _config;
        }

        Queue::ptr retrieve_queue(
            uint32_t queue_family_index,
            uint32_t queue_index
        );

        ~Device();

    protected:
        Context::ptr _context;
        PhysicalDevice::ptr _physical_device;
        DeviceConfig _config;

        VkDevice vk_device = nullptr;

        Device(
            const Context::ptr& context,
            const PhysicalDevice::ptr& physical_device,
            const DeviceConfig& config
        );

        friend class Swapchain;
        friend class ImageView;
        friend class ShaderModule;

    };

    // https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VkImage.html
    class Image
    {
    public:
        using ptr = std::shared_ptr<Image>;

        Image() = delete;
        Image(const Image& other) = delete;
        Image(Image&& other) = default;

    protected:
        VkImage vk_image;

        Image(VkImage vk_image);

        friend class ImageView;

    };

    // provided by VK_KHR_swapchain
    // https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VkSwapchainKHR.html
    class Swapchain
    {
    public:
        using ptr = std::shared_ptr<Swapchain>;

        Swapchain() = delete;
        Swapchain(const Swapchain& other) = delete;
        Swapchain(Swapchain&& other) = default;

        static Result<Swapchain::ptr> create(
            const Device::ptr& device,
            const Surface::ptr& surface,
            const SwapchainConfig& config,
            const Swapchain::ptr& old_swapchain = nullptr
        );

        constexpr const Device::ptr& device() const
        {
            return _device;
        }

        constexpr const Surface::ptr& surface() const
        {
            return _surface;
        }

        constexpr const SwapchainConfig& config() const
        {
            return _config;
        }

        constexpr const Swapchain::ptr& old_swapchain() const
        {
            return _old_swapchain;
        }

        const std::vector<Image::ptr>& images() const
        {
            return _images;
        }

        ~Swapchain();

    protected:
        Device::ptr _device;
        Surface::ptr _surface;
        SwapchainConfig _config;
        Swapchain::ptr _old_swapchain;

        VkSwapchainKHR vk_swapchain = nullptr;

        std::vector<Image::ptr> _images;

        Swapchain(
            const Device::ptr& device,
            const Surface::ptr& surface,
            const SwapchainConfig& config,
            const Swapchain::ptr& old_swapchain = nullptr
        );

    };

    // https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VkImageView.html
    class ImageView
    {
    public:
        using ptr = std::shared_ptr<ImageView>;

        ImageView() = delete;
        ImageView(const ImageView& other) = delete;
        ImageView(ImageView&& other) = default;

        static Result<ImageView::ptr> create(
            const Device::ptr& device,
            const Image::ptr& image,
            const ImageViewConfig& config
        );

        constexpr const Device::ptr& device() const
        {
            return _device;
        }

        constexpr const Image::ptr& image() const
        {
            return _image;
        }

        constexpr const ImageViewConfig& config() const
        {
            return _config;
        }

        ~ImageView();

    protected:
        Device::ptr _device;
        Image::ptr _image;
        ImageViewConfig _config;

        VkImageView vk_image_view = nullptr;

        ImageView(
            const Device::ptr& device,
            const Image::ptr& image,
            const ImageViewConfig& config
        );

    };

    // https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VkShaderModule.html
    // https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VkShaderModuleCreateInfo.html
    class ShaderModule
    {
    public:
        using ptr = std::shared_ptr<ShaderModule>;

        ShaderModule() = delete;
        ShaderModule(const ShaderModule& other) = delete;
        ShaderModule(ShaderModule&& other) = default;

        static Result<ShaderModule::ptr> create(
            const Device::ptr& device,
            const std::vector<uint8_t>& code
        );

        constexpr const Device::ptr& device() const
        {
            return _device;
        }

        ~ShaderModule();

    protected:
        Device::ptr _device;

        VkShaderModule vk_shader_module = nullptr;

        ShaderModule(const Device::ptr& device);

        friend VkPipelineShaderStageCreateInfo ShaderStage_to_vk(
            const ShaderStage& stage,
            VkSpecializationInfo& waste_vk_specialization_info,
            std::vector<VkSpecializationMapEntry>& waste_vk_map_entries,
            std::vector<uint8_t>& waste_data
        );

    };

}
