#pragma once
#ifdef USE_VULKAN_RENDERER
#include <volk.h>
#include "Renderer/BufferDescriptor.h"
#include "Renderer/TextureFormat.h"
#include "Renderer/RenderPassDescriptor.h"
#include "Renderer/PipelineStateDescriptor.h"

namespace Gleam {

#define VK_CHECK(x) {VkResult result = (x);\
					GLEAM_ASSERT(result == VK_SUCCESS, VkResultToString(x));}

#define VK_FLAGS_NONE 0

struct VulkanDrawable
{
	VkImage image{ VK_NULL_HANDLE };
	VkImageView view{ VK_NULL_HANDLE };
};

struct VulkanQueue
{
	VkQueue handle{ VK_NULL_HANDLE };
	uint32_t index{ 0 };
};

static constexpr const char* VkResultToString(VkResult result)
{
	switch (result)
	{
		case VK_SUCCESS:                                            return "VK_SUCCESS";
		case VK_NOT_READY:                                          return "VK_NOT_READY";
		case VK_TIMEOUT:                                            return "VK_TIMEOUT";
		case VK_EVENT_SET:                                          return "VK_EVENT_SET";
		case VK_EVENT_RESET:                                        return "VK_EVENT_RESET";
		case VK_INCOMPLETE:                                         return "VK_INCOMPLETE";
		case VK_ERROR_OUT_OF_HOST_MEMORY:                           return "VK_ERROR_OUT_OF_HOST_MEMORY";
		case VK_ERROR_OUT_OF_DEVICE_MEMORY:                         return "VK_ERROR_OUT_OF_DEVICE_MEMORY";
		case VK_ERROR_INITIALIZATION_FAILED:                        return "VK_ERROR_INITIALIZATION_FAILED";
		case VK_ERROR_DEVICE_LOST:                                  return "VK_ERROR_DEVICE_LOST";
		case VK_ERROR_MEMORY_MAP_FAILED:                            return "VK_ERROR_MEMORY_MAP_FAILED";
		case VK_ERROR_LAYER_NOT_PRESENT:                            return "VK_ERROR_LAYER_NOT_PRESENT";
		case VK_ERROR_EXTENSION_NOT_PRESENT:                        return "VK_ERROR_EXTENSION_NOT_PRESENT";
		case VK_ERROR_FEATURE_NOT_PRESENT:                          return "VK_ERROR_FEATURE_NOT_PRESENT";
		case VK_ERROR_INCOMPATIBLE_DRIVER:                          return "VK_ERROR_INCOMPATIBLE_DRIVER";
		case VK_ERROR_TOO_MANY_OBJECTS:                             return "VK_ERROR_TOO_MANY_OBJECTS";
		case VK_ERROR_FORMAT_NOT_SUPPORTED:                         return "VK_ERROR_FORMAT_NOT_SUPPORTED";
		case VK_ERROR_FRAGMENTED_POOL:                              return "VK_ERROR_FRAGMENTED_POOL";
		case VK_ERROR_UNKNOWN:                                      return "VK_ERROR_UNKNOWN";
		case VK_ERROR_OUT_OF_POOL_MEMORY:                           return "VK_ERROR_OUT_OF_POOL_MEMORY";
		case VK_ERROR_INVALID_EXTERNAL_HANDLE:                      return "VK_ERROR_INVALID_EXTERNAL_HANDLE";
		case VK_ERROR_FRAGMENTATION:                                return "VK_ERROR_FRAGMENTATION";
		case VK_ERROR_INVALID_OPAQUE_CAPTURE_ADDRESS:               return "VK_ERROR_INVALID_OPAQUE_CAPTURE_ADDRESS";
		case VK_ERROR_SURFACE_LOST_KHR:                             return "VK_ERROR_SURFACE_LOST_KHR";
		case VK_ERROR_NATIVE_WINDOW_IN_USE_KHR:                     return "VK_ERROR_NATIVE_WINDOW_IN_USE_KHR";
		case VK_SUBOPTIMAL_KHR:                                     return "VK_SUBOPTIMAL_KHR";
		case VK_ERROR_OUT_OF_DATE_KHR:                              return "VK_ERROR_OUT_OF_DATE_KHR";
		case VK_ERROR_INCOMPATIBLE_DISPLAY_KHR:                     return "VK_ERROR_INCOMPATIBLE_DISPLAY_KHR";
		case VK_ERROR_VALIDATION_FAILED_EXT:                        return "VK_ERROR_VALIDATION_FAILED_EXT";
		case VK_ERROR_INVALID_SHADER_NV:                            return "VK_ERROR_INVALID_SHADER_NV";
		case VK_ERROR_INVALID_DRM_FORMAT_MODIFIER_PLANE_LAYOUT_EXT: return "VK_ERROR_INVALID_DRM_FORMAT_MODIFIER_PLANE_LAYOUT_EXT";
		case VK_ERROR_NOT_PERMITTED_EXT:                            return "VK_ERROR_NOT_PERMITTED_EXT";
		case VK_ERROR_FULL_SCREEN_EXCLUSIVE_MODE_LOST_EXT:          return "VK_ERROR_FULL_SCREEN_EXCLUSIVE_MODE_LOST_EXT";
		case VK_THREAD_IDLE_KHR:                                    return "VK_THREAD_IDLE_KHR";
		case VK_THREAD_DONE_KHR:                                    return "VK_THREAD_DONE_KHR";
		case VK_OPERATION_DEFERRED_KHR:                             return "VK_OPERATION_DEFERRED_KHR";
		case VK_OPERATION_NOT_DEFERRED_KHR:                         return "VK_OPERATION_NOT_DEFERRED_KHR";
		case VK_PIPELINE_COMPILE_REQUIRED_EXT:                      return "VK_PIPELINE_COMPILE_REQUIRED_EXT";
		default:                                                    return "UNKNOWN VULKAN ERROR";
	}
}

