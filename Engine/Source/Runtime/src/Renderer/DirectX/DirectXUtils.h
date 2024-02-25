#pragma once
#ifdef USE_DIRECTX_RENDERER
#include <d3d12.h>
#include <D3D12MemAlloc.h>
#include "Renderer/TextureFormat.h"
#include "Renderer/HeapDescriptor.h"
#include "Renderer/BufferDescriptor.h"
#include "Renderer/RenderPassDescriptor.h"
#include "Renderer/PipelineStateDescriptor.h"

namespace Gleam {

#define DX_CHECK(x) GLEAM_ASSERT(SUCCEEDED((x)))

static constexpr TextureFormat DXGI_FORMATtoTextureFormat(DXGI_FORMAT format)
{
	switch (format)
	{
		case DXGI_FORMAT_R8_TYPELESS: return TextureFormat::R8_SRGB;
		case DXGI_FORMAT_R8G8_TYPELESS: return TextureFormat::R8G8_SRGB;
		case DXGI_FORMAT_R8G8B8A8_TYPELESS: return TextureFormat::R8G8B8A8_SRGB;

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

		case DXGI_FORMAT_B8G8R8A8_TYPELESS: return TextureFormat::B8G8R8A8_SRGB;
		case DXGI_FORMAT_B8G8R8A8_UNORM: return TextureFormat::B8G8R8A8_UNorm;

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
		case TextureFormat::R8_SRGB: return DXGI_FORMAT_R8_TYPELESS;
		case TextureFormat::R8G8_SRGB: return DXGI_FORMAT_R8G8_TYPELESS;
		case TextureFormat::R8G8B8A8_SRGB: return DXGI_FORMAT_R8G8B8A8_TYPELESS;

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

} // namespace Gleam
#endif
