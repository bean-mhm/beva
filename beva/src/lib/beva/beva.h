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

namespace bv
{

#pragma region data-only structs, enums, and type aliases

    // defines a string conversion function for an enum that meets the following
    // requirements:
    // - enum must have a value named _ at the end
    // - enum must be based on uint8_t
    // - there must be a static constexpr const char* array named
    //   EnumName_string representing the enum values as strings.
    // - the enum values must start from 0 and increase by one (ordered)
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

    // https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VkResult.html
    class ApiResult
    {
    public:
        ApiResult(VkResult vk_result);

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

    AllocationScope AllocationScope_from_vk(
        VkSystemAllocationScope vk_scope
    );

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

    InternalAllocationType InternalAllocationType_from_vk(
        VkInternalAllocationType vk_type
    );

    enum class VulkanApiVersion : uint8_t
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

    uint32_t VulkanApiVersion_to_vk(VulkanApiVersion version);

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

    PhysicalDeviceType PhysicalDeviceType_from_vk(
        VkPhysicalDeviceType vk_type
    );

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

    SampleCountFlags SampleCountFlags_from_vk(
        VkSampleCountFlags vk_flags
    );

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

    SampleCount SampleCount_from_vk(VkSampleCountFlagBits vk_sample_count);
    VkSampleCountFlagBits SampleCount_to_vk(SampleCount sample_count);

    // https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VkQueueFlagBits.html
    // present: https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/vkGetPhysicalDeviceSurfaceSupportKHR.html
    struct QueueFlags
    {
        bool graphics : 1 = false;
        bool presentation : 1 = false;
        bool compute : 1 = false;
        bool transfer : 1 = false;
        bool sparse_binding : 1 = false;
        bool protected_ : 1 = false;
        bool video_decode : 1 = false;
        bool optical_flow_nv : 1 = false;
    };

    QueueFlags QueueFlags_from_vk(
        const VkQueueFlags& vk_queue_flags,
        VkBool32 vk_presentation_support
    );

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

    ObjectType ObjectType_from_vk(VkObjectType vk_type);

    // https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VkDebugUtilsMessageSeverityFlagBitsEXT.html
    struct DebugMessageSeverityFlags
    {
        bool verbose : 1 = false;
        bool info : 1 = false;
        bool warning : 1 = false;
        bool error : 1 = false;
    };

    VkDebugUtilsMessageSeverityFlagsEXT DebugMessageSeverityFlags_to_vk(
        const DebugMessageSeverityFlags& flags
    );

    // https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VkDebugUtilsMessageTypeFlagBitsEXT.html
    struct DebugMessageTypeFlags
    {
        bool general : 1 = false;
        bool validation : 1 = false;
        bool performance : 1 = false;
        bool device_address_binding : 1 = false;
    };

    DebugMessageTypeFlags DebugMessageTypeFlags_from_vk(
        VkDebugUtilsMessageTypeFlagsEXT vk_flags
    );

    VkDebugUtilsMessageTypeFlagsEXT DebugMessageTypeFlags_to_vk(
        const DebugMessageTypeFlags& flags
    );

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

    DebugMessageSeverity DebugMessageSeverity_from_vk(
        VkDebugUtilsMessageSeverityFlagBitsEXT vk_severity
    );

    // https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VkDeviceQueueCreateFlagBits.html
    struct QueueRequestFlags
    {
        bool protected_ : 1 = false;
    };

    VkDeviceQueueCreateFlags QueueRequestFlags_to_vk(
        const QueueRequestFlags& flags
    );

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

    SurfaceTransformFlags SurfaceTransformFlags_from_vk(
        const VkSurfaceTransformFlagsKHR& vk_flags
    );

    // https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VkSurfaceTransformFlagBitsKHR.html
    enum class SurfaceTransform : uint8_t
    {
        Identity,
        Rotate90,
        Rotate180,
        Rotate270,
        HorizontalMirror,
        HorizontalMirrorRotate90,
        HorizontalMirrorRotate180,
        HorizontalMirrorRotate270,
        Inherit,
        _
    };

    static constexpr const char* SurfaceTransform_string[]
    {
        "Identity",
        "Rotate 90",
        "Rotate 180",
        "Rotate 270",
        "Horizontal Mirror",
        "Horizontal Mirror Rotate 90",
        "Horizontal Mirror Rotate 180",
        "Horizontal Mirror Rotate 270",
        "Inherit"
    };

    BEVA_DEFINE_ENUM_TO_STRING_FUNCTION(SurfaceTransform);

    SurfaceTransform SurfaceTransform_from_vk(
        VkSurfaceTransformFlagBitsKHR vk_transform
    );

