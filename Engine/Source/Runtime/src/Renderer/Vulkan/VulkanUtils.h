#pragma once
#ifdef USE_VULKAN_RENDERER
#include <volk.h>
#include "Renderer/Buffer.h"
#include "Renderer/RendererContext.h"
#include "Renderer/TextureFormat.h"
#include "Renderer/RenderPassDescriptor.h"
#include "Renderer/PipelineStateDescriptor.h"

namespace Gleam {

#define VK_CHECK(x) {VkResult result = (x);\
					GLEAM_ASSERT(result == VK_SUCCESS, VkResultToString(x));}

#define VulkanDevice As<VkDevice>(RendererContext::GetDevice())

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
	return sampleCount > 1 ? static_cast<VkSampleCountFlagBits>(BIT(sampleCount - 1)) : VK_SAMPLE_COUNT_1_BIT;
}

static constexpr TextureFormat VkFormatToTextureFormat(VkFormat format)
{
	switch (format)
	{
		case VK_FORMAT_B8G8R8A8_UNORM: return TextureFormat::B8G8R8A8_UNorm;
		case VK_FORMAT_R8G8B8A8_UNORM: return TextureFormat::R8G8B8A8_UNorm;
		// TODO:
		default: return TextureFormat::None;
	}
}

static constexpr VkFormat TextureFormatToVkFormat(TextureFormat format)
{
	switch (format)
	{
		case TextureFormat::B8G8R8A8_UNorm: return VK_FORMAT_B8G8R8A8_UNORM;
		case TextureFormat::R8G8B8A8_UNorm: return VK_FORMAT_R8G8B8A8_UNORM;
		// TODO:
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

} // namespace Gleam
#endif
