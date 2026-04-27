#pragma once
#ifdef USE_DIRECTX_RENDERER
#include <d3d12.h>
#include <WinPixEventRuntime/pix3.h>

#include "Renderer/Barrier.h"
#include "Renderer/TextureFormat.h"
#include "Renderer/HeapDescriptor.h"
#include "Renderer/RenderPassDescriptor.h"
#include "Renderer/PipelineStateDescriptor.h"

namespace Gleam {

#define DX_CHECK(x) {HRESULT result = (x);\
					GLEAM_ASSERT(SUCCEEDED(result), HRESULTtoString(x));}

static constexpr const char* HRESULTtoString(HRESULT result)
{
	switch (result)
	{
		case E_ABORT:									return "Operation aborted";
		case E_ACCESSDENIED:							return "General access denied error";
		case E_FAIL:									return "Unspecified failure";
		case E_HANDLE:									return "Handle that is not valid";
		case E_INVALIDARG:								return "One or more arguments are not valid";
		case E_NOINTERFACE:								return "No such interface supported";
		case E_NOTIMPL:									return "Not implemented";
		case E_OUTOFMEMORY:								return "Failed to allocate necessary memory";
		case E_POINTER:									return "Pointer that is not valid";
		case E_UNEXPECTED:								return "Unexpected failure";

		// DXGI error codes
		case DXGI_ERROR_INVALID_CALL:					return "The method call is invalid";
		case DXGI_ERROR_NOT_FOUND:						return "The requested item was not found";
		case DXGI_ERROR_MORE_DATA:						return "The buffer supplied is too small";
		case DXGI_ERROR_UNSUPPORTED:					return "The requested functionality is not supported";
		case DXGI_ERROR_DEVICE_REMOVED:					return "GPU device has been removed";
		case DXGI_ERROR_DEVICE_HUNG:					return "GPU device is hung";
		case DXGI_ERROR_DEVICE_RESET:					return "GPU device has been reset";
		case DXGI_ERROR_WAS_STILL_DRAWING:				return "Previous frame still rendering";
		case DXGI_ERROR_FRAME_STATISTICS_DISJOINT:		return "Frame statistics are disjoint";
		case DXGI_ERROR_GRAPHICS_VIDPN_SOURCE_IN_USE:	return "Video present source is in use";
		case DXGI_ERROR_DRIVER_INTERNAL_ERROR:			return "Driver internal error";
		case DXGI_ERROR_NONEXCLUSIVE:					return "Resource is not in exclusive mode";
		case DXGI_ERROR_NOT_CURRENTLY_AVAILABLE:		return "Resource is not currently available";
		case DXGI_ERROR_REMOTE_CLIENT_DISCONNECTED:		return "Remote client disconnected";
		case DXGI_ERROR_REMOTE_OUTOFMEMORY:				return "Remote device out of memory";
		case DXGI_ERROR_ACCESS_LOST:					return "Access to resource lost";
		case DXGI_ERROR_WAIT_TIMEOUT:					return "Wait operation timed out";
		case DXGI_ERROR_SESSION_DISCONNECTED:			return "Session disconnected";
		case DXGI_ERROR_RESTRICT_TO_OUTPUT_STALE:		return "Output restriction is stale";
		case DXGI_ERROR_CANNOT_PROTECT_CONTENT:			return "Cannot protect content";
		case DXGI_ERROR_ACCESS_DENIED:					return "Access denied to resource";
		case DXGI_ERROR_NAME_ALREADY_EXISTS:			return "Name already exists";
		case DXGI_ERROR_SDK_COMPONENT_MISSING:			return "SDK component missing";
		default:										return "UNKNOWN DIRECTX ERROR";
	}
}

