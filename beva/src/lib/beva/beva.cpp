#include "beva.hpp"

namespace bv
{

    // define a derived class named ClassName_public_ctor that lets us use the
    // previously private constructors (actually protected, just go with it) as
    // public ones so that they can be used in std::make_shared() or whatever
    // else.
#define BV_DEFINE_DERIVED_WITH_PUBLIC_CONSTRUCTOR(ClassName) \
    class ClassName##_public_ctor : public ClassName \
    { \
    public: \
        template<typename... Args> ClassName##_public_ctor(Args&&... args) \
            : ClassName(std::forward<Args>(args)...) \
        {} \
    };

    BV_DEFINE_DERIVED_WITH_PUBLIC_CONSTRUCTOR(PhysicalDevice);
    BV_DEFINE_DERIVED_WITH_PUBLIC_CONSTRUCTOR(Context);
    BV_DEFINE_DERIVED_WITH_PUBLIC_CONSTRUCTOR(DebugMessenger);
    BV_DEFINE_DERIVED_WITH_PUBLIC_CONSTRUCTOR(Surface);
    BV_DEFINE_DERIVED_WITH_PUBLIC_CONSTRUCTOR(Queue);
    BV_DEFINE_DERIVED_WITH_PUBLIC_CONSTRUCTOR(Device);
    BV_DEFINE_DERIVED_WITH_PUBLIC_CONSTRUCTOR(Image);
    BV_DEFINE_DERIVED_WITH_PUBLIC_CONSTRUCTOR(Swapchain);
    BV_DEFINE_DERIVED_WITH_PUBLIC_CONSTRUCTOR(ImageView);
    BV_DEFINE_DERIVED_WITH_PUBLIC_CONSTRUCTOR(ShaderModule);
    BV_DEFINE_DERIVED_WITH_PUBLIC_CONSTRUCTOR(Sampler);
    BV_DEFINE_DERIVED_WITH_PUBLIC_CONSTRUCTOR(DescriptorSetLayout);
    BV_DEFINE_DERIVED_WITH_PUBLIC_CONSTRUCTOR(PipelineLayout);
    BV_DEFINE_DERIVED_WITH_PUBLIC_CONSTRUCTOR(RenderPass);
    BV_DEFINE_DERIVED_WITH_PUBLIC_CONSTRUCTOR(GraphicsPipeline);
    BV_DEFINE_DERIVED_WITH_PUBLIC_CONSTRUCTOR(Framebuffer);
    BV_DEFINE_DERIVED_WITH_PUBLIC_CONSTRUCTOR(CommandBuffer);
    BV_DEFINE_DERIVED_WITH_PUBLIC_CONSTRUCTOR(CommandPool);
    BV_DEFINE_DERIVED_WITH_PUBLIC_CONSTRUCTOR(Semaphore);
    BV_DEFINE_DERIVED_WITH_PUBLIC_CONSTRUCTOR(Fence);
    BV_DEFINE_DERIVED_WITH_PUBLIC_CONSTRUCTOR(Buffer);
    BV_DEFINE_DERIVED_WITH_PUBLIC_CONSTRUCTOR(DeviceMemory);
    BV_DEFINE_DERIVED_WITH_PUBLIC_CONSTRUCTOR(DescriptorSet);
    BV_DEFINE_DERIVED_WITH_PUBLIC_CONSTRUCTOR(DescriptorPool);
    BV_DEFINE_DERIVED_WITH_PUBLIC_CONSTRUCTOR(BufferView);

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

#pragma region forward declarations

    static void* vk_allocation_callback(
        void* p_user_data,
        size_t size,
        size_t alignment,
        VkSystemAllocationScope allocation_scope
    );
    static void* vk_reallocation_callback(
        void* p_user_data,
        void* p_original,
        size_t size,
        size_t alignment,
        VkSystemAllocationScope allocation_scope
    );
    static void vk_free_callback(
        void* p_user_data,
        void* p_memory
    );
    static void vk_internal_allocation_notification(
        void* p_user_data,
        size_t size,
        VkInternalAllocationType allocation_type,
        VkSystemAllocationScope allocation_scope
    );
    static void vk_internal_free_notification(
        void* p_user_data,
        size_t size,
        VkInternalAllocationType allocation_type,
        VkSystemAllocationScope allocation_scope
    );
    static VKAPI_ATTR VkBool32 VKAPI_CALL vk_debug_callback(
        VkDebugUtilsMessageSeverityFlagBitsEXT message_severity,
        VkDebugUtilsMessageTypeFlagsEXT message_types,
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

            .framebuffer_color_sample_counts =
            vk_limits.framebufferColorSampleCounts,

            .framebuffer_depth_sample_counts =
            vk_limits.framebufferDepthSampleCounts,

            .framebuffer_stencil_sample_counts =
            vk_limits.framebufferStencilSampleCounts,

            .framebuffer_no_attachments_sample_counts =
            vk_limits.framebufferNoAttachmentsSampleCounts,

            .max_color_attachments =
            vk_limits.maxColorAttachments,

            .sampled_image_color_sample_counts =
            vk_limits.sampledImageColorSampleCounts,

            .sampled_image_integer_sample_counts =
            vk_limits.sampledImageIntegerSampleCounts,

            .sampled_image_depth_sample_counts =
            vk_limits.sampledImageDepthSampleCounts,

            .sampled_image_stencil_sample_counts =
            vk_limits.sampledImageStencilSampleCounts,

            .storage_image_sample_counts =
            vk_limits.storageImageSampleCounts,

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
            .device_type = vk_properties.deviceType,
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
        VkBool32 vk_surface_support
    )
    {
        return QueueFamily{
            .queue_flags = vk_family.queueFlags,
            .queue_count = vk_family.queueCount,
            .timestamp_valid_bits = vk_family.timestampValidBits,
            .min_image_transfer_granularity = Extent3d_from_vk(
                vk_family.minImageTransferGranularity
            ),
            .surface_support = (bool)vk_surface_support
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
            .supported_transforms = vk_capabilities.supportedTransforms,
            .current_transform = vk_capabilities.currentTransform,

            .supported_composite_alpha =
            vk_capabilities.supportedCompositeAlpha,

            .supported_usage_flags = vk_capabilities.supportedUsageFlags
        };
    }

    SurfaceFormat SurfaceFormat_from_vk(
        const VkSurfaceFormatKHR& vk_surface_format
    )
    {
        return SurfaceFormat{
            .format = vk_surface_format.format,
            .color_space = vk_surface_format.colorSpace
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
            .type = vk_info.objectType,
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
            .flags = request.flags,
            .queueFamilyIndex = request.queue_family_index,
            .queueCount = request.num_queues_to_create,
            .pQueuePriorities = waste_priorities.data()
        };
    }

    VkComponentMapping ComponentMapping_to_vk(const ComponentMapping& mapping)
    {
        return VkComponentMapping{
            .r = mapping.r,
            .g = mapping.g,
            .b = mapping.b,
            .a = mapping.a
        };
    }

