#include "beva.h"

namespace bv
{

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
        VkDebugUtilsMessageSeverityFlagBitsEXT message_severity,
        VkDebugUtilsMessageTypeFlagsEXT message_type,
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
            return string_VkResult(_undocumented_vk_result);
        }
        return ApiResultType_string[(uint8_t)_type];
    }

    SampleCountFlags SampleCountFlags_from_VkSampleCountFlags(
        VkSampleCountFlags vk_sample_count_flags
    )
    {
        return SampleCountFlags{
            ._1 = (bool)(vk_sample_count_flags & VK_SAMPLE_COUNT_1_BIT),
            ._2 = (bool)(vk_sample_count_flags & VK_SAMPLE_COUNT_2_BIT),
            ._4 = (bool)(vk_sample_count_flags & VK_SAMPLE_COUNT_4_BIT),
            ._8 = (bool)(vk_sample_count_flags & VK_SAMPLE_COUNT_8_BIT),
            ._16 = (bool)(vk_sample_count_flags & VK_SAMPLE_COUNT_16_BIT),
            ._32 = (bool)(vk_sample_count_flags & VK_SAMPLE_COUNT_32_BIT),
            ._64 = (bool)(vk_sample_count_flags & VK_SAMPLE_COUNT_64_BIT)
        };
    }

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

    std::string Version::to_string() const
    {
        return std::format("{}.{}.{}.{}", variant, major, minor, patch);
    }

    PhysicalDeviceLimits PhysicalDeviceLimits_from_VkPhysicalDeviceLimits(
        VkPhysicalDeviceLimits vk_physical_device_limits
    )
    {
        return PhysicalDeviceLimits{
            .max_image_dimension1d =
            vk_physical_device_limits.maxImageDimension1D,

            .max_image_dimension2d =
            vk_physical_device_limits.maxImageDimension2D,

            .max_image_dimension3d =
            vk_physical_device_limits.maxImageDimension3D,

            .max_image_dimension_cube =
            vk_physical_device_limits.maxImageDimensionCube,

            .max_image_array_layers =
            vk_physical_device_limits.maxImageArrayLayers,

            .max_texel_buffer_elements =
            vk_physical_device_limits.maxTexelBufferElements,

            .max_uniform_buffer_range =
            vk_physical_device_limits.maxUniformBufferRange,

            .max_storage_buffer_range =
            vk_physical_device_limits.maxStorageBufferRange,

            .max_push_constants_size =
            vk_physical_device_limits.maxPushConstantsSize,

            .max_memory_allocation_count =
            vk_physical_device_limits.maxMemoryAllocationCount,

            .max_sampler_allocation_count =
            vk_physical_device_limits.maxSamplerAllocationCount,

            .buffer_image_granularity =
            vk_physical_device_limits.bufferImageGranularity,

            .sparse_address_space_size =
            vk_physical_device_limits.sparseAddressSpaceSize,

            .max_bound_descriptor_sets =
            vk_physical_device_limits.maxBoundDescriptorSets,

            .max_per_stage_descriptor_samplers =
            vk_physical_device_limits.maxPerStageDescriptorSamplers,

            .max_per_stage_descriptor_uniform_buffers =
            vk_physical_device_limits.maxPerStageDescriptorUniformBuffers,

            .max_per_stage_descriptor_storage_buffers =
            vk_physical_device_limits.maxPerStageDescriptorStorageBuffers,

            .max_per_stage_descriptor_sampled_images =
            vk_physical_device_limits.maxPerStageDescriptorSampledImages,

            .max_per_stage_descriptor_storage_images =
            vk_physical_device_limits.maxPerStageDescriptorStorageImages,

            .max_per_stage_descriptor_input_attachments =
            vk_physical_device_limits.maxPerStageDescriptorInputAttachments,

            .max_per_stage_resources =
            vk_physical_device_limits.maxPerStageResources,

            .max_descriptor_set_samplers =
            vk_physical_device_limits.maxDescriptorSetSamplers,

            .max_descriptor_set_uniform_buffers =
            vk_physical_device_limits.maxDescriptorSetUniformBuffers,

            .max_descriptor_set_uniform_buffers_dynamic =
            vk_physical_device_limits.maxDescriptorSetUniformBuffersDynamic,

            .max_descriptor_set_storage_buffers =
            vk_physical_device_limits.maxDescriptorSetStorageBuffers,

            .max_descriptor_set_storage_buffers_dynamic =
            vk_physical_device_limits.maxDescriptorSetStorageBuffersDynamic,

            .max_descriptor_set_sampled_images =
            vk_physical_device_limits.maxDescriptorSetSampledImages,

            .max_descriptor_set_storage_images =
            vk_physical_device_limits.maxDescriptorSetStorageImages,

            .max_descriptor_set_input_attachments =
            vk_physical_device_limits.maxDescriptorSetInputAttachments,

            .max_vertex_input_attributes =
            vk_physical_device_limits.maxVertexInputAttributes,

            .max_vertex_input_bindings =
            vk_physical_device_limits.maxVertexInputBindings,

            .max_vertex_input_attribute_offset =
            vk_physical_device_limits.maxVertexInputAttributeOffset,

            .max_vertex_input_binding_stride =
            vk_physical_device_limits.maxVertexInputBindingStride,

            .max_vertex_output_components =
            vk_physical_device_limits.maxVertexOutputComponents,

            .max_tessellation_generation_level =
            vk_physical_device_limits.maxTessellationGenerationLevel,

            .max_tessellation_patch_size =
            vk_physical_device_limits.maxTessellationPatchSize,

            .max_tessellation_control_per_vertex_input_components =
            vk_physical_device_limits
            .maxTessellationControlPerVertexInputComponents,

            .max_tessellation_control_per_vertex_output_components =
            vk_physical_device_limits
            .maxTessellationControlPerVertexOutputComponents,

            .max_tessellation_control_per_patch_output_components =
            vk_physical_device_limits
            .maxTessellationControlPerPatchOutputComponents,

            .max_tessellation_control_total_output_components =
            vk_physical_device_limits
            .maxTessellationControlTotalOutputComponents,

            .max_tessellation_evaluation_input_components =
            vk_physical_device_limits.maxTessellationEvaluationInputComponents,

            .max_tessellation_evaluation_output_components =
            vk_physical_device_limits.maxTessellationEvaluationOutputComponents,

            .max_geometry_shader_invocations =
            vk_physical_device_limits.maxGeometryShaderInvocations,

            .max_geometry_input_components =
            vk_physical_device_limits.maxGeometryInputComponents,

            .max_geometry_output_components =
            vk_physical_device_limits.maxGeometryOutputComponents,

            .max_geometry_output_vertices =
            vk_physical_device_limits.maxGeometryOutputVertices,

            .max_geometry_total_output_components =
            vk_physical_device_limits.maxGeometryTotalOutputComponents,

            .max_fragment_input_components =
            vk_physical_device_limits.maxFragmentInputComponents,

            .max_fragment_output_attachments =
            vk_physical_device_limits.maxFragmentOutputAttachments,

            .max_fragment_dual_src_attachments =
            vk_physical_device_limits.maxFragmentDualSrcAttachments,

            .max_fragment_combined_output_resources =
            vk_physical_device_limits.maxFragmentCombinedOutputResources,

            .max_compute_shared_memory_size =
            vk_physical_device_limits.maxComputeSharedMemorySize,

            .max_compute_work_group_count = raw_arr_to_std<3>(
                vk_physical_device_limits.maxComputeWorkGroupCount
            ),

            .max_compute_work_group_invocations =
            vk_physical_device_limits.maxComputeWorkGroupInvocations,

            .max_compute_work_group_size = raw_arr_to_std<3>(
                vk_physical_device_limits.maxComputeWorkGroupSize
            ),

            .sub_pixel_precision_bits =
            vk_physical_device_limits.subPixelPrecisionBits,

            .sub_texel_precision_bits =
            vk_physical_device_limits.subTexelPrecisionBits,

            .mipmap_precision_bits =
            vk_physical_device_limits.mipmapPrecisionBits,

            .max_draw_indexed_index_value =
            vk_physical_device_limits.maxDrawIndexedIndexValue,

            .max_draw_indirect_count =
            vk_physical_device_limits.maxDrawIndirectCount,

            .max_sampler_lod_bias = vk_physical_device_limits.maxSamplerLodBias,

            .max_sampler_anisotropy =
            vk_physical_device_limits.maxSamplerAnisotropy,

            .max_viewports = vk_physical_device_limits.maxViewports,

            .max_viewport_dimensions = raw_arr_to_std<2>(
                vk_physical_device_limits.maxViewportDimensions
            ),

            .viewport_bounds_range = raw_arr_to_std<2>(
                vk_physical_device_limits.viewportBoundsRange
            ),

            .viewport_sub_pixel_bits =
            vk_physical_device_limits.viewportSubPixelBits,

            .min_memory_map_alignment =
            vk_physical_device_limits.minMemoryMapAlignment,

            .min_texel_buffer_offset_alignment =
            vk_physical_device_limits.minTexelBufferOffsetAlignment,

            .min_uniform_buffer_offset_alignment =
            vk_physical_device_limits.minUniformBufferOffsetAlignment,

            .min_storage_buffer_offset_alignment =
            vk_physical_device_limits.minStorageBufferOffsetAlignment,

            .min_texel_offset = vk_physical_device_limits.minTexelOffset,

            .max_texel_offset = vk_physical_device_limits.maxTexelOffset,

            .min_texel_gather_offset =
            vk_physical_device_limits.minTexelGatherOffset,

            .max_texel_gather_offset =
            vk_physical_device_limits.maxTexelGatherOffset,

            .min_interpolation_offset =
            vk_physical_device_limits.minInterpolationOffset,

            .max_interpolation_offset =
            vk_physical_device_limits.maxInterpolationOffset,

            .sub_pixel_interpolation_offset_bits =
            vk_physical_device_limits.subPixelInterpolationOffsetBits,

            .max_framebuffer_width =
            vk_physical_device_limits.maxFramebufferWidth,

            .max_framebuffer_height =
            vk_physical_device_limits.maxFramebufferHeight,

            .max_framebuffer_layers =
            vk_physical_device_limits.maxFramebufferLayers,

            .framebuffer_color_sample_counts =
            SampleCountFlags_from_VkSampleCountFlags(
                vk_physical_device_limits.framebufferColorSampleCounts
            ),

            .framebuffer_depth_sample_counts =
            SampleCountFlags_from_VkSampleCountFlags(
                vk_physical_device_limits.framebufferDepthSampleCounts
            ),

            .framebuffer_stencil_sample_counts =
            SampleCountFlags_from_VkSampleCountFlags(
                vk_physical_device_limits.framebufferStencilSampleCounts
            ),

            .framebuffer_no_attachments_sample_counts =
            SampleCountFlags_from_VkSampleCountFlags(
                vk_physical_device_limits.framebufferNoAttachmentsSampleCounts
            ),

            .max_color_attachments =
            vk_physical_device_limits.maxColorAttachments,

            .sampled_image_color_sample_counts =
            SampleCountFlags_from_VkSampleCountFlags(
                vk_physical_device_limits.sampledImageColorSampleCounts
            ),

            .sampled_image_integer_sample_counts =
            SampleCountFlags_from_VkSampleCountFlags(
                vk_physical_device_limits.sampledImageIntegerSampleCounts
            ),

            .sampled_image_depth_sample_counts =
            SampleCountFlags_from_VkSampleCountFlags(
                vk_physical_device_limits.sampledImageDepthSampleCounts
            ),

            .sampled_image_stencil_sample_counts =
            SampleCountFlags_from_VkSampleCountFlags(
                vk_physical_device_limits.sampledImageStencilSampleCounts
            ),

            .storage_image_sample_counts =
            SampleCountFlags_from_VkSampleCountFlags(
                vk_physical_device_limits.storageImageSampleCounts
            ),

            .max_sample_mask_words =
            vk_physical_device_limits.maxSampleMaskWords,

            .timestamp_compute_and_graphics =
            (bool)vk_physical_device_limits.timestampComputeAndGraphics,

            .timestamp_period = vk_physical_device_limits.timestampPeriod,

            .max_clip_distances = vk_physical_device_limits.maxClipDistances,

            .max_cull_distances = vk_physical_device_limits.maxCullDistances,

            .max_combined_clip_and_cull_distances =
            vk_physical_device_limits.maxCombinedClipAndCullDistances,

            .discrete_queue_priorities =
            vk_physical_device_limits.discreteQueuePriorities,

            .point_size_range = raw_arr_to_std<2>(
                vk_physical_device_limits.pointSizeRange
            ),

            .line_width_range = raw_arr_to_std<2>(
                vk_physical_device_limits.lineWidthRange
            ),

            .point_size_granularity =
            vk_physical_device_limits.pointSizeGranularity,

            .line_width_granularity =
            vk_physical_device_limits.lineWidthGranularity,

            .strict_lines = (bool)vk_physical_device_limits.strictLines,

            .standard_sample_locations =
            (bool)vk_physical_device_limits.standardSampleLocations,

            .optimal_buffer_copy_offset_alignment =
            vk_physical_device_limits.optimalBufferCopyOffsetAlignment,

            .optimal_buffer_copy_row_pitch_alignment =
            vk_physical_device_limits.optimalBufferCopyRowPitchAlignment,

            .non_coherent_atom_size =
            vk_physical_device_limits.nonCoherentAtomSize
        };
    }

    PhysicalDeviceFeatures
        PhysicalDeviceFeatures_from_VkPhysicalDeviceFeatures(
            VkPhysicalDeviceFeatures vk_physical_device_features
        )
    {
        return PhysicalDeviceFeatures{
            .robust_buffer_access =
            (bool)vk_physical_device_features.robustBufferAccess,

            .full_draw_index_uint32 =
            (bool)vk_physical_device_features.fullDrawIndexUint32,

            .image_cube_array =
            (bool)vk_physical_device_features.imageCubeArray,

            .independent_blend =
            (bool)vk_physical_device_features.independentBlend,

            .geometry_shader =
            (bool)vk_physical_device_features.geometryShader,

            .tessellation_shader =
            (bool)vk_physical_device_features.tessellationShader,

            .sample_rate_shading =
            (bool)vk_physical_device_features.sampleRateShading,

            .dual_src_blend =
            (bool)vk_physical_device_features.dualSrcBlend,

            .logic_op =
            (bool)vk_physical_device_features.logicOp,

            .multi_draw_indirect =
            (bool)vk_physical_device_features.multiDrawIndirect,

            .draw_indirect_first_instance =
            (bool)vk_physical_device_features.drawIndirectFirstInstance,

            .depth_clamp =
            (bool)vk_physical_device_features.depthClamp,

            .depth_bias_clamp =
            (bool)vk_physical_device_features.depthBiasClamp,

            .fill_mode_non_solid =
            (bool)vk_physical_device_features.fillModeNonSolid,

            .depth_bounds =
            (bool)vk_physical_device_features.depthBounds,

            .wide_lines =
            (bool)vk_physical_device_features.wideLines,

            .large_points =
            (bool)vk_physical_device_features.largePoints,

            .alpha_to_one =
            (bool)vk_physical_device_features.alphaToOne,

            .multi_viewport =
            (bool)vk_physical_device_features.multiViewport,

            .sampler_anisotropy =
            (bool)vk_physical_device_features.samplerAnisotropy,

            .texture_compression_etc2 =
            (bool)vk_physical_device_features.textureCompressionETC2,

            .texture_compression_astc_ldr =
            (bool)vk_physical_device_features.textureCompressionASTC_LDR,

            .texture_compression_bc =
            (bool)vk_physical_device_features.textureCompressionBC,

            .occlusion_query_precise =
            (bool)vk_physical_device_features.occlusionQueryPrecise,

            .pipeline_statistics_query =
            (bool)vk_physical_device_features.pipelineStatisticsQuery,

            .vertex_pipeline_stores_and_atomics =
            (bool)vk_physical_device_features.vertexPipelineStoresAndAtomics,

            .fragment_stores_and_atomics =
            (bool)vk_physical_device_features.fragmentStoresAndAtomics,

            .shader_tessellation_and_geometry_point_size =
            (bool)vk_physical_device_features
            .shaderTessellationAndGeometryPointSize,

            .shader_image_gather_extended =
            (bool)vk_physical_device_features.shaderImageGatherExtended,

            .shader_storage_image_extended_formats =
            (bool)vk_physical_device_features
            .shaderStorageImageExtendedFormats,

            .shader_storage_image_multisample =
            (bool)vk_physical_device_features.shaderStorageImageMultisample,

            .shader_storage_image_read_without_format =
            (bool)vk_physical_device_features
            .shaderStorageImageReadWithoutFormat,

            .shader_storage_image_write_without_format =
            (bool)vk_physical_device_features
            .shaderStorageImageWriteWithoutFormat,

            .shader_uniform_buffer_array_dynamic_indexing =
            (bool)vk_physical_device_features
            .shaderUniformBufferArrayDynamicIndexing,

            .shader_sampled_image_array_dynamic_indexing =
            (bool)vk_physical_device_features
            .shaderSampledImageArrayDynamicIndexing,

            .shader_storage_buffer_array_dynamic_indexing =
            (bool)vk_physical_device_features
            .shaderStorageBufferArrayDynamicIndexing,

            .shader_storage_image_array_dynamic_indexing =
            (bool)vk_physical_device_features
            .shaderStorageImageArrayDynamicIndexing,

            .shader_clip_distance =
            (bool)vk_physical_device_features.shaderClipDistance,

            .shader_cull_distance =
            (bool)vk_physical_device_features.shaderCullDistance,

            .shader_float64 =
            (bool)vk_physical_device_features.shaderFloat64,

            .shader_int64 =
            (bool)vk_physical_device_features.shaderInt64,

            .shader_int16 =
            (bool)vk_physical_device_features.shaderInt16,

            .shader_resource_residency =
            (bool)vk_physical_device_features.shaderResourceResidency,

            .shader_resource_min_lod =
            (bool)vk_physical_device_features.shaderResourceMinLod,

            .sparse_binding =
            (bool)vk_physical_device_features.sparseBinding,

            .sparse_residency_buffer =
            (bool)vk_physical_device_features.sparseResidencyBuffer,

            .sparse_residency_image2d =
            (bool)vk_physical_device_features.sparseResidencyImage2D,

            .sparse_residency_image3d =
            (bool)vk_physical_device_features.sparseResidencyImage3D,

            .sparse_residency2_samples =
            (bool)vk_physical_device_features.sparseResidency2Samples,

            .sparse_residency4_samples =
            (bool)vk_physical_device_features.sparseResidency4Samples,

            .sparse_residency8_samples =
            (bool)vk_physical_device_features.sparseResidency8Samples,

            .sparse_residency16_samples =
            (bool)vk_physical_device_features.sparseResidency16Samples,

            .sparse_residency_aliased =
            (bool)vk_physical_device_features.sparseResidencyAliased,

            .variable_multisample_rate =
            (bool)vk_physical_device_features.variableMultisampleRate,

            .inherited_queries =
            (bool)vk_physical_device_features.inheritedQueries
        };
    }

    VkPhysicalDeviceFeatures
        PhysicalDeviceFeatures_to_VkPhysicalDeviceFeatures(
            PhysicalDeviceFeatures physical_device_features
        )
    {
        return VkPhysicalDeviceFeatures{
            .robustBufferAccess =
            physical_device_features.robust_buffer_access,

            .fullDrawIndexUint32 =
            physical_device_features.full_draw_index_uint32,

            .imageCubeArray =
            physical_device_features.image_cube_array,

            .independentBlend =
            physical_device_features.independent_blend,

            .geometryShader =
            physical_device_features.geometry_shader,

            .tessellationShader =
            physical_device_features.tessellation_shader,

            .sampleRateShading =
            physical_device_features.sample_rate_shading,

            .dualSrcBlend =
            physical_device_features.dual_src_blend,

            .logicOp =
            physical_device_features.logic_op,

            .multiDrawIndirect =
            physical_device_features.multi_draw_indirect,

            .drawIndirectFirstInstance =
            physical_device_features.draw_indirect_first_instance,

            .depthClamp =
            physical_device_features.depth_clamp,

            .depthBiasClamp =
            physical_device_features.depth_bias_clamp,

            .fillModeNonSolid =
            physical_device_features.fill_mode_non_solid,

            .depthBounds =
            physical_device_features.depth_bounds,

            .wideLines =
            physical_device_features.wide_lines,

            .largePoints =
            physical_device_features.large_points,

            .alphaToOne =
            physical_device_features.alpha_to_one,

            .multiViewport =
            physical_device_features.multi_viewport,

            .samplerAnisotropy =
            physical_device_features.sampler_anisotropy,

            .textureCompressionETC2 =
            physical_device_features.texture_compression_etc2,

            .textureCompressionASTC_LDR =
            physical_device_features.texture_compression_astc_ldr,

            .textureCompressionBC =
            physical_device_features.texture_compression_bc,

            .occlusionQueryPrecise =
            physical_device_features.occlusion_query_precise,

            .pipelineStatisticsQuery =
            physical_device_features.pipeline_statistics_query,

            .vertexPipelineStoresAndAtomics =
            physical_device_features.vertex_pipeline_stores_and_atomics,

            .fragmentStoresAndAtomics =
            physical_device_features.fragment_stores_and_atomics,

            .shaderTessellationAndGeometryPointSize =
            physical_device_features
            .shader_tessellation_and_geometry_point_size,

            .shaderImageGatherExtended =
            physical_device_features.shader_image_gather_extended,

            .shaderStorageImageExtendedFormats =
            physical_device_features.shader_storage_image_extended_formats,

            .shaderStorageImageMultisample =
            physical_device_features.shader_storage_image_multisample,

            .shaderStorageImageReadWithoutFormat =
            physical_device_features.shader_storage_image_read_without_format,

            .shaderStorageImageWriteWithoutFormat =
            physical_device_features.shader_storage_image_write_without_format,

            .shaderUniformBufferArrayDynamicIndexing =
            physical_device_features
            .shader_uniform_buffer_array_dynamic_indexing,

            .shaderSampledImageArrayDynamicIndexing =
            physical_device_features
            .shader_sampled_image_array_dynamic_indexing,

            .shaderStorageBufferArrayDynamicIndexing =
            physical_device_features
            .shader_storage_buffer_array_dynamic_indexing,

            .shaderStorageImageArrayDynamicIndexing =
            physical_device_features
            .shader_storage_image_array_dynamic_indexing,

            .shaderClipDistance =
            physical_device_features.shader_clip_distance,

            .shaderCullDistance =
            physical_device_features.shader_cull_distance,

            .shaderFloat64 =
            physical_device_features.shader_float64,

            .shaderInt64 =
            physical_device_features.shader_int64,

            .shaderInt16 =
            physical_device_features.shader_int16,

            .shaderResourceResidency =
            physical_device_features.shader_resource_residency,

            .shaderResourceMinLod =
            physical_device_features.shader_resource_min_lod,

            .sparseBinding =
            physical_device_features.sparse_binding,

            .sparseResidencyBuffer =
            physical_device_features.sparse_residency_buffer,

            .sparseResidencyImage2D =
            physical_device_features.sparse_residency_image2d,

            .sparseResidencyImage3D =
            physical_device_features.sparse_residency_image3d,

            .sparseResidency2Samples =
            physical_device_features.sparse_residency2_samples,

            .sparseResidency4Samples =
            physical_device_features.sparse_residency4_samples,

            .sparseResidency8Samples =
            physical_device_features.sparse_residency8_samples,

            .sparseResidency16Samples =
            physical_device_features.sparse_residency16_samples,

            .sparseResidencyAliased =
            physical_device_features.sparse_residency_aliased,

            .variableMultisampleRate =
            physical_device_features.variable_multisample_rate,

            .inheritedQueries =
            physical_device_features.inherited_queries,
        };
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
            app_info.applicationVersion = VK_MAKE_API_VERSION(
                c->config().app_version.variant,
                c->config().app_version.major,
                c->config().app_version.minor,
                c->config().app_version.patch
            );

            app_info.pEngineName = c->config().engine_name.c_str();
            app_info.engineVersion = VK_MAKE_API_VERSION(
                c->config().engine_version.variant,
                c->config().engine_version.major,
                c->config().engine_version.minor,
                c->config().engine_version.patch
            );

            switch (c->config().vulkan_api_version)
            {
            case VulkanApiVersion::Vulkan1_0:
                app_info.apiVersion = VK_API_VERSION_1_0;
                break;
            case VulkanApiVersion::Vulkan1_1:
                app_info.apiVersion = VK_API_VERSION_1_1;
                break;
            case VulkanApiVersion::Vulkan1_2:
                app_info.apiVersion = VK_API_VERSION_1_2;
                break;
            case VulkanApiVersion::Vulkan1_3:
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

        Result fetch_physical_devices_result = c->fetch_physical_devices();
        if (!fetch_physical_devices_result.ok())
        {
            return Error(
                "failed to fetch physical devices: "
                + fetch_physical_devices_result.error().to_string()
            );
        }

        return c;
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
                .spec_version = Version(layer.specVersion),
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
        std::shared_ptr<Surface> surface
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

            PhysicalDeviceLimits limits =
                PhysicalDeviceLimits_from_VkPhysicalDeviceLimits(
                    vk_properties.limits
                );

            PhysicalDeviceSparseProperties sparse_properties{
                .residency_standard2d_block_shape = (bool)
                vk_properties.sparseProperties.residencyStandard2DBlockShape,

                .residency_standard2d_multisample_block_shape = (bool)
                vk_properties.sparseProperties
                .residencyStandard2DMultisampleBlockShape,

                .residency_standard3d_block_shape = (bool)
                vk_properties.sparseProperties.residencyStandard3DBlockShape,

                .residency_aligned_mip_size =
                (bool)vk_properties.sparseProperties.residencyAlignedMipSize,

                .residency_non_resident_strict =
                (bool)vk_properties.sparseProperties.residencyNonResidentStrict,

            };

            PhysicalDeviceProperties properties{
                .api_version = Version(vk_properties.apiVersion),
                .driver_version = vk_properties.driverVersion,
                .vendor_id = vk_properties.vendorID,
                .device_id = vk_properties.deviceID,
                .device_type = PhysicalDeviceType_from_VkPhysicalDeviceType(
                    vk_properties.deviceType
                ),
                .device_name = vk_properties.deviceName,
                .pipeline_cache_uuid = raw_arr_to_std<VK_UUID_SIZE>(
                    vk_properties.pipelineCacheUUID
                ),
                .limits = limits,
                .sparse_properties = sparse_properties
            };

            VkPhysicalDeviceFeatures vk_features;
            vkGetPhysicalDeviceFeatures(vk_physical_device, &vk_features);

            PhysicalDeviceFeatures features =
                PhysicalDeviceFeatures_from_VkPhysicalDeviceFeatures(
                    vk_features
                );

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

                QueueFlags queue_flags{
                    .graphics = (bool)
                    (vk_queue_family.queueFlags & VK_QUEUE_GRAPHICS_BIT),

                    .present = (bool)vk_present_support,

                    .compute = (bool)
                    (vk_queue_family.queueFlags & VK_QUEUE_COMPUTE_BIT),

                    .transfer = (bool)
                    (vk_queue_family.queueFlags & VK_QUEUE_TRANSFER_BIT),

                    .sparse_binding = (bool)
                    (vk_queue_family.queueFlags & VK_QUEUE_SPARSE_BINDING_BIT),

                    .protected_ = (bool)
                    (vk_queue_family.queueFlags & VK_QUEUE_PROTECTED_BIT),

                    .video_decode =
                    (bool)(
                        vk_queue_family.queueFlags
                        & VK_QUEUE_VIDEO_DECODE_BIT_KHR
                        ),

                    .optical_flow_nv = (bool)
                    (vk_queue_family.queueFlags & VK_QUEUE_OPTICAL_FLOW_BIT_NV)
                };

                if (queue_flags.graphics &&
                    !queue_family_indices.graphics.has_value())
                {
                    queue_family_indices.graphics = i;
                }
                if (queue_flags.present &&
                    !queue_family_indices.present.has_value())
                {
                    queue_family_indices.present = i;
                }
                if (queue_flags.compute &&
                    !queue_family_indices.compute.has_value())
                {
                    queue_family_indices.compute = i;
                }
                if (queue_flags.transfer &&
                    !queue_family_indices.transfer.has_value())
                {
                    queue_family_indices.transfer = i;
                }
                if (queue_flags.sparse_binding &&
                    !queue_family_indices.sparse_binding.has_value())
                {
                    queue_family_indices.sparse_binding = i;
                }
                if (queue_flags.protected_ &&
                    !queue_family_indices.protected_.has_value())
                {
                    queue_family_indices.protected_ = i;
                }
                if (queue_flags.video_decode &&
                    !queue_family_indices.video_decode.has_value())
                {
                    queue_family_indices.video_decode = i;
                }
                if (queue_flags.optical_flow_nv &&
                    !queue_family_indices.optical_flow_nv.has_value())
                {
                    queue_family_indices.optical_flow_nv = i;
                }

                bool supports_both_graphics_and_present =
                    queue_flags.graphics && queue_flags.present;

                bool already_have_shared_graphics_and_present_indices =
                    (
                        queue_family_indices.graphics.has_value()
                        && queue_family_indices.present.has_value()
                        )
                    && (
                        queue_family_indices.graphics.value()
                        == queue_family_indices.present.value()
                        );

                if (supports_both_graphics_and_present &&
                    !already_have_shared_graphics_and_present_indices)
                {
                    queue_family_indices.graphics = i;
                    queue_family_indices.present = i;
                }

                QueueFamily queue_family{
                    .queue_flags = queue_flags,
                    .queue_count = vk_queue_family.queueCount,
                    .timestamp_valid_bits = vk_queue_family.timestampValidBits,
                    .min_image_transfer_granularity = Extent3D(
                        vk_queue_family.minImageTransferGranularity
                    )
                };
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
        other._message_severity_filter = DebugMessageSeverityFilter{
            .verbose = false,
            .info = false,
            .warning = false,
            .error = false
        };

        _message_type_filter = other._message_type_filter;
        other._message_type_filter = DebugMessageTypeFilter{
            .general = false,
            .validation = false,
            .performance = false,
            .device_address_binding = false
        };

        _callback = other._callback;
        other._callback = nullptr;
    }

    Result<DebugMessenger::ptr> DebugMessenger::create(
        const Context::ptr& context,
        DebugMessageSeverityFilter message_severity_filter,
        DebugMessageTypeFilter message_type_filter,
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

        VkDebugUtilsMessengerCreateInfoEXT create_info{};
        create_info.sType =
            VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;

        create_info.messageSeverity = 0;
        if (messenger->message_severity_filter().verbose)
        {
            create_info.messageSeverity |=
                VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT;
        }
        if (messenger->message_severity_filter().info)
        {
            create_info.messageSeverity |=
                VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT;
        }
        if (messenger->message_severity_filter().warning)
        {
            create_info.messageSeverity |=
                VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT;
        }
        if (messenger->message_severity_filter().error)
        {
            create_info.messageSeverity |=
                VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
        }

        create_info.messageType = 0;
        if (messenger->message_type_filter().general)
        {
            create_info.messageType |=
                VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT;
        }
        if (messenger->message_type_filter().validation)
        {
            create_info.messageType |=
                VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT;
        }
        if (messenger->message_type_filter().performance)
        {
            create_info.messageType |=
                VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
        }
        if (messenger->message_type_filter().device_address_binding)
        {
            create_info.messageType |=
                VK_DEBUG_UTILS_MESSAGE_TYPE_DEVICE_ADDRESS_BINDING_BIT_EXT;
        }

        create_info.pfnUserCallback = vk_debug_callback;
        create_info.pUserData = messenger.get();

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
        const DebugMessageSeverityFilter& message_severity_filter,
        const DebugMessageTypeFilter& message_type_filter,
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

            VkDeviceQueueCreateFlags vk_flags = 0;
            if (queue_request.flags.protected_)
                vk_flags |= VK_DEVICE_QUEUE_CREATE_PROTECTED_BIT;

            vk_queue_requests.push_back(VkDeviceQueueCreateInfo{
                .sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
                .pNext = nullptr,
                .flags = vk_flags,
                .queueFamilyIndex = queue_request.queue_family_index,
                .queueCount = queue_request.num_queues_to_create,
                .pQueuePriorities = queue_request.priorities.data()
                });
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
            PhysicalDeviceFeatures_to_VkPhysicalDeviceFeatures(
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

    static VKAPI_ATTR VkBool32 VKAPI_CALL vk_debug_callback(
        VkDebugUtilsMessageSeverityFlagBitsEXT message_severity,
        VkDebugUtilsMessageTypeFlagsEXT message_type,
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

        DebugMessageSeverity severity;
        if (message_severity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT)
            severity = DebugMessageSeverity::Error;
        else if (message_severity &
            VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT)
            severity = DebugMessageSeverity::Warning;
        else if (message_severity &
            VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT)
            severity = DebugMessageSeverity::Info;
        else if (message_severity &
            VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT)
            severity = DebugMessageSeverity::Verbose;
        else
            severity = DebugMessageSeverity::Verbose;

        DebugMessageType type = DebugMessageType::General;
        if (message_type & VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT)
            type = DebugMessageType::Validation;
        else if (message_type & VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT)
            type = DebugMessageType::Performance;
        else if (message_type &
            VK_DEBUG_UTILS_MESSAGE_TYPE_DEVICE_ADDRESS_BINDING_BIT_EXT)
            type = DebugMessageType::DeviceAddressBinding;
        else if (message_type & VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT)
            type = DebugMessageType::General;
        else
            type = DebugMessageType::General;

        std::vector<DebugLabel> queue_labels;
        for (size_t i = 0; i < p_callback_data->queueLabelCount; i++)
        {
            VkDebugUtilsLabelEXT vk_label = p_callback_data->pQueueLabels[i];

            DebugLabel label{ .name = vk_label.pLabelName };
            std::copy(vk_label.color, vk_label.color + 4, label.color.data());

            queue_labels.push_back(label);
        }

        std::vector<DebugLabel> cmd_buf_labels;
        for (size_t i = 0; i < p_callback_data->cmdBufLabelCount; i++)
        {
            VkDebugUtilsLabelEXT vk_label = p_callback_data->pCmdBufLabels[i];

            DebugLabel label{ .name = vk_label.pLabelName };
            std::copy(vk_label.color, vk_label.color + 4, label.color.data());

            cmd_buf_labels.push_back(label);
        }

        std::vector<DebugObjectInfo> objects;
        for (size_t i = 0; i < p_callback_data->objectCount; i++)
        {
            VkDebugUtilsObjectNameInfoEXT vk_object =
                p_callback_data->pObjects[i];

            objects.push_back(DebugObjectInfo{
                .type = ObjectType_from_VkObjectType(vk_object.objectType),
                .handle = vk_object.objectHandle,
                .name = vk_object.pObjectName
                });
        }

        DebugMessageData message_data{
            .message_id_name = p_callback_data->pMessageIdName,
            .message_id_number = p_callback_data->messageIdNumber,
            .message = p_callback_data->pMessage,
            .queue_labels = queue_labels,
            .cmd_buf_labels = cmd_buf_labels,
            .objects = objects
        };

        messenger->callback()(
            severity,
            type,
            message_data
            );

        return VK_FALSE;
    }

}