    VkSurfaceTransformFlagBitsKHR SurfaceTransform_to_vk(
        SurfaceTransform transform
    );

    // https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VkCompositeAlphaFlagBitsKHR.html
    struct CompositeAlphaFlags
    {
        bool opaque : 1 = false;
        bool pre_multiplied : 1 = false;
        bool post_multiplied : 1 = false;
        bool inherit : 1 = false;
    };

    CompositeAlphaFlags CompositeAlphaFlags_from_vk(
        VkCompositeAlphaFlagsKHR vk_flags
    );

    // https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VkCompositeAlphaFlagBitsKHR.html
    enum class CompositeAlpha : uint8_t
    {
        Opaque,
        PreMultiplied,
        PostMultiplied,
        Inherit,
        _
    };

    static constexpr const char* CompositeAlpha_string[]
    {
        "Opaque",
        "Pre-Multiplied",
        "Post-Multiplied",
        "Inherit"
    };

    BEVA_DEFINE_ENUM_TO_STRING_FUNCTION(CompositeAlpha);

    VkCompositeAlphaFlagBitsKHR CompositeAlpha_to_vk(CompositeAlpha alpha);

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

    ImageUsageFlags ImageUsageFlags_from_vk(VkImageUsageFlags vk_flags);
    VkImageUsageFlags ImageUsageFlags_to_vk(const ImageUsageFlags& flags);

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
    enum class Format : uint8_t
    {
        Undefined,
        R4G4_UNORM_PACK8,
        R4G4B4A4_UNORM_PACK16,
        B4G4R4A4_UNORM_PACK16,
        R5G6B5_UNORM_PACK16,
        B5G6R5_UNORM_PACK16,
        R5G5B5A1_UNORM_PACK16,
        B5G5R5A1_UNORM_PACK16,
        A1R5G5B5_UNORM_PACK16,
        R8_UNORM,
        R8_SNORM,
        R8_USCALED,
        R8_SSCALED,
        R8_UINT,
        R8_SINT,
        R8_SRGB,
        R8G8_UNORM,
        R8G8_SNORM,
        R8G8_USCALED,
        R8G8_SSCALED,
        R8G8_UINT,
        R8G8_SINT,
        R8G8_SRGB,
        R8G8B8_UNORM,
        R8G8B8_SNORM,
        R8G8B8_USCALED,
        R8G8B8_SSCALED,
        R8G8B8_UINT,
        R8G8B8_SINT,
        R8G8B8_SRGB,
        B8G8R8_UNORM,
        B8G8R8_SNORM,
        B8G8R8_USCALED,
        B8G8R8_SSCALED,
        B8G8R8_UINT,
        B8G8R8_SINT,
        B8G8R8_SRGB,
        R8G8B8A8_UNORM,
        R8G8B8A8_SNORM,
        R8G8B8A8_USCALED,
        R8G8B8A8_SSCALED,
        R8G8B8A8_UINT,
        R8G8B8A8_SINT,
        R8G8B8A8_SRGB,
        B8G8R8A8_UNORM,
        B8G8R8A8_SNORM,
        B8G8R8A8_USCALED,
        B8G8R8A8_SSCALED,
        B8G8R8A8_UINT,
        B8G8R8A8_SINT,
        B8G8R8A8_SRGB,
        A8B8G8R8_UNORM_PACK32,
        A8B8G8R8_SNORM_PACK32,
        A8B8G8R8_USCALED_PACK32,
        A8B8G8R8_SSCALED_PACK32,
        A8B8G8R8_UINT_PACK32,
        A8B8G8R8_SINT_PACK32,
        A8B8G8R8_SRGB_PACK32,
        A2R10G10B10_UNORM_PACK32,
        A2R10G10B10_SNORM_PACK32,
        A2R10G10B10_USCALED_PACK32,
        A2R10G10B10_SSCALED_PACK32,
        A2R10G10B10_UINT_PACK32,
        A2R10G10B10_SINT_PACK32,
        A2B10G10R10_UNORM_PACK32,
        A2B10G10R10_SNORM_PACK32,
        A2B10G10R10_USCALED_PACK32,
        A2B10G10R10_SSCALED_PACK32,
        A2B10G10R10_UINT_PACK32,
        A2B10G10R10_SINT_PACK32,
        R16_UNORM,
        R16_SNORM,
        R16_USCALED,
        R16_SSCALED,
        R16_UINT,
        R16_SINT,
        R16_SFLOAT,
        R16G16_UNORM,
        R16G16_SNORM,
        R16G16_USCALED,
        R16G16_SSCALED,
        R16G16_UINT,
        R16G16_SINT,
        R16G16_SFLOAT,
        R16G16B16_UNORM,
        R16G16B16_SNORM,
        R16G16B16_USCALED,
        R16G16B16_SSCALED,
        R16G16B16_UINT,
        R16G16B16_SINT,
        R16G16B16_SFLOAT,
        R16G16B16A16_UNORM,
        R16G16B16A16_SNORM,
        R16G16B16A16_USCALED,
        R16G16B16A16_SSCALED,
        R16G16B16A16_UINT,
        R16G16B16A16_SINT,
        R16G16B16A16_SFLOAT,
        R32_UINT,
        R32_SINT,
        R32_SFLOAT,
        R32G32_UINT,
        R32G32_SINT,
        R32G32_SFLOAT,
        R32G32B32_UINT,
        R32G32B32_SINT,
        R32G32B32_SFLOAT,
        R32G32B32A32_UINT,
        R32G32B32A32_SINT,
        R32G32B32A32_SFLOAT,
        R64_UINT,
        R64_SINT,
        R64_SFLOAT,
        R64G64_UINT,
        R64G64_SINT,
        R64G64_SFLOAT,
        R64G64B64_UINT,
        R64G64B64_SINT,
        R64G64B64_SFLOAT,
        R64G64B64A64_UINT,
        R64G64B64A64_SINT,
        R64G64B64A64_SFLOAT,
        B10G11R11_UFLOAT_PACK32,
        E5B9G9R9_UFLOAT_PACK32,
        D16_UNORM,
        X8_D24_UNORM_PACK32,
        D32_SFLOAT,
        S8_UINT,
        D16_UNORM_S8_UINT,
        D24_UNORM_S8_UINT,
        D32_SFLOAT_S8_UINT,
        BC1_RGB_UNORM_BLOCK,
        BC1_RGB_SRGB_BLOCK,
        BC1_RGBA_UNORM_BLOCK,
        BC1_RGBA_SRGB_BLOCK,
        BC2_UNORM_BLOCK,
        BC2_SRGB_BLOCK,
        BC3_UNORM_BLOCK,
        BC3_SRGB_BLOCK,
        BC4_UNORM_BLOCK,
        BC4_SNORM_BLOCK,
        BC5_UNORM_BLOCK,
        BC5_SNORM_BLOCK,
        BC6H_UFLOAT_BLOCK,
        BC6H_SFLOAT_BLOCK,
        BC7_UNORM_BLOCK,
        BC7_SRGB_BLOCK,
        ETC2_R8G8B8_UNORM_BLOCK,
        ETC2_R8G8B8_SRGB_BLOCK,
        ETC2_R8G8B8A1_UNORM_BLOCK,
        ETC2_R8G8B8A1_SRGB_BLOCK,
        ETC2_R8G8B8A8_UNORM_BLOCK,
        ETC2_R8G8B8A8_SRGB_BLOCK,
        EAC_R11_UNORM_BLOCK,
        EAC_R11_SNORM_BLOCK,
        EAC_R11G11_UNORM_BLOCK,
        EAC_R11G11_SNORM_BLOCK,
        ASTC_4x4_UNORM_BLOCK,
        ASTC_4x4_SRGB_BLOCK,
        ASTC_5x4_UNORM_BLOCK,
        ASTC_5x4_SRGB_BLOCK,
        ASTC_5x5_UNORM_BLOCK,
        ASTC_5x5_SRGB_BLOCK,
        ASTC_6x5_UNORM_BLOCK,
        ASTC_6x5_SRGB_BLOCK,
        ASTC_6x6_UNORM_BLOCK,
        ASTC_6x6_SRGB_BLOCK,
        ASTC_8x5_UNORM_BLOCK,
        ASTC_8x5_SRGB_BLOCK,
        ASTC_8x6_UNORM_BLOCK,
        ASTC_8x6_SRGB_BLOCK,
        ASTC_8x8_UNORM_BLOCK,
        ASTC_8x8_SRGB_BLOCK,
        ASTC_10x5_UNORM_BLOCK,
        ASTC_10x5_SRGB_BLOCK,
        ASTC_10x6_UNORM_BLOCK,
        ASTC_10x6_SRGB_BLOCK,
        ASTC_10x8_UNORM_BLOCK,
        ASTC_10x8_SRGB_BLOCK,
        ASTC_10x10_UNORM_BLOCK,
        ASTC_10x10_SRGB_BLOCK,
        ASTC_12x10_UNORM_BLOCK,
        ASTC_12x10_SRGB_BLOCK,
        ASTC_12x12_UNORM_BLOCK,
        ASTC_12x12_SRGB_BLOCK,
        G8B8G8R8_422_UNORM,
        B8G8R8G8_422_UNORM,
        G8_B8_R8_3PLANE_420_UNORM,
        G8_B8R8_2PLANE_420_UNORM,
        G8_B8_R8_3PLANE_422_UNORM,
        G8_B8R8_2PLANE_422_UNORM,
        G8_B8_R8_3PLANE_444_UNORM,
        R10X6_UNORM_PACK16,
        R10X6G10X6_UNORM_2PACK16,
        R10X6G10X6B10X6A10X6_UNORM_4PACK16,
        G10X6B10X6G10X6R10X6_422_UNORM_4PACK16,
        B10X6G10X6R10X6G10X6_422_UNORM_4PACK16,
        G10X6_B10X6_R10X6_3PLANE_420_UNORM_3PACK16,
        G10X6_B10X6R10X6_2PLANE_420_UNORM_3PACK16,
        G10X6_B10X6_R10X6_3PLANE_422_UNORM_3PACK16,
        G10X6_B10X6R10X6_2PLANE_422_UNORM_3PACK16,
        G10X6_B10X6_R10X6_3PLANE_444_UNORM_3PACK16,
        R12X4_UNORM_PACK16,
        R12X4G12X4_UNORM_2PACK16,
        R12X4G12X4B12X4A12X4_UNORM_4PACK16,
        G12X4B12X4G12X4R12X4_422_UNORM_4PACK16,
        B12X4G12X4R12X4G12X4_422_UNORM_4PACK16,
        G12X4_B12X4_R12X4_3PLANE_420_UNORM_3PACK16,
        G12X4_B12X4R12X4_2PLANE_420_UNORM_3PACK16,
        G12X4_B12X4_R12X4_3PLANE_422_UNORM_3PACK16,
        G12X4_B12X4R12X4_2PLANE_422_UNORM_3PACK16,
        G12X4_B12X4_R12X4_3PLANE_444_UNORM_3PACK16,
        G16B16G16R16_422_UNORM,
        B16G16R16G16_422_UNORM,
        G16_B16_R16_3PLANE_420_UNORM,
        G16_B16R16_2PLANE_420_UNORM,
        G16_B16_R16_3PLANE_422_UNORM,
        G16_B16R16_2PLANE_422_UNORM,
        G16_B16_R16_3PLANE_444_UNORM,
        G8_B8R8_2PLANE_444_UNORM,
        G10X6_B10X6R10X6_2PLANE_444_UNORM_3PACK16,
        G12X4_B12X4R12X4_2PLANE_444_UNORM_3PACK16,
        G16_B16R16_2PLANE_444_UNORM,
        A4R4G4B4_UNORM_PACK16,
        A4B4G4R4_UNORM_PACK16,
        ASTC_4x4_SFLOAT_BLOCK,
        ASTC_5x4_SFLOAT_BLOCK,
        ASTC_5x5_SFLOAT_BLOCK,
        ASTC_6x5_SFLOAT_BLOCK,
        ASTC_6x6_SFLOAT_BLOCK,
        ASTC_8x5_SFLOAT_BLOCK,
        ASTC_8x6_SFLOAT_BLOCK,
        ASTC_8x8_SFLOAT_BLOCK,
        ASTC_10x5_SFLOAT_BLOCK,
        ASTC_10x6_SFLOAT_BLOCK,
        ASTC_10x8_SFLOAT_BLOCK,
        ASTC_10x10_SFLOAT_BLOCK,
        ASTC_12x10_SFLOAT_BLOCK,
        ASTC_12x12_SFLOAT_BLOCK,
        PVRTC1_2BPP_UNORM_BLOCK_IMG,
        PVRTC1_4BPP_UNORM_BLOCK_IMG,
        PVRTC2_2BPP_UNORM_BLOCK_IMG,
        PVRTC2_4BPP_UNORM_BLOCK_IMG,
        PVRTC1_2BPP_SRGB_BLOCK_IMG,
        PVRTC1_4BPP_SRGB_BLOCK_IMG,
        PVRTC2_2BPP_SRGB_BLOCK_IMG,
        PVRTC2_4BPP_SRGB_BLOCK_IMG,
        R16G16_S10_5_NV,
        _
    };