static constexpr VkSampleCountFlagBits GetVkSampleCount(uint32_t sampleCount)
{
	return static_cast<VkSampleCountFlagBits>(BIT(sampleCount - 1));
}

static constexpr TextureFormat VkFormatToTextureFormat(VkFormat format)
{
	switch (format)
	{
		case VK_FORMAT_R8_SRGB: return TextureFormat::R8_SRGB;
		case VK_FORMAT_R8G8_SRGB: return TextureFormat::R8G8_SRGB;
		case VK_FORMAT_R8G8B8A8_SRGB: return TextureFormat::R8G8B8A8_SRGB;

		case VK_FORMAT_R8_UNORM: return TextureFormat::R8_UNorm;
		case VK_FORMAT_R8G8_UNORM: return TextureFormat::R8G8_UNorm;
		case VK_FORMAT_R8G8B8A8_UNORM: return TextureFormat::R8G8B8A8_UNorm;

		case VK_FORMAT_R8_SNORM: return TextureFormat::R8_SNorm;
		case VK_FORMAT_R8G8_SNORM: return TextureFormat::R8G8_SNorm;
		case VK_FORMAT_R8G8B8A8_SNORM: return TextureFormat::R8G8B8A8_SNorm;

		case VK_FORMAT_R8_UINT: return TextureFormat::R8_UInt;
		case VK_FORMAT_R8G8_UINT: return TextureFormat::R8G8_UInt;
		case VK_FORMAT_R8G8B8A8_UINT: return TextureFormat::R8G8B8A8_UInt;

		case VK_FORMAT_R8_SINT: return TextureFormat::R8_SInt;
		case VK_FORMAT_R8G8_SINT: return TextureFormat::R8G8_SInt;
		case VK_FORMAT_R8G8B8A8_SINT: return TextureFormat::R8G8B8A8_SInt;

		case VK_FORMAT_R16_UNORM: return TextureFormat::R16_UNorm;
		case VK_FORMAT_R16G16_UNORM: return TextureFormat::R16G16_UNorm;
		case VK_FORMAT_R16G16B16A16_UNORM: return TextureFormat::R16G16B16A16_UNorm;

		case VK_FORMAT_R16_SNORM: return TextureFormat::R16_SNorm;
		case VK_FORMAT_R16G16_SNORM: return TextureFormat::R16G16_SNorm;
		case VK_FORMAT_R16G16B16A16_SNORM: return TextureFormat::R16G16B16A16_SNorm;

		case VK_FORMAT_R16_UINT: return TextureFormat::R16_UInt;
		case VK_FORMAT_R16G16_UINT: return TextureFormat::R16G16_UInt;
		case VK_FORMAT_R16G16B16A16_UINT: return TextureFormat::R16G16B16A16_UInt;

		case VK_FORMAT_R16_SINT: return TextureFormat::R16_SInt;
		case VK_FORMAT_R16G16_SINT: return TextureFormat::R16G16_SInt;
		case VK_FORMAT_R16G16B16A16_SINT: return TextureFormat::R16G16B16A16_SInt;

		case VK_FORMAT_R16_SFLOAT: return TextureFormat::R16_SFloat;
		case VK_FORMAT_R16G16_SFLOAT: return TextureFormat::R16G16_SFloat;
		case VK_FORMAT_R16G16B16A16_SFLOAT: return TextureFormat::R16G16B16A16_SFloat;

		case VK_FORMAT_R32_UINT: return TextureFormat::R32_UInt;
		case VK_FORMAT_R32G32_UINT: return TextureFormat::R32G32_UInt;
		case VK_FORMAT_R32G32B32A32_UINT: return TextureFormat::R32G32B32A32_UInt;

		case VK_FORMAT_R32_SINT: return TextureFormat::R32_SInt;
		case VK_FORMAT_R32G32_SINT: return TextureFormat::R32G32_SInt;
		case VK_FORMAT_R32G32B32A32_SINT: return TextureFormat::R32G32B32A32_SInt;

		case VK_FORMAT_R32_SFLOAT: return TextureFormat::R32_SFloat;
		case VK_FORMAT_R32G32_SFLOAT: return TextureFormat::R32G32_SFloat;
		case VK_FORMAT_R32G32B32A32_SFLOAT: return TextureFormat::R32G32B32A32_SFloat;

		case VK_FORMAT_B8G8R8A8_SRGB: return TextureFormat::B8G8R8A8_SRGB;
		case VK_FORMAT_B8G8R8A8_UNORM: return TextureFormat::B8G8R8A8_UNorm;

		// Depth - Stencil formats
		case VK_FORMAT_S8_UINT: return TextureFormat::S8_UInt;
		case VK_FORMAT_D16_UNORM: return TextureFormat::D16_UNorm;
		case VK_FORMAT_D32_SFLOAT: return TextureFormat::D32_SFloat;
		case VK_FORMAT_D32_SFLOAT_S8_UINT: return TextureFormat::D32_SFloat_S8_UInt;

		default: return TextureFormat::None;
	}
}