static constexpr const char* ID3D12CommandListTypeToString(D3D12_COMMAND_LIST_TYPE type)
{
	switch (type)
	{
		case D3D12_COMMAND_LIST_TYPE_DIRECT: return "CommandList::Direct";
		case D3D12_COMMAND_LIST_TYPE_BUNDLE: return "CommandList::Bundle";
		case D3D12_COMMAND_LIST_TYPE_COMPUTE: return "CommandList::Compute";
		case D3D12_COMMAND_LIST_TYPE_COPY: return "CommandList::Copy";
		case D3D12_COMMAND_LIST_TYPE_VIDEO_DECODE: return "CommandList::VideoDecode";
		case D3D12_COMMAND_LIST_TYPE_VIDEO_PROCESS: return "CommandList::VideoProcess";
		case D3D12_COMMAND_LIST_TYPE_VIDEO_ENCODE: return "CommandList::VideoEncode";
		default:
		{
			GLEAM_ASSERT(false);
			return "INVALID D3D12_COMMAND_LIST_TYPE";
		}
	}
}

static void WaitForID3D12Fence(ID3D12Fence* fence, uint64_t value)
{
	if (fence->GetCompletedValue() >= value)
	{
		return;
	}

	HANDLE fenceEvent = ::CreateEvent(NULL, FALSE, FALSE, NULL);
	DX_CHECK(fence->SetEventOnCompletion(value, fenceEvent));

	if (fenceEvent != 0)
	{
		DWORD result = ::WaitForSingleObject(fenceEvent, INFINITE);
		::CloseHandle(fenceEvent);
		GLEAM_ASSERT(result == WAIT_OBJECT_0);
	}
}

static constexpr TextureFormat DXGI_FORMATtoTextureFormat(DXGI_FORMAT format)
{
	switch (format)
	{
		case DXGI_FORMAT_R8G8B8A8_UNORM_SRGB: return TextureFormat::R8G8B8A8_SRGB;

		case DXGI_FORMAT_R8_UNORM: return TextureFormat::R8_UNorm;
		case DXGI_FORMAT_R8G8_UNORM: return TextureFormat::R8G8_UNorm;
		case DXGI_FORMAT_R8G8B8A8_UNORM: return TextureFormat::R8G8B8A8_UNorm;

		case DXGI_FORMAT_R8_SNORM: return TextureFormat::R8_SNorm;
		case DXGI_FORMAT_R8G8_SNORM: return TextureFormat::R8G8_SNorm;
		case DXGI_FORMAT_R8G8B8A8_SNORM: return TextureFormat::R8G8B8A8_SNorm;

		case DXGI_FORMAT_R8_UINT: return TextureFormat::R8_UInt;
		case DXGI_FORMAT_R8G8_UINT: return TextureFormat::R8G8_UInt;
		case DXGI_FORMAT_R8G8B8A8_UINT: return TextureFormat::R8G8B8A8_UInt;

		case DXGI_FORMAT_R8_SINT: return TextureFormat::R8_SInt;
		case DXGI_FORMAT_R8G8_SINT: return TextureFormat::R8G8_SInt;
		case DXGI_FORMAT_R8G8B8A8_SINT: return TextureFormat::R8G8B8A8_SInt;

		case DXGI_FORMAT_R16_UNORM: return TextureFormat::R16_UNorm;
		case DXGI_FORMAT_R16G16_UNORM: return TextureFormat::R16G16_UNorm;
		case DXGI_FORMAT_R16G16B16A16_UNORM: return TextureFormat::R16G16B16A16_UNorm;

		case DXGI_FORMAT_R16_SNORM: return TextureFormat::R16_SNorm;
		case DXGI_FORMAT_R16G16_SNORM: return TextureFormat::R16G16_SNorm;
		case DXGI_FORMAT_R16G16B16A16_SNORM: return TextureFormat::R16G16B16A16_SNorm;

		case DXGI_FORMAT_R16_UINT: return TextureFormat::R16_UInt;
		case DXGI_FORMAT_R16G16_UINT: return TextureFormat::R16G16_UInt;
		case DXGI_FORMAT_R16G16B16A16_UINT: return TextureFormat::R16G16B16A16_UInt;

		case DXGI_FORMAT_R16_SINT: return TextureFormat::R16_SInt;
		case DXGI_FORMAT_R16G16_SINT: return TextureFormat::R16G16_SInt;
		case DXGI_FORMAT_R16G16B16A16_SINT: return TextureFormat::R16G16B16A16_SInt;

		case DXGI_FORMAT_R16_FLOAT: return TextureFormat::R16_SFloat;
		case DXGI_FORMAT_R16G16_FLOAT: return TextureFormat::R16G16_SFloat;
		case DXGI_FORMAT_R16G16B16A16_FLOAT: return TextureFormat::R16G16B16A16_SFloat;

		case DXGI_FORMAT_R32_UINT: return TextureFormat::R32_UInt;
		case DXGI_FORMAT_R32G32_UINT: return TextureFormat::R32G32_UInt;
		case DXGI_FORMAT_R32G32B32A32_UINT: return TextureFormat::R32G32B32A32_UInt;

		case DXGI_FORMAT_R32_SINT: return TextureFormat::R32_SInt;
		case DXGI_FORMAT_R32G32_SINT: return TextureFormat::R32G32_SInt;
		case DXGI_FORMAT_R32G32B32A32_SINT: return TextureFormat::R32G32B32A32_SInt;

		case DXGI_FORMAT_R32_FLOAT: return TextureFormat::R32_SFloat;
		case DXGI_FORMAT_R32G32_FLOAT: return TextureFormat::R32G32_SFloat;
		case DXGI_FORMAT_R32G32B32A32_FLOAT: return TextureFormat::R32G32B32A32_SFloat;

		case DXGI_FORMAT_B8G8R8A8_UNORM_SRGB: return TextureFormat::B8G8R8A8_SRGB;
		case DXGI_FORMAT_B8G8R8A8_UNORM: return TextureFormat::B8G8R8A8_UNorm;

		case DXGI_FORMAT_R9G9B9E5_SHAREDEXP: return TextureFormat::R9G9B9E5_SFloat;
		case DXGI_FORMAT_R11G11B10_FLOAT: return TextureFormat::R11G11B10_SFloat;
		case DXGI_FORMAT_R10G10B10A2_UNORM: return TextureFormat::R10G10B10A2_Unorm;

		// Depth - Stencil formats
		case DXGI_FORMAT_D16_UNORM: return TextureFormat::D16_UNorm;
		case DXGI_FORMAT_D32_FLOAT: return TextureFormat::D32_SFloat;
        case DXGI_FORMAT_D24_UNORM_S8_UINT: return TextureFormat::D24_UNorm_S8_UInt;
		case DXGI_FORMAT_D32_FLOAT_S8X24_UINT: return TextureFormat::D32_SFloat_S8_UInt;

		default: return TextureFormat::None;
	}
}

