#pragma once
#include "Core/EngineDefines.h"
#include "Container/Array.h"

namespace Gleam {

enum class BarrierStage
{
	None,
	All,
	AllShading,
	NonFragmentShading,
	VertexShading,
	FragmentShading,
	ComputeShading,
	RenderTarget,
	DepthStencil,
	Copy,
	BuildRayTracingAccelerationStructure
};

enum class BarrierAccess
{
	None,
	Common,
	RenderTarget,
	ShaderResource,
	UnorderedAccess,
	DepthStencilRead,
	DepthStencilWrite,
	CopySource,
	CopyDest
};

enum class BarrierLayout
{
	Undefined,
	Common,
	RenderTarget,
	ShaderResource,
	UnorderedAccess,
	DepthStencilRead,
	DepthStencilWrite,
	CopySource,
	CopyDest,
	RayTracingAccelerationStructureRead,
	RayTracingAccelerationStructureWrite
};

struct BufferBarrier
{
	NativeGraphicsHandle resource = nullptr;
	BarrierStage srcStage = BarrierStage::None;
	BarrierStage dstStage = BarrierStage::None;
	BarrierAccess srcAccess = BarrierAccess::None;
	BarrierAccess dstAccess = BarrierAccess::None;
};

struct TextureBarrier
{
	NativeGraphicsHandle resource = nullptr;
	BarrierStage srcStage = BarrierStage::None;
	BarrierStage dstStage = BarrierStage::None;
	BarrierAccess srcAccess = BarrierAccess::None;
	BarrierAccess dstAccess = BarrierAccess::None;
	BarrierLayout oldLayout = BarrierLayout::Undefined;
	BarrierLayout newLayout = BarrierLayout::Undefined;
	bool discard = false;
};

struct BarrierGroup
{
	TArray<BufferBarrier> bufferBarriers;
	TArray<TextureBarrier> textureBarriers;
};

struct BarrierState
{
	BarrierStage stage = BarrierStage::None;
	BarrierAccess access = BarrierAccess::None;
	BarrierLayout layout = BarrierLayout::Undefined;

	static BarrierState NonAliased()
	{
		return {};
	}
};

} // namespace Gleam