static constexpr VkFormat TextureFormatToVkFormat(TextureFormat format)
{
	switch (format)
	{
		case TextureFormat::R8_SRGB: return VK_FORMAT_R8_SRGB;
		case TextureFormat::R8G8_SRGB: return VK_FORMAT_R8G8_SRGB;
		case TextureFormat::R8G8B8A8_SRGB: return VK_FORMAT_R8G8B8A8_SRGB;

		case TextureFormat::R8_UNorm: return VK_FORMAT_R8_UNORM;
		case TextureFormat::R8G8_UNorm: return VK_FORMAT_R8G8_UNORM;
		case TextureFormat::R8G8B8A8_UNorm: return VK_FORMAT_R8G8B8A8_UNORM;

		case TextureFormat::R8_SNorm: return VK_FORMAT_R8_SNORM;
		case TextureFormat::R8G8_SNorm: return VK_FORMAT_R8G8_SNORM;
		case TextureFormat::R8G8B8A8_SNorm: return VK_FORMAT_R8G8B8A8_SNORM;

		case TextureFormat::R8_UInt: return VK_FORMAT_R8_UINT;
		case TextureFormat::R8G8_UInt: return VK_FORMAT_R8G8_UINT;
		case TextureFormat::R8G8B8A8_UInt: return VK_FORMAT_R8G8B8A8_UINT;

		case TextureFormat::R8_SInt: return VK_FORMAT_R8_SINT;
		case TextureFormat::R8G8_SInt: return VK_FORMAT_R8G8_SINT;
		case TextureFormat::R8G8B8A8_SInt: return VK_FORMAT_R8G8B8A8_SINT;

		case TextureFormat::R16_UNorm: return VK_FORMAT_R16_UNORM;
		case TextureFormat::R16G16_UNorm: return VK_FORMAT_R16G16_UNORM;
		case TextureFormat::R16G16B16A16_UNorm: return VK_FORMAT_R16G16B16A16_UNORM;

		case TextureFormat::R16_SNorm: return VK_FORMAT_R16_SNORM;
		case TextureFormat::R16G16_SNorm: return VK_FORMAT_R16G16_SNORM;
		case TextureFormat::R16G16B16A16_SNorm: return VK_FORMAT_R16G16B16A16_SNORM;

		case TextureFormat::R16_UInt: return VK_FORMAT_R16_UINT;
		case TextureFormat::R16G16_UInt: return VK_FORMAT_R16G16_UINT;
		case TextureFormat::R16G16B16A16_UInt: return VK_FORMAT_R16G16B16A16_UINT;

		case TextureFormat::R16_SInt: return VK_FORMAT_R16_SINT;
		case TextureFormat::R16G16_SInt: return VK_FORMAT_R16G16_SINT;
		case TextureFormat::R16G16B16A16_SInt: return VK_FORMAT_R16G16B16A16_SINT;

		case TextureFormat::R16_SFloat: return VK_FORMAT_R16_SFLOAT;
		case TextureFormat::R16G16_SFloat: return VK_FORMAT_R16G16_SFLOAT;
		case TextureFormat::R16G16B16A16_SFloat: return VK_FORMAT_R16G16B16A16_SFLOAT;

		case TextureFormat::R32_UInt: return VK_FORMAT_R32_UINT;
		case TextureFormat::R32G32_UInt: return VK_FORMAT_R32G32_UINT;
		case TextureFormat::R32G32B32A32_UInt: return VK_FORMAT_R32G32B32A32_UINT;

		case TextureFormat::R32_SInt: return VK_FORMAT_R32_SINT;
		case TextureFormat::R32G32_SInt: return VK_FORMAT_R32G32_SINT;
		case TextureFormat::R32G32B32A32_SInt: return VK_FORMAT_R32G32B32A32_SINT;

		case TextureFormat::R32_SFloat: return VK_FORMAT_R32_SFLOAT;
		case TextureFormat::R32G32_SFloat: return VK_FORMAT_R32G32_SFLOAT;
		case TextureFormat::R32G32B32A32_SFloat: return VK_FORMAT_R32G32B32A32_SFLOAT;

		case TextureFormat::B8G8R8A8_SRGB: return VK_FORMAT_B8G8R8A8_SRGB;
		case TextureFormat::B8G8R8A8_UNorm: return VK_FORMAT_B8G8R8A8_UNORM;

		// Depth - Stencil formats
		case TextureFormat::S8_UInt: return VK_FORMAT_S8_UINT;
		case TextureFormat::D16_UNorm: return VK_FORMAT_D16_UNORM;
		case TextureFormat::D32_SFloat: return VK_FORMAT_D32_SFLOAT;
		case TextureFormat::D32_SFloat_S8_UInt: return VK_FORMAT_D32_SFLOAT_S8_UINT;

		default: return VK_FORMAT_UNDEFINED;
	}
}