    static constexpr const char* Format_string[]
    {
        "Undefined",
        "R4G4_UNORM_PACK8",
        "R4G4B4A4_UNORM_PACK16",
        "B4G4R4A4_UNORM_PACK16",
        "R5G6B5_UNORM_PACK16",
        "B5G6R5_UNORM_PACK16",
        "R5G5B5A1_UNORM_PACK16",
        "B5G5R5A1_UNORM_PACK16",
        "A1R5G5B5_UNORM_PACK16",
        "R8_UNORM",
        "R8_SNORM",
        "R8_USCALED",
        "R8_SSCALED",
        "R8_UINT",
        "R8_SINT",
        "R8_SRGB",
        "R8G8_UNORM",
        "R8G8_SNORM",
        "R8G8_USCALED",
        "R8G8_SSCALED",
        "R8G8_UINT",
        "R8G8_SINT",
        "R8G8_SRGB",
        "R8G8B8_UNORM",
        "R8G8B8_SNORM",
        "R8G8B8_USCALED",
        "R8G8B8_SSCALED",
        "R8G8B8_UINT",
        "R8G8B8_SINT",
        "R8G8B8_SRGB",
        "B8G8R8_UNORM",
        "B8G8R8_SNORM",
        "B8G8R8_USCALED",
        "B8G8R8_SSCALED",
        "B8G8R8_UINT",
        "B8G8R8_SINT",
        "B8G8R8_SRGB",
        "R8G8B8A8_UNORM",
        "R8G8B8A8_SNORM",
        "R8G8B8A8_USCALED",
        "R8G8B8A8_SSCALED",
        "R8G8B8A8_UINT",
        "R8G8B8A8_SINT",
        "R8G8B8A8_SRGB",
        "B8G8R8A8_UNORM",
        "B8G8R8A8_SNORM",
        "B8G8R8A8_USCALED",
        "B8G8R8A8_SSCALED",
        "B8G8R8A8_UINT",
        "B8G8R8A8_SINT",
        "B8G8R8A8_SRGB",
        "A8B8G8R8_UNORM_PACK32",
        "A8B8G8R8_SNORM_PACK32",
        "A8B8G8R8_USCALED_PACK32",
        "A8B8G8R8_SSCALED_PACK32",
        "A8B8G8R8_UINT_PACK32",
        "A8B8G8R8_SINT_PACK32",
        "A8B8G8R8_SRGB_PACK32",
        "A2R10G10B10_UNORM_PACK32",
        "A2R10G10B10_SNORM_PACK32",
        "A2R10G10B10_USCALED_PACK32",
        "A2R10G10B10_SSCALED_PACK32",
        "A2R10G10B10_UINT_PACK32",
        "A2R10G10B10_SINT_PACK32",
        "A2B10G10R10_UNORM_PACK32",
        "A2B10G10R10_SNORM_PACK32",
        "A2B10G10R10_USCALED_PACK32",
        "A2B10G10R10_SSCALED_PACK32",
        "A2B10G10R10_UINT_PACK32",
        "A2B10G10R10_SINT_PACK32",
        "R16_UNORM",
        "R16_SNORM",
        "R16_USCALED",
        "R16_SSCALED",
        "R16_UINT",
        "R16_SINT",
        "R16_SFLOAT",
        "R16G16_UNORM",
        "R16G16_SNORM",
        "R16G16_USCALED",
        "R16G16_SSCALED",
        "R16G16_UINT",
        "R16G16_SINT",
        "R16G16_SFLOAT",
        "R16G16B16_UNORM",
        "R16G16B16_SNORM",
        "R16G16B16_USCALED",
        "R16G16B16_SSCALED",
        "R16G16B16_UINT",
        "R16G16B16_SINT",
        "R16G16B16_SFLOAT",
        "R16G16B16A16_UNORM",
        "R16G16B16A16_SNORM",
        "R16G16B16A16_USCALED",
        "R16G16B16A16_SSCALED",
        "R16G16B16A16_UINT",
        "R16G16B16A16_SINT",
        "R16G16B16A16_SFLOAT",
        "R32_UINT",
        "R32_SINT",
        "R32_SFLOAT",
        "R32G32_UINT",
        "R32G32_SINT",
        "R32G32_SFLOAT",
        "R32G32B32_UINT",
        "R32G32B32_SINT",
        "R32G32B32_SFLOAT",
        "R32G32B32A32_UINT",
        "R32G32B32A32_SINT",
        "R32G32B32A32_SFLOAT",
        "R64_UINT",
        "R64_SINT",
        "R64_SFLOAT",
        "R64G64_UINT",
        "R64G64_SINT",
        "R64G64_SFLOAT",
        "R64G64B64_UINT",
        "R64G64B64_SINT",
        "R64G64B64_SFLOAT",
        "R64G64B64A64_UINT",
        "R64G64B64A64_SINT",
        "R64G64B64A64_SFLOAT",
        "B10G11R11_UFLOAT_PACK32",
        "E5B9G9R9_UFLOAT_PACK32",
        "D16_UNORM",
        "X8_D24_UNORM_PACK32",
        "D32_SFLOAT",
        "S8_UINT",
        "D16_UNORM_S8_UINT",
        "D24_UNORM_S8_UINT",
        "D32_SFLOAT_S8_UINT",
        "BC1_RGB_UNORM_BLOCK",
        "BC1_RGB_SRGB_BLOCK",
        "BC1_RGBA_UNORM_BLOCK",
        "BC1_RGBA_SRGB_BLOCK",
        "BC2_UNORM_BLOCK",
        "BC2_SRGB_BLOCK",
        "BC3_UNORM_BLOCK",
        "BC3_SRGB_BLOCK",
        "BC4_UNORM_BLOCK",
        "BC4_SNORM_BLOCK",
        "BC5_UNORM_BLOCK",
        "BC5_SNORM_BLOCK",
        "BC6H_UFLOAT_BLOCK",
        "BC6H_SFLOAT_BLOCK",
        "BC7_UNORM_BLOCK",
        "BC7_SRGB_BLOCK",
        "ETC2_R8G8B8_UNORM_BLOCK",
        "ETC2_R8G8B8_SRGB_BLOCK",
        "ETC2_R8G8B8A1_UNORM_BLOCK",
        "ETC2_R8G8B8A1_SRGB_BLOCK",
        "ETC2_R8G8B8A8_UNORM_BLOCK",
        "ETC2_R8G8B8A8_SRGB_BLOCK",
        "EAC_R11_UNORM_BLOCK",
        "EAC_R11_SNORM_BLOCK",
        "EAC_R11G11_UNORM_BLOCK",
        "EAC_R11G11_SNORM_BLOCK",
        "ASTC_4x4_UNORM_BLOCK",
        "ASTC_4x4_SRGB_BLOCK",
        "ASTC_5x4_UNORM_BLOCK",
        "ASTC_5x4_SRGB_BLOCK",
        "ASTC_5x5_UNORM_BLOCK",
        "ASTC_5x5_SRGB_BLOCK",
        "ASTC_6x5_UNORM_BLOCK",
        "ASTC_6x5_SRGB_BLOCK",
        "ASTC_6x6_UNORM_BLOCK",
        "ASTC_6x6_SRGB_BLOCK",
        "ASTC_8x5_UNORM_BLOCK",
        "ASTC_8x5_SRGB_BLOCK",
        "ASTC_8x6_UNORM_BLOCK",
        "ASTC_8x6_SRGB_BLOCK",
        "ASTC_8x8_UNORM_BLOCK",
        "ASTC_8x8_SRGB_BLOCK",
        "ASTC_10x5_UNORM_BLOCK",
        "ASTC_10x5_SRGB_BLOCK",
        "ASTC_10x6_UNORM_BLOCK",
        "ASTC_10x6_SRGB_BLOCK",
        "ASTC_10x8_UNORM_BLOCK",
        "ASTC_10x8_SRGB_BLOCK",
        "ASTC_10x10_UNORM_BLOCK",
        "ASTC_10x10_SRGB_BLOCK",
        "ASTC_12x10_UNORM_BLOCK",
        "ASTC_12x10_SRGB_BLOCK",
        "ASTC_12x12_UNORM_BLOCK",
        "ASTC_12x12_SRGB_BLOCK",
        "G8B8G8R8_422_UNORM",
        "B8G8R8G8_422_UNORM",
        "G8_B8_R8_3PLANE_420_UNORM",
        "G8_B8R8_2PLANE_420_UNORM",
        "G8_B8_R8_3PLANE_422_UNORM",
        "G8_B8R8_2PLANE_422_UNORM",
        "G8_B8_R8_3PLANE_444_UNORM",
        "R10X6_UNORM_PACK16",
        "R10X6G10X6_UNORM_2PACK16",
        "R10X6G10X6B10X6A10X6_UNORM_4PACK16",
        "G10X6B10X6G10X6R10X6_422_UNORM_4PACK16",
        "B10X6G10X6R10X6G10X6_422_UNORM_4PACK16",
        "G10X6_B10X6_R10X6_3PLANE_420_UNORM_3PACK16",
        "G10X6_B10X6R10X6_2PLANE_420_UNORM_3PACK16",
        "G10X6_B10X6_R10X6_3PLANE_422_UNORM_3PACK16",
        "G10X6_B10X6R10X6_2PLANE_422_UNORM_3PACK16",
        "G10X6_B10X6_R10X6_3PLANE_444_UNORM_3PACK16",
        "R12X4_UNORM_PACK16",
        "R12X4G12X4_UNORM_2PACK16",
        "R12X4G12X4B12X4A12X4_UNORM_4PACK16",
        "G12X4B12X4G12X4R12X4_422_UNORM_4PACK16",
        "B12X4G12X4R12X4G12X4_422_UNORM_4PACK16",
        "G12X4_B12X4_R12X4_3PLANE_420_UNORM_3PACK16",
        "G12X4_B12X4R12X4_2PLANE_420_UNORM_3PACK16",
        "G12X4_B12X4_R12X4_3PLANE_422_UNORM_3PACK16",
        "G12X4_B12X4R12X4_2PLANE_422_UNORM_3PACK16",
        "G12X4_B12X4_R12X4_3PLANE_444_UNORM_3PACK16",
        "G16B16G16R16_422_UNORM",
        "B16G16R16G16_422_UNORM",
        "G16_B16_R16_3PLANE_420_UNORM",
        "G16_B16R16_2PLANE_420_UNORM",
        "G16_B16_R16_3PLANE_422_UNORM",
        "G16_B16R16_2PLANE_422_UNORM",
        "G16_B16_R16_3PLANE_444_UNORM",
        "G8_B8R8_2PLANE_444_UNORM",
        "G10X6_B10X6R10X6_2PLANE_444_UNORM_3PACK16",
        "G12X4_B12X4R12X4_2PLANE_444_UNORM_3PACK16",
        "G16_B16R16_2PLANE_444_UNORM",
        "A4R4G4B4_UNORM_PACK16",
        "A4B4G4R4_UNORM_PACK16",
        "ASTC_4x4_SFLOAT_BLOCK",
        "ASTC_5x4_SFLOAT_BLOCK",
        "ASTC_5x5_SFLOAT_BLOCK",
        "ASTC_6x5_SFLOAT_BLOCK",
        "ASTC_6x6_SFLOAT_BLOCK",
        "ASTC_8x5_SFLOAT_BLOCK",
        "ASTC_8x6_SFLOAT_BLOCK",
        "ASTC_8x8_SFLOAT_BLOCK",
        "ASTC_10x5_SFLOAT_BLOCK",
        "ASTC_10x6_SFLOAT_BLOCK",
        "ASTC_10x8_SFLOAT_BLOCK",
        "ASTC_10x10_SFLOAT_BLOCK",
        "ASTC_12x10_SFLOAT_BLOCK",
        "ASTC_12x12_SFLOAT_BLOCK",
        "PVRTC1_2BPP_UNORM_BLOCK_IMG",
        "PVRTC1_4BPP_UNORM_BLOCK_IMG",
        "PVRTC2_2BPP_UNORM_BLOCK_IMG",
        "PVRTC2_4BPP_UNORM_BLOCK_IMG",
        "PVRTC1_2BPP_SRGB_BLOCK_IMG",
        "PVRTC1_4BPP_SRGB_BLOCK_IMG",
        "PVRTC2_2BPP_SRGB_BLOCK_IMG",
        "PVRTC2_4BPP_SRGB_BLOCK_IMG",
        "R16G16_S10_5_NV"
    };