static constexpr DXGI_FORMAT TextureFormatToDXGI_FORMAT(TextureFormat format)
{
	switch (format)
	{
		case TextureFormat::R8G8B8A8_SRGB: return DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;

		case TextureFormat::R8_UNorm: return DXGI_FORMAT_R8_UNORM;
		case TextureFormat::R8G8_UNorm: return DXGI_FORMAT_R8G8_UNORM;
		case TextureFormat::R8G8B8A8_UNorm: return DXGI_FORMAT_R8G8B8A8_UNORM;

		case TextureFormat::R8_SNorm: return DXGI_FORMAT_R8_SNORM;
		case TextureFormat::R8G8_SNorm: return DXGI_FORMAT_R8G8_SNORM;
		case TextureFormat::R8G8B8A8_SNorm: return DXGI_FORMAT_R8G8B8A8_SNORM;

		case TextureFormat::R8_UInt: return DXGI_FORMAT_R8_UINT;
		case TextureFormat::R8G8_UInt: return DXGI_FORMAT_R8G8_UINT;
		case TextureFormat::R8G8B8A8_UInt: return DXGI_FORMAT_R8G8B8A8_UINT;

		case TextureFormat::R8_SInt: return DXGI_FORMAT_R8_SINT;
		case TextureFormat::R8G8_SInt: return DXGI_FORMAT_R8G8_SINT;
		case TextureFormat::R8G8B8A8_SInt: return DXGI_FORMAT_R8G8B8A8_SINT;

		case TextureFormat::R16_UNorm: return DXGI_FORMAT_R16_UNORM;
		case TextureFormat::R16G16_UNorm: return DXGI_FORMAT_R16G16_UNORM;
		case TextureFormat::R16G16B16A16_UNorm: return DXGI_FORMAT_R16G16B16A16_UNORM;

		case TextureFormat::R16_SNorm: return DXGI_FORMAT_R16_SNORM;
		case TextureFormat::R16G16_SNorm: return DXGI_FORMAT_R16G16_SNORM;
		case TextureFormat::R16G16B16A16_SNorm: return DXGI_FORMAT_R16G16B16A16_SNORM;

		case TextureFormat::R16_UInt: return DXGI_FORMAT_R16_UINT;
		case TextureFormat::R16G16_UInt: return DXGI_FORMAT_R16G16_UINT;
		case TextureFormat::R16G16B16A16_UInt: return DXGI_FORMAT_R16G16B16A16_UINT;

		case TextureFormat::R16_SInt: return DXGI_FORMAT_R16_SINT;
		case TextureFormat::R16G16_SInt: return DXGI_FORMAT_R16G16_SINT;
		case TextureFormat::R16G16B16A16_SInt: return DXGI_FORMAT_R16G16B16A16_SINT;

		case TextureFormat::R16_SFloat: return DXGI_FORMAT_R16_FLOAT;
		case TextureFormat::R16G16_SFloat: return DXGI_FORMAT_R16G16_FLOAT;
		case TextureFormat::R16G16B16A16_SFloat: return DXGI_FORMAT_R16G16B16A16_FLOAT;

		case TextureFormat::R32_UInt: return DXGI_FORMAT_R32_UINT;
		case TextureFormat::R32G32_UInt: return DXGI_FORMAT_R32G32_UINT;
		case TextureFormat::R32G32B32A32_UInt: return DXGI_FORMAT_R32G32B32A32_UINT;

		case TextureFormat::R32_SInt: return DXGI_FORMAT_R32_SINT;
		case TextureFormat::R32G32_SInt: return DXGI_FORMAT_R32G32_SINT;
		case TextureFormat::R32G32B32A32_SInt: return DXGI_FORMAT_R32G32B32A32_SINT;

		case TextureFormat::R32_SFloat: return DXGI_FORMAT_R32_FLOAT;
		case TextureFormat::R32G32_SFloat: return DXGI_FORMAT_R32G32_FLOAT;
		case TextureFormat::R32G32B32A32_SFloat: return DXGI_FORMAT_R32G32B32A32_FLOAT;

		case TextureFormat::B8G8R8A8_SRGB: return DXGI_FORMAT_B8G8R8A8_TYPELESS;
		case TextureFormat::B8G8R8A8_UNorm: return DXGI_FORMAT_B8G8R8A8_UNORM;

		case TextureFormat::R9G9B9E5_SFloat: return DXGI_FORMAT_R9G9B9E5_SHAREDEXP;
		case TextureFormat::R11G11B10_SFloat: return DXGI_FORMAT_R11G11B10_FLOAT;
		case TextureFormat::R10G10B10A2_Unorm: return DXGI_FORMAT_R10G10B10A2_UNORM;

		// Depth - Stencil formats
		case TextureFormat::D16_UNorm: return DXGI_FORMAT_D16_UNORM;
		case TextureFormat::D32_SFloat: return DXGI_FORMAT_D32_FLOAT;
        case TextureFormat::D24_UNorm_S8_UInt: return DXGI_FORMAT_D24_UNORM_S8_UINT;
		case TextureFormat::D32_SFloat_S8_UInt: return DXGI_FORMAT_D32_FLOAT_S8X24_UINT;

		default: return DXGI_FORMAT_UNKNOWN;
	}
}

