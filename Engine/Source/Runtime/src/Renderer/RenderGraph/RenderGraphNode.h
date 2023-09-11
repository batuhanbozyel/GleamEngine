//
//  RenderGraphNode.h
//  GleamEngine
//
//  Created by Batuhan Bozyel on 11.03.2023.
//

#pragma once
#include "RenderGraphResource.h"

#include <functional>

namespace Gleam {

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
    
    TArray<ResourceHandle> resourceReads;
    TArray<ResourceHandle> resourceWrites;
    TArray<ResourceHandle> resourceCreates;
    
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
        return colorAttachments.empty() && depthAttachment.IsValid();
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
    BufferDescriptor descriptor;
    size_t offset = 0;
};

struct RenderGraphTextureNode final : public RenderGraphResourceNode
{
    RenderTextureDescriptor descriptor;
};

} // namespace Gleam