    BEVA_DEFINE_ENUM_TO_STRING_FUNCTION(Format);

    Format Format_from_vk(VkFormat vk_format);
    VkFormat Format_to_vk(Format format);

    // https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VkColorSpaceKHR.html
    enum class ColorSpace : uint8_t
    {
        SrgbNonlinear,
        DisplayP3Nonlinear,
        ExtendedSrgbLinear,
        DisplayP3Linear,
        DciP3Nonlinear,
        Bt709Linear,
        Bt709Nonlinear,
        Bt2020Linear,
        Hdr10St2084,
        DolbyVision,
        Hdr10Hlg,
        AdobeRgbLinear,
        AdobeRgbNonlinear,
        PassThrough,
        ExtendedSrgbNonlinear,
        DisplayNativeAmd,
        _
    };

    static constexpr const char* ColorSpace_string[]
    {
        "sRGB Nonlinear",
        "Display P3 Nonlinear",
        "Extended sRGB Linear",
        "Display P3 Linear",
        "DCI-P3 Nonlinear",
        "BT.709 Linear",
        "BT.709 Nonlinear",
        "BT.2020 Linear",
        "HDR10 ST.2084",
        "Dolby Vision",
        "HDR10 HLG",
        "Adobe RGB Linear",
        "Adobe RGB Nonlinear",
        "Pass Through",
        "Extended sRGB Nonlinear",
        "Display Native AMD"
    };