static constexpr VkAttachmentLoadOp AttachmentLoadActionToVkAttachmentLoadOp(AttachmentLoadAction loadAction)
{
	switch (loadAction)
	{
		case AttachmentLoadAction::Load: return VK_ATTACHMENT_LOAD_OP_LOAD;
		case AttachmentLoadAction::Clear: return VK_ATTACHMENT_LOAD_OP_CLEAR;
		case AttachmentLoadAction::DontCare: return VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		default: return VK_ATTACHMENT_LOAD_OP_MAX_ENUM;
	}
}

static constexpr VkAttachmentStoreOp AttachmentStoreActionToVkAttachmentStoreOp(AttachmentStoreAction storeAction)
{
	switch (storeAction)
	{
		case AttachmentStoreAction::Store: return VK_ATTACHMENT_STORE_OP_STORE;
		case AttachmentStoreAction::DontCare: return VK_ATTACHMENT_STORE_OP_DONT_CARE;
		default: return VK_ATTACHMENT_STORE_OP_MAX_ENUM;
	}
}

static constexpr VkPrimitiveTopology PrimitiveToplogyToVkPrimitiveTopology(PrimitiveTopology topology)
{
	switch (topology)
	{
		case PrimitiveTopology::Points: return VK_PRIMITIVE_TOPOLOGY_POINT_LIST;
		case PrimitiveTopology::Lines: return VK_PRIMITIVE_TOPOLOGY_LINE_LIST;
		case PrimitiveTopology::LineStrip: return VK_PRIMITIVE_TOPOLOGY_LINE_STRIP;
		case PrimitiveTopology::Triangles: return VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
        case PrimitiveTopology::TriangleStrip: return VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP;
		default: return VK_PRIMITIVE_TOPOLOGY_MAX_ENUM;
	}
}