    VkImageSubresourceRange ImageSubresourceRange_to_vk(
        const ImageSubresourceRange& range
    )
    {
        return VkImageSubresourceRange{
            .aspectMask = range.aspect_mask,
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
        waste_vk_map_entries.resize(info.map_entries.size());
        for (size_t i = 0; i < info.map_entries.size(); i++)
        {
            waste_vk_map_entries[i] = SpecializationMapEntry_to_vk(
                info.map_entries[i]
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
        ShaderModulePtr& waste_module,
        VkSpecializationInfo& waste_vk_specialization_info,
        std::vector<VkSpecializationMapEntry>& waste_vk_map_entries,
        std::vector<uint8_t>& waste_data
    )
    {
        waste_module = stage.module;

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
            .flags = stage.flags,
            .stage = stage.stage,
            .module = waste_module->handle(),
            .pName = stage.entry_point.c_str(),

            .pSpecializationInfo =
            stage.specialization_info.has_value()
            ? &waste_vk_specialization_info
            : nullptr
        };
    }

    VkPipelineDynamicStateCreateInfo DynamicStates_to_vk(
        const DynamicStates& states,
        std::vector<VkDynamicState>& waste_dynamic_states
    )
    {
        waste_dynamic_states = states;
        return VkPipelineDynamicStateCreateInfo{
            .sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO,
            .pNext = nullptr,
            .flags = 0,
            .dynamicStateCount = (uint32_t)waste_dynamic_states.size(),
            .pDynamicStates = waste_dynamic_states.data()
        };
    }

    VkVertexInputBindingDescription VertexInputBindingDescription_to_vk(
        const VertexInputBindingDescription& description
    )
    {
        return VkVertexInputBindingDescription{
            .binding = description.binding,
            .stride = description.stride,
            .inputRate = description.input_rate
        };
    }

    VkVertexInputAttributeDescription VertexInputAttributeDescription_to_vk(
        const VertexInputAttributeDescription& description
    )
    {
        return VkVertexInputAttributeDescription{
            .location = description.location,
            .binding = description.binding,
            .format = description.format,
            .offset = description.offset
        };
    }

    VkPipelineVertexInputStateCreateInfo VertexInputState_to_vk(
        const VertexInputState& state,

        std::vector<VkVertexInputBindingDescription>&
        waste_vk_binding_descriptions,

        std::vector<VkVertexInputAttributeDescription>&
        waste_vk_attribute_descriptions
    )
    {
        waste_vk_binding_descriptions.resize(state.binding_descriptions.size());
        for (size_t i = 0; i < state.binding_descriptions.size(); i++)
        {
            waste_vk_binding_descriptions[i] =
                VertexInputBindingDescription_to_vk(
                    state.binding_descriptions[i]
                );
        }

        waste_vk_attribute_descriptions.resize(
            state.attribute_descriptions.size()
        );
        for (size_t i = 0; i < state.attribute_descriptions.size(); i++)
        {
            waste_vk_attribute_descriptions[i] =
                VertexInputAttributeDescription_to_vk(
                    state.attribute_descriptions[i]
                );
        }

        return VkPipelineVertexInputStateCreateInfo{
            .sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
            .pNext = nullptr,
            .flags = 0,

            .vertexBindingDescriptionCount =
            (uint32_t)waste_vk_binding_descriptions.size(),

            .pVertexBindingDescriptions = waste_vk_binding_descriptions.data(),

            .vertexAttributeDescriptionCount =
            (uint32_t)waste_vk_attribute_descriptions.size(),

            .pVertexAttributeDescriptions =
            waste_vk_attribute_descriptions.data()
        };
    }

    VkPipelineInputAssemblyStateCreateInfo InputAssemblyState_to_vk(
        const InputAssemblyState& state
    )
    {
        return VkPipelineInputAssemblyStateCreateInfo{
            .sType =
            VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,

            .pNext = nullptr,
            .flags = 0,
            .topology = state.topology,
            .primitiveRestartEnable = state.primitive_restart_enable
        };
    }

    VkPipelineTessellationStateCreateInfo TessellationState_to_vk(
        const TessellationState& state
    )
    {
        return VkPipelineTessellationStateCreateInfo{
            .sType = VK_STRUCTURE_TYPE_PIPELINE_TESSELLATION_STATE_CREATE_INFO,
            .pNext = nullptr,
            .flags = 0,
            .patchControlPoints = state.patch_control_points
        };
    }

    VkViewport Viewport_to_vk(const Viewport& viewport)
    {
        return VkViewport{
            .x = viewport.x,
            .y = viewport.y,
            .width = viewport.width,
            .height = viewport.height,
            .minDepth = viewport.min_depth,
            .maxDepth = viewport.max_depth
        };
    }

    VkOffset2D Offset2d_to_vk(const Offset2d& offset)
    {
        return VkOffset2D{
            .x = offset.x,
            .y = offset.y
        };
    }

    VkOffset3D Offset3d_to_vk(const Offset3d& offset)
    {
        return VkOffset3D{
            .x = offset.x,
            .y = offset.y,
            .z = offset.z
        };
    }

    VkRect2D Rect2d_to_vk(const Rect2d& rect)
    {
        return VkRect2D{
            .offset = Offset2d_to_vk(rect.offset),
            .extent = Extent2d_to_vk(rect.extent)
        };
    }

    VkPipelineViewportStateCreateInfo ViewportState_to_vk(
        const ViewportState& state,
        std::vector<VkViewport>& waste_vk_viewports,
        std::vector<VkRect2D>& waste_vk_scissors
    )
    {
        waste_vk_viewports.resize(state.viewports.size());
        for (size_t i = 0; i < state.viewports.size(); i++)
        {
            waste_vk_viewports[i] = Viewport_to_vk(state.viewports[i]);
        }

        waste_vk_scissors.resize(state.scissors.size());
        for (size_t i = 0; i < state.scissors.size(); i++)
        {
            waste_vk_scissors[i] = Rect2d_to_vk(state.scissors[i]);
        }

        return VkPipelineViewportStateCreateInfo{
            .sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
            .pNext = nullptr,
            .flags = 0,
            .viewportCount = (uint32_t)waste_vk_viewports.size(),
            .pViewports = waste_vk_viewports.data(),
            .scissorCount = (uint32_t)waste_vk_scissors.size(),
            .pScissors = waste_vk_scissors.data()
        };
    }

    VkPipelineRasterizationStateCreateInfo RasterizationState_to_vk(
        const RasterizationState& state
    )
    {
        return VkPipelineRasterizationStateCreateInfo{
            .sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
            .pNext = nullptr,
            .flags = 0,
            .depthClampEnable = state.depth_clamp_enable,
            .rasterizerDiscardEnable = state.rasterizer_discard_enable,
            .polygonMode = state.polygon_mode,
            .cullMode = state.cull_mode,
            .frontFace = state.front_face,
            .depthBiasEnable = state.depth_bias_enable,
            .depthBiasConstantFactor = state.depth_bias_constant_factor,
            .depthBiasClamp = state.depth_bias_clamp,
            .depthBiasSlopeFactor = state.depth_bias_slope_factor,
            .lineWidth = state.line_width
        };
    }

    VkPipelineMultisampleStateCreateInfo MultisampleState_to_vk(
        const MultisampleState& state,
        std::vector<VkSampleMask>& waste_sample_mask
    )
    {
        waste_sample_mask = state.sample_mask;
        return VkPipelineMultisampleStateCreateInfo{
            .sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
            .pNext = nullptr,
            .flags = 0,
            .rasterizationSamples = state.rasterization_samples,
            .sampleShadingEnable = state.sample_shading_enable,
            .minSampleShading = state.min_sample_shading,
            .pSampleMask = waste_sample_mask.data(),
            .alphaToCoverageEnable = state.alpha_to_coverage_enable,
            .alphaToOneEnable = state.alpha_to_one_enable
        };
    }

    VkStencilOpState StencilOpState_to_vk(const StencilOpState& state)
    {
        return VkStencilOpState{
            .failOp = state.fail_op,
            .passOp = state.pass_op,
            .depthFailOp = state.depth_fail_op,
            .compareOp = state.compare_op,
            .compareMask = state.compare_mask,
            .writeMask = state.write_mask,
            .reference = state.reference
        };
    }

    VkPipelineDepthStencilStateCreateInfo DepthStencilState_to_vk(
        const DepthStencilState& state
    )
    {
        return VkPipelineDepthStencilStateCreateInfo{
            .sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO,
            .pNext = nullptr,
            .flags = state.flags,
            .depthTestEnable = state.depth_test_enable,
            .depthWriteEnable = state.depth_write_enable,
            .depthCompareOp = state.depth_compare_op,
            .depthBoundsTestEnable = state.depth_bounds_test_enable,
            .stencilTestEnable = state.stencil_test_enable,
            .front = StencilOpState_to_vk(state.front),
            .back = StencilOpState_to_vk(state.back),
            .minDepthBounds = state.min_depth_bounds,
            .maxDepthBounds = state.max_depth_bounds
        };
    }

    VkPipelineColorBlendAttachmentState ColorBlendAttachment_to_vk(
        const ColorBlendAttachment& attachment
    )
    {
        return VkPipelineColorBlendAttachmentState{
            .blendEnable = attachment.blend_enable,
            .srcColorBlendFactor = attachment.src_color_blend_factor,
            .dstColorBlendFactor = attachment.dst_color_blend_factor,
            .colorBlendOp = attachment.color_blend_op,
            .srcAlphaBlendFactor = attachment.src_alpha_blend_factor,
            .dstAlphaBlendFactor = attachment.dst_alpha_blend_factor,
            .alphaBlendOp = attachment.alpha_blend_op,
            .colorWriteMask = attachment.color_write_mask
        };
    }

    VkPipelineColorBlendStateCreateInfo ColorBlendState_to_vk(
        const ColorBlendState& state,

        std::vector<VkPipelineColorBlendAttachmentState>&
        waste_vk_color_blend_attachments
    )
    {
        waste_vk_color_blend_attachments.resize(state.attachments.size());
        for (size_t i = 0; i < state.attachments.size(); i++)
        {
            waste_vk_color_blend_attachments[i] = ColorBlendAttachment_to_vk(
                state.attachments[i]
            );
        }

        VkPipelineColorBlendStateCreateInfo info{
            .sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
            .pNext = nullptr,
            .flags = state.flags,
            .logicOpEnable = state.logic_op_enable,
            .logicOp = state.logic_op,

            .attachmentCount =
            (uint32_t)waste_vk_color_blend_attachments.size(),

            .pAttachments = waste_vk_color_blend_attachments.data()
        };
        std::copy(
            state.blend_constants.data(),
            state.blend_constants.data() + 4,
            info.blendConstants
        );
        return info;
    }

    VkDescriptorSetLayoutBinding DescriptorSetLayoutBinding_to_vk(
        const DescriptorSetLayoutBinding& binding,
        std::vector<SamplerPtr>& waste_immutable_samplers,
        std::vector<VkSampler>& waste_vk_immutable_samplers
    )
    {
        waste_immutable_samplers = binding.immutable_samplers;
        waste_vk_immutable_samplers.resize(binding.immutable_samplers.size());
        for (size_t i = 0; i < binding.immutable_samplers.size(); i++)
        {
            waste_vk_immutable_samplers[i] =
                binding.immutable_samplers[i]->handle();
        }

        return VkDescriptorSetLayoutBinding{
            .binding = binding.binding,
            .descriptorType = binding.descriptor_type,
            .descriptorCount = binding.descriptor_count,
            .stageFlags = binding.stage_flags,

            .pImmutableSamplers =
            waste_vk_immutable_samplers.empty()
            ? nullptr
            : waste_vk_immutable_samplers.data()
        };
    }

    VkPushConstantRange PushConstantRange_to_vk(const PushConstantRange& range)
    {
        return VkPushConstantRange{
            .stageFlags = range.stage_flags,
            .offset = range.offset,
            .size = range.size
        };
    }

    VkAttachmentDescription Attachment_to_vk(
        const Attachment& attachment
    )
    {
        return VkAttachmentDescription{
            .flags = attachment.flags,
            .format = attachment.format,
            .samples = attachment.samples,
            .loadOp = attachment.load_op,
            .storeOp = attachment.store_op,
            .stencilLoadOp = attachment.stencil_load_op,
            .stencilStoreOp = attachment.stencil_store_op,
            .initialLayout = attachment.initial_layout,
            .finalLayout = attachment.final_layout
        };
    }

    VkAttachmentReference AttachmentReference_to_vk(
        const AttachmentReference& ref
    )
    {
        return VkAttachmentReference{
            .attachment = ref.attachment,
            .layout = ref.layout
        };
    }

    VkSubpassDescription Subpass_to_vk(
        const Subpass& subpass,
        std::vector<VkAttachmentReference>& waste_vk_input_attachments,
        std::vector<VkAttachmentReference>& waste_vk_color_attachments,
        std::vector<VkAttachmentReference>& waste_vk_resolve_attachments,
        VkAttachmentReference& waste_vk_depth_stencil_attachment,
        std::vector<uint32_t>& waste_preserve_attachment_indices
    )
    {
        waste_vk_input_attachments.resize(subpass.input_attachments.size());
        for (size_t i = 0; i < subpass.input_attachments.size(); i++)
        {
            waste_vk_input_attachments[i] = AttachmentReference_to_vk(
                subpass.input_attachments[i]
            );
        }

        waste_vk_color_attachments.resize(subpass.color_attachments.size());
        for (size_t i = 0; i < subpass.color_attachments.size(); i++)
        {
            waste_vk_color_attachments[i] = AttachmentReference_to_vk(
                subpass.color_attachments[i]
            );
        }

        waste_vk_resolve_attachments.resize(subpass.resolve_attachments.size());
        for (size_t i = 0; i < subpass.resolve_attachments.size(); i++)
        {
            waste_vk_resolve_attachments[i] = AttachmentReference_to_vk(
                subpass.resolve_attachments[i]
            );
        }

        if (subpass.depth_stencil_attachment.has_value())
        {
            waste_vk_depth_stencil_attachment = AttachmentReference_to_vk(
                subpass.depth_stencil_attachment.value()
            );
        }

        waste_preserve_attachment_indices = subpass.preserve_attachment_indices;

        return VkSubpassDescription{
            .flags = subpass.flags,
            .pipelineBindPoint = subpass.pipeline_bind_point,
            .inputAttachmentCount = (uint32_t)waste_vk_input_attachments.size(),
            .pInputAttachments = waste_vk_input_attachments.data(),
            .colorAttachmentCount = (uint32_t)waste_vk_color_attachments.size(),
            .pColorAttachments = waste_vk_color_attachments.data(),

            .pResolveAttachments =
            waste_vk_resolve_attachments.empty()
            ? nullptr
            : waste_vk_resolve_attachments.data(),

            .pDepthStencilAttachment =
            subpass.depth_stencil_attachment.has_value()
            ? &waste_vk_depth_stencil_attachment
            : nullptr,

            .preserveAttachmentCount =
            (uint32_t)waste_preserve_attachment_indices.size(),

            .pPreserveAttachments = waste_preserve_attachment_indices.data()
        };
    }

    VkSubpassDependency SubpassDependency_to_vk(const SubpassDependency& dep)
    {
        return VkSubpassDependency{
            .srcSubpass = dep.src_subpass,
            .dstSubpass = dep.dst_subpass,
            .srcStageMask = dep.src_stage_mask,
            .dstStageMask = dep.dst_stage_mask,
            .srcAccessMask = dep.src_access_mask,
            .dstAccessMask = dep.dst_access_mask,
            .dependencyFlags = dep.dependency_flags
        };
    }

    MemoryRequirements MemoryRequirements_from_vk(
        const VkMemoryRequirements& req
    )
    {
        return MemoryRequirements{
            .size = req.size,
            .alignment = req.alignment,
            .memory_type_bits = req.memoryTypeBits
        };
    }

    MemoryType MemoryType_from_vk(const VkMemoryType& type)
    {
        return MemoryType{
            .property_flags = type.propertyFlags,
            .heap_index = type.heapIndex
        };
    }

    MemoryHeap MemoryHeap_from_vk(const VkMemoryHeap& heap)
    {
        return MemoryHeap{
            .size = heap.size,
            .flags = heap.flags
        };
    }

    PhysicalDeviceMemoryProperties PhysicalDeviceMemoryProperties_from_vk(
        const VkPhysicalDeviceMemoryProperties& properties
    )
    {
        std::vector<MemoryType> memory_types(properties.memoryTypeCount);
        for (size_t i = 0; i < properties.memoryTypeCount; i++)
        {
            memory_types[i] = MemoryType_from_vk(properties.memoryTypes[i]);
        }

        std::vector<MemoryHeap> memory_heaps(properties.memoryHeapCount);
        for (size_t i = 0; i < properties.memoryHeapCount; i++)
        {
            memory_heaps[i] = MemoryHeap_from_vk(properties.memoryHeaps[i]);
        }

        return PhysicalDeviceMemoryProperties{
            .memory_types = memory_types,
            .memory_heaps = memory_heaps
        };
    }

    VkDescriptorPoolSize DescriptorPoolSize_to_vk(
        const DescriptorPoolSize& pool_size
    )
    {
        return VkDescriptorPoolSize{
            .type = pool_size.type,
            .descriptorCount = pool_size.descriptor_count
        };
    }

    VkDescriptorImageInfo DescriptorImageInfo_to_vk(
        const DescriptorImageInfo& info,
        SamplerPtr& waste_sampler,
        ImageViewPtr& waste_image_view
    )
    {
        waste_sampler = info.sampler;
        waste_image_view = info.image_view;

        return VkDescriptorImageInfo{
            .sampler = waste_sampler->handle(),
            .imageView = waste_image_view->handle(),
            .imageLayout = info.image_layout
        };
    }

    VkDescriptorBufferInfo DescriptorBufferInfo_to_vk(
        const DescriptorBufferInfo& info,
        BufferPtr& waste_buffer
    )
    {
        waste_buffer = info.buffer;
        return VkDescriptorBufferInfo{
            .buffer = waste_buffer->handle(),
            .offset = info.offset,
            .range = info.range
        };
    }

    VkWriteDescriptorSet WriteDescriptorSet_to_vk(
        const WriteDescriptorSet& write,
        DescriptorSetPtr& waste_dst_set,

        std::vector<VkDescriptorImageInfo>& waste_vk_image_infos,
        std::vector<SamplerPtr>& waste_image_infos_sampler,
        std::vector<ImageViewPtr>& waste_image_infos_image_view,

        std::vector<VkDescriptorBufferInfo>& waste_vk_buffer_infos,
        std::vector<BufferPtr>& waste_buffer_infos_buffer,

        std::vector<BufferViewPtr>& waste_texel_buffer_views,
        std::vector<VkBufferView>& waste_vk_texel_buffer_views
    )
    {
        waste_dst_set = write.dst_set;

        waste_vk_image_infos.resize(write.image_infos.size());
        waste_image_infos_sampler.resize(write.image_infos.size());
        waste_image_infos_image_view.resize(write.image_infos.size());
        for (size_t i = 0; i < write.image_infos.size(); i++)
        {
            waste_vk_image_infos[i] = DescriptorImageInfo_to_vk(
                write.image_infos[i],
                waste_image_infos_sampler[i],
                waste_image_infos_image_view[i]
            );
        }

        waste_vk_buffer_infos.resize(write.buffer_infos.size());
        waste_buffer_infos_buffer.resize(write.buffer_infos.size());
        for (size_t i = 0; i < write.buffer_infos.size(); i++)
        {
            waste_vk_buffer_infos[i] = DescriptorBufferInfo_to_vk(
                write.buffer_infos[i],
                waste_buffer_infos_buffer[i]
            );
        }

        waste_texel_buffer_views.resize(write.texel_buffer_views.size());
        waste_vk_texel_buffer_views.resize(write.texel_buffer_views.size());
        for (size_t i = 0; i < write.texel_buffer_views.size(); i++)
        {
            waste_texel_buffer_views[i] = write.texel_buffer_views[i];
            waste_vk_texel_buffer_views[i] =
                write.texel_buffer_views[i]->handle();
        }

        return VkWriteDescriptorSet{
            .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
            .pNext = nullptr,
            .dstSet = waste_dst_set->handle(),
            .dstBinding = write.dst_binding,
            .dstArrayElement = write.dst_array_element,
            .descriptorCount = write.descriptor_count,
            .descriptorType = write.descriptor_type,

            .pImageInfo = waste_vk_image_infos.empty()
            ? nullptr : waste_vk_image_infos.data(),

            .pBufferInfo = waste_vk_buffer_infos.empty()
            ? nullptr : waste_vk_buffer_infos.data(),

            .pTexelBufferView = waste_vk_texel_buffer_views.empty()
            ? nullptr : waste_vk_texel_buffer_views.data()
        };
    }

    VkCopyDescriptorSet CopyDescriptorSet_to_vk(
        const CopyDescriptorSet& copy,
        DescriptorSetPtr& waste_src_set,
        DescriptorSetPtr& waste_dst_set
    )
    {
        waste_src_set = copy.src_set;
        waste_dst_set = copy.dst_set;

        return VkCopyDescriptorSet{
            .sType = VK_STRUCTURE_TYPE_COPY_DESCRIPTOR_SET,
            .pNext = nullptr,
            .srcSet = waste_src_set->handle(),
            .srcBinding = copy.src_binding,
            .srcArrayElement = copy.src_array_element,
            .dstSet = waste_dst_set->handle(),
            .dstBinding = copy.dst_binding,
            .dstArrayElement = copy.dst_array_element,
            .descriptorCount = copy.descriptor_count
        };
    }

    FormatProperties FormatProperties_from_vk(
        const VkFormatProperties& properties
    )
    {
        return FormatProperties{
            .linear_tiling_features = properties.linearTilingFeatures,
            .optimal_tiling_features = properties.optimalTilingFeatures,
            .buffer_features = properties.bufferFeatures
        };
    }

    ImageFormatProperties ImageFormatProperties_from_vk(
        const VkImageFormatProperties& properties
    )
    {
        return ImageFormatProperties{
            .max_extent = Extent3d_from_vk(properties.maxExtent),
            .max_mip_levels = properties.maxMipLevels,
            .max_array_layers = properties.maxArrayLayers,
            .sample_counts = properties.sampleCounts,
            .max_resource_size = properties.maxResourceSize
        };
    }

    Error::Error()
        : _message("no error information provided"),
        _api_result(std::nullopt),
        print_api_result(false)
    {}

    Error::Error(
        std::string message,
        const std::optional<ApiResult>& api_result,
        bool api_result_already_embedded_in_message
    )
        : _message(std::move(message)),
        _api_result(api_result),
        print_api_result(!api_result_already_embedded_in_message)
    {}

    std::string Error::to_string() const
    {
        std::string s = message();
        if (api_result().has_value() && print_api_result)
        {
            if (!message().empty())
            {
                s += ": ";
            }
            s += ApiResult_to_string(api_result().value());
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
            handle(),
            layer_name_cstr,
            &count,
            nullptr
        );

        std::vector<VkExtensionProperties> vk_extensions(count);
        VkResult vk_result = vkEnumerateDeviceExtensionProperties(
            handle(),
            layer_name_cstr,
            &count,
            vk_extensions.data()
        );
        if (vk_result != VK_SUCCESS && vk_result != VK_INCOMPLETE)
        {
            return Error("", (ApiResult)vk_result, false);
        }

        std::vector<ExtensionProperties> extensions;
        extensions.reserve(vk_extensions.size());
        for (const auto& vk_ext : vk_extensions)
        {
            extensions.push_back(ExtensionProperties_from_vk(vk_ext));
        }
        return extensions;
    }

    Result<> PhysicalDevice::update_swapchain_support(
        const SurfacePtr& surface
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
            handle(),
            surface->handle(),
            &vk_capabilities
        );
        if (vk_result != VK_SUCCESS)
        {
            return Error("", (ApiResult)vk_result, false);
        }

        std::vector<SurfaceFormat> surface_formats;
        {
            uint32_t surface_format_count;
            vkGetPhysicalDeviceSurfaceFormatsKHR(
                handle(),
                surface->handle(),
                &surface_format_count,
                nullptr
            );

            std::vector<VkSurfaceFormatKHR> vk_surface_formats(
                surface_format_count
            );
            vk_result = vkGetPhysicalDeviceSurfaceFormatsKHR(
                handle(),
                surface->handle(),
                &surface_format_count,
                vk_surface_formats.data()
            );
            if (vk_result != VK_SUCCESS && vk_result != VK_INCOMPLETE)
            {
                return Error("", (ApiResult)vk_result, false);
            }

            surface_formats.reserve(vk_surface_formats.size());
            for (const auto& vk_surface_format : vk_surface_formats)
            {
                surface_formats.push_back(
                    SurfaceFormat_from_vk(vk_surface_format)
                );
            }
        }

        std::vector<VkPresentModeKHR> present_modes;
        {
            uint32_t present_mode_count;
            vkGetPhysicalDeviceSurfacePresentModesKHR(
                handle(),
                surface->handle(),
                &present_mode_count,
                nullptr
            );

            present_modes.resize(present_mode_count);
            vk_result = vkGetPhysicalDeviceSurfacePresentModesKHR(
                handle(),
                surface->handle(),
                &present_mode_count,
                present_modes.data()
            );
            if (vk_result != VK_SUCCESS && vk_result != VK_INCOMPLETE)
            {
                return Error("", (ApiResult)vk_result, false);
            }
        }

        _swapchain_support = SwapchainSupport{
            .capabilities = SurfaceCapabilities_from_vk(vk_capabilities),
            .surface_formats = surface_formats,
            .present_modes = present_modes
        };

        return Result();
    }

    FormatProperties PhysicalDevice::fetch_format_properties(VkFormat format)
    {
        VkFormatProperties vk_properties;
        vkGetPhysicalDeviceFormatProperties(
            handle(),
            format,
            &vk_properties
        );
        return FormatProperties_from_vk(vk_properties);
    }

    Result<VkFormat> PhysicalDevice::find_supported_image_format(
        const std::vector<VkFormat>& candidates,
        VkImageTiling tiling,
        VkFormatFeatureFlags features
    )
    {
        for (VkFormat format : candidates)
        {
            auto props = fetch_format_properties(format);
            if (tiling == VK_IMAGE_TILING_LINEAR
                && (props.linear_tiling_features & features) == features)
            {
                return format;
            }
            else if (tiling == VK_IMAGE_TILING_OPTIMAL
                && (props.optimal_tiling_features & features) == features)
            {
                return format;
            }
        }
        return Error(
            "none of the provided candidates for an image format are supported "
            "with the provided tiling and required features.",
            std::nullopt,
            false
        );
    }

    Result<ImageFormatProperties> PhysicalDevice::fetch_image_format_properties(
        VkFormat format,
        VkImageType type,
        VkImageTiling tiling,
        VkImageUsageFlags usage,
        VkImageCreateFlags flags
    )
    {
        VkImageFormatProperties vk_properties;
        VkResult vk_result = vkGetPhysicalDeviceImageFormatProperties(
            handle(),
            format,
            type,
            tiling,
            usage,
            flags,
            &vk_properties
        );
        if (vk_result != VK_SUCCESS)
        {
            return Error("", (ApiResult)vk_result, false);
        }
        return ImageFormatProperties_from_vk(vk_properties);
    }

    PhysicalDevice::PhysicalDevice(
        VkPhysicalDevice handle,
        const PhysicalDeviceProperties& properties,
        const PhysicalDeviceFeatures& features,
        const PhysicalDeviceMemoryProperties& memory_properties,
        const std::vector<QueueFamily>& queue_families,
        const QueueFamilyIndices& queue_family_indices
    )
        : _handle(handle),
        _properties(properties),
        _features(features),
        _memory_properties(memory_properties),
        _queue_families(queue_families),
        _queue_family_indices(queue_family_indices)
    {}

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

    Result<ContextPtr> Context::create(
        const ContextConfig& config,
        const AllocatorPtr& allocator
    )
    {
        ContextPtr c = std::make_shared<Context_public_ctor>(
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

            app_info.apiVersion = VulkanApiVersion_encode(
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
            return Error("", (ApiResult)vk_result, false);
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
            return Error("", (ApiResult)vk_result, false);
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
            return Error("", (ApiResult)vk_result, false);
        }

        std::vector<ExtensionProperties> extensions;
        extensions.reserve(vk_extensions.size());
        for (const auto& vk_ext : vk_extensions)
        {
            extensions.push_back(ExtensionProperties_from_vk(vk_ext));
        }
        return extensions;
    }

    void Context::set_allocator(const AllocatorPtr& allocator)
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

    Result<std::vector<PhysicalDevicePtr>> Context::fetch_physical_devices(
        const SurfacePtr& surface
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
            return Error("", (ApiResult)vk_result, false);
        }

        std::vector<PhysicalDevicePtr> physical_devices;
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

            VkPhysicalDeviceMemoryProperties vk_memory_properties;
            vkGetPhysicalDeviceMemoryProperties(
                vk_physical_device,
                &vk_memory_properties
            );
            auto memory_properties = PhysicalDeviceMemoryProperties_from_vk(
                vk_memory_properties
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

                VkBool32 vk_surface_support = VK_FALSE;
                if (surface != nullptr)
                {
                    vk_result = vkGetPhysicalDeviceSurfaceSupportKHR(
                        vk_physical_device,
                        (uint32_t)i,
                        surface->handle(),
                        &vk_surface_support
                    );
                    if (vk_result != VK_SUCCESS)
                    {
                        return Error(
                            "failed to check surface support",
                            (ApiResult)vk_result,
                            false
                        );
                    }
                }

                auto queue_family = QueueFamily_from_vk(
                    vk_queue_family,
                    vk_surface_support
                );

                if ((queue_family.queue_flags & VK_QUEUE_GRAPHICS_BIT)
                    && !queue_family_indices.graphics.has_value())
                {
                    queue_family_indices.graphics = i;
                }
                if (queue_family.surface_support
                    && !queue_family_indices.presentation.has_value())
                {
                    queue_family_indices.presentation = i;
                }
                if ((queue_family.queue_flags & VK_QUEUE_COMPUTE_BIT)
                    && !queue_family_indices.compute.has_value())
                {
                    queue_family_indices.compute = i;
                }
                if ((queue_family.queue_flags & VK_QUEUE_TRANSFER_BIT)
                    && !queue_family_indices.transfer.has_value())
                {
                    queue_family_indices.transfer = i;
                }
                if ((queue_family.queue_flags & VK_QUEUE_SPARSE_BINDING_BIT)
                    && !queue_family_indices.sparse_binding.has_value())
                {
                    queue_family_indices.sparse_binding = i;
                }
                if ((queue_family.queue_flags & VK_QUEUE_PROTECTED_BIT)
                    && !queue_family_indices.protected_.has_value())
                {
                    queue_family_indices.protected_ = i;
                }
                if ((queue_family.queue_flags & VK_QUEUE_VIDEO_DECODE_BIT_KHR)
                    && !queue_family_indices.video_decode.has_value())
                {
                    queue_family_indices.video_decode = i;
                }
                if ((queue_family.queue_flags & VK_QUEUE_OPTICAL_FLOW_BIT_NV)
                    && !queue_family_indices.optical_flow_nv.has_value())
                {
                    queue_family_indices.optical_flow_nv = i;
                }

                bool supports_both_graphics_and_presentation =
                    (queue_family.queue_flags & VK_QUEUE_GRAPHICS_BIT)
                    && queue_family.surface_support;

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
                    memory_properties,
                    queue_families,
                    queue_family_indices
                )
            );

            auto update_swapchain_support_result =
                physical_devices.back()->update_swapchain_support(surface);
            if (!update_swapchain_support_result.ok())
            {
                auto err = update_swapchain_support_result.error();
                return Error(
                    "failed to fetch swapchain support details for a physical "
                    "device: "
                    + err.to_string(),
                    err.api_result(),
                    true
                );
            }
        }