    BEVA_DEFINE_ENUM_TO_STRING_FUNCTION(ColorSpace);

    ColorSpace ColorSpace_from_vk(VkColorSpaceKHR vk_space);
    VkColorSpaceKHR ColorSpace_to_vk(ColorSpace space);

    // https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VkSurfaceFormatKHR.html
    struct SurfaceFormat
    {
        Format format;
        ColorSpace color_space;
    };

    SurfaceFormat SurfaceFormat_from_vk(
        const VkSurfaceFormatKHR& vk_surface_format
    );

    // https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VkPresentModeKHR.html
    enum class PresentMode : uint8_t
    {
        Immediate,
        Mailbox,
        Fifo,
        FifoRelaxed,
        SharedDemandRefresh,
        SharedContinuousRefresh,
        _
    };

    static constexpr const char* PresentMode_string[]
    {
        "Immediate",
        "Mailbox",
        "FIFO",
        "FIFO Relaxed",
        "Shared Demand Refresh",
        "Shared Continuous Refresh"
    };

    BEVA_DEFINE_ENUM_TO_STRING_FUNCTION(PresentMode);

    PresentMode PresentMode_from_vk(VkPresentModeKHR vk_mode);
    VkPresentModeKHR PresentMode_to_vk(PresentMode mode);

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

