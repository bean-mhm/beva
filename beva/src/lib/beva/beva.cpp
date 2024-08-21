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
    DEFINE_DERIVED_WITH_PUBLIC_CONSTRUCTOR(ShaderModule);

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

    std::string ApiResult_to_string(ApiResult result)
    {
        if (ApiResult_strmap.contains(result))
        {
            return ApiResult_strmap[result];
        }
        return std::format(
            "undocumented VkResult: {}",
            string_VkResult((VkResult)result)
        );
    }

    uint32_t VulkanApiVersion_encode(VulkanApiVersion version)
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

    SurfaceFormat SurfaceFormat_from_vk(
        const VkSurfaceFormatKHR& vk_surface_format
    )
    {
        return SurfaceFormat{
            .format = Format_from_vk(vk_surface_format.format),
            .color_space = ColorSpace_from_vk(vk_surface_format.colorSpace)
        };
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
        const QueueRequest& request,
        std::vector<float>& waste_priorities
    )
    {
        waste_priorities = request.priorities;
        return VkDeviceQueueCreateInfo{
            .sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
            .pNext = nullptr,
            .flags = QueueRequestFlags_to_vk(request.flags),
            .queueFamilyIndex = request.queue_family_index,
            .queueCount = request.num_queues_to_create,
            .pQueuePriorities = waste_priorities.data()
        };
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

    VkSpecializationMapEntry SpecializationMapEntry_to_vk(
        const SpecializationMapEntry& entry
    )
    {
        return VkSpecializationMapEntry{
            .constantID = entry.constant_id,
            .offset = entry.offset,
            .size = entry.size
        };
    }

    VkSpecializationInfo SpecializationInfo_to_vk(
        const SpecializationInfo& info,
        std::vector<VkSpecializationMapEntry>& waste_vk_map_entries,
        std::vector<uint8_t>& waste_data
    )
    {
        waste_vk_map_entries.clear();
        for (const auto& map_entry : info.map_entries)
        {
            waste_vk_map_entries.push_back(
                SpecializationMapEntry_to_vk(map_entry)
            );
        }

        waste_data = info.data;

        return VkSpecializationInfo{
            .mapEntryCount = (uint32_t)waste_vk_map_entries.size(),
            .pMapEntries = waste_vk_map_entries.data(),
            .dataSize = waste_data.size(),
            .pData = reinterpret_cast<const void*>(waste_data.data())
        };
    }

    VkPipelineShaderStageCreateInfo ShaderStage_to_vk(
        const ShaderStage& stage,
        VkSpecializationInfo& waste_vk_specialization_info,
        std::vector<VkSpecializationMapEntry>& waste_vk_map_entries,
        std::vector<uint8_t>& waste_data
    )
    {
        if (stage.specialization_info.has_value())
        {
            waste_vk_specialization_info = SpecializationInfo_to_vk(
                stage.specialization_info.value(),
                waste_vk_map_entries,
                waste_data
            );
        }

        return VkPipelineShaderStageCreateInfo{
            .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
            .pNext = nullptr,
            .flags = ShaderStageFlags_to_vk(stage.flags),
            .stage = ShaderStageType_to_vk(stage.type),
            .module = stage.module->vk_shader_module,
            .pName = stage.entry_point.c_str(),

            .pSpecializationInfo =
            stage.specialization_info.has_value()
            ? &waste_vk_specialization_info
            : nullptr
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
            s += ApiResult_to_string(api_result.value());
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

    Context::Context(Context&& other) noexcept
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
                        (uint32_t)i,
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

    Queue::Queue(VkQueue vk_queue)
        : vk_queue(vk_queue)
    {}

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
        std::vector<std::vector<float>> wastes_priorities;
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

            wastes_priorities.push_back(std::vector<float>());
            vk_queue_requests.push_back(
                QueueRequest_to_vk(
                    queue_request,
                    wastes_priorities.back()
                )
            );
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

    Image::Image(VkImage vk_image)
        : vk_image(vk_image)
    {}

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

    Result<ShaderModule::ptr> ShaderModule::create(
        const Device::ptr& device,
        const std::vector<uint8_t>& code
    )
    {
        ShaderModule::ptr module = std::make_shared<ShaderModule_public_ctor>(
            device
        );

        std::vector<uint8_t> code_aligned = code;
        if (code_aligned.size() % 8 != 0)
        {
            for (size_t i = 0; i < 8 - (code_aligned.size() % 8); i++)
            {
                code_aligned.push_back(0);
            }
        }

        VkShaderModuleCreateInfo create_info{
            .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
            .pNext = nullptr,
            .flags = 0,
            .codeSize = code.size(),
            .pCode = reinterpret_cast<const uint32_t*>(code_aligned.data())
        };

        VkResult vk_result = vkCreateShaderModule(
            module->device()->vk_device,
            &create_info,
            module->device()->context()->vk_allocator_ptr(),
            &module->vk_shader_module
        );
        if (vk_result != VK_SUCCESS)
        {
            return Error(vk_result);
        }
        return module;
    }

    ShaderModule::~ShaderModule()
    {
        vkDestroyShaderModule(
            device()->vk_device,
            vk_shader_module,
            device()->context()->vk_allocator_ptr()
        );
    }

    ShaderModule::ShaderModule(const Device::ptr& device)
        : _device(device)
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
