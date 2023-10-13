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

#include <functional>

namespace Gleam {

class CommandBuffer;

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
};

struct RenderPassNode final : public RenderGraphNode
{
    GLEAM_NONCOPYABLE(RenderPassNode);
    
    using PassCallback = std::function<void(const CommandBuffer* cmd)>;
    
    TString name;
    
    std::any data;
    PassCallback callback;
    
    bool hasSideEffect = false;
    
	TArray<BufferHandle> bufferReads;
	TArray<BufferHandle> bufferWrites;
	TArray<BufferHandle> bufferCreates;

	TArray<TextureHandle> textureReads;
	TArray<TextureHandle> textureWrites;
	TArray<TextureHandle> textureCreates;
    
    TArray<TextureHandle> colorAttachments;
    TextureHandle depthAttachment;
    
    HashSet<RenderPassNode*> dependents;
    
    template<typename PassData>
    RenderPassNode(uint32_t uniqueId, const TStringView name, RenderFunc<PassData>&& execute)
        : RenderGraphNode(uniqueId), name(name), data(std::make_any<PassData>())
    {
        callback = [execute = std::move(execute), this](const CommandBuffer* cmd)
        {
            std::invoke(execute, cmd, std::any_cast<const PassData&>(data));
        };
    }
    
    bool isCulled() const
    {
        return refCount == 0 && !hasSideEffect;
    }
    
    bool isCustomPass() const
    {
        return colorAttachments.empty() && !depthAttachment.IsValid();
    }
};

struct RenderGraphResourceNode : public RenderGraphNode
{
    const bool transient;
    RenderPassNode* creator = nullptr;
    RenderPassNode* lastModifier = nullptr;
    RenderPassNode* lastReference = nullptr;
    TArray<RenderPassNode*> producers;
    
    RenderGraphResourceNode(uint32_t uniqueId, bool transient)
        : RenderGraphNode(uniqueId), transient(transient)
    {
        
    }
};

struct RenderGraphBufferNode final : public RenderGraphResourceNode
{
    Buffer buffer = Buffer();

	RenderGraphBufferNode(uint32_t uniqueId, const BufferDescriptor& descriptor, bool transient)
		: RenderGraphResourceNode(uniqueId, transient), buffer(nullptr, descriptor)
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

	RenderGraphTextureNode(uint32_t uniqueId, const RenderTextureDescriptor& descriptor, bool transient)
		: RenderGraphResourceNode(uniqueId, transient), texture(descriptor, false),
		clearColor(descriptor.clearColor),
		clearStencil(descriptor.clearStencil),
		clearDepth(descriptor.clearDepth),
		clearBuffer(descriptor.clearBuffer)
	{

	}
};

} // namespace Gleam