    // https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VkDebugUtilsLabelEXT.html
    struct DebugLabel
    {
        std::string name;
        std::array<float, 4> color;
    };

    DebugLabel DebugLabel_from_vk(const VkDebugUtilsLabelEXT& vk_label);

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
        const QueueRequest& request
    );

    // https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VkDeviceCreateInfo.html
    struct DeviceConfig
    {
        std::vector<QueueRequest> queue_requests;
        std::vector<std::string> extensions;
        PhysicalDeviceFeatures enabled_features;
    };

    // https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VkSwapchainCreateFlagBitsKHR.html
    struct SwapchainFlags
    {
        bool split_instance_bind_regions : 1 = false;
        bool protected_ : 1 = false;
        bool mutable_format : 1 = false;
        bool deferred_memory_allocation : 1 = false;
    };

    VkSwapchainCreateFlagsKHR SwapchainFlags_to_vk(const SwapchainFlags& flags);

    enum class SharingMode : uint8_t
    {
        Exclusive,
        Concurrent,
        _
    };

    static constexpr const char* SharingMode_string[]
    {
        "Exclusive",
        "Concurrent"
    };

    BEVA_DEFINE_ENUM_TO_STRING_FUNCTION(SharingMode);

    VkSharingMode SharingMode_to_vk(SharingMode mode);

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