static constexpr D3D12_RENDER_PASS_BEGINNING_ACCESS_TYPE AttachmentLoadActionToDX_TYPE(AttachmentLoadAction loadAction)
{
	switch (loadAction)
	{
		case AttachmentLoadAction::Load: return D3D12_RENDER_PASS_BEGINNING_ACCESS_TYPE_PRESERVE;
		case AttachmentLoadAction::Clear: return D3D12_RENDER_PASS_BEGINNING_ACCESS_TYPE_CLEAR;
		case AttachmentLoadAction::DontCare: return D3D12_RENDER_PASS_BEGINNING_ACCESS_TYPE_DISCARD;
		default: return D3D12_RENDER_PASS_BEGINNING_ACCESS_TYPE_NO_ACCESS;
	}
}

static constexpr D3D12_RENDER_PASS_ENDING_ACCESS_TYPE AttachmentStoreActionToDX_TYPE(AttachmentStoreAction storeAction)
{
	switch (storeAction)
	{
		case AttachmentStoreAction::Store: return D3D12_RENDER_PASS_ENDING_ACCESS_TYPE_PRESERVE;
		case AttachmentStoreAction::DontCare: return D3D12_RENDER_PASS_ENDING_ACCESS_TYPE_DISCARD;
		default: return D3D12_RENDER_PASS_ENDING_ACCESS_TYPE_NO_ACCESS;
	}
}