static constexpr VkCullModeFlags CullModeToVkCullMode(CullMode cullMode)
{
	switch (cullMode)
	{
		case CullMode::Off: return VK_CULL_MODE_NONE;
		case CullMode::Front: return VK_CULL_MODE_FRONT_BIT;
		case CullMode::Back: return VK_CULL_MODE_BACK_BIT;
		default: return VK_CULL_MODE_FLAG_BITS_MAX_ENUM;
	}
}

static constexpr VkBufferUsageFlags BufferUsageToVkBufferUsage(BufferUsage usage)
{
	switch (usage)
	{
		case BufferUsage::VertexBuffer: return VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
		case BufferUsage::IndexBuffer: return VK_BUFFER_USAGE_INDEX_BUFFER_BIT;
		case BufferUsage::StorageBuffer: return VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
		case BufferUsage::UniformBuffer: return VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
		case BufferUsage::StagingBuffer: return VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
		default: return VK_BUFFER_USAGE_FLAG_BITS_MAX_ENUM;
	}
}

static constexpr VkDescriptorType BufferUsageToVkDescriptorType(BufferUsage usage)
{
	switch (usage)
	{
		case BufferUsage::VertexBuffer: return VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
		case BufferUsage::StorageBuffer: return VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
		case BufferUsage::UniformBuffer: return VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		default: return VK_DESCRIPTOR_TYPE_MAX_ENUM;
	}
}

static constexpr VkPipelineBindPoint PipelineBindPointToVkPipelineBindPoint(PipelineBindPoint pipeline)
{
	switch (pipeline)
	{
		case PipelineBindPoint::Graphics: return VK_PIPELINE_BIND_POINT_GRAPHICS;
		case PipelineBindPoint::Compute: return VK_PIPELINE_BIND_POINT_COMPUTE;
		case PipelineBindPoint::RayTracing: return VK_PIPELINE_BIND_POINT_RAY_TRACING_KHR;
		default: return VK_PIPELINE_BIND_POINT_MAX_ENUM;
	}
} 

} // namespace Gleam
#endif