        return physical_devices;
    }

    void Context::destroy()
    {
        vkDestroyInstance(_vk_instance, vk_allocator_ptr());
    }

    Context::Context(
        const ContextConfig& config,
        const AllocatorPtr& allocator
    )
        : _config(config), _allocator(allocator)
    {}

    Result<DebugMessengerPtr> DebugMessenger::create(
        const ContextPtr& context,
        VkDebugUtilsMessageSeverityFlagsEXT message_severity_filter,
        VkDebugUtilsMessageTypeFlagsEXT message_type_filter,
        const DebugCallback& callback
    )
    {
        DebugMessengerPtr messenger =
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
            .messageSeverity = messenger->message_severity_filter(),
            .messageType = messenger->message_type_filter(),
            .pfnUserCallback = vk_debug_callback,
            .pUserData = messenger.get()
        };

        VkResult vk_result = CreateDebugUtilsMessengerEXT(
            messenger->context()->vk_instance(),
            &create_info,
            messenger->context()->vk_allocator_ptr(),
            &messenger->_handle
        );
        if (vk_result != VK_SUCCESS)
        {
            return Error("", (ApiResult)vk_result, false);
        }
        return messenger;
    }

    void DebugMessenger::destroy()
    {
        DestroyDebugUtilsMessengerEXT(
            context()->vk_instance(),
            handle(),
            context()->vk_allocator_ptr()
        );
    }

    DebugMessenger::DebugMessenger(
        const ContextPtr& context,
        VkDebugUtilsMessageSeverityFlagsEXT message_severity_filter,
        VkDebugUtilsMessageTypeFlagsEXT message_type_filter,
        const DebugCallback& callback
    )
        : _context(context),
        _message_severity_filter(message_severity_filter),
        _message_type_filter(message_type_filter),
        _callback(callback)
    {}

    SurfacePtr Surface::create(
        const ContextPtr& context,
        VkSurfaceKHR handle
    )
    {
        return std::make_shared<Surface_public_ctor>(context, handle);
    }

    void Surface::destroy()
    {
        vkDestroySurfaceKHR(
            _context->vk_instance(),
            _handle,
            _context->vk_allocator_ptr()
        );
    }

    Surface::Surface(
        const ContextPtr& context,
        VkSurfaceKHR handle
    )
        : _context(context), _handle(handle)
    {}

    Result<> Queue::submit(
        const std::vector<VkPipelineStageFlags>& wait_stages,
        const std::vector<SemaphorePtr>& wait_semaphores,
        const std::vector<CommandBufferPtr>& command_buffers,
        const std::vector<SemaphorePtr>& signal_semaphores,
        const FencePtr& signal_fence
    )
    {
        std::vector<VkSemaphore> vk_semaphores(
            wait_semaphores.size() + signal_semaphores.size()
        );
        for (size_t i = 0; i < wait_semaphores.size(); i++)
        {
            vk_semaphores[i] = wait_semaphores[i]->handle();
        }
        for (size_t i = 0; i < signal_semaphores.size(); i++)
        {
            vk_semaphores[wait_semaphores.size() + i] =
                signal_semaphores[i]->handle();
        }

        std::vector<VkCommandBuffer> vk_command_buffers(command_buffers.size());
        for (size_t i = 0; i < command_buffers.size(); i++)
        {
            vk_command_buffers[i] = command_buffers[i]->handle();
        }

        VkSubmitInfo submit_info{
            .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
            .pNext = nullptr,
            .waitSemaphoreCount = (uint32_t)wait_semaphores.size(),
            .pWaitSemaphores = vk_semaphores.data(),
            .pWaitDstStageMask = wait_stages.data(),
            .commandBufferCount = (uint32_t)vk_command_buffers.size(),
            .pCommandBuffers = vk_command_buffers.data(),
            .signalSemaphoreCount = (uint32_t)signal_semaphores.size(),

            .pSignalSemaphores =
            signal_semaphores.empty()
            ? nullptr : vk_semaphores.data() + wait_semaphores.size()
        };

        VkResult vk_result = vkQueueSubmit(
            handle(),
            1,
            &submit_info,
            signal_fence == nullptr ? nullptr : signal_fence->handle()
        );
        if (vk_result != VK_SUCCESS)
        {
            return Error("", (ApiResult)vk_result, false);
        }
        return Result();
    }

    Result<> Queue::present(
        const std::vector<SemaphorePtr>& wait_semaphores,
        const SwapchainPtr& swapchain,
        uint32_t image_index,
        ApiResult* out_api_result
    )
    {
        std::vector<VkSemaphore> vk_semaphores(wait_semaphores.size());
        for (size_t i = 0; i < wait_semaphores.size(); i++)
        {
            vk_semaphores[i] = wait_semaphores[i]->handle();
        }

        VkSwapchainKHR vk_swapchain = swapchain->handle();

        VkPresentInfoKHR present_info{
            .sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
            .pNext = nullptr,
            .waitSemaphoreCount = (uint32_t)wait_semaphores.size(),
            .pWaitSemaphores = vk_semaphores.data(),
            .swapchainCount = 1,
            .pSwapchains = &vk_swapchain,
            .pImageIndices = &image_index,
            .pResults = nullptr
        };

        VkResult vk_result = vkQueuePresentKHR(
            handle(),
            &present_info
        );
        if (out_api_result != nullptr)
        {
            *out_api_result = (ApiResult)vk_result;
        }
        if (vk_result != VK_SUCCESS && vk_result != VK_SUBOPTIMAL_KHR)
        {
            return Error("", (ApiResult)vk_result, false);
        }
        return Result();
    }

    Result<> Queue::wait_idle()
    {
        VkResult vk_result = vkQueueWaitIdle(handle());
        if (vk_result != VK_SUCCESS)
        {
            return Error("", (ApiResult)vk_result, false);
        }
        return Result();
    }

    Queue::Queue(
        const DeviceWPtr& device,
        uint32_t queue_family_index,
        uint32_t queue_index,
        VkQueue handle
    )
        : _device(device),
        _queue_family_index(queue_family_index),
        _queue_index(queue_index),
        _handle(handle)
    {}

    Result<DevicePtr>Device::create(
        const ContextPtr& context,
        const PhysicalDevicePtr& physical_device,
        const DeviceConfig& config
    )
    {
        DevicePtr device = std::make_shared<Device_public_ctor>(
            context,
            physical_device,
            config
        );

        std::vector<VkDeviceQueueCreateInfo> vk_queue_requests;
        std::vector<std::vector<float>> wastes_priorities(
            device->config().queue_requests.size()
        );
        for (size_t i = 0; i < device->config().queue_requests.size(); i++)
        {
            const auto& queue_request = device->config().queue_requests[i];

            if (queue_request.priorities.size()
                != queue_request.num_queues_to_create)
            {
                return Error(
                    "there should be the same number of queue priorities as "
                    "the number of queues to create",
                    std::nullopt,
                    false
                );
            }

            vk_queue_requests.push_back(
                QueueRequest_to_vk(
                    queue_request,
                    wastes_priorities[i]
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
            device->physical_device()->handle(),
            &create_info,
            device->context()->vk_allocator_ptr(),
            &device->_handle
        );
        if (vk_result != VK_SUCCESS)
        {
            return Error("", (ApiResult)vk_result, false);
        }
        return device;
    }

    QueuePtr Device::retrieve_queue(
        const DevicePtr& device,
        uint32_t queue_family_index,
        uint32_t queue_index
    )
    {
        VkQueue vk_queue;
        vkGetDeviceQueue(
            device->handle(),
            queue_family_index,
            queue_index,
            &vk_queue
        );
        return std::make_shared<Queue_public_ctor>(
            (DeviceWPtr)device,
            queue_family_index,
            queue_index,
            vk_queue
        );
    }

    Result<> Device::wait_idle()
    {
        VkResult vk_result = vkDeviceWaitIdle(_handle);
        if (vk_result != VK_SUCCESS)
        {
            return Error("", (ApiResult)vk_result, false);
        }
        return Result();
    }

    void Device::destroy()
    {
        vkDestroyDevice(handle(), context()->vk_allocator_ptr());
    }

    Device::Device(
        const ContextPtr& context,
        const PhysicalDevicePtr& physical_device,
        const DeviceConfig& config
    )
        : _context(context),
        _physical_device(physical_device),
        _config(config)
    {}

    Result<ImagePtr> Image::create(
        const DevicePtr& device,
        const ImageConfig& config
    )
    {
        auto img_format_props_result =
            device->physical_device()->fetch_image_format_properties(
                config.format,
                config.image_type,
                config.tiling,
                config.usage,
                config.flags
            );
        if (!img_format_props_result.ok()
            && img_format_props_result.error().api_result().has_value()
            && img_format_props_result.error().api_result().value()
            == ApiResult::ErrorFormatNotSupported)
        {
            return Error(
                "image format not supported with the provided parameters",
                img_format_props_result.error().api_result(),
                true
            );
        }
        else if (!img_format_props_result.ok())
        {
            return Error(
                "failed to fetch image format properties: "
                + img_format_props_result.error().to_string(),

                img_format_props_result.error().api_result(),
                true
            );
        }

        ImagePtr img = std::make_shared<Image_public_ctor>(
            device,
            config
        );

        VkImageCreateInfo create_info{
            .sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
            .pNext = nullptr,
            .flags = img->config().flags,
            .imageType = img->config().image_type,
            .format = img->config().format,
            .extent = Extent3d_to_vk(img->config().extent),
            .mipLevels = img->config().mip_levels,
            .arrayLayers = img->config().array_layers,
            .samples = img->config().samples,
            .tiling = img->config().tiling,
            .usage = img->config().usage,
            .sharingMode = img->config().sharing_mode,

            .queueFamilyIndexCount =
            (uint32_t)img->config().queue_family_indices.size(),

            .pQueueFamilyIndices = img->config().queue_family_indices.data(),
            .initialLayout = img->config().initial_layout
        };

        VkResult vk_result = vkCreateImage(
            img->device()->handle(),
            &create_info,
            img->device()->context()->vk_allocator_ptr(),
            &img->_handle
        );
        if (vk_result != VK_SUCCESS)
        {
            return Error("", (ApiResult)vk_result, false);
        }

        VkMemoryRequirements vk_mem_requirements;
        vkGetImageMemoryRequirements(
            img->device()->handle(),
            img->handle(),
            &vk_mem_requirements
        );
        img->_memory_requirements = MemoryRequirements_from_vk(
            vk_mem_requirements
        );

        return img;
    }

    Result<> Image::bind_memory(
        const DeviceMemoryPtr& memory,
        VkDeviceSize memory_offset
    )
    {
        VkResult vk_result = vkBindImageMemory(
            device()->handle(),
            handle(),
            memory->handle(),
            memory_offset
        );
        if (vk_result != VK_SUCCESS)
        {
            return Error("", (ApiResult)vk_result, false);
        }
        return Result();
    }

    void Image::destroy()
    {
        if (created_externally())
        {
            return;
        }
        vkDestroyImage(
            device()->handle(),
            handle(),
            device()->context()->vk_allocator_ptr()
        );
    }

    Image::Image(const DevicePtr& device, const ImageConfig& config)
        : _created_externally(false),
        _device(device),
        _config(config),
        _memory_requirements({}),
        _handle(nullptr)
    {}

    Image::Image(VkImage handle_created_externally)
        : _created_externally(true),
        _device(nullptr),
        _config({}),
        _memory_requirements({}),
        _handle(handle_created_externally)
    {}

    Result<SwapchainPtr> Swapchain::create(
        const DevicePtr& device,
        const SurfacePtr& surface,
        const SwapchainConfig& config,
        const SwapchainPtr& old_swapchain
    )
    {
        SwapchainPtr sc = std::make_shared<Swapchain_public_ctor>(
            device,
            surface,
            config,
            old_swapchain
        );

        VkSwapchainKHR vk_old_swapchain = nullptr;
        if (sc->old_swapchain() != nullptr)
        {
            vk_old_swapchain = sc->old_swapchain()->handle();
        }

        VkSwapchainCreateInfoKHR create_info{
            .sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
            .pNext = nullptr,
            .flags = sc->config().flags,
            .surface = sc->surface()->handle(),
            .minImageCount = sc->config().min_image_count,
            .imageFormat = sc->config().image_format,
            .imageColorSpace = sc->config().image_color_space,
            .imageExtent = Extent2d_to_vk(sc->config().image_extent),
            .imageArrayLayers = sc->config().image_array_layers,
            .imageUsage = sc->config().image_usage,
            .imageSharingMode = sc->config().image_sharing_mode,

            .queueFamilyIndexCount =
            (uint32_t)sc->config().queue_family_indices.size(),

            .pQueueFamilyIndices = sc->config().queue_family_indices.data(),
            .preTransform = sc->config().pre_transform,
            .compositeAlpha = sc->config().composite_alpha,
            .presentMode = sc->config().present_mode,
            .clipped = sc->config().clipped,
            .oldSwapchain = vk_old_swapchain
        };

        VkResult vk_result = vkCreateSwapchainKHR(
            sc->device()->handle(),
            &create_info,
            sc->device()->context()->vk_allocator_ptr(),
            &sc->_handle
        );
        if (vk_result != VK_SUCCESS)
        {
            return Error("", (ApiResult)vk_result, false);
        }

        uint32_t actual_image_count;
        vkGetSwapchainImagesKHR(
            sc->device()->handle(),
            sc->handle(),
            &actual_image_count,
            nullptr
        );

        std::vector<VkImage> vk_images(actual_image_count);
        vk_result = vkGetSwapchainImagesKHR(
            sc->device()->handle(),
            sc->handle(),
            &actual_image_count,
            vk_images.data()
        );
        if (vk_result != VK_SUCCESS && vk_result != VK_INCOMPLETE)
        {
            return Error(
                "failed to retrieve images",
                (ApiResult)vk_result,
                false
            );
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

    Result<uint32_t> Swapchain::acquire_next_image(
        const SemaphorePtr& semaphore,
        const FencePtr& fence,
        uint64_t timeout,
        ApiResult* out_api_result
    )
    {
        uint32_t image_index;
        VkResult vk_result = vkAcquireNextImageKHR(
            device()->handle(),
            handle(),
            timeout,
            semaphore == nullptr ? nullptr : semaphore->handle(),
            fence == nullptr ? nullptr : fence->handle(),
            &image_index
        );
        if (out_api_result != nullptr)
        {
            *out_api_result = (ApiResult)vk_result;
        }
        if (vk_result == VK_SUCCESS
            || vk_result == VK_TIMEOUT
            || vk_result == VK_SUBOPTIMAL_KHR)
        {
            return image_index;
        }
        return Error("", (ApiResult)vk_result, false);
    }

    void Swapchain::destroy()
    {
        vkDestroySwapchainKHR(
            device()->handle(),
            handle(),
            device()->context()->vk_allocator_ptr()
        );
    }

    Swapchain::Swapchain(
        const DevicePtr& device,
        const SurfacePtr& surface,
        const SwapchainConfig& config,
        const SwapchainPtr& old_swapchain
    )
        : _device(device),
        _surface(surface),
        _config(config),
        _old_swapchain(old_swapchain)
    {}

    Result<ImageViewPtr> ImageView::create(
        const DevicePtr& device,
        const ImagePtr& image,
        const ImageViewConfig& config
    )
    {
        ImageViewPtr view = std::make_shared<ImageView_public_ctor>(
            device,
            image,
            config
        );

        VkImageViewCreateInfo create_info{
            .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
            .pNext = nullptr,
            .flags = view->config().flags,
            .image = view->image()->handle(),
            .viewType = view->config().view_type,
            .format = view->config().format,
            .components = ComponentMapping_to_vk(view->config().components),
            .subresourceRange = ImageSubresourceRange_to_vk(
                view->config().subresource_range
            )
        };

        VkResult vk_result = vkCreateImageView(
            view->device()->handle(),
            &create_info,
            view->device()->context()->vk_allocator_ptr(),
            &view->_handle
        );
        if (vk_result != VK_SUCCESS)
        {
            return Error("", (ApiResult)vk_result, false);
        }
        return view;
    }

    void ImageView::destroy()
    {
        vkDestroyImageView(
            device()->handle(),
            handle(),
            device()->context()->vk_allocator_ptr()
        );
    }

    ImageView::ImageView(
        const DevicePtr& device,
        const ImagePtr& image,
        const ImageViewConfig& config
    )
        : _device(device),
        _image(image),
        _config(config)
    {}

    Result<ShaderModulePtr> ShaderModule::create(
        const DevicePtr& device,
        const std::vector<uint8_t>& code
    )
    {
        ShaderModulePtr module = std::make_shared<ShaderModule_public_ctor>(
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
            module->device()->handle(),
            &create_info,
            module->device()->context()->vk_allocator_ptr(),
            &module->_handle
        );
        if (vk_result != VK_SUCCESS)
        {
            return Error("", (ApiResult)vk_result, false);
        }
        return module;
    }

    void ShaderModule::destroy()
    {
        vkDestroyShaderModule(
            device()->handle(),
            handle(),
            device()->context()->vk_allocator_ptr()
        );
    }

    ShaderModule::ShaderModule(const DevicePtr& device)
        : _device(device)
    {}

    Result<SamplerPtr> Sampler::create(
        const DevicePtr& device,
        const SamplerConfig& config
    )
    {
        SamplerPtr sampler = std::make_shared<Sampler_public_ctor>(
            device,
            config
        );

        VkSamplerCreateInfo create_info{
            .sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
            .pNext = nullptr,
            .flags = sampler->config().flags,
            .magFilter = sampler->config().mag_filter,
            .minFilter = sampler->config().min_filter,
            .mipmapMode = sampler->config().mipmap_mode,
            .addressModeU = sampler->config().address_mode_u,
            .addressModeV = sampler->config().address_mode_v,
            .addressModeW = sampler->config().address_mode_w,
            .mipLodBias = sampler->config().mip_lod_bias,
            .anisotropyEnable = sampler->config().anisotropy_enable,
            .maxAnisotropy = sampler->config().max_anisotropy,
            .compareEnable = sampler->config().compare_enable,
            .compareOp = sampler->config().compare_op,
            .minLod = sampler->config().min_lod,
            .maxLod = sampler->config().max_lod,
            .borderColor = sampler->config().border_color,

            .unnormalizedCoordinates =
            sampler->config().unnormalized_coordinates
        };

        VkResult vk_result = vkCreateSampler(
            sampler->device()->handle(),
            &create_info,
            sampler->device()->context()->vk_allocator_ptr(),
            &sampler->_handle
        );
        if (vk_result != VK_SUCCESS)
        {
            return Error("", (ApiResult)vk_result, false);
        }
        return sampler;
    }

    void Sampler::destroy()
    {
        vkDestroySampler(
            device()->handle(),
            handle(),
            device()->context()->vk_allocator_ptr()
        );
    }

    Sampler::Sampler(
        const DevicePtr& device,
        const SamplerConfig& config
    )
        : _device(device),
        _config(config)
    {}

    Result<DescriptorSetLayoutPtr> DescriptorSetLayout::create(
        const DevicePtr& device,
        const DescriptorSetLayoutConfig& config
    )
    {
        DescriptorSetLayoutPtr layout =
            std::make_shared<DescriptorSetLayout_public_ctor>(
                device,
                config
            );

        std::vector<VkDescriptorSetLayoutBinding> vk_bindings(
            layout->config().bindings.size()
        );
        std::vector<std::vector<SamplerPtr>>
            wastes_immutable_samplers(
                layout->config().bindings.size()
            );
        std::vector<std::vector<VkSampler>> wastes_vk_immutable_samplers(
            layout->config().bindings.size()
        );
        for (size_t i = 0; i < layout->config().bindings.size(); i++)
        {
            vk_bindings[i] = DescriptorSetLayoutBinding_to_vk(
                layout->config().bindings[i],
                wastes_immutable_samplers[i],
                wastes_vk_immutable_samplers[i]
            );
        }

        VkDescriptorSetLayoutCreateInfo create_info{
            .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
            .pNext = nullptr,
            .flags = layout->config().flags,
            .bindingCount = (uint32_t)vk_bindings.size(),
            .pBindings = vk_bindings.data()
        };

        VkResult vk_result = vkCreateDescriptorSetLayout(
            layout->device()->handle(),
            &create_info,
            layout->device()->context()->vk_allocator_ptr(),
            &layout->_handle
        );
        if (vk_result != VK_SUCCESS)
        {
            return Error("", (ApiResult)vk_result, false);
        }
        return layout;
    }

    void DescriptorSetLayout::destroy()
    {
        vkDestroyDescriptorSetLayout(
            device()->handle(),
            handle(),
            device()->context()->vk_allocator_ptr()
        );
    }

    DescriptorSetLayout::DescriptorSetLayout(
        const DevicePtr& device,
        const DescriptorSetLayoutConfig& config
    )
        : _device(device),
        _config(config)
    {}

    Result<PipelineLayoutPtr> PipelineLayout::create(
        const DevicePtr& device,
        const PipelineLayoutConfig& config
    )
    {
        PipelineLayoutPtr layout =
            std::make_shared<PipelineLayout_public_ctor>(
                device,
                config
            );

        std::vector<VkDescriptorSetLayout> vk_set_layouts(
            layout->config().set_layouts.size()
        );
        for (size_t i = 0; i < layout->config().set_layouts.size(); i++)
        {
            vk_set_layouts[i] = layout->config().set_layouts[i]->handle();
        }

        std::vector<VkPushConstantRange> vk_push_constant_ranges(
            layout->config().push_constant_ranges.size()
        );
        for (size_t i = 0;
            i < layout->config().push_constant_ranges.size();
            i++)
        {
            vk_push_constant_ranges[i] = PushConstantRange_to_vk(
                layout->config().push_constant_ranges[i]
            );
        }

        VkPipelineLayoutCreateInfo create_info{
            .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
            .pNext = nullptr,
            .flags = layout->config().flags,
            .setLayoutCount = (uint32_t)vk_set_layouts.size(),
            .pSetLayouts = vk_set_layouts.data(),
            .pushConstantRangeCount = (uint32_t)vk_push_constant_ranges.size(),
            .pPushConstantRanges = vk_push_constant_ranges.data()
        };

        VkResult vk_result = vkCreatePipelineLayout(
            layout->device()->handle(),
            &create_info,
            layout->device()->context()->vk_allocator_ptr(),
            &layout->_handle
        );
        if (vk_result != VK_SUCCESS)
        {
            return Error("", (ApiResult)vk_result, false);
        }
        return layout;
    }

    void PipelineLayout::destroy()
    {
        vkDestroyPipelineLayout(
            device()->handle(),
            handle(),
            device()->context()->vk_allocator_ptr()
        );
    }

    PipelineLayout::PipelineLayout(
        const DevicePtr& device,
        const PipelineLayoutConfig& config
    )
        : _device(device), _config(config)
    {}

    Result<RenderPassPtr> RenderPass::create(
        const DevicePtr& device,
        const RenderPassConfig& config
    )
    {
        RenderPassPtr pass = std::make_shared<RenderPass_public_ctor>(
            device,
            config
        );

        std::vector<VkAttachmentDescription> vk_attachments(
            pass->config().attachments.size()
        );
        for (size_t i = 0; i < pass->config().attachments.size(); i++)
        {
            vk_attachments[i] = Attachment_to_vk(pass->config().attachments[i]);
        }

        std::vector<VkSubpassDescription> vk_subpasses(
            pass->config().subpasses.size()
        );
        std::vector<std::vector<VkAttachmentReference>>
            wastes_vk_input_attachments(pass->config().subpasses.size());
        std::vector<std::vector<VkAttachmentReference>>
            wastes_vk_color_attachments(pass->config().subpasses.size());
        std::vector<std::vector<VkAttachmentReference>>
            wastes_vk_resolve_attachments(pass->config().subpasses.size());
        std::vector<VkAttachmentReference> wastes_vk_depth_stencil_attachment(
            pass->config().subpasses.size()
        );
        std::vector<std::vector<uint32_t>> wastes_preserve_attachment_indices(
            pass->config().subpasses.size()
        );
        for (size_t i = 0; i < pass->config().subpasses.size(); i++)
        {
            vk_subpasses[i] = Subpass_to_vk(
                pass->config().subpasses[i],
                wastes_vk_input_attachments[i],
                wastes_vk_color_attachments[i],
                wastes_vk_resolve_attachments[i],
                wastes_vk_depth_stencil_attachment[i],
                wastes_preserve_attachment_indices[i]
            );
        }

        std::vector<VkSubpassDependency> vk_dependencies(
            pass->config().dependencies.size()
        );
        for (size_t i = 0; i < pass->config().dependencies.size(); i++)
        {
            vk_dependencies[i] = SubpassDependency_to_vk(
                pass->config().dependencies[i]
            );
        }

        VkRenderPassCreateInfo create_info{
            .sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
            .pNext = nullptr,
            .flags = pass->config().flags,
            .attachmentCount = (uint32_t)vk_attachments.size(),
            .pAttachments = vk_attachments.data(),
            .subpassCount = (uint32_t)vk_subpasses.size(),
            .pSubpasses = vk_subpasses.data(),
            .dependencyCount = (uint32_t)vk_dependencies.size(),
            .pDependencies = vk_dependencies.data()
        };

        VkResult vk_result = vkCreateRenderPass(
            pass->device()->handle(),
            &create_info,
            pass->device()->context()->vk_allocator_ptr(),
            &pass->_handle
        );
        if (vk_result != VK_SUCCESS)
        {
            return Error("", (ApiResult)vk_result, false);
        }
        return pass;
    }

    void RenderPass::destroy()
    {
        vkDestroyRenderPass(
            device()->handle(),
            handle(),
            device()->context()->vk_allocator_ptr()
        );
    }

    RenderPass::RenderPass(
        const DevicePtr& device,
        const RenderPassConfig& config
    )
        : _device(device), _config(config)
    {}

    Result<GraphicsPipelinePtr> GraphicsPipeline::create(
        const DevicePtr& device,
        const GraphicsPipelineConfig& config
    )
    {
        GraphicsPipelinePtr pipe =
            std::make_shared<GraphicsPipeline_public_ctor>(
                device,
                config
            );

        std::vector<VkPipelineShaderStageCreateInfo> vk_stages(
            pipe->config().stages.size()
        );
        std::vector<ShaderModulePtr> wastes_module(
            pipe->config().stages.size()
        );
        std::vector<VkSpecializationInfo> wastes_vk_specialization_info(
            pipe->config().stages.size()
        );
        std::vector<std::vector<VkSpecializationMapEntry>>
            wastes_vk_map_entries(pipe->config().stages.size());
        std::vector<std::vector<uint8_t>> wastes_data(
            pipe->config().stages.size()
        );
        for (size_t i = 0; i < pipe->config().stages.size(); i++)
        {
            vk_stages[i] = ShaderStage_to_vk(
                pipe->config().stages[i],
                wastes_module[i],
                wastes_vk_specialization_info[i],
                wastes_vk_map_entries[i],
                wastes_data[i]
            );
        }

        VkPipelineVertexInputStateCreateInfo vk_vertex_input_state{};
        std::vector<VkVertexInputBindingDescription>
            waste_vk_binding_descriptions;
        std::vector<VkVertexInputAttributeDescription>
            waste_vk_attribute_descriptions;
        if (pipe->config().vertex_input_state.has_value())
        {
            vk_vertex_input_state = VertexInputState_to_vk(
                pipe->config().vertex_input_state.value(),
                waste_vk_binding_descriptions,
                waste_vk_attribute_descriptions
            );
        }

        VkPipelineInputAssemblyStateCreateInfo vk_input_assembly_state{};
        if (pipe->config().input_assembly_state.has_value())
        {
            vk_input_assembly_state = InputAssemblyState_to_vk(
                pipe->config().input_assembly_state.value()
            );
        }

        VkPipelineTessellationStateCreateInfo vk_tessellation_state{};
        if (pipe->config().tessellation_state.has_value())
        {
            vk_tessellation_state = TessellationState_to_vk(
                pipe->config().tessellation_state.value()
            );
        }

        VkPipelineViewportStateCreateInfo vk_viewport_state{};
        std::vector<VkViewport> waste_vk_viewports;
        std::vector<VkRect2D> waste_vk_scissors;
        if (pipe->config().viewport_state.has_value())
        {
            vk_viewport_state = ViewportState_to_vk(
                pipe->config().viewport_state.value(),
                waste_vk_viewports,
                waste_vk_scissors
            );
        }

        VkPipelineRasterizationStateCreateInfo vk_rasterization_state{};
        if (pipe->config().rasterization_state.has_value())
        {
            vk_rasterization_state = RasterizationState_to_vk(
                pipe->config().rasterization_state.value()
            );
        }

        VkPipelineMultisampleStateCreateInfo vk_multisample_state{};
        std::vector<VkSampleMask> waste_sample_mask;
        if (pipe->config().multisample_state.has_value())
        {
            vk_multisample_state = MultisampleState_to_vk(
                pipe->config().multisample_state.value(),
                waste_sample_mask
            );
        }

        VkPipelineDepthStencilStateCreateInfo vk_depth_stencil_state{};
        if (pipe->config().depth_stencil_state.has_value())
        {
            vk_depth_stencil_state = DepthStencilState_to_vk(
                pipe->config().depth_stencil_state.value()
            );
        }

        VkPipelineColorBlendStateCreateInfo vk_color_blend_state{};
        std::vector<VkPipelineColorBlendAttachmentState>
            waste_vk_color_blend_attachments;
        if (pipe->config().color_blend_state.has_value())
        {
            vk_color_blend_state = ColorBlendState_to_vk(
                pipe->config().color_blend_state.value(),
                waste_vk_color_blend_attachments
            );
        }

        std::vector<VkDynamicState> waste_dynamic_states;
        VkPipelineDynamicStateCreateInfo vk_dynamic_states =
            DynamicStates_to_vk(
                pipe->config().dynamic_states,
                waste_dynamic_states
            );

        VkGraphicsPipelineCreateInfo create_info{
            .sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
            .pNext = nullptr,
            .flags = pipe->config().flags,
            .stageCount = (uint32_t)vk_stages.size(),
            .pStages = vk_stages.data(),

            .pVertexInputState =
            pipe->config().vertex_input_state.has_value()
            ? &vk_vertex_input_state : nullptr,

            .pInputAssemblyState =
            pipe->config().input_assembly_state.has_value()
            ? &vk_input_assembly_state : nullptr,

            .pTessellationState =
            pipe->config().tessellation_state.has_value()
            ? &vk_tessellation_state : nullptr,

            .pViewportState =
            pipe->config().viewport_state.has_value()
            ? &vk_viewport_state : nullptr,

            .pRasterizationState =
            pipe->config().rasterization_state.has_value()
            ? &vk_rasterization_state : nullptr,

            .pMultisampleState =
            pipe->config().multisample_state.has_value()
            ? &vk_multisample_state : nullptr,

            .pDepthStencilState =
            pipe->config().depth_stencil_state.has_value()
            ? &vk_depth_stencil_state : nullptr,

            .pColorBlendState =
            pipe->config().color_blend_state.has_value()
            ? &vk_color_blend_state : nullptr,

            .pDynamicState =
            pipe->config().dynamic_states.empty()
            ? nullptr : &vk_dynamic_states,

            .layout = pipe->config().layout->handle(),
            .renderPass = pipe->config().render_pass->handle(),
            .subpass = pipe->config().subpass_index,

            .basePipelineHandle =
            pipe->config().base_pipeline == nullptr
            ? nullptr : pipe->config().base_pipeline->handle(),

            .basePipelineIndex = -1
        };

        VkResult vk_result = vkCreateGraphicsPipelines(
            pipe->device()->handle(),
            nullptr,
            1,
            &create_info,
            pipe->device()->context()->vk_allocator_ptr(),
            &pipe->_handle
        );
        if (vk_result != VK_SUCCESS)
        {
            return Error("", (ApiResult)vk_result, false);
        }
        return pipe;
    }

    void GraphicsPipeline::destroy()
    {
        vkDestroyPipeline(
            device()->handle(),
            handle(),
            device()->context()->vk_allocator_ptr()
        );
    }

    GraphicsPipeline::GraphicsPipeline(
        const DevicePtr& device,
        const GraphicsPipelineConfig& config
    )
        : _device(device), _config(config)
    {}

    Result<FramebufferPtr> Framebuffer::create(
        const DevicePtr& device,
        const FramebufferConfig& config
    )
    {
        FramebufferPtr buf = std::make_shared<Framebuffer_public_ctor>(
            device,
            config
        );

        std::vector<VkImageView> vk_attachments(
            buf->config().attachments.size()
        );
        for (size_t i = 0; i < buf->config().attachments.size(); i++)
        {
            vk_attachments[i] = buf->config().attachments[i]->handle();
        }

        VkFramebufferCreateInfo create_info{
            .sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
            .pNext = nullptr,
            .flags = buf->config().flags,
            .renderPass = buf->config().render_pass->handle(),
            .attachmentCount = (uint32_t)vk_attachments.size(),
            .pAttachments = vk_attachments.data(),
            .width = buf->config().width,
            .height = buf->config().height,
            .layers = buf->config().layers
        };

        VkResult vk_result = vkCreateFramebuffer(
            buf->device()->handle(),
            &create_info,
            buf->device()->context()->vk_allocator_ptr(),
            &buf->_handle
        );
        if (vk_result != VK_SUCCESS)
        {
            return Error("", (ApiResult)vk_result, false);
        }
        return buf;
    }

    void Framebuffer::destroy()
    {
        vkDestroyFramebuffer(
            device()->handle(),
            handle(),
            device()->context()->vk_allocator_ptr()
        );
    }

    Framebuffer::Framebuffer(
        const DevicePtr& device,
        const FramebufferConfig& config
    )
        : _device(device), _config(config)
    {}

    Result<> CommandBuffer::reset(VkCommandBufferResetFlags flags)
    {
        VkResult vk_result = vkResetCommandBuffer(_handle, flags);
        if (vk_result != VK_SUCCESS)
        {
            return Error("", (ApiResult)vk_result, false);
        }
        return Result();
    }

    Result<> CommandBuffer::begin(
        VkCommandBufferUsageFlags flags,
        std::optional<CommandBufferInheritance> inheritance
    )
    {
        VkCommandBufferInheritanceInfo vk_inheritance;
        if (inheritance.has_value())
        {
            vk_inheritance = VkCommandBufferInheritanceInfo{
                .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_INHERITANCE_INFO,
                .pNext = nullptr,
                .renderPass = inheritance.value().render_pass->handle(),
                .subpass = inheritance.value().subpass_index,
                .framebuffer = inheritance.value().framebuffer->handle(),

                .occlusionQueryEnable =
                inheritance.value().occlusion_query_enable,

                .queryFlags = inheritance.value().query_flags,
                .pipelineStatistics = inheritance.value().pipeline_statistics
            };
        }

        VkCommandBufferBeginInfo begin_info{};
        begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        begin_info.pNext = nullptr;
        begin_info.flags = flags;
        begin_info.pInheritanceInfo =
            inheritance.has_value() ? &vk_inheritance : nullptr;

        VkResult vk_result = vkBeginCommandBuffer(handle(), &begin_info);
        if (vk_result != VK_SUCCESS)
        {
            return Error("", (ApiResult)vk_result, false);
        }
        return Result();
    }

    Result<> CommandBuffer::end()
    {
        VkResult vk_result = vkEndCommandBuffer(handle());
        if (vk_result != VK_SUCCESS)
        {
            return Error("", (ApiResult)vk_result, false);
        }
        return Result();
    }

    void CommandBuffer::destroy()
    {
        if (pool().expired())
        {
            return;
        }
        auto pool_locked = pool().lock();
        vkFreeCommandBuffers(
            pool_locked->device()->handle(),
            pool_locked->handle(),
            1,
            &_handle
        );
    }

    CommandBuffer::CommandBuffer(
        const CommandPoolWPtr& pool,
        VkCommandBuffer handle
    )
        : _pool(pool),
        _handle(handle)
    {}

    Result<CommandPoolPtr> CommandPool::create(
        const DevicePtr& device,
        const CommandPoolConfig& config
    )
    {
        CommandPoolPtr pool = std::make_shared<CommandPool_public_ctor>(
            device,
            config
        );

        VkCommandPoolCreateInfo create_info{
            .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
            .pNext = nullptr,
            .flags = pool->config().flags,
            .queueFamilyIndex = pool->config().queue_family_index
        };

        VkResult vk_result = vkCreateCommandPool(
            pool->device()->handle(),
            &create_info,
            pool->device()->context()->vk_allocator_ptr(),
            &pool->_handle
        );
        if (vk_result != VK_SUCCESS)
        {
            return Error("", (ApiResult)vk_result, false);
        }
        return pool;
    }

    Result<CommandBufferPtr> CommandPool::allocate_buffer(
        const CommandPoolPtr& pool,
        VkCommandBufferLevel level
    )
    {
        VkCommandBufferAllocateInfo alloc_info{
            .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
            .pNext = nullptr,
            .commandPool = pool->handle(),
            .level = level,
            .commandBufferCount = 1
        };

        VkCommandBuffer vk_command_buffer;
        VkResult vk_result = vkAllocateCommandBuffers(
            pool->device()->handle(),
            &alloc_info,
            &vk_command_buffer
        );
        if (vk_result != VK_SUCCESS)
        {
            return Error("", (ApiResult)vk_result, false);
        }
        return (CommandBufferPtr)std::make_shared<CommandBuffer_public_ctor>(
            (CommandPoolWPtr)pool,
            vk_command_buffer
        );
    }

    Result<std::vector<CommandBufferPtr>> CommandPool::allocate_buffers(
        const CommandPoolPtr& pool,
        VkCommandBufferLevel level,
        uint32_t count
    )
    {
        VkCommandBufferAllocateInfo alloc_info{
            .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
            .pNext = nullptr,
            .commandPool = pool->handle(),
            .level = level,
            .commandBufferCount = count
        };

        std::vector<VkCommandBuffer> vk_command_buffers(count);
        VkResult vk_result = vkAllocateCommandBuffers(
            pool->device()->handle(),
            &alloc_info,
            vk_command_buffers.data()
        );
        if (vk_result != VK_SUCCESS)
        {
            return Error("", (ApiResult)vk_result, false);
        }

        std::vector<CommandBufferPtr> command_buffers(count);
        for (size_t i = 0; i < count; i++)
        {
            command_buffers[i] = std::make_shared<CommandBuffer_public_ctor>(
                (CommandPoolWPtr)pool,
                vk_command_buffers[i]
            );
        }
        return command_buffers;
    }

    void CommandPool::destroy()
    {
        vkDestroyCommandPool(
            device()->handle(),
            handle(),
            device()->context()->vk_allocator_ptr()
        );
    }

    CommandPool::CommandPool(
        const DevicePtr& device,
        const CommandPoolConfig& config
    )
        : _device(device), _config(config)
    {}

    Result<SemaphorePtr> Semaphore::create(const DevicePtr& device)
    {
        SemaphorePtr sema = std::make_shared<Semaphore_public_ctor>(device);

        VkSemaphoreCreateInfo  create_info{
            .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
            .pNext = nullptr,
            .flags = 0
        };

        VkResult vk_result = vkCreateSemaphore(
            sema->device()->handle(),
            &create_info,
            sema->device()->context()->vk_allocator_ptr(),
            &sema->_handle
        );
        if (vk_result != VK_SUCCESS)
        {
            return Error("", (ApiResult)vk_result, false);
        }
        return sema;
    }

    void Semaphore::destroy()
    {
        vkDestroySemaphore(
            device()->handle(),
            handle(),
            device()->context()->vk_allocator_ptr()
        );
    }

    Semaphore::Semaphore(const DevicePtr& device)
        : _device(device)
    {}

    Result<FencePtr> Fence::create(
        const DevicePtr& device,
        VkFenceCreateFlags flags
    )
    {
        FencePtr fence = std::make_shared<Fence_public_ctor>(device);

        VkFenceCreateInfo create_info{
            .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
            .pNext = nullptr,
            .flags = flags
        };

        VkResult vk_result = vkCreateFence(
            fence->device()->handle(),
            &create_info,
            fence->device()->context()->vk_allocator_ptr(),
            &fence->_handle
        );
        if (vk_result != VK_SUCCESS)
        {
            return Error("", (ApiResult)vk_result, false);
        }
        return fence;
    }

    Result<> Fence::wait(uint64_t timeout)
    {
        VkResult vk_result = vkWaitForFences(
            device()->handle(),
            1,
            &_handle,
            VK_TRUE,
            timeout
        );
        if (vk_result != VK_SUCCESS)
        {
            return Error("", (ApiResult)vk_result, false);
        }
        return Result();
    }

    Result<> Fence::wait_multiple(
        const std::vector<FencePtr>& fences,
        bool wait_all,
        uint64_t timeout
    )
    {
        if (fences.empty())
        {
            return Result();
        }

        std::vector<VkFence> vk_fences(fences.size());
        for (size_t i = 0; i < fences.size(); i++)
        {
            vk_fences[i] = fences[i]->handle();
        }

        VkResult vk_result = vkWaitForFences(
            fences[0]->device()->handle(),
            vk_fences.size(),
            vk_fences.data(),
            wait_all,
            timeout
        );
        if (vk_result != VK_SUCCESS)
        {
            return Error("", (ApiResult)vk_result, false);
        }
        return Result();
    }

    Result<> Fence::reset()
    {
        VkResult vk_result = vkResetFences(
            device()->handle(),
            1,
            &_handle
        );
        if (vk_result != VK_SUCCESS)
        {
            return Error("", (ApiResult)vk_result, false);
        }
        return Result();
    }

    Result<bool> Fence::is_signaled() const
    {
        VkResult vk_result = vkGetFenceStatus(
            device()->handle(),
            handle()
        );
        if (vk_result == VK_SUCCESS)
        {
            return true;
        }
        else if (vk_result == VK_NOT_READY)
        {
            return false;
        }
        return Error("", (ApiResult)vk_result, false);
    }

    void Fence::destroy()
    {
        vkDestroyFence(
            device()->handle(),
            handle(),
            device()->context()->vk_allocator_ptr()
        );
    }

    Fence::Fence(const DevicePtr& device)
        : _device(device)
    {}

    Result<BufferPtr> Buffer::create(
        const DevicePtr& device,
        const BufferConfig& config
    )
    {
        BufferPtr buf = std::make_shared<Buffer_public_ctor>(
            device,
            config
        );

        VkBufferCreateInfo create_info{
            .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
            .pNext = nullptr,
            .flags = buf->config().flags,
            .size = buf->config().size,
            .usage = buf->config().usage,
            .sharingMode = buf->config().sharing_mode,

            .queueFamilyIndexCount =
            (uint32_t)buf->config().queue_family_indices.size(),

            .pQueueFamilyIndices = buf->config().queue_family_indices.data()
        };

        VkResult vk_result = vkCreateBuffer(
            buf->device()->handle(),
            &create_info,
            buf->device()->context()->vk_allocator_ptr(),
            &buf->_handle
        );
        if (vk_result != VK_SUCCESS)
        {
            return Error("", (ApiResult)vk_result, false);
        }

        VkMemoryRequirements vk_mem_requirements;
        vkGetBufferMemoryRequirements(
            buf->device()->handle(),
            buf->handle(),
            &vk_mem_requirements
        );
        buf->_memory_requirements = MemoryRequirements_from_vk(
            vk_mem_requirements
        );

        return buf;
    }

    Result<> Buffer::bind_memory(
        const DeviceMemoryPtr& memory,
        VkDeviceSize memory_offset
    )
    {
        VkResult vk_result = vkBindBufferMemory(
            device()->handle(),
            handle(),
            memory->handle(),
            memory_offset
        );
        if (vk_result != VK_SUCCESS)
        {
            return Error("", (ApiResult)vk_result, false);
        }
        return Result();
    }

    void Buffer::destroy()
    {
        vkDestroyBuffer(
            device()->handle(),
            handle(),
            device()->context()->vk_allocator_ptr()
        );
    }

    Buffer::Buffer(const DevicePtr& device, const BufferConfig& config)
        : _device(device), _config(config)
    {}

    Result<DeviceMemoryPtr> DeviceMemory::allocate(
        const DevicePtr& device,
        const DeviceMemoryConfig& config
    )
    {
        DeviceMemoryPtr mem = std::make_shared<DeviceMemory_public_ctor>(
            device,
            config
        );

        VkMemoryAllocateInfo allocate_info{
            .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
            .pNext = nullptr,
            .allocationSize = mem->config().allocation_size,
            .memoryTypeIndex = mem->config().memory_type_index
        };

        VkResult vk_result = vkAllocateMemory(
            device->handle(),
            &allocate_info,
            device->context()->vk_allocator_ptr(),
            &mem->_handle
        );
        if (vk_result != VK_SUCCESS)
        {
            return Error("", (ApiResult)vk_result, false);
        }
        return mem;
    }

    Result<void*> DeviceMemory::map(VkDeviceSize offset, VkDeviceSize size)
    {
        if (device().expired())
        {
            return Error(
                "device shared pointer has expired",
                std::nullopt,
                false
            );
        }
        auto device_locked = device().lock();

        void* p;
        VkResult vk_result = vkMapMemory(
            device_locked->handle(),
            handle(),
            offset,
            size,
            0,
            &p
        );
        if (vk_result != VK_SUCCESS)
        {
            unmap();
            return Error("", (ApiResult)vk_result, false);
        }
        return p;
    }

    void DeviceMemory::unmap()
    {
        if (device().expired())
        {
            return;
        }
        auto device_locked = device().lock();
        vkUnmapMemory(device_locked->handle(), handle());
    }

    Result<> DeviceMemory::flush_mapped_range(
        VkDeviceSize offset,
        VkDeviceSize size
    )
    {
        if (device().expired())
        {
            return Error(
                "device shared pointer has expired",
                std::nullopt,
                false
            );
        }
        auto device_locked = device().lock();

        VkMappedMemoryRange vk_mapped_range{
            .sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE,
            .pNext = nullptr,
            .memory = handle(),
            .offset = offset,
            .size = size
        };

        VkResult vk_result = vkFlushMappedMemoryRanges(
            device_locked->handle(),
            1,
            &vk_mapped_range
        );
        if (vk_result != VK_SUCCESS)
        {
            return Error("", (ApiResult)vk_result, false);
        }
        return Result();
    }

    Result<> DeviceMemory::invalidate_mapped_range(
        VkDeviceSize offset,
        VkDeviceSize size
    )
    {
        if (device().expired())
        {
            return Error(
                "device shared pointer has expired",
                std::nullopt,
                false
            );
        }
        auto device_locked = device().lock();

        VkMappedMemoryRange vk_mapped_range{
            .sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE,
            .pNext = nullptr,
            .memory = handle(),
            .offset = offset,
            .size = size
        };

        VkResult vk_result = vkInvalidateMappedMemoryRanges(
            device_locked->handle(),
            1,
            &vk_mapped_range
        );
        if (vk_result != VK_SUCCESS)
        {
            return Error("", (ApiResult)vk_result, false);
        }
        return Result();
    }

    Result<> DeviceMemory::upload(void* data, VkDeviceSize data_size)
    {
        if (data_size > config().allocation_size)
        {
            return Error("data is too big", std::nullopt, false);
        }

        auto map_result = map(0, config().allocation_size);
        if (!map_result.ok())
        {
            return Error(
                "failed to map memory: " + map_result.error().to_string(),
                map_result.error().api_result(),
                true
            );
        }

        auto mapped_data = map_result.value();
        {
            std::copy(
                (uint8_t*)data,
                (uint8_t*)data + data_size,
                (uint8_t*)mapped_data
            );
        }

        auto flush_result = flush_mapped_range(0, VK_WHOLE_SIZE);
        if (!flush_result.ok())
        {
            return Error(
                "failed to flush mapped memory range: "
                + flush_result.error().to_string(),
                flush_result.error().api_result(),
                true
            );
        }

        unmap();

        return Result();
    }

    void DeviceMemory::destroy()
    {
        if (device().expired())
        {
            return;
        }
        auto device_locked = device().lock();
        vkFreeMemory(
            device_locked->handle(),
            handle(),
            device_locked->context()->vk_allocator_ptr()
        );
    }

    DeviceMemory::DeviceMemory(
        const DeviceWPtr& device,
        const DeviceMemoryConfig& config
    )
        : _device(device), _config(config)
    {}

    void DescriptorSet::update_sets(
        const DevicePtr& device,
        const std::vector<WriteDescriptorSet>& writes,
        const std::vector<CopyDescriptorSet>& copies
    )
    {
        if (writes.empty() && copies.empty())
        {
            return;
        }

        std::vector<VkWriteDescriptorSet> vk_writes(writes.size());
        std::vector<DescriptorSetPtr> wastes_dst_set(writes.size());
        std::vector<std::vector<VkDescriptorImageInfo>> wastes_vk_image_infos(
            writes.size()
        );
        std::vector<std::vector<SamplerPtr>> wastes_image_infos_sampler(
            writes.size()
        );
        std::vector<std::vector<ImageViewPtr>> wastes_image_infos_image_view(
            writes.size()
        );
        std::vector<std::vector<VkDescriptorBufferInfo>> wastes_vk_buffer_infos(
            writes.size()
        );
        std::vector<std::vector<BufferPtr>> wastes_buffer_infos_buffer(
            writes.size()
        );
        std::vector<std::vector<BufferViewPtr>> wastes_texel_buffer_views(
            writes.size()
        );
        std::vector<std::vector<VkBufferView>> wastes_vk_texel_buffer_views(
            writes.size()
        );
        for (size_t i = 0; i < writes.size(); i++)
        {
            vk_writes[i] = WriteDescriptorSet_to_vk(
                writes[i],
                wastes_dst_set[i],
                wastes_vk_image_infos[i],
                wastes_image_infos_sampler[i],
                wastes_image_infos_image_view[i],
                wastes_vk_buffer_infos[i],
                wastes_buffer_infos_buffer[i],
                wastes_texel_buffer_views[i],
                wastes_vk_texel_buffer_views[i]
            );
        }

        std::vector<VkCopyDescriptorSet> vk_copies(copies.size());
        std::vector<DescriptorSetPtr> wastes_src_set(copies.size());
        std::vector<DescriptorSetPtr> wastes_dst_set2(copies.size());
        for (size_t i = 0; i < copies.size(); i++)
        {
            vk_copies[i] = CopyDescriptorSet_to_vk(
                copies[i],
                wastes_src_set[i],
                wastes_dst_set2[i]
            );
        }

        vkUpdateDescriptorSets(
            device->handle(),
            (uint32_t)vk_writes.size(),
            vk_writes.data(),
            (uint32_t)vk_copies.size(),
            vk_copies.data()
        );
    }

    void DescriptorSet::destroy()
    {
        if (pool().expired())
        {
            return;
        }
        auto pool_locked = pool().lock();
        vkFreeDescriptorSets(
            pool_locked->device()->handle(),
            pool_locked->handle(),
            1,
            &_handle
        );
    }

    DescriptorSet::DescriptorSet(
        const DescriptorPoolWPtr& pool,
        VkDescriptorSet handle
    )
        : _pool(pool), _handle(handle)
    {}

    Result<DescriptorPoolPtr> DescriptorPool::create(
        const DevicePtr& device,
        const DescriptorPoolConfig& config
    )
    {
        DescriptorPoolPtr pool = std::make_shared<DescriptorPool_public_ctor>(
            device,
            config
        );

        std::vector<VkDescriptorPoolSize> vk_pool_sizes(
            pool->config().pool_sizes.size()
        );
        for (size_t i = 0; i < pool->config().pool_sizes.size(); i++)
        {
            vk_pool_sizes[i] = DescriptorPoolSize_to_vk(
                pool->config().pool_sizes[i]
            );
        }

        VkDescriptorPoolCreateInfo create_info{
            .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
            .pNext = nullptr,
            .flags = pool->config().flags,
            .maxSets = pool->config().max_sets,
            .poolSizeCount = (uint32_t)vk_pool_sizes.size(),
            .pPoolSizes = vk_pool_sizes.data()
        };

        VkResult vk_result = vkCreateDescriptorPool(
            pool->device()->handle(),
            &create_info,
            pool->device()->context()->vk_allocator_ptr(),
            &pool->_handle
        );
        if (vk_result != VK_SUCCESS)
        {
            return Error("", (ApiResult)vk_result, false);
        }
        return pool;
    }

    Result<std::vector<DescriptorSetPtr>> DescriptorPool::allocate_sets(
        const DescriptorPoolPtr& pool,
        uint32_t count,
        const std::vector<DescriptorSetLayoutPtr>& set_layouts
    )
    {
        std::vector<VkDescriptorSetLayout> vk_set_layouts(set_layouts.size());
        for (size_t i = 0; i < set_layouts.size(); i++)
        {
            vk_set_layouts[i] = set_layouts[i]->handle();
        }

        VkDescriptorSetAllocateInfo alloc_info{
            .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
            .pNext = nullptr,
            .descriptorPool = pool->handle(),
            .descriptorSetCount = count,
            .pSetLayouts = vk_set_layouts.data()
        };

        std::vector<VkDescriptorSet> vk_sets(count);
        VkResult vk_result = vkAllocateDescriptorSets(
            pool->device()->handle(),
            &alloc_info,
            vk_sets.data()
        );
        if (vk_result != VK_SUCCESS)
        {
            return Error("", (ApiResult)vk_result, false);
        }

        std::vector<DescriptorSetPtr> sets;
        for (auto vk_set : vk_sets)
        {
            sets.push_back(std::make_shared<DescriptorSet_public_ctor>(
                (DescriptorPoolWPtr)pool,
                vk_set
            ));
        }
        return sets;
    }

    void DescriptorPool::destroy()
    {
        vkDestroyDescriptorPool(
            device()->handle(),
            handle(),
            device()->context()->vk_allocator_ptr()
        );
    }

    DescriptorPool::DescriptorPool(
        const DevicePtr& device,
        const DescriptorPoolConfig& config
    )
        : _device(device), _config(config)
    {}

    Result<BufferViewPtr> BufferView::create(
        const DevicePtr& device,
        const BufferPtr& buffer,
        const BufferViewConfig& config
    )
    {
        BufferViewPtr view = std::make_shared<BufferView_public_ctor>(
            device,
            buffer,
            config
        );

        VkBufferViewCreateInfo create_info{
            .sType = VK_STRUCTURE_TYPE_BUFFER_VIEW_CREATE_INFO,
            .pNext = nullptr,
            .flags = 0,
            .buffer = view->buffer()->handle(),
            .format = view->config().format,
            .offset = view->config().offset,
            .range = view->config().range
        };

        VkResult vk_result = vkCreateBufferView(
            view->device()->handle(),
            &create_info,
            view->device()->context()->vk_allocator_ptr(),
            &view->_handle
        );
        if (vk_result != VK_SUCCESS)
        {
            return Error("", (ApiResult)vk_result, false);
        }
        return view;
    }

    void BufferView::destroy()
    {
        vkDestroyBufferView(
            device()->handle(),
            handle(),
            device()->context()->vk_allocator_ptr()
        );
    }

    BufferView::BufferView(
        const DevicePtr& device,
        const BufferPtr& buffer,
        const BufferViewConfig& config
    )
        : _device(device),
        _buffer(buffer),
        _config(config)
    {}

    bool format_has_depth_component(VkFormat format)
    {
        return
            format == VK_FORMAT_D16_UNORM
            || format == VK_FORMAT_X8_D24_UNORM_PACK32
            || format == VK_FORMAT_D32_SFLOAT
            || format == VK_FORMAT_D16_UNORM_S8_UINT
            || format == VK_FORMAT_D24_UNORM_S8_UINT
            || format == VK_FORMAT_D32_SFLOAT_S8_UINT;
    }

    bool format_has_stencil_component(VkFormat format)
    {
        return
            format == VK_FORMAT_S8_UINT
            || format == VK_FORMAT_D16_UNORM_S8_UINT
            || format == VK_FORMAT_D24_UNORM_S8_UINT
            || format == VK_FORMAT_D32_SFLOAT_S8_UINT;
    }

#pragma region Vulkan callbacks

    static void* vk_allocation_callback(
        void* p_user_data,
        size_t size,
        size_t alignment,
        VkSystemAllocationScope allocation_scope
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
            allocation_scope
        );
    }

    static void* vk_reallocation_callback(
        void* p_user_data,
        void* p_original,
        size_t size,
        size_t alignment,
        VkSystemAllocationScope allocation_scope
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
        VkInternalAllocationType allocation_type,
        VkSystemAllocationScope allocation_scope
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
            allocation_type,
            allocation_scope
        );
    }

    static void vk_internal_free_notification(
        void* p_user_data,
        size_t size,
        VkInternalAllocationType allocation_type,
        VkSystemAllocationScope allocation_scope
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
            allocation_type,
            allocation_scope
        );
    }

    static VKAPI_ATTR VkBool32 VKAPI_CALL vk_debug_callback(
        VkDebugUtilsMessageSeverityFlagBitsEXT message_severity,
        VkDebugUtilsMessageTypeFlagsEXT message_types,
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
            message_severity,
            message_types,
            DebugMessageData_from_vk(*p_callback_data)
            );

        return VK_FALSE;
    }

#pragma endregion

}
