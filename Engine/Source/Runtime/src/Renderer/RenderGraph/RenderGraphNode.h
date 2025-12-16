//
//  RenderGraphNode.h
//  GleamEngine
//
//  Created by Batuhan Bozyel on 11.03.2023.
//

#pragma once
#include "RenderGraphResource.h"
#include "Renderer/Buffer.h"
#include "Renderer/Texture.h"
#include "Renderer/Barrier.h"

#include <functional>
#include <any>

namespace Gleam {

class CommandBuffer;
class CopyCommandBuffer;

template<typename PassData>
using CopyFunc = std::function<void(const CopyCommandBuffer* cmd, const PassData& passData)>;

template<typename PassData>
using RenderFunc = std::function<void(const CommandBuffer* cmd, const PassData& passData)>;

struct RenderGraphNode
{
    const uint32_t uniqueId;
    uint32_t refCount = 0;
    
    RenderGraphNode(uint32_t uniqueId)
        : uniqueId(uniqueId)
    {
        
    }
	
	virtual ~RenderGraphNode() = default;
};

enum class RenderGraphPassType
{
	Copy,
	Raster,
	Compute,
	Native
};

struct RenderGraphPassNode : public RenderGraphNode
{
    GLEAM_NONCOPYABLE(RenderGraphPassNode);
    
    using PassCallback = std::function<void(const void* userData)>;
    
    TString name;
	std::any data;
    PassCallback callback;
	RenderGraphPassType type = RenderGraphPassType::Native;
    
    bool hasSideEffect = false;
    
	TArray<BufferHandle> bufferReads;
	TArray<BufferHandle> bufferWrites;
	TArray<BufferHandle> bufferCreates;

	TArray<TextureHandle> textureReads;
	TArray<TextureHandle> textureWrites;
	TArray<TextureHandle> textureCreates;
    
    TArray<TextureHandle> colorAttachments;
    TextureHandle depthAttachment;
    
    HashSet<RenderGraphPassNode*> dependents;
    
    RenderGraphPassNode(RenderGraphPassType type, uint32_t uniqueId, const TStringView name)
        : RenderGraphNode(uniqueId)
		, name(name)
		, type(type)
    {
        
    }
	
	virtual ~RenderGraphPassNode() = default;
};

struct RenderGraphCopyPassNode final : public RenderGraphPassNode
{
	template<typename PassData>
	RenderGraphCopyPassNode(uint32_t uniqueId, const TStringView name, CopyFunc<PassData>&& execute)
		: RenderGraphPassNode(RenderGraphPassType::Copy, uniqueId, name)
	{
		data = std::make_any<PassData>();
		callback = [execute = std::move(execute), this](const void* userData)
		{
			std::invoke(execute, static_cast<const CopyCommandBuffer*>(userData), std::any_cast<const PassData&>(data));
		};
	}
};

struct RenderGraphRenderPassNode final : public RenderGraphPassNode
{
	template<typename PassData>
	RenderGraphRenderPassNode(uint32_t uniqueId, const TStringView name, RenderFunc<PassData>&& execute)
		: RenderGraphPassNode(RenderGraphPassType::Raster, uniqueId, name)
	{
		data = std::make_any<PassData>();
		callback = [execute = std::move(execute), this](const void* userData)
		{
			std::invoke(execute, static_cast<const CommandBuffer*>(userData), std::any_cast<const PassData&>(data));
		};
	}
};

struct RenderGraphComputePassNode final : public RenderGraphPassNode
{
	template<typename PassData>
	RenderGraphComputePassNode(uint32_t uniqueId, const TStringView name, RenderFunc<PassData>&& execute)
		: RenderGraphPassNode(RenderGraphPassType::Compute, uniqueId, name)
	{
		data = std::make_any<PassData>();
		callback = [execute = std::move(execute), this](const void* userData)
		{
			std::invoke(execute, static_cast<const CommandBuffer*>(userData), std::any_cast<const PassData&>(data));
		};
	}
};

struct RenderGraphResourceNode : public RenderGraphNode
{
    const bool transient;
    RenderGraphPassNode* creator = nullptr;
    RenderGraphPassNode* lastModifier = nullptr;
    RenderGraphPassNode* lastReference = nullptr;
    TArray<RenderGraphPassNode*> producers;
    
    RenderGraphResourceNode(uint32_t uniqueId, bool transient)
        : RenderGraphNode(uniqueId), transient(transient)
    {
        
    }
};

struct RenderGraphBufferNode final : public RenderGraphResourceNode
{
    Buffer buffer = Buffer();

	struct BarrierState
	{
		BarrierStage stage = BarrierStage::None;
		BarrierAccess access = BarrierAccess::None;
	} barrierState;

	RenderGraphBufferNode(uint32_t uniqueId, const BufferDescriptor& descriptor, bool transient)
		: RenderGraphResourceNode(uniqueId, transient), buffer(descriptor)
	{

	}
};

struct RenderGraphTextureNode final : public RenderGraphResourceNode
{
	Color clearColor = Color::clear;
	uint32_t clearStencil = 0u;
	float clearDepth = 1.0f;
	bool clearBuffer = false;
	Texture texture = Texture();

	struct BarrierState
	{
		BarrierStage stage = BarrierStage::None;
		BarrierAccess access = BarrierAccess::None;
		BarrierLayout layout = BarrierLayout::Undefined;
	} barrierState;

	RenderGraphTextureNode(uint32_t uniqueId, const RenderTextureDescriptor& descriptor, bool transient)
		: RenderGraphResourceNode(uniqueId, transient), texture(descriptor),
		clearColor(descriptor.clearColor),
		clearStencil(descriptor.clearStencil),
		clearDepth(descriptor.clearDepth),
		clearBuffer(descriptor.clearBuffer)
	{

	}
};

} // namespace Gleam