static constexpr D3D_PRIMITIVE_TOPOLOGY PrimitiveToplogyToD3D_PRIMITIVE_TOPOLOGY(PrimitiveTopology topology)
{
	switch (topology)
	{
		case PrimitiveTopology::Points: return D3D_PRIMITIVE_TOPOLOGY_POINTLIST;
		case PrimitiveTopology::Lines: return D3D_PRIMITIVE_TOPOLOGY_LINELIST;
		case PrimitiveTopology::LineStrip: return D3D_PRIMITIVE_TOPOLOGY_LINESTRIP;
		case PrimitiveTopology::Triangles: return D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
        case PrimitiveTopology::TriangleStrip: return D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP;
		default: return D3D_PRIMITIVE_TOPOLOGY_UNDEFINED;
	}
}

static constexpr D3D12_PRIMITIVE_TOPOLOGY_TYPE PrimitiveToplogyToD3D12_PRIMITIVE_TOPOLOGY_TYPE(PrimitiveTopology topology)
{
	switch (topology)
	{
		case PrimitiveTopology::Points: return D3D12_PRIMITIVE_TOPOLOGY_TYPE_POINT;

		case PrimitiveTopology::Lines:
		case PrimitiveTopology::LineStrip: return D3D12_PRIMITIVE_TOPOLOGY_TYPE_LINE;

		case PrimitiveTopology::Triangles:;
		case PrimitiveTopology::TriangleStrip: return D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;

		default: return D3D12_PRIMITIVE_TOPOLOGY_TYPE_UNDEFINED;
	}
}

static constexpr D3D12_CULL_MODE CullModeToD3D12_CULL_MODE(CullMode cullMode)
{
	switch (cullMode)
	{
		case CullMode::Off: return D3D12_CULL_MODE_NONE;
		case CullMode::Front: return D3D12_CULL_MODE_FRONT;
		case CullMode::Back: return D3D12_CULL_MODE_BACK;
		default: return D3D12_CULL_MODE_NONE;
	}
}

static constexpr D3D12_STENCIL_OP StencilOpToD3D12_STENCIL_OP(StencilOp stencilOp)
{
	switch (stencilOp)
	{
		case StencilOp::Keep: return D3D12_STENCIL_OP_KEEP;
		case StencilOp::Zero: return D3D12_STENCIL_OP_ZERO;
		case StencilOp::Replace: return D3D12_STENCIL_OP_REPLACE;
		case StencilOp::IncrementClamp: return D3D12_STENCIL_OP_INCR_SAT;
		case StencilOp::IncrementWrap: return D3D12_STENCIL_OP_INCR;
		case StencilOp::DecrementClamp: return D3D12_STENCIL_OP_DECR_SAT;
		case StencilOp::DecrementWrap: return D3D12_STENCIL_OP_DECR;
		case StencilOp::Invert: return D3D12_STENCIL_OP_INVERT;
		default: return D3D12_STENCIL_OP_KEEP;
	}
}

static constexpr D3D12_COMPARISON_FUNC CompareFunctionToD3D12_COMPARISON_FUNC(CompareFunction compareFunction)
{
	switch (compareFunction)
	{
		case CompareFunction::Never: return D3D12_COMPARISON_FUNC_NEVER;
		case CompareFunction::Less: return D3D12_COMPARISON_FUNC_LESS;
		case CompareFunction::Equal: return D3D12_COMPARISON_FUNC_EQUAL;
		case CompareFunction::LessEqual: return D3D12_COMPARISON_FUNC_LESS_EQUAL;
		case CompareFunction::Greater: return D3D12_COMPARISON_FUNC_GREATER;
		case CompareFunction::NotEqual: return D3D12_COMPARISON_FUNC_NOT_EQUAL;
		case CompareFunction::GreaterEqual: return D3D12_COMPARISON_FUNC_GREATER_EQUAL;
		case CompareFunction::Always: return D3D12_COMPARISON_FUNC_ALWAYS;
		default: return D3D12_COMPARISON_FUNC_NONE;
	}
}