    VkImageViewCreateFlags ImageViewFlags_to_vk(const ImageViewFlags& flags);

    // https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VkImageViewType.html
    enum class ImageViewType : uint8_t
    {
        _1d,
        _2d,
        _3d,
        Cube,
        _1dArray,
        _2dArray,
        CubeArray,
        _
    };

    static constexpr const char* ImageViewType_string[]
    {
        "1D",
        "2D",
        "3D",
        "Cube",
        "1D Array",
        "2D Array",
        "Cube Array"
    };

    BEVA_DEFINE_ENUM_TO_STRING_FUNCTION(ImageViewType);

    VkImageViewType ImageViewType_to_vk(ImageViewType type);

    // https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VkComponentSwizzle.html
    enum class ComponentSwizzle : uint8_t
    {
        Identity,
        Zero,
        One,
        R,
        G,
        B,
        A,
        _
    };

    static constexpr const char* ComponentSwizzle_string[]
    {
        "Identity",
        "Zero",
        "One",
        "R",
        "G",
        "B",
        "A"
    };

    BEVA_DEFINE_ENUM_TO_STRING_FUNCTION(ComponentSwizzle);

    VkComponentSwizzle ComponentSwizzle_to_vk(ComponentSwizzle swizzle);

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

    VkImageAspectFlags ImageAspectFlags_to_vk(const ImageAspectFlags& flags);

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
        Context(Context&& other);

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

