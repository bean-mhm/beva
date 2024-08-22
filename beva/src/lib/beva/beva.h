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

    template<typename Enum>
    using EnumStrMap = std::unordered_map<Enum, std::string>;

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

    enum class VulkanApiVersion
    {
        Vulkan1_0,
        Vulkan1_1,
        Vulkan1_2,
        Vulkan1_3
    };

    uint32_t VulkanApiVersion_encode(VulkanApiVersion version);

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
        VkSampleCountFlags framebuffer_color_sample_counts;
        VkSampleCountFlags framebuffer_depth_sample_counts;
        VkSampleCountFlags framebuffer_stencil_sample_counts;
        VkSampleCountFlags framebuffer_no_attachments_sample_counts;
        uint32_t max_color_attachments;
        VkSampleCountFlags sampled_image_color_sample_counts;
        VkSampleCountFlags sampled_image_integer_sample_counts;
        VkSampleCountFlags sampled_image_depth_sample_counts;
        VkSampleCountFlags sampled_image_stencil_sample_counts;
        VkSampleCountFlags storage_image_sample_counts;
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
        VkPhysicalDeviceType device_type;
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
    // presentation_support: 
    struct QueueFamily
    {
        VkQueueFlags queue_flags;
        uint32_t queue_count;
        uint32_t timestamp_valid_bits;
        Extent3d min_image_transfer_granularity;
        bool surface_support;
    };

    QueueFamily QueueFamily_from_vk(
        const VkQueueFamilyProperties& vk_family,
        VkBool32 vk_surface_support
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
    // https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VkSurfaceCapabilitiesKHR.html
    struct SurfaceCapabilities
    {
        uint32_t min_image_count;
        uint32_t max_image_count;
        Extent2d current_extent;
        Extent2d min_image_extent;
        Extent2d max_image_extent;
        uint32_t max_image_array_layers;
        VkSurfaceTransformFlagsKHR supported_transforms;
        VkSurfaceTransformFlagBitsKHR current_transform;
        VkCompositeAlphaFlagsKHR supported_composite_alpha;
        VkImageUsageFlags supported_usage_flags;
    };

    SurfaceCapabilities SurfaceCapabilities_from_vk(
        const VkSurfaceCapabilitiesKHR& vk_capabilities
    );

    // provided by VK_KHR_surface
    // https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VkSurfaceFormatKHR.html
    struct SurfaceFormat
    {
        VkFormat format;
        VkColorSpaceKHR color_space;
    };

    SurfaceFormat SurfaceFormat_from_vk(
        const VkSurfaceFormatKHR& vk_surface_format
    );

    // provided by VK_KHR_surface
    // https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/vkGetPhysicalDeviceSurfaceCapabilitiesKHR.html
    // https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/vkGetPhysicalDeviceSurfaceFormatsKHR.html
    // https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/vkGetPhysicalDeviceSurfacePresentModesKHR.html
    struct SwapchainSupport
    {
        SurfaceCapabilities capabilities;
        std::vector<SurfaceFormat> surface_formats;
        std::vector<VkPresentModeKHR> present_modes;
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
        VkObjectType type;
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
        VkDebugUtilsMessageSeverityFlagBitsEXT,
        VkDebugUtilsMessageTypeFlagsEXT,
        const DebugMessageData&
        )>;

    // https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VkDeviceQueueCreateInfo.html
    struct QueueRequest
    {
        VkDeviceQueueCreateFlags flags;
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
    // https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VkSwapchainCreateInfoKHR.html
    struct SwapchainConfig
    {
        VkSwapchainCreateFlagsKHR flags;
        uint32_t min_image_count;
        VkFormat image_format;
        VkColorSpaceKHR image_color_space;
        Extent2d image_extent;
        uint32_t image_array_layers;
        VkImageUsageFlags image_usage;
        VkSharingMode image_sharing_mode;
        std::vector<uint32_t> queue_family_indices;
        VkSurfaceTransformFlagBitsKHR pre_transform;
        VkCompositeAlphaFlagBitsKHR composite_alpha;
        VkPresentModeKHR present_mode;
        bool clipped;
    };

    // https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VkComponentMapping.html
    struct ComponentMapping
    {
        VkComponentSwizzle r = VK_COMPONENT_SWIZZLE_IDENTITY;
        VkComponentSwizzle g = VK_COMPONENT_SWIZZLE_IDENTITY;
        VkComponentSwizzle b = VK_COMPONENT_SWIZZLE_IDENTITY;
        VkComponentSwizzle a = VK_COMPONENT_SWIZZLE_IDENTITY;
    };

    VkComponentMapping ComponentMapping_to_vk(const ComponentMapping& mapping);

    // https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VkImageSubresourceRange.html
    struct ImageSubresourceRange
    {
        VkImageAspectFlags aspect_mask;
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
        VkImageViewCreateFlags flags;
        VkImageViewType view_type;
        VkFormat format;
        ComponentMapping components;
        ImageSubresourceRange subresource_range;
    };

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
        VkPipelineShaderStageCreateFlags flags;
        VkShaderStageFlagBits stage;
        std::shared_ptr<ShaderModule> module;
        std::string entry_point;
        std::optional<SpecializationInfo> specialization_info;
    };

    VkPipelineShaderStageCreateInfo ShaderStage_to_vk(
        const ShaderStage& stage,
        std::shared_ptr<ShaderModule>& waste_module,
        VkSpecializationInfo& waste_vk_specialization_info,
        std::vector<VkSpecializationMapEntry>& waste_vk_map_entries,
        std::vector<uint8_t>& waste_data
    );

    // https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VkPipelineDynamicStateCreateInfo.html
    using DynamicStates = std::vector<VkDynamicState>;

    VkPipelineDynamicStateCreateInfo DynamicStates_to_vk(
        const DynamicStates& states,
        std::vector<VkDynamicState>& waste_dynamic_states
    );

    // https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VkVertexInputBindingDescription.html
    struct VertexInputBindingDescription
    {
        uint32_t binding;
        uint32_t stride;
        VkVertexInputRate input_rate;
    };

    VkVertexInputBindingDescription VertexInputBindingDescription_to_vk(
        const VertexInputBindingDescription& description
    );

    // https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VkVertexInputAttributeDescription.html
    struct VertexInputAttributeDescription
    {
        uint32_t location;
        uint32_t binding;
        VkFormat format;
        uint32_t offset;
    };

    VkVertexInputAttributeDescription VertexInputAttributeDescription_to_vk(
        const VertexInputAttributeDescription& description
    );

    // https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VkPipelineVertexInputStateCreateInfo.html
    struct VertexInputState
    {
        std::vector<VertexInputBindingDescription> binding_descriptions;
        std::vector<VertexInputAttributeDescription> attribute_descriptions;
    };

    VkPipelineVertexInputStateCreateInfo VertexInputState_to_vk(
        const VertexInputState& state,

        std::vector<VkVertexInputBindingDescription>&
        waste_vk_binding_descriptions,

        std::vector<VkVertexInputAttributeDescription>&
        waste_vk_attribute_descriptions
    );

    // https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VkPipelineInputAssemblyStateCreateInfo.html
    struct InputAssemblyState
    {
        VkPrimitiveTopology topology;
        bool primitive_restart_enable;
    };

    VkPipelineInputAssemblyStateCreateInfo InputAssemblyState_to_vk(
        const InputAssemblyState& state
    );

    // https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VkViewport.html
    struct Viewport
    {
        float x;
        float y;
        float width;
        float height;
        float min_depth;
        float max_depth;
    };

    VkViewport Viewport_to_vk(const Viewport& viewport);

    // https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VkOffset2D.html
    struct Offset2d
    {
        int32_t x;
        int32_t y;
    };

    VkOffset2D Offset2d_to_vk(const Offset2d& offset);

    // https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VkOffset3D.html
    struct Offset3d
    {
        int32_t x;
        int32_t y;
        int32_t z;
    };

    VkOffset3D Offset3d_to_vk(const Offset3d& offset);

    // https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VkRect2D.html
    struct Rect2d
    {
        Offset2d offset;
        Extent2d extent;
    };

    VkRect2D Rect2d_to_vk(const Rect2d& rect);

    // https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VkPipelineViewportStateCreateInfo.html
    struct ViewportState
    {
        std::vector<Viewport> viewports;
        std::vector<Rect2d> scissors;
    };

    VkPipelineViewportStateCreateInfo ViewportState_to_vk(
        const ViewportState& state,
        std::vector<VkViewport>& waste_vk_viewports,
        std::vector<VkRect2D>& waste_vk_scissors
    );

    // https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VkPipelineRasterizationStateCreateInfo.html
    struct RasterizationState
    {
        bool depth_clamp_enable;
        bool rasterizer_discard_enable;
        VkPolygonMode polygon_mode;
        VkCullModeFlags cull_mode;
        VkFrontFace front_face;
        bool depth_bias_enable;
        float depth_bias_constant_factor;
        float depth_bias_clamp;
        float depth_bias_slope_factor;
        float line_width;
    };

    VkPipelineRasterizationStateCreateInfo RasterizationState_to_vk(
        const RasterizationState& state
    );

    // https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VkPipelineMultisampleStateCreateInfo.html
    struct MultisampleState
    {
        VkSampleCountFlagBits rasterization_samples;
        bool sample_shading_enable;
        float min_sample_shading;
        std::vector<VkSampleMask> sample_mask;
        bool alpha_to_coverage_enable;
        bool alpha_to_one_enable;
    };

    VkPipelineMultisampleStateCreateInfo MultisampleState_to_vk(
        const MultisampleState& state,
        std::vector<VkSampleMask>& waste_sample_mask
    );

    // https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VkPipelineDepthStencilStateCreateInfo.html
    struct DepthStencilState
    {
        VkPipelineDepthStencilStateCreateFlags flags;
        bool depth_test_enable;
        bool depth_write_enable;
        VkCompareOp depth_compare_op;
        bool depth_bounds_test_enable;
        bool stencil_test_enable;
        VkStencilOpState front;
        VkStencilOpState back;
        float min_depth_bounds;
        float max_depth_bounds;
    };

    VkPipelineDepthStencilStateCreateInfo DepthStencilState_to_vk(
        const DepthStencilState& state
    );

    // https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VkPipelineColorBlendAttachmentState.html
    struct ColorBlendAttachment
    {
        bool blend_enable;
        VkBlendFactor src_color_blend_factor;
        VkBlendFactor dst_color_blend_factor;
        VkBlendOp color_blend_op;
        VkBlendFactor src_alpha_blend_factor;
        VkBlendFactor dst_alpha_blend_factor;
        VkBlendOp alpha_blend_op;
        VkColorComponentFlags color_write_mask;
    };

    VkPipelineColorBlendAttachmentState ColorBlendAttachment_to_vk(
        const ColorBlendAttachment& attachment
    );

    // https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VkPipelineColorBlendStateCreateInfo.html
    struct ColorBlendState
    {
        VkPipelineColorBlendStateCreateFlags flags;
        bool logic_op_enable;
        VkLogicOp logic_op;
        std::vector<ColorBlendAttachment> attachments;
        std::array<float, 4> blend_constants;
    };

    VkPipelineColorBlendStateCreateInfo ColorBlendState_to_vk(
        const ColorBlendState& state,

        std::vector<VkPipelineColorBlendAttachmentState>&
        waste_vk_color_blend_attachments
    );

    // https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VkSamplerCreateInfo.html
    struct SamplerConfig
    {
        VkSamplerCreateFlags flags;
        VkFilter mag_filter;
        VkFilter min_filter;
        VkSamplerMipmapMode mipmap_mode;
        VkSamplerAddressMode address_mode_u;
        VkSamplerAddressMode address_mode_v;
        VkSamplerAddressMode address_mode_w;
        float mip_lod_bias;
        bool anisotropy_enable;
        float max_anisotropy;
        bool compare_enable;
        VkCompareOp compare_op;
        float min_lod;
        float max_lod;
        VkBorderColor border_color;
        bool unnormalized_coordinates;
    };

    class Sampler;

    // https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VkDescriptorSetLayoutBinding.html
    struct DescriptorSetLayoutBinding
    {
        uint32_t binding;
        VkDescriptorType descriptor_type;
        uint32_t descriptor_count;
        VkShaderStageFlags stage_flags;
        std::vector<std::shared_ptr<Sampler>> immutable_samplers;
    };

    VkDescriptorSetLayoutBinding DescriptorSetLayoutBinding_to_vk(
        const DescriptorSetLayoutBinding& binding,
        std::vector<std::shared_ptr<Sampler>>& waste_immutable_samplers,
        std::vector<VkSampler>& waste_vk_immutable_samplers
    );

    // https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VkDescriptorSetLayoutCreateInfo.html
    struct DescriptorSetLayoutConfig
    {
        VkDescriptorSetLayoutCreateFlags flags;
        std::vector<DescriptorSetLayoutBinding> bindings;
    };

    class DescriptorSetLayout;

    // https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VkPushConstantRange.html
    struct PushConstantRange
    {
        VkShaderStageFlags stage_flags;
        uint32_t offset;
        uint32_t size;
    };

    VkPushConstantRange PushConstantRange_to_vk(const PushConstantRange& range);

    // https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VkPipelineLayoutCreateInfo.html
    struct PipelineLayoutConfig
    {
        VkPipelineLayoutCreateFlags flags;
        std::vector<std::shared_ptr<DescriptorSetLayout>> set_layouts;
        std::vector<PushConstantRange> push_constant_ranges;
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
        Error(VkResult vk_result);
        Error(std::string message, VkResult vk_result);

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
            VkSystemAllocationScope allocation_scope
        ) = 0;

        virtual void* reallocate(
            void* original,
            size_t size,
            size_t alignment,
            VkSystemAllocationScope allocation_scope
        ) = 0;

        virtual void free(void* memory) = 0;

        virtual void internal_allocation_notification(
            size_t size,
            VkInternalAllocationType allocation_type,
            VkSystemAllocationScope allocation_scope
        ) = 0;

        virtual void internal_free_notification(
            size_t size,
            VkInternalAllocationType allocation_type,
            VkSystemAllocationScope allocation_scope
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
            VkDebugUtilsMessageSeverityFlagsEXT message_severity_filter,
            VkDebugUtilsMessageTypeFlagsEXT message_type_filter,
            const DebugCallback& callback
        );

        constexpr const Context::ptr& context() const
        {
            return _context;
        }

        constexpr const VkDebugUtilsMessageSeverityFlagsEXT&
            message_severity_filter() const
        {
            return _message_severity_filter;
        }

        constexpr const VkDebugUtilsMessageTypeFlagsEXT&
            message_type_filter() const
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
        VkDebugUtilsMessageSeverityFlagsEXT _message_severity_filter;
        VkDebugUtilsMessageTypeFlagsEXT _message_type_filter;
        DebugCallback _callback;

        VkDebugUtilsMessengerEXT vk_debug_messenger = nullptr;

        DebugMessenger(
            const Context::ptr& context,
            VkDebugUtilsMessageSeverityFlagsEXT message_severity_filter,
            VkDebugUtilsMessageTypeFlagsEXT message_type_filter,
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
        friend class Sampler;
        friend class DescriptorSetLayout;
        friend class PipelineLayout;

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
            std::shared_ptr<ShaderModule>& waste_module,
            VkSpecializationInfo& waste_vk_specialization_info,
            std::vector<VkSpecializationMapEntry>& waste_vk_map_entries,
            std::vector<uint8_t>& waste_data
        );

    };

    // https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VkSampler.html
    class Sampler
    {
    public:
        using ptr = std::shared_ptr<Sampler>;

        Sampler() = delete;
        Sampler(const Sampler& other) = delete;
        Sampler(Sampler&& other) = default;

        static Result<Sampler::ptr> create(
            const Device::ptr& device,
            const SamplerConfig& config
        );

        constexpr const Device::ptr& device() const
        {
            return _device;
        }

        constexpr const SamplerConfig& config() const
        {
            return _config;
        }

        ~Sampler();

    protected:
        Device::ptr _device;
        SamplerConfig _config;

        VkSampler vk_sampler = nullptr;

        Sampler(
            const Device::ptr& device,
            const SamplerConfig& config
        );

        friend VkDescriptorSetLayoutBinding DescriptorSetLayoutBinding_to_vk(
            const DescriptorSetLayoutBinding& binding,
            std::vector<std::shared_ptr<Sampler>>& waste_immutable_samplers,
            std::vector<VkSampler>& waste_vk_immutable_samplers
        );

    };

    // https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VkDescriptorSetLayout.html
    class DescriptorSetLayout
    {
    public:
        using ptr = std::shared_ptr<DescriptorSetLayout>;

        DescriptorSetLayout() = delete;
        DescriptorSetLayout(const DescriptorSetLayout& other) = delete;
        DescriptorSetLayout(DescriptorSetLayout&& other) = default;

        static Result<DescriptorSetLayout::ptr> create(
            const Device::ptr& device,
            const DescriptorSetLayoutConfig& config
        );

        constexpr const Device::ptr& device() const
        {
            return _device;
        }

        constexpr const DescriptorSetLayoutConfig& config() const
        {
            return _config;
        }

        ~DescriptorSetLayout();

    protected:
        Device::ptr _device;
        DescriptorSetLayoutConfig _config;

        VkDescriptorSetLayout vk_descriptor_set_layout = nullptr;

        DescriptorSetLayout(
            const Device::ptr& device,
            const DescriptorSetLayoutConfig& config
        );

        friend class PipelineLayout;

    };

    // https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VkPipelineLayout.html
    class PipelineLayout
    {
    public:
        using ptr = std::shared_ptr<PipelineLayout>;

        PipelineLayout() = delete;
        PipelineLayout(const PipelineLayout& other) = delete;
        PipelineLayout(PipelineLayout&& other) = default;

        static Result<PipelineLayout::ptr> create(
            const Device::ptr& device,
            const PipelineLayoutConfig& config
        );

        constexpr const Device::ptr& device() const
        {
            return _device;
        }

        constexpr const PipelineLayoutConfig& config() const
        {
            return _config;
        }

        ~PipelineLayout();

    protected:
        Device::ptr _device;
        PipelineLayoutConfig _config;

        VkPipelineLayout vk_pipeline_layout = nullptr;

        PipelineLayout(
            const Device::ptr& device,
            const PipelineLayoutConfig& config
        );

    };

}