static constexpr D3D12_BLEND BlendModeToD3D12_BLEND(BlendMode blendMode)
{
	switch (blendMode)
	{
		case BlendMode::Zero: return D3D12_BLEND_ZERO;
		case BlendMode::One: return D3D12_BLEND_ONE;
		case BlendMode::DstColor: return D3D12_BLEND_DEST_COLOR;
		case BlendMode::SrcColor: return D3D12_BLEND_SRC_COLOR;
		case BlendMode::OneMinusDstColor: return D3D12_BLEND_INV_DEST_COLOR;
		case BlendMode::SrcAlpha: return D3D12_BLEND_SRC_ALPHA;
		case BlendMode::OneMinusSrcColor: return D3D12_BLEND_INV_SRC_COLOR;
		case BlendMode::DstAlpha: return D3D12_BLEND_DEST_ALPHA;
		case BlendMode::OneMinusDstAlpha: return D3D12_BLEND_INV_DEST_ALPHA;
		case BlendMode::SrcAlphaClamp: return D3D12_BLEND_SRC_ALPHA_SAT;
		case BlendMode::OneMinusSrcAlpha: return D3D12_BLEND_INV_SRC_ALPHA;
		default: return D3D12_BLEND_ZERO;
	}
}

static constexpr D3D12_BLEND_OP BlendOpToD3D12_BLEND_OP(BlendOp blendOp)
{
	switch (blendOp)
	{
		case BlendOp::Add: return D3D12_BLEND_OP_ADD;
		case BlendOp::Subtract: return D3D12_BLEND_OP_SUBTRACT;
		case BlendOp::ReverseSubtract: return D3D12_BLEND_OP_REV_SUBTRACT;
		case BlendOp::Min: return D3D12_BLEND_OP_MIN;
		case BlendOp::Max: return D3D12_BLEND_OP_MAX;
		default: return D3D12_BLEND_OP_ADD;
	}
}

static constexpr D3D12_COLOR_WRITE_ENABLE ColorWriteMaskToD3D12_COLOR_WRITE_ENABLE(ColorWriteMask mask)
{
	switch (mask)
	{
		case ColorWriteMask::Red: return D3D12_COLOR_WRITE_ENABLE_RED;
		case ColorWriteMask::Green: return D3D12_COLOR_WRITE_ENABLE_GREEN;
		case ColorWriteMask::Blue: return D3D12_COLOR_WRITE_ENABLE_BLUE;
		case ColorWriteMask::All: return D3D12_COLOR_WRITE_ENABLE_ALL;
		default: return D3D12_COLOR_WRITE_ENABLE_ALL;
	}
}

static constexpr D3D12_RESOURCE_DIMENSION TextureDimensionToD3D12_RESOURCE_DIMENSION(TextureDimension dimension)
{
	switch (dimension)
	{
		case TextureDimension::Texture2D: return D3D12_RESOURCE_DIMENSION_TEXTURE2D;
		case TextureDimension::TextureCube: return D3D12_RESOURCE_DIMENSION_TEXTURE2D;
		case TextureDimension::Texture3D: return D3D12_RESOURCE_DIMENSION_TEXTURE3D;
		default: return D3D12_RESOURCE_DIMENSION_UNKNOWN;
	}
}