    // * requires extension VK_EXT_DEBUG_UTILS_EXTENSION_NAME
    // https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VkDebugUtilsMessengerEXT.html
    class DebugMessenger
    {
    public:
        using ptr = std::shared_ptr<DebugMessenger>;

        DebugMessenger(const DebugMessenger& other) = delete;
        DebugMessenger(DebugMessenger&& other);

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

    // https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VkSurfaceKHR.html
    class Surface
    {
    public:
        using ptr = std::shared_ptr<Surface>;

        Surface() = delete;
        Surface(const Surface& other) = delete;
        Surface(Surface&& other);

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
        Queue(Queue&& other);

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
        Device(Device&& other);

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

    };

    // https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VkImage.html
    class Image
    {
    public:
        using ptr = std::shared_ptr<Image>;

        Image() = delete;
        Image(const Image& other) = delete;
        Image(Image&& other);

    protected:
        VkImage vk_image;

        Image(VkImage vk_image);

        friend class ImageView;

    };

    // https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VkSwapchainKHR.html
    class Swapchain
    {
    public:
        using ptr = std::shared_ptr<Swapchain>;

        Swapchain() = delete;
        Swapchain(const Swapchain& other) = delete;
        Swapchain(Swapchain&& other);

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
        ImageView(ImageView&& other);

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

}