static constexpr D3D12_BARRIER_SYNC BarrierStageToD3D12_BARRIER_SYNC(BarrierStage stage)
{
	switch (stage)
	{
		case BarrierStage::None: return D3D12_BARRIER_SYNC_NONE;
		case BarrierStage::All: return D3D12_BARRIER_SYNC_ALL;
		case BarrierStage::AllShading: return D3D12_BARRIER_SYNC_ALL_SHADING;
		case BarrierStage::NonFragmentShading: return D3D12_BARRIER_SYNC_NON_PIXEL_SHADING;
		case BarrierStage::VertexShading: return D3D12_BARRIER_SYNC_VERTEX_SHADING;
		case BarrierStage::FragmentShading: return D3D12_BARRIER_SYNC_PIXEL_SHADING;
		case BarrierStage::ComputeShading: return D3D12_BARRIER_SYNC_COMPUTE_SHADING;
		case BarrierStage::RenderTarget: return D3D12_BARRIER_SYNC_RENDER_TARGET;
		case BarrierStage::DepthStencil: return D3D12_BARRIER_SYNC_DEPTH_STENCIL;
		case BarrierStage::Copy: return D3D12_BARRIER_SYNC_COPY;
		case BarrierStage::BuildRayTracingAccelerationStructure: return D3D12_BARRIER_SYNC_BUILD_RAYTRACING_ACCELERATION_STRUCTURE;
		default: return D3D12_BARRIER_SYNC_NONE;
	}
}

static constexpr D3D12_BARRIER_ACCESS BarrierAccessToD3D12_BARRIER_ACCESS(BarrierAccess access)
{
	switch (access)
	{
		case BarrierAccess::None: return D3D12_BARRIER_ACCESS_NO_ACCESS;
		case BarrierAccess::Common: return D3D12_BARRIER_ACCESS_COMMON;
		case BarrierAccess::RenderTarget: return D3D12_BARRIER_ACCESS_RENDER_TARGET;
		case BarrierAccess::ShaderResource: return D3D12_BARRIER_ACCESS_SHADER_RESOURCE;
		case BarrierAccess::UnorderedAccess: return D3D12_BARRIER_ACCESS_UNORDERED_ACCESS;
		case BarrierAccess::DepthStencilRead: return D3D12_BARRIER_ACCESS_DEPTH_STENCIL_WRITE; // HACK TO AVOID READONLY DSVs
		case BarrierAccess::DepthStencilWrite: return D3D12_BARRIER_ACCESS_DEPTH_STENCIL_WRITE;
		case BarrierAccess::CopySource: return D3D12_BARRIER_ACCESS_COPY_SOURCE;
		case BarrierAccess::CopyDest: return D3D12_BARRIER_ACCESS_COPY_DEST;
		case BarrierAccess::RayTracingAccelerationStructureRead: return D3D12_BARRIER_ACCESS_RAYTRACING_ACCELERATION_STRUCTURE_READ;
		case BarrierAccess::RayTracingAccelerationStructureWrite: return D3D12_BARRIER_ACCESS_RAYTRACING_ACCELERATION_STRUCTURE_WRITE;
		default: return D3D12_BARRIER_ACCESS_NO_ACCESS;
	}
}

static constexpr D3D12_BARRIER_LAYOUT BarrierLayoutToD3D12_BARRIER_LAYOUT(BarrierLayout layout)
{
	switch (layout)
	{
		case BarrierLayout::Undefined: return D3D12_BARRIER_LAYOUT_UNDEFINED;
		case BarrierLayout::Common: return D3D12_BARRIER_LAYOUT_COMMON;
		case BarrierLayout::RenderTarget: return D3D12_BARRIER_LAYOUT_RENDER_TARGET;
		case BarrierLayout::ShaderResource: return D3D12_BARRIER_LAYOUT_SHADER_RESOURCE;
		case BarrierLayout::UnorderedAccess: return D3D12_BARRIER_LAYOUT_UNORDERED_ACCESS;
		case BarrierLayout::DepthStencilRead: return D3D12_BARRIER_LAYOUT_DEPTH_STENCIL_WRITE; // HACK TO AVOID READONLY DSVs
		case BarrierLayout::DepthStencilWrite: return D3D12_BARRIER_LAYOUT_DEPTH_STENCIL_WRITE;
		case BarrierLayout::CopySource: return D3D12_BARRIER_LAYOUT_COPY_SOURCE;
		case BarrierLayout::CopyDest: return D3D12_BARRIER_LAYOUT_COPY_DEST;
		default: return D3D12_BARRIER_LAYOUT_UNDEFINED;
	}
}

} // namespace Gleam
#endif
